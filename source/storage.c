#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <3ds.h>
#include "storage.h"
#include <stdlib.h>

bool load_token(char **token, off_t *token_size)
{
  mkdir("sdmc:/3ds/streetrelay", 777);
  struct stat tokenStat;
  int exists = stat("sdmc:/3ds/streetrelay/cred.txt", &tokenStat);

  if (exists != 0 || tokenStat.st_size == 0)
  {
    return true;
  }

  FILE *file = fopen("sdmc:/3ds/streetrelay/cred.txt", "r");
  if (file == NULL)
  {
    printf("Could not open credential file.\nPress any key to exit");
    return false;
  }

  *token_size = tokenStat.st_size;
  *token = malloc(*token_size);
  fread(*token, 1, *token_size, file);
  fclose(file);

  return true;
}

bool store_token(const char *token, off_t token_size)
{
  FILE *file = fopen("sdmc:/3ds/streetrelay/cred.txt", "w");
  fwrite(token, 1, token_size, file);
  fflush(file);
  fclose(file);
  return true;
}
