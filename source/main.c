#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>

#include <curl/curl.h>
#include <3ds.h>

#include "3ds/services/cecd.h"
#include "auth.h"
#include "common.h"

bool running = false;

int main(int argc,char*argv[]) {
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	u32 *socBuffer = memalign(0x1000, 0x100000);
	if (socBuffer == NULL) {
		printf("Could not allocate memory\n");
		hang();
		goto exit;
	}
	socInit(socBuffer, 0x100000);

	curl_global_init(CURL_GLOBAL_ALL);

	Result res;

	res = cecdInit();
	if (R_FAILED(res)) {
		printf("Could not initialize CECD module: 0x%08x.\n", (unsigned int)res);
		hang();
		goto exit;
	}

  mkdir("sdmc:/3ds/streetrelay", 777);
	struct stat fileStat;
	int exists = stat("sdmc:/3ds/streetrelay/cred.txt", &fileStat);
	char* token = NULL;
	
	if (exists == 0 && fileStat.st_size != 0) {
		// file exists
		// read file
		FILE *file = fopen("sdmc:/3ds/streetrelay/cred.txt", "r");
		if (file == NULL) {
			printf("could not open file\n");
			// TODO prompt login
			hang();
			goto exit;
		}
		char* oldToken = malloc(fileStat.st_size);
		fread(oldToken, fileStat.st_size, 1, file);
		fclose(file);
		// attempt login
		bool success;
		AuthResult res = refreshToken(oldToken, fileStat.st_size, &success, token);
		free(oldToken);
		if (res & AUTH_CURL) {
			printf("cURL error [%d]", res ^ AUTH_CURL);
			hang();
			goto exit;
		} if (res == AUTH_UPDATE_REQUIRED) {
			printf("Update required. Download the latest version of StreetRelay (3DS) from GitHub to use StreetRelay.");
			hang();
			goto exit;
		} if (res == AUTH_OUT_OF_MEM) {
			printf("Out of memory");
			hang();
			goto exit;
		}

		if (success) {
			printf("login success!");
			hang();
			goto exit;
		} else {
			// prompt login
			printf("token invalid");
			hang();
			goto exit;
		}
	} else {
		// file does not exist
		// prompt login
		printf("credential file does not exist");
		hang();
		goto exit;
	}
	
	exit:
	curl_global_cleanup();
	socExit();
	free(socBuffer);
	cecdExit();
	gfxExit();
	return 0;
}
