#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <mast.h>
#include <getopt.h>
#include <sys/types.h>

#ifdef USE_MENCODER
#include "mencoder.h"
#define HAS_AVI
#define AVIOpen MencoderOpen
#define AVIVideoData MencoderVideoData
#define AVIAudioData MencoderAudioData
#endif

#ifdef USE_VFW
#include "libvfw.h"
#define HAS_AVI
#define AVIOpen VFWOpen
#define AVIVideoData VFWVideoData
#define AVIAudioData VFWAudioData
#endif

unsigned char paldata[256][3];

int xlef, ytop, xwid, yhei, framerate=60;

unsigned char *videodata;

void MdrawCall() {
	int i;
	unsigned char *curline;

	if (Mdraw.PalChange) {
		for (i = 0; i < 256; i++) {
#ifdef USE_VFW
			paldata[i][0] = (Mdraw.Pal[i] & 0x0f00) >>-(4 - 8); /* blue */
			paldata[i][1] = (Mdraw.Pal[i] & 0x00f0) << (4 - 4); /* green */
			paldata[i][2] = (Mdraw.Pal[i] & 0x000f) << (4 - 0); /* red */
#else
			paldata[i][0] = (Mdraw.Pal[i] & 0x000f) << (4 - 0); /* red */
			paldata[i][1] = (Mdraw.Pal[i] & 0x00f0) << (4 - 4); /* green */
			paldata[i][2] = (Mdraw.Pal[i] & 0x0f00) >>-(4 - 8); /* blue */
#endif
		}
	}

	if (Mdraw.Line<ytop || Mdraw.Line>=ytop+yhei) return;

#ifdef USE_VFW
	curline = videodata+((yhei-1-Mdraw.Line+ytop)*xwid*3);
#else
	curline = videodata+((Mdraw.Line-ytop)*xwid*3);
#endif

	for (i = 0; i < xwid; i++) {
		memcpy(curline+i*3, paldata[Mdraw.Data[i+xlef]], 3);
	}
}

void MvidModeChanged() {
	framerate = (MastEx & MX_PAL) ? 50 : 60;
}

void MvidMovieStopped() {}

void usage(char *name) {
	printf("DegAVI -- mmv movie encoder\n"
	       "Usage:\n"
	       "%s [-f frames] [-m movie.mmv] "
#ifdef HAS_AVI
	                                     "[-o output.avi] "
#endif
	                                                     "[-a audio.raw]\n"
	       "  [-v video.raw] [-bng] rom.sms"
#ifdef USE_MENCODER
	                                      " -- mencoder-opts..."
#endif
	       "\n\n"
	       "  -f frames     encode specified number of frames after movie end (default: 0)\n"
	       "  -m movie.mmv  use the given movie file (without this option, encode a movie\n"
	       "                with no input and the number of frames given by -f)\n"
	       "  -a audio.raw  write raw audio data to given file\n"
	       "  -v video.raw  write raw video data to given file\n"
	       "  -b            show button presses as an overlay on each frame\n"
	       "  -n            show a frame number as an overlay on each frame\n"
	       "  -g            use this flag for Game Gear games\n"
	       "  rom.sms       the ROM file to load\n"
#ifdef HAS_AVI
	       "  -o output.avi the name of the encoded movie (need not be AVI). You can omit\n"
	       "                this parameter if -a and -v are given\n"
#ifdef USE_MENCODER
	       "  mencoder-opts if -o option given, supply the given options to mencoder (must\n"
	       "                at least include -oac and -ovc options)\n"
#endif
#endif
	       "\n"
#ifdef HAS_AVI
	       "Note: at least one of -a, -v or -o must be given.\n"
#else
	       "Note: at least one of -a or -v must be given.\n"
#endif
	,name);
}

