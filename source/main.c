#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>

#include <curl/curl.h>
#include <3ds.h>
#include "3ds/services/cecd.h"

#include "common.h"
#include "storage.h"
#include "auth.h"
#include "profile.h"

bool running = false;

int main(int argc, char *argv[])
{
	char *token = NULL;
	char *authHeader = NULL;
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
	gfxSetDoubleBuffering(GFX_TOP, true);

	u32 *socBuffer = memalign(0x1000, 0x100000);
	if (socBuffer == NULL)
	{
		printf("Could not allocate memory\n");
		hang();
		goto exit;
	}
	socInit(socBuffer, 0x100000);

	curl_global_init(CURL_GLOBAL_ALL);

	Result res;

	res = cecdInit();
	if (R_FAILED(res))
	{
		printf("Could not initialize CECD module: 0x%08x.\n", (unsigned int)res);
		hang();
		goto exit;
	}

	// load token from file
	off_t token_size;

	char *old_token;
	off_t old_token_size;
	if (!load_token(&old_token, &old_token_size))
	{
		hang();
		goto exit;
	}
	if (old_token_size != 0)
	{
		goto login;
	}

scanCode:
	printf("Could not load token.");
	hang();
	goto exit;
	// scanLoginCode();

login:
	char *old_auth_header = get_auth_header(old_token, old_token_size);
	free(old_token);
	if (!auth_perform_refresh(old_auth_header, &token, &token_size))
	{
		free(old_auth_header);
		printf("Press A to scan a new token code, press START to exit.\n");
		while (1)
		{
			if (!aptMainLoop())
				goto exit;
			hidScanInput();
			u32 kDown = hidKeysDown();
			if (kDown & KEY_START)
				goto exit;
			if (kDown & KEY_A)
				goto scanCode;
		}
	}
	free(old_auth_header);

	// store new token
	store_token(token, token_size);
	authHeader = get_auth_header(token, token_size);
	free(token);
	token = NULL;

	// upload profile
	// TODO - get information from login about stale profile information
	if (!synchronizeProfile(authHeader))
	{
		printf("Press START to exit.");
		hang();
		goto exit;
	}

	// if (!synchronize_data())
	// {
	// 	hang();
	// 	goto exit;
	// }

exit:
	free(token);
	free(authHeader);
	curl_global_cleanup();
	cecdExit();
	socExit();
	gfxExit();
	return 0;
}
