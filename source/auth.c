#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include <memory.h>
#include <json-c/json.h>
#include "auth.h"
#include "common.h"

/// @brief Wrap token in authentication header
/// @param token Authentication token to wrap
/// @param token_length Length of authentication token
/// @return Authentication header
char *get_auth_header(const char *token, size_t token_length)
{
  char *header = malloc(24 + token_length);
  if (header == NULL)
    return NULL;
  strcpy(header, "Authorization: Bearer ");
  strcat(header, token);
  return header;
}

typedef struct new_token_data_s
{
  size_t size;
  char *buffer;
} new_token_data;

size_t write_refreshed_token(void *data, size_t size, size_t nmemb, void *userdata)
{
  new_token_data *new_data = (new_token_data *)userdata;

  size_t available = size * nmemb;
  char *ptr = realloc(new_data->buffer, new_data->size + available + 1);
  if (!ptr)
  {
    printf("out of memory!");
    return 0;
  }

  memcpy(&(new_data->buffer[new_data->size]), data, available);
  new_data->size += available;
  return available;
}

bool auth_perform_refresh(const char *auth_header, char **new_token, off_t *new_token_size)
{
  CURL *curl = curl_easy_init();
  struct curl_slist *headers;
  init_request(curl, BASE_URL "/login", auth_header, &headers);

  new_token_data token_data = {0};
  token_data.buffer = malloc(500);
  if (token_data.buffer == NULL)
  {
    return false;
  }

  CURLcode res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_refreshed_token);
  if (res != CURLE_OK)
  {
    printf("Could not initialize request: %d", res);
    return false;
  }
  res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &token_data);
  if (res != CURLE_OK)
  {
    printf("Could not initialize request: %d", res);
    return false;
  }
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    printf("Could not perform request: %d", res);
    return false;
  }
  curl_slist_free_all(headers);

  long status;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  struct json_object *response = json_tokener_parse(token_data.buffer);
  if (response == NULL)
  {
    printf("Could not parse server response.");
    return false;
  }
  if (status != 200)
  {
    // error occured during authentication!
    struct json_object *err_message = json_object_object_get(response, "message");
    printf("An error occured while refreshing the token: %s\n", json_object_get_string(err_message));
    json_object_put(response);
    return false;
  }

  if (json_object_get_type(response) != json_type_string)
  {
    printf("Could not parse server response.");
    return false;
  }
  *new_token_size = json_object_get_string_len(response);
  *new_token = malloc(*new_token_size);
  strcpy(*new_token, json_object_get_string(response));
  json_object_put(response);
  return true;
}