int main(int argc, char **argv) {
	int i, frames = 0;

	unsigned char *rom;
	int romlength;

	int additionalFrames = 0;
	char *movieFile = 0;
	char *rawAudioFile = 0, *rawVideoFile = 0;
	FILE *rawa = 0, *rawv = 0;
#ifdef USE_MENCODER
	Mencoder enc;
#endif
#ifdef USE_VFW
	VFW enc;
	VFWOptions encOpts;
#endif
#ifdef HAS_AVI
	char *aviOutput = 0;
#endif

	int opt;
	for (opt = 0; opt < argc; opt++) {
		printf("argv[%d] = '%s'\n", opt, argv[opt]);
	}

	while ((opt = getopt(argc, argv, "f:o:m:a:v:bng")) != -1) {
		switch (opt) {
			case 'f':
				additionalFrames = atoi(optarg);
				break;
#ifdef HAS_AVI
			case 'o':
				aviOutput = optarg;
				break;
#endif
			case 'm':
				movieFile = optarg;
				break;
			case 'a':
				rawAudioFile = optarg;
				break;
			case 'v':
				rawVideoFile = optarg;
				break;
			case 'b':
				MdrawOsdOptions |= OSD_BUTTONS;
				break;
			case 'n':
				MdrawOsdOptions |= OSD_FRAMECOUNT;
				break;
			case 'g':
				MastEx |= MX_GG;
				break;
			default:
				usage(argv[0]);
				exit(1);
		}
	}

	if (!(rawAudioFile || rawVideoFile
#ifdef HAS_AVI
	                                   || aviOutput
#endif
	                                               )) {
		usage(argv[0]);
		exit(1);
	}


	if (optind >= argc) {
		usage(argv[0]);
		exit(1);
	}

	if ((rawAudioFile != 0) != (rawVideoFile != 0)) {
		usage(argv[0]);
		exit(1);
	}

	xlef = MastEx&MX_GG ? 64 : 16;
	ytop = MastEx&MX_GG ? 24 : 0;
	xwid = MastEx&MX_GG ? 160 : 256;
	yhei = MastEx&MX_GG ? 144 : 192;

	MastInit();
	MastLoadRom(argv[optind], &rom, &romlength);
	MastSetRom(rom,romlength);
	MastHardReset();

	MsndRate=44100;
	MsndInit();

	if (movieFile != 0) {
		frames = MvidStart(movieFile, PLAYBACK_MODE, 0, 0);
	}

	MsndLen=(MsndRate+(framerate>>1))/framerate;
	pMsndOut=malloc(MsndLen*2*2);
	videodata = malloc(xwid*yhei*3);

	if (rawAudioFile) {
		rawa = fopen(rawAudioFile, "wb");
		if (!rawa) {
			perror("fopen");
			return 1;
		}
	}

	if (rawVideoFile) {
		rawv = fopen(rawVideoFile, "wb");
		if (!rawv) {
			perror("fopen");
			return 1;
		}
	}

#ifdef HAS_AVI
	if (aviOutput) {
#ifdef USE_MENCODER
		enc.mencoder = 0;
#endif

		enc.width = xwid;
		enc.height = yhei;
		enc.fps = framerate;

		enc.channels = 2;
		enc.rate = MsndRate;
		enc.samplesize = 2;

#ifdef USE_MENCODER
		enc.params = argv+optind+1;
#endif
		enc.output = aviOutput;

#ifdef USE_VFW
		VFWOptionsInit(&encOpts, &enc);
		encOpts.parent = 0;
		
		if (VFWOptionsChooseAudioCodec(&encOpts) != 0) {
			puts("Unable to choose audio codec or user cancelled");
			return 1;
		}
		
		if (VFWOptionsChooseVideoCodec(&encOpts) != 0) {
			puts("Unable to choose video codec or user cancelled");
			return 1;
		}

		enc.opts = &encOpts;
#endif

		if (AVIOpen(&enc) != 0) {
			perror("AVIOpen");
			return 1;
		}
	}
#endif

	for (i = 0; i < frames+additionalFrames; i++) {
		MastFrame();

#ifdef HAS_AVI
		if (aviOutput) {
			int rv;
#ifdef USE_MENCODER
#define CHECKRV(fn) \
	if (rv == 0) { \
		puts("Buffer overrun!"); \
		return 1; \
	} \
	if (rv == -1) { \
		perror(fn); \
		return 1; \
	}
#endif
#ifdef USE_VFW
#define CHECKRV(fn) \
	if (rv != 0) { \
		puts(fn ": unable to write data"); \
		return 1; \
	}
#endif
			rv = AVIVideoData(&enc, videodata);
			CHECKRV("AVIVideoData")
			rv = AVIAudioData(&enc, pMsndOut);
			CHECKRV("AVIAudioData")
#undef CHECKRV
		}
#endif

		if (rawv) {
			if (!fwrite(videodata, xwid*yhei*3, 1, rawv)) {
				perror("fwrite");
				return 1;
			}
		}

		if (rawa) {
			if (!fwrite(pMsndOut, MsndLen*2*2, 1, rawa)) {
				perror("fwrite");
				return 1;
			}
		}
	}

#ifdef USE_MENCODER
	if (aviOutput) {
		int status;
		if (MencoderClose(&enc, &status) == -1) {
			perror("MencoderClose");
			return 1;
		} else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			printf("mencoder subprocess exited with return code %d\n", WEXITSTATUS(status));
			return WEXITSTATUS(status);
		} else if (WIFSIGNALED(status)) {
			printf("mencoder subprocess terminated with signal %d\n", WTERMSIG(status));
			return 1;
		}
	}
#endif

#ifdef USE_VFW
	if (aviOutput) {
		if (VFWClose(&enc) == -1) {
			perror("VFWClose");
			return 1;
		}
	}
#endif

	if (rawv && fclose(rawv) == EOF) {
		perror("fclose");
		return 1;
	}
	if (rawa && fclose(rawa) == EOF) {
		perror("fclose");
		return 1;
	}
	return 0;
}

