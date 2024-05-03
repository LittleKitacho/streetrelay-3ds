#include <stdlib.h>
#include <3ds.h>
#include <curl/curl.h>

#define BASE_URL "http://192.168.68.117:5173"
#define VERSION "1.0.0"

typedef struct _DownloadDataMemory
{
  size_t size;
  char *buffer;
} DownloadDataMemory;

CURLcode init_request(CURL *curl, const char *path, const char *authHeader, struct curl_slist **headers);

size_t downloadDataCallback(void *data, size_t size, size_t nmemb, void *handle);

void hang();
