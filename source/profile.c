#include <stdlib.h>
#include <memory.h>
#include <3ds.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "profile.h"
#include "common.h"

const char *getConsoleName(u8 console)
{
  switch (console)
  {
  case CFG_MODEL_3DS:
    return "3DS";
  case CFG_MODEL_3DSXL:
    return "3DS XL";
  case CFG_MODEL_2DS:
    return "2DS";
  case CFG_MODEL_N3DS:
    return "New 3DS";
  case CFG_MODEL_N3DSXL:
    return "New 3DS XL";
  case CFG_MODEL_N2DSXL:
    return "New 2DS XL";
  }
}

bool synchronizeProfile(const char *authHeader)
{
  Result res = frdInit();
  if (R_FAILED(res))
  {
    printf("Could not init friend service: %08lX\n", res);
    return false;
  }

  MiiData *mii = malloc(sizeof(MiiData));
  if (mii == NULL)
  {
    printf("Could not allocate memory.\n");
    return false;
  }
  res = FRD_GetMyMii(mii);
  if (R_FAILED(res))
  {
    printf("Could not get Mii: %08lX\n", res);
    return false;
  }

  char *screenName = malloc(FRIEND_SCREEN_NAME_SIZE);
  if (screenName == NULL)
  {
    printf("Could not allocate memory.\n");
    return false;
  }
  res = FRD_GetMyScreenName(screenName, FRIEND_SCREEN_NAME_SIZE);
  if (R_FAILED(res))
  {
    printf("Could not get screen name: %08lX\n", res);
    return false;
  }
  frdExit();
  res = cfguInit();
  if (R_FAILED(res))
  {
    printf("Could not intialize configuration service: %08lX\n", res);
    return false;
  }
  u8 console;
  res = CFGU_GetSystemModel(&console);
  if (R_FAILED(res))
  {
    printf("Could not get system model: %08lX\n", res);
    return false;
  }
  cfguExit();

  CURL *curl = curl_easy_init();
  struct curl_slist *headers;
  CURLcode cres = init_request(curl, BASE_URL "/me/profile", authHeader, &headers);
  if (cres != CURLE_OK)
  {
    printf("Could not initialize request.\n");
    return false;
  }

  curl_mime *mime = curl_mime_init(curl);
  curl_mimepart *part = curl_mime_addpart(mime);

  cres = curl_mime_name(part, "nickname");
  if (cres != CURLE_OK)
  {
    printf("Could not add form part: %i\n", cres);
    return false;
  }
  cres = curl_mime_data(part, screenName, strlen(screenName));
  if (cres != CURLE_OK)
  {
    printf("Could not add form part: %i\n", cres);
    return false;
  }
  curl_mime_type(part, "text/plain");

  part = curl_mime_addpart(mime);
  cres = curl_mime_name(part, "console");
  if (cres != CURLE_OK)
  {
    printf("Could not add form part: %i\n", cres);
    return false;
  }
  const char *consoleStr = getConsoleName(console);
  cres = curl_mime_data(part, consoleStr, strlen(consoleStr));
  if (cres != CURLE_OK)
  {
    printf("Could not add form part: %i\n", cres);
    return false;
  }
  curl_mime_type(part, "text/plain");

  part = curl_mime_addpart(mime);
  cres = curl_mime_name(part, "mii");
  if (cres != CURLE_OK)
  {
    printf("Could not add form part: %i\n", cres);
    return false;
  }
  cres = curl_mime_data(part, (char *)mii, sizeof(MiiData));
  if (cres != CURLE_OK)
  {
    printf("Could not add form part: %i\n", cres);
    return false;
  }
  curl_mime_type(part, "application/octet-stream");

  cres = curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
  if (cres != CURLE_OK)
  {
    printf("Could not set form data: %i\n", cres);
    return false;
  }
  cres = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  if (cres != CURLE_OK)
  {
    printf("Could not set request type: %i\n", cres);
    return false;
  }

  DownloadDataMemory responseData;
  responseData.buffer = malloc(1);
  responseData.size = 0;
  cres = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, downloadDataCallback);
  if (cres != CURLE_OK)
  {
    printf("Could not configure request: %i\n", cres);
    return false;
  }
  cres = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
  if (cres != CURLE_OK)
  {
    printf("Could not configure request: %i\n", cres);
    return false;
  }

  cres = curl_easy_perform(curl);
  if (cres != CURLE_OK)
  {
    printf("Could not perform request: %i\n", cres);
    return false;
  }
  curl_slist_free_all(headers);
  long status;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  if (status != 200)
  {
    json_object *response = json_tokener_parse(responseData.buffer);
    if (response == NULL)
    {
      printf("An unknown error has occured.\n");
      return false;
    }
    json_object *errMsg = json_object_object_get(response, "message");
    if (errMsg == NULL)
    {
      printf("An unknown error has occured.");
      return false;
    }
    printf("An error occurred while synchronizing your profile data: %s\n", json_object_get_string(errMsg));
    free(mii);
    free(screenName);
    curl_easy_cleanup(curl);
    curl_mime_free(mime);
    json_object_put(response);
    return false;
  }

  free(mii);
  free(screenName);
  curl_easy_cleanup(curl);
  curl_mime_free(mime);
  return true;
}
