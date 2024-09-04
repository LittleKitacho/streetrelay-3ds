#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <3ds.h>
#include <curl/curl.h>
#include "common.h"
#include "cJSON.h"

CURLcode init_request(CURL *curl, const char *url, const char *authHeader, struct curl_slist **headers)
{
  *headers = curl_slist_append(NULL, authHeader);
  curl_slist_append(*headers, "User-Agent: streetrelay-3ds/" VERSION);
  curl_slist_append(*headers, "Accept: application/json");
  curl_slist_append(*headers, "Accept: application/octet-stream");

  CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, url);
  if (res != CURLE_OK)
    return res;

  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
  if (res != CURLE_OK)
    return res;
  return curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
}

size_t downloadDataCallback(void *data, size_t size, size_t nmemb, void *handle)
{
  DownloadDataMemory *memory = (DownloadDataMemory *)handle;

  size_t available = size * nmemb;
  void *ptr = realloc(memory->buffer, memory->size + available + 1);
  if (ptr == NULL)
  {
    printf("Ran out of memory while downloading data!\n");
    return 0;
  }

  memcpy(&(ptr[memory->size]), data, available);
  memory->buffer = ptr;
  memory->size += available;
  memory->buffer[memory->size] = 0;
  return available;
}

void hang()
{
  while (aptMainLoop())
  {
    gspWaitForVBlank();
    hidScanInput();
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START)
      break;
    gfxFlushBuffers();
    gfxSwapBuffers();
  }
}

typedef struct
{
  CURL *curl;
  DownloadDataMemory data;
} ApiRequestConfig;

CURLcode apiInit(CURL *curl, const char *path, int pathLen, const char *authHeader, struct curl_slist **headers, DownloadDataMemory *download)
{
  *headers = curl_slist_append(*headers, authHeader);
  *headers = curl_slist_append(*headers, "User-Agent: streetrelay-3ds/" VERSION);
  *headers = curl_slist_append(*headers, "Accept: application/json");
  *headers = curl_slist_append(*headers, "Accept: application/octet-stream");

  // create full api path
  int fullPathLen = sizeof(BASE_URL) + pathLen;
  char *fullPath = malloc(fullPathLen + 1);
  strncpy(fullPath, BASE_URL, fullPathLen);
  strncat(fullPath, path, pathLen);

  // setup data download object
  download->buffer = malloc(1);
  download->size = 0;

  // setup curl object
  CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, fullPath);
  free(fullPath);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, download);
  if (res != CURLE_OK)
    return res;
  res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, downloadDataCallback);
  if (res != CURLE_OK)
    return res;
}

/**
 * needs path to get from and auth header
 * returns status code and json data
 */
CURLcode apiGet(const char *path, int pathLen, const char *authHeader, long *status, cJSON **response)
{
  // init curl
  CURL *curl = curl_easy_init();
  struct curl_slist *headers = NULL;
  DownloadDataMemory download = {0, NULL};

  CURLcode res = apiInit(curl, path, pathLen, authHeader, &headers, &download);
  if (res != CURLE_OK)
    goto cleanup;

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    goto cleanup;

  // parse json response and extract status code
  res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, status);
  *response = cJSON_Parse(download.buffer);

// cleanup
cleanup:
  free(download.buffer);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  return res;
}

CURLcode apiGetRaw(const char *path, int pathLen, const char *authHeader, long *status, void **response, size_t *responseSize)
{
  // init curl
  CURL *curl = curl_easy_init();
  struct curl_slist *headers = NULL;
  DownloadDataMemory download = {0, NULL};

  CURLcode res = apiInit(curl, path, pathLen, authHeader, &headers, &download);
  if (res != CURLE_OK)
    goto cleanup;

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    free(download.buffer);
    goto cleanup;
  }

  res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, status);
  *response = download.buffer;
  *responseSize = download.size;

cleanup:
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  return res;
}

CURLcode apiPut(CURL *curl, curl_mime *mime, const char *path, int pathLen, const char *authHeader, long *status, cJSON **response)
{
  struct curl_slist *headers = NULL;
  DownloadDataMemory download;

  CURLcode res = apiInit(curl, path, pathLen, authHeader, &headers, &download);
  if (res != CURLE_OK)
    goto cleanup;

  res = curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
  if (res != CURLE_OK)
    goto cleanup;
  res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  if (res != CURLE_OK)
    goto cleanup;

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    goto cleanup;

  if (status != NULL)
    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, status);
  if (response != NULL)
    *response = cJSON_ParseWithLength(download.buffer, download.size);

cleanup:
  free(download.buffer);
  curl_easy_cleanup(curl);
  curl_mime_free(mime);
  curl_slist_free_all(headers);

  return res;
}

CURLcode apiDelete(const char *path, int pathLen, const char *authHeader, long *status, cJSON **response)
{
  // todo
  CURL *curl = curl_easy_init();
  struct curl_slist *headers = NULL;
  DownloadDataMemory download = {0, NULL};

  CURLcode res = apiInit(curl, path, pathLen, authHeader, &headers, &download);
  if (res != CURLE_OK)
    goto cleanup;

  res = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  if (res != CURLE_OK)
    goto cleanup;

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    goto cleanup;

  res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, status);
  *response = cJSON_Parse(download.buffer);

// cleanup
cleanup:
  free(download.buffer);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  return res;
}
