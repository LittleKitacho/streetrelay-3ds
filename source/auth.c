#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include <memory.h>
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

struct refreshedTokenData {
  FILE *file;
  char *token;
  size_t size;
};

size_t writeRefreshedToken(char *data, size_t size, size_t nmemb, struct refreshedTokenData *memory) {
  size_t realsize = size * nmemb;

  char *token = realloc(memory -> token, memory -> size + realsize + 1);
  if (token == NULL) return 0; // OUT OF MEMORY

  memory -> token = token;
  memcpy(&(memory -> token[memory -> size]), data, realsize);
  memory -> size += realsize;
  memory -> token[memory -> size] = 0;

  fwrite(data, size, nmemb, memory -> file);

  return realsize;
}

AuthResult refreshToken(const char* oldToken, size_t oldTokenSize, bool* succeeded, char* newToken) {
  CURL *curl = curl_easy_init();
  struct refreshedTokenData refreshedData = {
    fopen("sdmc:/streetrelay/cred.txt", "w")
  };
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeRefreshedToken);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &refreshedData);
  char* authHeader = getAuthHeader(oldToken, oldTokenSize);
  if (authHeader == NULL) return AUTH_OUT_OF_MEM;
  CURLcode res = initRequest(curl, M_GET, "/login", authHeader);
  if (res != CURLE_OK) return AUTH_CURL & res;

  long responseCode;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
  curl_easy_cleanup(curl);

  if (responseCode == 426) return AUTH_UPDATE_REQUIRED;

  if (responseCode != 200) {
    *succeeded = false;
    return 0;
  } else {
    *succeeded = true;
    newToken = refreshedData.token;
    return 0;
  }
}
