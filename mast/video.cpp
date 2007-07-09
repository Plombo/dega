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
static int vidFrameCount, rerecordCount, beginReset, vidFlags;
#define VIDFLAG_PAL   (1<<1)
#define VIDFLAG_JAPAN (1<<2)

static int stateOffset, inputOffset, packetSize;

static char videoAuthor[64];

int frameCount;

#define HEADER_SIZE 0x64
#define SAVE_STATE_SIZE 25088

static int MastAcbVideoRead(struct MastArea *area) {
	return fread(area->Data, area->Len, 1, videoFile);
}

static int MastAcbVideoWrite(struct MastArea *area) {
	return fwrite(area->Data, area->Len, 1, videoFile);
}

int MvidStart(char *filename, int mode, int reset) {
	int changed = 0, fileHeaderSize; 
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

			fread(&stateOffset, 4, 1, videoFile);
			fread(&inputOffset, 4, 1, videoFile);
			fread(&packetSize, 4, 1, videoFile);

			if (stateOffset==0 && !reset) {
				stateOffset = 0x60; // hardcoded constants from 1st version
			}

			if (inputOffset==0) {
				inputOffset = reset ? 0x60 : 0x60+25088; // hardcoded constants from 1st version
			}

			if (packetSize==0) {
				packetSize = 2; // hardcoded constants from 1st version
			}

			fread(videoAuthor, sizeof(videoAuthor), 1, videoFile);

			fileHeaderSize = reset ? inputOffset : stateOffset;

			if (fileHeaderSize >= 0x64) {
				fread(&vidFlags, 4, 1, videoFile);
			} else {
				vidFlags = 0;
			}

			changed = ((vidFlags & VIDFLAG_PAL) != 0) ^ ((MastEx & MX_PAL) != 0);

			if (vidFlags & VIDFLAG_PAL) {
				MastEx |= MX_PAL;
			} else {
				MastEx &= ~MX_PAL;
			}
			if (changed) MvidModeChanged();

			if (vidFlags & VIDFLAG_JAPAN) {
				MastEx |= MX_JAPAN;
			} else {
				MastEx &= ~MX_JAPAN;
			}

			if (reset) {
				MastHardReset();
			} else {
				fseek(videoFile, stateOffset, SEEK_SET);
				MastAcb = MastAcbVideoRead;
				MastAreaDega();
			}
			fseek(videoFile, inputOffset, SEEK_SET);
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
		
			stateOffset = reset ? 0 : HEADER_SIZE;
			fwrite(&stateOffset, 4, 1, videoFile); // 0014: offset of state information

			inputOffset = reset ? HEADER_SIZE : HEADER_SIZE+SAVE_STATE_SIZE;
			fwrite(&inputOffset, 4, 1, videoFile); // 0018: offset of input data

			packetSize = sizeof(MastInput);
			fwrite(&packetSize, 4, 1, videoFile);  // 001c: size of input packet

			memset(videoAuthor, 0, sizeof(videoAuthor));
			fwrite(videoAuthor, sizeof(videoAuthor), 1, videoFile); // 0020-005f: author

			vidFlags = 0;
			if (MastEx & MX_PAL) {
				vidFlags |= VIDFLAG_PAL;
			}
			if (MastEx & MX_JAPAN) {
				vidFlags |= VIDFLAG_JAPAN;
			}
			fwrite(&vidFlags, 4, 1, videoFile);    // 0060: flags

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

	char zero = 0;
	int i;

	if (videoFile != NULL) {
		switch (videoMode) {
			case PLAYBACK_MODE:
				result = fread(MastInput, sizeof(MastInput), 1, videoFile);

				if (result == 0) {
					MvidStop();
				} else {
					fseek(videoFile, packetSize-sizeof(MastInput), SEEK_CUR);
				}

				break;
			case RECORD_MODE:
				fwrite(MastInput, sizeof(MastInput), 1, videoFile);
				for (i = 0; i < packetSize-sizeof(MastInput); i++) {
					fwrite(&zero, 1, 1, videoFile);
				}

				fseek(videoFile, 0x8, SEEK_SET);
				fwrite(&frameCount, 4, 1, videoFile);
				fseek(videoFile, 0, SEEK_END);

				break;
		}
	}
}

#ifdef unix
static void Truncate(FILE *file, int size) {
	ftruncate(fileno(file), size);
}
#else
#ifdef WIN32
static void Truncate(FILE *file, int size) {
	long oldOffset = ftell(file);
	fseek(file, size, SEEK_SET);
	SetEndOfFile((HANDLE)_get_osfhandle(_fileno(file)));
	fseek(file, oldOffset, SEEK_SET);
}
#else
#error no truncate implementation available!
#endif
#endif

static void MvidOldPostLoadState(int readonly) {
	if (videoFile != NULL) {
		int newPosition = inputOffset + frameCount*packetSize;

		if (readonly != 0 && videoMode == PLAYBACK_MODE) {
			fseek(videoFile, newPosition, SEEK_SET);
		} else {
			fclose(videoFile);

			videoFile = fopen(videoFilename, "r+b");
			Truncate(videoFile, newPosition);
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
		int size, embInputOffset, embPacketSize;
		if (frameCount == 0) {
			MvidStop();
			return;
		}

		size = inputOffset + frameCount*packetSize;

		if (readonly != 0) {
			if (videoMode == RECORD_MODE) {
				fclose(videoFile);
				videoFile = fopen(videoFilename, "rb");
				videoMode = PLAYBACK_MODE;
			}
			fseek(videoFile, size, SEEK_SET);
			return;
		}

		data = (unsigned char *)malloc(0x20);

		ma.Len = 0x20; ma.Data = data;
		if (MastAcb(&ma) == 0) { /* old-style save state without embedded video file */
			free(data);
			MvidOldPostLoadState(readonly);
			return;
		}

		embInputOffset = *(int *)(data + 0x18); if (embInputOffset == 0) embInputOffset = beginReset ? 0x60 : 0x60+25088;
		embPacketSize = *(int *)(data + 0x1c); if (embPacketSize == 0) embPacketSize = 2;

		free(data);
		data = (unsigned char *)malloc(embInputOffset - 0x20);
		ma.Len = embInputOffset - 0x20; ma.Data = data;
		MastAcb(&ma);

		free(data);
		data = (unsigned char *)malloc(frameCount*embPacketSize);
		ma.Len = frameCount*embPacketSize; ma.Data = data;
		MastAcb(&ma);

		rerecordCount++;

		vidFrameCount = frameCount;

		fclose(videoFile);
		videoFile = fopen(videoFilename, "r+b");
		Truncate(videoFile, inputOffset);

		fseek(videoFile, 0x8, SEEK_SET);
		fwrite(&frameCount, 4, 1, videoFile);
		fwrite(&rerecordCount, 4, 1, videoFile);

		packetSize = embPacketSize;
		fseek(videoFile, 0x1c, SEEK_SET);
		fwrite(&packetSize, 4, 1, videoFile);

		fseek(videoFile, 0, SEEK_END);
		fwrite(data, frameCount*embPacketSize, 1, videoFile);
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

		size = inputOffset + frameCount*packetSize;
		data = (unsigned char *)malloc(size);

		readVideoFile = fopen(videoFilename, "rb");
		fread(data, size, 1, readVideoFile);

		*(int *)(data + 0x8) = frameCount; /* TODO magic number */

		ma.Len = size; ma.Data = data; MastAcb(&ma);

		free(data);
		fclose(readVideoFile);
	}
}
