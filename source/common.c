#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <3ds.h>
#include <curl/curl.h>
#include "common.h"

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
    printf("Ran out of memory while downloading data!");
    return 0;
  }

  memcpy(&(memory->buffer[memory->size]), data, available);
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
