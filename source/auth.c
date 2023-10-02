#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include <memory.h>
#include <json-c/json.h>
#include "auth.h"
#include "common.h"

/// @brief Wrap token in authentication header
/// @param token Authentication token to wrap (freed after this function call)
/// @return Authentication header
char* getAuthHeader(const char* token, size_t tokenSize) {
  char* authHeader = malloc(sizeof(char) * 20 + tokenSize);
  if (authHeader == NULL) return NULL;
  strcpy(authHeader, "Authorization: JWT ");
  strcat(authHeader, token);
  return authHeader;
}

size_t writeRefreshedToken(void *data, size_t size, size_t nmemb, void *userdata) {
  FILE *file = (FILE *) userdata;
  return fwrite(data, size, nmemb, file);
}

AuthResult refreshToken(const char* oldToken, size_t oldTokenSize, bool* succeeded, char* newToken) {
  CURL *curl = curl_easy_init();

  // open temporary file for writing the new credential to
  FILE *newTokenFile = fopen("sdmc:/3ds/streetrelay/new_cred.txt", "w+");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeRefreshedToken);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, newTokenFile);

  // wrap old token in authentication header
  char* authHeader = getAuthHeader(oldToken, oldTokenSize);
  if (authHeader == NULL) return AUTH_OUT_OF_MEM;

  // start request
  int res = initRequest(curl, M_GET, "/login", authHeader);
  if (res != 0) return AUTH_OUT_OF_MEM;
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) return AUTH_CURL & res;
  // clean up new credential file
  fclose(newTokenFile);

  // get response code
  long responseCode;
  res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
  if (res != CURLE_OK) return AUTH_CURL & res;
  if (responseCode == 426) return AUTH_UPDATE_REQUIRED;

  // if request failed
  if (responseCode == 500) {
    return AUTH_SERVER_ERROR;
  } else if (responseCode != 200) {
    *succeeded = false;
  } else {
    *succeeded = true;

    // load token from credential file
    json_object *tokenObj = json_object_from_file("sdmc:/3ds/streetrelay/new_cred.txt");
    if (tokenObj == NULL) return AUTH_INVALID_TOKEN_RETURN;

    // json-c manages it's memory, work around that
    int newTokenSize = json_object_get_string_len(tokenObj);
    newToken = malloc(newTokenSize);
    memcpy(newToken, json_object_get_string(tokenObj), newTokenSize);
    json_object_put(tokenObj);

    // write token to credential file
    FILE *tokenFile = fopen("sdmc:/3ds/streetrelay/cred.txt", "w");
    fwrite(newToken, newTokenSize, 1, tokenFile);
    fflush(tokenFile);
    fclose(tokenFile);
  }

  remove("sdmc:/3ds/streetrelay/new_cred.txt");
  curl_easy_cleanup(curl);
  return 0;
}
