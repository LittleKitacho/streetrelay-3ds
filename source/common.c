#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <3ds.h>
#include <curl/curl.h>
#include "common.h"

CURLcode initRequest(CURL *curl, const RequestMethod method, const char* path, const char* authHeader) {
  struct curl_slist *headerList = curl_slist_append(headerList, authHeader);
  curl_slist_append(headerList, "User-Agent: streetrelay-3ds/" VERSION);

  char* url = malloc(sizeof(BASE_URL) + sizeof(path));
  if (url == NULL) return -1;
  strcpy(url, BASE_URL);
  strcat(url, path);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  free(url);
  
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

  switch (method) {
    case M_GET:
      // curl_easy_setopt(curl, CURLOPT_HTTPGET, "GET");
      break;
    case M_POST:
      break;
    case M_PUT:
      break;
    case M_PATCH:
      break;
    case M_DELETE:
      break;
  }

  return curl_easy_perform(curl);
}

void hang() {
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
		gfxFlushBuffers();
		gfxSwapBuffers();
	}
}
