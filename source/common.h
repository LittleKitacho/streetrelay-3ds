#include <stdlib.h>
#include <3ds.h>
#include <curl/curl.h>
#include "cJSON.h"

#define BASE_URL "http://172.20.10.11:5173"
#define VERSION "1.0.0"

typedef struct _DownloadDataMemory
{
  size_t size;
  char *buffer;
} DownloadDataMemory;

CURLcode init_request(CURL *curl, const char *path, const char *authHeader, struct curl_slist **headers);

size_t downloadDataCallback(void *data, size_t size, size_t nmemb, void *handle);

CURLcode apiGet(const char *path, int pathLen, const char *authHeader, long *status, cJSON **response);
CURLcode apiGetRaw(const char *path, int pathLen, const char *authHeader, long *status, void **response, size_t *responseSize);
CURLcode apiPut(CURL *curl, curl_mime *mime, const char *path, int pathlen, const char *authHeader, long *status, cJSON **response);
CURLcode apiDelete(const char *path, int pathLen, const char *authHeader, long *status, cJSON **response);

void hang();
