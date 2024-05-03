#include <3ds/result.h>

char *get_auth_header(const char *token, size_t tokenSize);

bool auth_perform_refresh(const char *auth_header, char **new_token, off_t *new_token_size);
