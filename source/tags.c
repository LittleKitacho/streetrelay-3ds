// #include "tags.h"
#include <stdlib.h>
#include <3ds.h>
#include <3ds/services/cecd.h>
#include "common.h"
#include <curl/curl.h>
#include <string.h>
#include "tags.h"
#include <memory.h>
#include "cjson.h"

// source: https://gitlab.com/3ds-netpass/netpass
// licensed under GPL-v3 or later
// see source for more details
void getCurrentTime(CecTimestamp *cts)
{
  time_t unix_time = time(NULL);
  struct tm *ts = gmtime((const time_t *)&unix_time);
  cts->hour = ts->tm_hour;
  cts->minute = ts->tm_min;
  cts->second = ts->tm_sec;
  cts->millisecond = 0;
  cts->year = ts->tm_year + 1900;
  cts->month = ts->tm_mon + 1;
  cts->day = ts->tm_mday;
  cts->weekDay = ts->tm_wday;
}

// Loop through all games, upload outbox data and public data, download incoming tags
bool synchronizeData(char *authHeader)
{
  Result res;
  // get game list
  CecMBoxListHeader *mboxlist = malloc(sizeof(CecMBoxListHeader));
  res = cecdOpenAndRead(mboxlist, sizeof(CecMBoxListHeader), NULL, 0, CEC_PATH_MBOX_LIST, CEC_READ);
  if (R_FAILED(res))
  {
    printf("Could not open and read mailbox list: %lx", res);
    return false;
  }

  char *mailbox;
  u32 mailboxID;
  for (int i = 0; i < mboxlist->numBoxes; i++)
  {
    mailbox = (char *)mboxlist->boxNames[i];
    mailboxID = strtol((const char *)mboxlist->boxNames[i], NULL, 16);

    int gameBaseURISize = 28; /* /me/games/[gameID, 16 chars] */
    char *gameBaseURI = malloc(gameBaseURISize /* /me/games/[gameID, 16 char max]*/);
    snprintf(gameBaseURI, gameBaseURISize, "/me/games/%s", mailbox);

    // get outbox info size, then read outbox info
    u32 obInfoSize;
    res = cecdOpen(mailboxID, CEC_PATH_OUTBOX_INFO, CEC_READ, &obInfoSize);
    if (R_FAILED(res))
    {
      printf("Could not open outbox index: %08lX\n%s\nPress START to continue\n", res, osStrError(res));
      hang();
      continue;
    }
    CecBoxInfoFull *obInfo = malloc(obInfoSize);
    res = cecdRead(NULL, obInfo, obInfoSize);
    if (R_FAILED(res))
    {
      printf("Could not read outbox index: %08lX\nPress START to continue\n", res);
      hang();
      continue;
    }

    if (obInfo->header.numMessages == 0)
    {
      printf("No outbox data available for %s, skipping...\n", mailbox);
      continue;
    }

    printf("Synchrnozing data for title %s...\n", mailbox);

    // initialize request
    CURL *curl = curl_easy_init();
    curl_mime *mime = curl_mime_init(curl);
    CecMessageHeader **dataPtrs = malloc(4 * obInfo->header.numMessages);

    // load all outbox messages (most every game only has one)
    for (int j = 0; j < obInfo->header.numMessages; j++)
    {
      dataPtrs[j] = malloc(obInfo->messages[j].messageSize);
      res = cecdReadMessage(mailboxID, true, obInfo->messages[j].messageId, 8, NULL, dataPtrs[j], obInfo->messages[j].messageSize);
      if (R_FAILED(res))
      {
        printf("Could not read outbox message %d for title %s: %08lX\n", j, mailbox, res);
        continue;
      }
      curl_mimepart *part = curl_mime_addpart(mime);
      curl_mime_data(part, (char *)dataPtrs[j], obInfo->messages[j].messageSize);
      curl_mime_name(part, "message");
      curl_mime_type(part, "application/octet-stream");
    }

    long status;
    cJSON *response;
    CURLcode cres = apiPut(curl, mime, gameBaseURI, gameBaseURISize, authHeader, &status, &response);
    if (cres != CURLE_OK)
    {
      printf("An error occured while performing API request: %d\nPress START to continue\n", cres);
      hang();
      continue;
    }

    // handle errors given by server
    if (status != 200)
    {
      if (cJSON_IsInvalid(response) == cJSON_False)
      {
        cJSON *msg = cJSON_GetObjectItemCaseSensitive(response, "message");
        if (cJSON_IsString(msg) == cJSON_True)
          printf("An error has occured while uploading outbox for game %s: %s\n", mailbox, msg->valuestring);
        else
          printf("An unknown error occured while uploading outbox for game %s\n", mailbox);
      }
      else
        printf("An unknown error occured while uploading outbox for game %s\n", mailbox);

      for (int j = 0; j < obInfo->header.numMessages; j++)
        free(dataPtrs[j]);
      free(obInfo);
      cJSON_Delete(response);
      continue;
    }

    // free data pointers for message data
    for (int j = 0; j < obInfo->header.numMessages; j++)
    {
      free(dataPtrs[j]);
    }

    // server responds true if game doesn't have public data - crowdsource
    if (cJSON_IsTrue(response))
    {
      // upload public data
      int publicDataURISize = gameBaseURISize + 8;
      char *publicDataURI = malloc(publicDataURISize);
      strncpy(publicDataURI, gameBaseURI, gameBaseURISize);
      strcat(publicDataURI, "/public");

      curl = curl_easy_init();
      mime = curl_mime_init(curl);

      // open and read game icon
      u32 iconSize;
      res = cecdOpen(mailboxID, CEC_MBOX_ICON, CEC_READ, &iconSize);
      if (R_FAILED(res))
      {
        printf("Could not open icon file for mailbox %s: %s (%08lX)\n", mailbox, osStrError(res), res);
        curl_easy_cleanup(curl);
      }
      char *icon = malloc(iconSize);
      res = cecdRead(NULL, icon, iconSize);
      if (R_FAILED(res))
      {
        free(icon);
        curl_easy_cleanup(curl);
        printf("Could not read icon file for mailbox %s: %s (%08lX)\n", mailbox, osStrError(res), res);
        continue;
      }

      // open and read title
      u32 titleSize;
      res = cecdOpen(mailboxID, CEC_MBOX_TITLE, CEC_READ, &titleSize);
      if (R_FAILED(res))
      {
        printf("Could not open title file for mailbox %s: %s (%08lX)\n", mailbox, osStrError(res), res);
        curl_easy_cleanup(curl);
        free(icon);
        continue;
      }
      char *title = malloc(titleSize);
      res = cecdRead(NULL, title, titleSize);
      if (R_FAILED(res))
      {
        printf("Could not read title file for mailbox %s: %s (%08lX)\n", mailbox, osStrError(res), res);
        curl_easy_cleanup(curl);
        free(title);
        free(icon);
        continue;
      }

      // build request mime data
      curl_mimepart *part = curl_mime_addpart(mime);
      cres = curl_mime_data(part, icon, iconSize);
      if (cres != CURLE_OK)
      {
        printf("Could not init request: %d\n", cres);
        free(title);
        free(icon);
        curl_mime_free(mime);
        curl_easy_cleanup(curl);
        continue;
      }
      curl_mime_name(part, "icon");
      curl_mime_type(part, "application/octet-stream");
      curl_mime_filename(part, "icon.icn");

      part = curl_mime_addpart(mime);
      curl_mime_data(part, title, titleSize);
      curl_mime_name(part, "title");
      curl_mime_type(part, "text/plain");

      cres = apiPut(curl, mime, publicDataURI, publicDataURISize, authHeader, NULL, NULL);

      free(icon);
      free(title);
    }
    cJSON_Delete(response);
    free(obInfo);

    // download tag list
    // step 1: load inbox info
    u32 inboxInfoSize;
    res = cecdOpen(mailboxID, CEC_PATH_INBOX_INFO, CEC_READ & CEC_WRITE, &inboxInfoSize);
    if (R_FAILED(res))
    {
      printf("Could not read inbox index: %08lX\nPress START to continue\n", res);
      hang();
      continue;
    }
    CecBoxInfoFull *inboxInfo = malloc(inboxInfoSize);
    res = cecdRead(NULL, inboxInfo, inboxInfoSize);
    if (R_FAILED(res))
    {
      printf("Could not read inbox index: %08lX\nPress START to continue\n", res);
      hang();
      continue;
    }

    // step 2: check if inbox has space
    int inboxSpace = inboxInfo->header.maxNumMessages - inboxInfo->header.numMessages;
    if (inboxSpace > 0)
    {
      // open mboxinfo for hmac key
      CecMBoxInfoHeader *mboxinfo = malloc(sizeof(CecMBoxInfoHeader));
      res = cecdOpenAndRead(mboxinfo, sizeof(CecMBoxInfoHeader), NULL, mailboxID, CEC_PATH_MBOX_INFO, CEC_READ);
      if (R_FAILED(res))
      {
        printf("Could not read data: %08lu\n", res);
        hang();
        goto endgamesync;
      }

      // fetch first 10 tags (common max tag number)
      response = NULL;
      long status;
      cres = apiGet(gameBaseURI, gameBaseURISize, authHeader, &status, &response);
      if (cres != CURLE_OK)
      {
        printf("API request failed: %d\nPress START to continue\n", cres);
        hang();
        free(inboxInfo);
        if (response != NULL && cJSON_IsInvalid(response) == cJSON_False)
          cJSON_Delete(response);
        goto endgamesync;
      }
      if (status != 200)
      {
        cJSON *msg = cJSON_GetObjectItemCaseSensitive(response, "message");
        if (cJSON_IsString(msg) == cJSON_True)
        {
          printf("An error has occured while getting tags for game %s: %s\n", mailbox, msg->valuestring);
        }
        else
        {
          printf("An unknown error has occured while fetching tags for game %s\n", mailbox);
        }
        hang();
        free(inboxInfo);
        if (cJSON_IsInvalid(response) == cJSON_False)
          cJSON_Delete(response);

        goto endgamesync;
      }

      int available = cJSON_GetArraySize(response);
      cJSON *tagID;
      char *tagURI;
      int tagURISize;

      // loop until inbox is full or no more tags are available
      // available will keep being refilled with new tags from server when empty
      while (inboxSpace > 0 && available > 0)
      {
        printf("test\n");
        tagID = cJSON_GetArrayItem(response, 0);

        tagURISize = gameBaseURISize + strlen(tagID->valuestring) + 1;
        tagURI = malloc(tagURISize);
        strncpy(tagURI, gameBaseURI, tagURISize);
        strcat(tagURI, "/");
        strcat(tagURI, tagID->valuestring);

        CecMessageHeader *message;
        size_t messageSize;

        cres = apiGetRaw(tagURI, tagURISize, authHeader, &status, (void **)&message, &messageSize);
        if (cres != CURLE_OK)
        {
          printf("API request failed: %d\n", cres);
          hang();
          break;
        }
        if (status != 200)
        {
          cJSON *errResponse = cJSON_ParseWithLength((char *)message, messageSize);
          if (cJSON_IsInvalid(errResponse) == cJSON_True)
          {
            printf("An unknown error occured while communicating with the server.\n");
          }
          else
          {
            cJSON *errMsg = cJSON_GetObjectItemCaseSensitive(errResponse, "message");
            printf("Server returned an error: %s\n", errMsg->valuestring);
          }
          cJSON_Delete(errResponse);
          hang();
          break;
        }

        // todo sanity checks
        // message already in box
        // max message size
        // max box size

        // modify message
        message->unopened = 1;
        message->newFlag = 1;
        getCurrentTime(&(message->received));

        res = cecdWriteMessageWithHmac(mailboxID, false, message->messageId, 8, message, messageSize, mboxinfo->hmacKey);
        if (R_FAILED(res))
        {
          printf("failed %08lx\n", res);
          hang();
          break;
          /* todo */
        }
        res = cecdReadMessage(mailboxID, false, message->messageId, 8, NULL, NULL, 0);
        if (R_FAILED(res))
        {
          printf("Message was not written.\n");
          hang();
          break;
          /* todo */
        }

        CecBoxInfoFull *newInbox = realloc(inboxInfo, inboxInfoSize + sizeof(CecMessageHeader));
        memcpy(&(newInbox->messages[newInbox->header.numMessages]), message, sizeof(CecMessageHeader));
        newInbox->header.numMessages++;
        newInbox->header.boxSize += message->messageSize;

        inboxInfo = newInbox;
        mboxinfo->flag3 = 1;

        cJSON_DeleteItemFromArray(response, 0);
        apiDelete(tagURI, tagURISize, authHeader, &status, NULL);
        available--;
        if (available == 0)
        {
          // todo - get more tags
        }

        inboxSpace--;
      }

      res = cecdOpenAndWrite(inboxInfo, inboxInfoSize, mailboxID, CEC_PATH_INBOX_INFO, CEC_WRITE);
      if (R_FAILED(res))
      {
        printf("Failed writing new inbox data:\n%s (0x%08lx)\n", osStrError(res), res);
        hang();
        hidScanInput();
        if (hidKeysHeld() & KEY_A)
        {
          free(gameBaseURI);
          return false;
        }
      }

      res = cecdOpenAndWrite(mboxinfo, sizeof(CecMBoxInfoHeader), mailboxID, CEC_PATH_MBOX_INFO, CEC_WRITE);
    }

  endgamesync:

    free(gameBaseURI);
  }

  return true;
}
