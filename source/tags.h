#include <stdlib.h>
#include <3ds.h>
#include <3ds/services/cecd.h>

typedef struct
{
  CecBoxInfoHeader header;
  CecMessageHeader messages[];
} CecBoxInfoFull;

bool synchronizeData(char *authHeader);