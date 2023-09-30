#include <stdlib.h>
#include <3ds.h>
#include <curl/curl.h>

#define BASE_URL "http://192.168.68.101:5173"
#define VERSION "1.0.0"

typedef enum {
  RR_OUT_OF_MEMORY,
} RequestResult;

typedef enum {
  M_GET,
  M_POST,
  M_PUT,
  M_PATCH,
  M_DELETE
} RequestMethod;

char* getAuthHeader(char* token, size_t tokenSize);

RequestResult initRequest(CURL *curl, const RequestMethod method, const char* path, const char* authHeader, struct curl_slist *headerList);

void hang();
