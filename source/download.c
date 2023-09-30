// #include <stdlib.h>
// #include <stdio.h>
// #include <inttypes.h>
// #include <3ds.h>
// #include "common.h"

// Result downloadGame(char* authToken, char* tag, char* game) {
//   httpcContext getContext;

//   httpcContext doneContext;
// }

// Result downloadTag(char* authToken, char* tag) {
//   httpcContext context;
// }

// Result downloadAllTags(char* authToken) {
//   Result ret;
//   // get list of tags
//   httpcContext context;
//   sendRequest(&context, HTTPC_METHOD_GET, "https://streetrelay-web.vercel.app/me/tags", authToken, NULL);
//   if (R_FAILED(ret)) return ret;

//   u32 status;
//   httpcGetResponseStatusCode(&context, &status);
//   if (status != 200) return -1 * status;

//   u32 size;
//   httpcGetDownloadSizeState(&context, NULL, &size);
//   char* tags = malloc(size);
//   httpcDownloadData(&context, &tags, size, NULL);

//   printf(tags);
//   // for tag in list of tags
//   //   get games from tag
//   //   for game in games
//   //     if game is in my games
//   //       download game data
//   //       modify data
//   //       write data
//   //     mark data as done
//   //   mark tag as done
// }