#include "mastint.h"
#include <stdio.h>
#ifdef unix
#include <unistd.h>
#include <sys/types.h>
#endif
#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif

static FILE *videoFile = NULL;
static char videoFilename[256] = "";
static int videoMode = 0;
static int vidFrameCount, rerecordCount, beginReset;

static char videoAuthor[64];

int frameCount;

#define HEADER_SIZE 0x60
#define SAVE_STATE_SIZE 25088

static int MastAcbVideoRead(struct MastArea *area) {
	return fread(area->Data, area->Len, 1, videoFile);
}

static int MastAcbVideoWrite(struct MastArea *area) {
	return fwrite(area->Data, area->Len, 1, videoFile);
}

int MvidStart(char *filename, int mode, int reset) {
	vidFrameCount = 0;

	if (videoFile != NULL) {
		fclose(videoFile);
		videoFile = NULL;
	}

	videoMode = mode;
	strncpy(videoFilename, filename, sizeof(videoFilename)); videoFilename[sizeof(videoFilename)-1] = 0;
	frameCount = 0;

	if (mode == PLAYBACK_MODE) {
		videoFile = fopen(filename, "rb");
		if (videoFile != NULL) {
			fseek(videoFile, 0x8, SEEK_SET);
			fread(&vidFrameCount, 4, 1, videoFile);
			fread(&rerecordCount, 4, 1, videoFile);
			fread(&reset, 4, 1, videoFile);

			fseek(videoFile, 0x20, SEEK_SET);
			fread(videoAuthor, sizeof(videoAuthor), 1, videoFile);

			fseek(videoFile, HEADER_SIZE, SEEK_SET);
			if (reset) {
				MastHardReset();
			} else {
				MastAcb = MastAcbVideoRead;
				MastAreaDega();
			}
		}
	} else if (mode == RECORD_MODE) {
		videoFile = fopen(filename, "wb");
		if (videoFile != NULL) {
			int zero = 0;
			fwrite("MMV", 4, 1, videoFile);    // 0000: identifier
			fwrite(&MastVer, 4, 1, videoFile); // 0004: version
			fwrite(&zero, 4, 1, videoFile);    // 0008: frame count
			fwrite(&zero, 4, 1, videoFile);    // 000c: rerecord count
			rerecordCount = 0;
			fwrite(&reset, 4, 1, videoFile);   // 0010: begin from reset?
		
			fwrite(&zero, 4, 1, videoFile);    // 0014-001f: reserved
			fwrite(&zero, 4, 1, videoFile);
			fwrite(&zero, 4, 1, videoFile);

			memset(videoAuthor, 0, sizeof(videoAuthor));
			fwrite(videoAuthor, sizeof(videoAuthor), 1, videoFile); // 0020-005f: author

			if (reset) {
				MastHardReset();
			} else {
				MastAcb = MastAcbVideoWrite;
				MastAreaDega();
			}
		}
	}

	if (videoFile == NULL) {
		perror("while opening video file: fopen");
	}

	beginReset = reset;

	return vidFrameCount;
}

int MvidGetFrameCount() {
	return vidFrameCount;
}

int MvidGetRerecordCount() {
	return rerecordCount;
}

void MvidStop() {
	if (videoFile != NULL) {
		MastInput[0] = MastInput[1] = 0;
		fclose(videoFile);
		videoFile = NULL;
	}
}

int MvidSetAuthor(char *author) {
	FILE *rwVideoFile;

	if (*videoFilename == 0) {
		return 0;
	}

	memset(videoAuthor, 0, sizeof(videoAuthor));
	strncpy(videoAuthor, author, sizeof(videoAuthor));

	rwVideoFile = fopen(videoFilename, "r+b");
	fseek(rwVideoFile, 0x20, SEEK_SET);
	fwrite(videoAuthor, sizeof(videoAuthor), 1, rwVideoFile);
	fclose(rwVideoFile);

	return 1;
}

char *MvidGetAuthor() {
	return videoAuthor;
}

int MvidGotProperties() {
	return (*videoFilename != 0);
}

int MvidInVideo() {
	return (videoFile != 0);
}

void MvidPreFrame() {
	int result;
	frameCount++;

	if (videoFile != NULL) {
		switch (videoMode) {
			case PLAYBACK_MODE:
				result = fread(MastInput, sizeof(MastInput), 1, videoFile);

				if (result == 0) {
					MvidStop();
				}

				break;
			case RECORD_MODE:
				fwrite(MastInput, sizeof(MastInput), 1, videoFile);

				fseek(videoFile, 0x8, SEEK_SET);
				fwrite(&frameCount, 4, 1, videoFile);
				fseek(videoFile, 0, SEEK_END);

				break;
		}
	}
}

static void MvidOldPostLoadState(int readonly) {
	if (videoFile != NULL) {
		int newPosition = HEADER_SIZE + (beginReset ? 0 : SAVE_STATE_SIZE) + frameCount*sizeof(MastInput);

		if (readonly != 0 && videoMode == PLAYBACK_MODE) {
			fseek(videoFile, newPosition, SEEK_SET);
		} else {
			fclose(videoFile);

			videoFile = fopen(videoFilename, "r+b");
#ifdef unix
			ftruncate(fileno(videoFile), newPosition);
#else
#ifdef WIN32
			fseek(videoFile, newPosition, SEEK_SET);
			SetEndOfFile((HANDLE)_get_osfhandle(_fileno(videoFile)));
#else
#error no truncate implementation available!
#endif
#endif
			rerecordCount++;

			fseek(videoFile, 0xc, SEEK_SET);
			fwrite(&rerecordCount, 4, 1, videoFile);

			fseek(videoFile, 0, SEEK_END);
			videoMode = RECORD_MODE;
		}
	}
}

void MvidPostLoadState(int readonly) {
	if (videoFile != NULL) {
		MastArea ma;
		unsigned char *data;
		int size;
		if (frameCount == 0) {
			MvidStop();
			return;
		}

		size = HEADER_SIZE + (beginReset ? 0 : SAVE_STATE_SIZE) + frameCount*sizeof(MastInput);

		if (readonly != 0 && videoMode == PLAYBACK_MODE) {
			fseek(videoFile, size, SEEK_SET);
			return;
		}

		data = (unsigned char *)malloc(size);

		ma.Len = size; ma.Data = data;
		if (MastAcb(&ma) == 0) { /* old-style save state without embedded video file */
			MvidOldPostLoadState(readonly);
			return;
		}

		rerecordCount++;
		*(int *)(data + 0xc) = rerecordCount; /* TODO magic number */

		vidFrameCount = frameCount;

		fclose(videoFile);
		videoFile = fopen(videoFilename, "wb");
		fwrite(data, size, 1, videoFile);
		free(data);

		videoMode = RECORD_MODE;
	}
}

void MvidPostSaveState() {
	if (videoFile != NULL) {
		MastArea ma;
		unsigned char *data;
		int size;
		FILE *readVideoFile;

		size = HEADER_SIZE + (beginReset ? 0 : SAVE_STATE_SIZE) + frameCount*sizeof(MastInput);
		data = (unsigned char *)malloc(size);

		readVideoFile = fopen(videoFilename, "rb");
		fread(data, size, 1, readVideoFile);

		*(int *)(data + 0x8) = frameCount; /* TODO magic number */

		ma.Len = size; ma.Data = data; MastAcb(&ma);

		free(data);
		fclose(readVideoFile);
	}
}
