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

  mkdir("streetrelay", 777);

	FILE* file;
	int exists = access("streetrelay/cred.txt", F_OK);
	if (exists == 0) {
		file = fopen("streetrelay/cred.txt", "r+");
	} else {
		file = fopen("streetrelay/cred.txt","w+");
	}
	
	if (file == NULL) {
		printf("Could not open file");
		hang();
		goto exit;
	}

	fseek(file, 0, SEEK_END);
	off_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size == 0) {
		printf("cred.txt is empty\n");
		hang();
		goto exit;
	}

	char* oldToken = malloc(size);
	size_t read = fread(oldToken, size, 1, file);

	if (read != size) {
		if (feof(file)) {
			printf("Unexpected EOF in cred.txt");
			hang();
			goto exit;
		} else if (ferror(file)) {
			printf("Error while reading cred.txt");
			hang();
			goto exit;
		}
	}
	fclose(file);

	char* authHeader = getAuthHeader(oldToken, size);
	if (authHeader == NULL) {
		printf("Out of memory\n");
		hang();
		goto exit;
	}
	free(oldToken);

	CURL* curl = curl_easy_init();
	struct curl_slist *headers;
	if (curl == NULL) {
		printf("Out of memory\n");
		hang();
		goto exit;
	}
	initRequest(curl, M_GET, "/login", authHeader, headers);
	CURLcode ret = curl_easy_perform(curl);
	curl_slist_free_all(headers);

	if (ret != CURLE_OK) {
		printf("An error occured: CURL %d\n", ret);
		hang();
		goto exit;
	}
	
	long responseCode;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
	printf("Response code: %lu\n", responseCode);
	curl_easy_cleanup(curl);
	hang();
	
	exit:
	free(authHeader);
	curl_global_cleanup();
	socExit();
	free(socBuffer);
	cecdExit();
	gfxExit();
	return 0;
}
