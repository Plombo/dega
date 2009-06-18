#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "avioutput.h"

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

int width, height, framerate;
FILE *rawa = 0, *rawv = 0;
#ifdef USE_MENCODER
Mencoder enc;
#endif
#ifdef USE_VFW
VFW enc;
VFWOptions encOpts;
#endif

static int EncInit(int _width, int _height, int _framerate, int frameCount, void *data) {
#ifdef HAS_AVI
	if (enc.output) {
		enc.width = _width;
		enc.height = _height;
		enc.fps = _framerate;

		if (AVIOpen(&enc) != 0) {
			perror("AVIOpen");
			return 1;
		}
	}
#endif

	return 0;
}

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

static int EncVideoFrame(unsigned char *videoData, int videoDataSize, void *data) {
#ifdef HAS_AVI
	if (enc.output) {
		int rv;
		rv = AVIVideoData(&enc, videoData);
		CHECKRV("AVIVideoData")
	}
#endif

	if (rawv) {
		if (!fwrite(videoData, videoDataSize, 1, rawv)) {
			perror("fwrite");
			return 1;
		}
	}

	return 0;
}

static int EncAudioFrame(short *audioData, int audioDataSize, void *data) {
#ifdef HAS_AVI
	if (enc.output) {
		int rv;
		rv = AVIAudioData(&enc, audioData);
		CHECKRV("AVIAudioData")
	}
#endif

	if (rawa) {
		if (!fwrite(audioData, audioDataSize, 1, rawa)) {
			perror("fwrite");
			return 1;
		}
	}

	return 0;
}


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
	                                      " [-- mencoder-opts...]"
#endif
	       "\n\n"
	       "  -f frames     encode specified number of frames after movie end (default: 0)\n"
	       "  -m movie.mmv  use the given movie file (without this option, encode a movie\n"
	       "                with no input and the number of frames given by -f)\n"
	       "  -a audio.raw  write raw audio data to given file\n"
	       "  -v video.raw  write raw video data to given file\n"
	       "  -b            show button presses as an overlay on each frame\n"
	       "  -n            show a frame number as an overlay on each frame\n"
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
	int i, frames = 0, osd = 0, rv, opt;

	int additionalFrames = 0;
	char *movieFile = 0;
	char *rawAudioFile = 0, *rawVideoFile = 0;
#ifdef HAS_AVI
	char *aviOutput = 0;
#endif

	while ((opt = getopt(argc, argv, "f:o:m:a:v:bn")) != -1) {
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
				osd |= AVI_OSD_BUTTONS;
				break;
			case 'n':
				osd |= AVI_OSD_FRAMECOUNT;
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

#ifdef HAS_AVI
	if (aviOutput) {
#ifdef USE_MENCODER
		enc.mencoder = 0;
#endif

		enc.channels = 2;
		enc.rate = 44100;
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
	} else {
		enc.output = 0;
	}
#endif

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

	rv = AVIOutputRun(argv[optind], movieFile, additionalFrames, osd, 44100,
#ifdef USE_VFW
		aviOutput ? VFORMAT_DIB : VFORMAT_RGB,
#else
		VFORMAT_RGB,
#endif
		EncInit, 0,
		EncVideoFrame, 0,
		EncAudioFrame, 0);
	if (rv != 0) return rv;

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

