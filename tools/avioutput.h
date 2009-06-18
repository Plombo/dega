#define VFORMAT_RGB 0
#define VFORMAT_DIB 1

#define AVI_OSD_BUTTONS 1
#define AVI_OSD_FRAMECOUNT 2

typedef int (*AVIOutputInitCB)(int width, int height, int framerate, int frameCount, void *data);
typedef int (*AVIOutputVideoFrameCB)(unsigned char *videoData, int videoDataSize, void *data);
typedef int (*AVIOutputAudioFrameCB)(short *audioData, int audioDataSize, void *data);

int AVIOutputRun(char *romFile, char *movieFile, int additionalFrames, int osd, int sndrate, int _vformat,
		AVIOutputInitCB initcb, void *initcbData,
		AVIOutputVideoFrameCB vframecb, void *vframecbData,
		AVIOutputAudioFrameCB aframecb, void *aframecbData);
