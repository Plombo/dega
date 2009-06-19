#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mast.h>
#include "avioutput.h"

int framerate, xlef, xwid, ytop, yhei, vformat;
unsigned char *videodata;
unsigned char paldata[256][3];

void MdrawCall() {
	int i;
	unsigned char *curline = 0;

	if (Mdraw.PalChange) {
		if (vformat == VFORMAT_DIB) {
			for (i = 0; i < 256; i++) {
				paldata[i][0] = (Mdraw.Pal[i] & 0x0f00) >>-(4 - 8); /* blue */
				paldata[i][1] = (Mdraw.Pal[i] & 0x00f0) << (4 - 4); /* green */
				paldata[i][2] = (Mdraw.Pal[i] & 0x000f) << (4 - 0); /* red */
			}
		} else if (vformat == VFORMAT_RGB) {
			for (i = 0; i < 256; i++) {
				paldata[i][0] = (Mdraw.Pal[i] & 0x000f) << (4 - 0); /* red */
				paldata[i][1] = (Mdraw.Pal[i] & 0x00f0) << (4 - 4); /* green */
				paldata[i][2] = (Mdraw.Pal[i] & 0x0f00) >>-(4 - 8); /* blue */
			}
		}
	}

	if (Mdraw.Line<ytop || Mdraw.Line>=ytop+yhei) return;

	if (vformat == VFORMAT_DIB) {
		curline = videodata+((yhei-1-Mdraw.Line+ytop)*xwid*3);
	} else if (vformat == VFORMAT_RGB) {
		curline = videodata+((Mdraw.Line-ytop)*xwid*3);
	}

	for (i = 0; i < xwid; i++) {
		memcpy(curline+i*3, paldata[Mdraw.Data[i+xlef]], 3);
	}
}

void MvidModeChanged() {
	framerate = (MastEx & MX_PAL) ? 50 : 60;
}

void MvidMovieStopped() {}

int AVIOutputRun(char *romFile, char *movieFile, int additionalFrames, int osd, int sndrate, int _vformat,
		AVIOutputInitCB initcb, void *initcbData,
		AVIOutputVideoFrameCB vframecb, void *vframecbData,
		AVIOutputAudioFrameCB aframecb, void *aframecbData) {
	int frames = 0, rv, i;
	unsigned char *rom;
	int romlength;

	vformat = _vformat;
	framerate = 60;

	MdrawOsdOptions = 0;
	if (osd & AVI_OSD_BUTTONS) MdrawOsdOptions |= OSD_BUTTONS;
	if (osd & AVI_OSD_FRAMECOUNT) MdrawOsdOptions |= OSD_FRAMECOUNT;

	MastInit();
	rv = MastLoadRom(romFile, &rom, &romlength);
	if (rv != 0) return rv;
	MastSetRom(rom,romlength);

	MastHardReset();

	MsndRate=sndrate;
	MsndInit();

	MastFlagsFromHeader();
	xlef = MastEx&MX_GG ? 64 : 16;
	ytop = MastEx&MX_GG ? 24 : 0;
	xwid = MastEx&MX_GG ? 160 : 256;
	yhei = MastEx&MX_GG ? 144 : 192;

	if (movieFile != 0) {
		printf("movieFile = '%s'\n", movieFile);
		frames = MvidStart(movieFile, PLAYBACK_MODE, 0, 0);
		if (frames < 0) return frames;
	}

	rv = initcb(xwid, yhei, framerate, frames+additionalFrames, initcbData);
	if (rv != 0) return rv;

	MsndLen = (MsndRate+(framerate>>1))/framerate;

	pMsndOut = malloc(MsndLen*2*2);
	videodata = malloc(xwid*yhei*3);

	for (i = 0; i < frames+additionalFrames; i++) {
		MastFrame();
		rv = vframecb(videodata, xwid*yhei*3, vframecbData);
		if (rv != 0) return rv;
		rv = aframecb(pMsndOut, MsndLen*2*2, aframecbData);
		if (rv != 0) return rv;
	}

	return 0;
}
