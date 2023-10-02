#include <3ds/result.h>

typedef enum {
  AUTH_CURL = 0x100,
  AUTH_UPDATE_REQUIRED = 0x200,
  AUTH_OUT_OF_MEM = 0x300,
  AUTH_SERVER_ERROR = 0x400,
  AUTH_INVALID_TOKEN_RETURN = 0x500
} AuthResult;

AuthResult refreshToken(const char* oldToken, size_t oldTokenSize, bool* succeeded, char* newToken);
