#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <mast.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>

unsigned char paldata[256][3];

int mencoder_pid;
int videofd, audiofd;

int xlef, ytop, xwid, yhei, framerate=60;

struct FifoElem {
	unsigned char *DataCur, *DataStart;
	int Remaining;
};

#define FIFO_ELEMS 1024

typedef struct FifoElem Fifo[FIFO_ELEMS];

void FifoInit(Fifo fifo) {
	int i;
	for (i = 0; i < FIFO_ELEMS; i++) {
		fifo[i].DataCur = 0;
		fifo[i].DataStart = 0;
		fifo[i].Remaining = 0;
	}
}

void *FifoPush(Fifo fifo, int size) {
	if (fifo[FIFO_ELEMS-1].DataCur != 0) {
		puts("Buffer overrun!");
		return 0;
	}
	void *data = malloc(size);
	int i;
	for (i = 0; i < FIFO_ELEMS; i++) {
		if (fifo[i].DataCur == 0) {
			fifo[i].DataCur = fifo[i].DataStart = data;
			fifo[i].Remaining = size;
			return data;
		}
	}
	free(data);
	puts("Buffer overrun!");
	return 0;
}

void FifoPop(Fifo fifo) {
	int i;

	free(fifo[0].DataStart);
	for (i = 0; i < FIFO_ELEMS-1; i++) {
		fifo[i].DataCur = fifo[i+1].DataCur;
		fifo[i].DataStart = fifo[i+1].DataStart;
		fifo[i].Remaining = fifo[i+1].Remaining;
		if (fifo[i+1].DataCur == 0) break;
	}

	fifo[FIFO_ELEMS-1].DataCur = fifo[FIFO_ELEMS-1].DataStart = 0;
	fifo[FIFO_ELEMS-1].Remaining = 0;
}

int FifoFlush(Fifo fifo, int fd) {
	int size;
	
	if (fifo[0].DataCur == 0) {
		return 0;
	}

	size = fifo[0].Remaining;

	while (1) {
		int newsize = write(fd, fifo[0].DataCur, size);
		if (newsize == -1 && errno == EAGAIN) {
			size /= 2;
		} else {
			size = newsize;
			break;
		}
	}
	if (size == fifo[0].Remaining) {
		FifoPop(fifo);
	} else {
		fifo[0].DataCur += size;
		fifo[0].Remaining -= size;
	}

	return 1;
}

Fifo audiofifo, videofifo;

unsigned char *videodata;

void MdrawCall() {
	int i;

	if (Mdraw.PalChange) {
		for (i = 0; i < 256; i++) {
			paldata[i][0] = (Mdraw.Pal[i] & 0007) << (5 - 0); /* red */
			paldata[i][1] = (Mdraw.Pal[i] & 0070) << (5 - 3); /* green */
			paldata[i][2] = (Mdraw.Pal[i] & 0700) >>-(5 - 6); /* blue */
		}
	}

	if (Mdraw.Line<ytop || Mdraw.Line>=ytop+yhei) return;

	for (i = 0; i < xwid; i++) {
		memcpy(videodata+((Mdraw.Line-ytop)*xwid*3)+i*3, paldata[Mdraw.Data[i+xlef]], 3);
	}
}

void MvidModeChanged() {
	framerate = (MastEx & MX_PAL) ? 50 : 60;
}

void MvidMovieStopped() {}

int spawn_mencoder(char *params, char *output) {
	int videofds[2], audiofds[2], mencoder_pid;

	pipe(videofds);
	pipe(audiofds);

	if ((mencoder_pid = fork()) == 0) {
		close(videofds[1]);
		close(audiofds[1]);

		char cmdline[512];
		snprintf(cmdline, 512, "mencoder -demuxer rawvideo -rawvideo format=0x52474218:w=%d:h=%d:fps=%d -audio-demuxer rawaudio -rawaudio channels=2:rate=44100:samplesize=2 -audiofile /dev/fd/%d %s -o %s /dev/fd/%d", xwid, yhei, framerate, audiofds[0], params, output, videofds[0]);

		execl("/bin/sh", "sh", "-c", cmdline, (char *) NULL);
	} else {
		close(videofds[0]);
		close(audiofds[0]);

		videofd = videofds[1];
		audiofd = audiofds[1];

		fcntl(videofd, F_SETFL, O_NONBLOCK);
		fcntl(audiofd, F_SETFL, O_NONBLOCK);
	}

	return mencoder_pid;
}

void usage(char *name) {
	printf("DegAVI -- mmv movie encoder for Unix\n"
	       "Usage:\n"
	       "%s [-f frames] [-o options] [-m movie.mmv] [-a audio.raw]\n"
	       "  [-v video.raw] [-bng] rom.sms movie.avi\n"
	       "\n"
	       "  -f frames     encode specified number of frames after movie end (default: 0)\n"
	       "  -o options    supply the given options to mencoder (default: low quality avi)\n"
	       "  -m movie.mmv  use the given movie file (without this option, encode a movie\n"
	       "                with no input and the number of frames given by -f)\n"
	       "  -a audio.raw  write raw audio data to given file instead of passing directly\n"
	       "                to mencoder (must be used with -v)\n"
	       "  -v video.raw  write raw video data to given file instead of passing directly\n"
	       "                to mencoder (must be used with -a)\n"
	       "  -b            show button presses as an overlay on each frame\n"
	       "  -n            show a frame number as an overlay on each frame\n"
	       "  -g            use this flag for Game Gear games\n"
	       "  rom.sms       the ROM file to load\n"
	       "  movie.avi     the name of the encoded movie (needs not be AVI). You can omit\n"
	       "                this parameter if -a and -v are given\n",
	name);
}

int main(int argc, char **argv) {
	int i, frames = 0, status;

	unsigned char *rom;
	int romlength;

	int additionalFrames = 0;
	char *mencoderOptions = "-oac mp3lame -ovc xvid -xvidencopts \"bitrate=200:me_quality=1:gmc:aspect=4/3\" -lameopts preset=30:aq=9 -noskip -mc 0", *movieFile = 0;
	char *rawAudioFile = 0, *rawVideoFile = 0;
	int opt;

	while ((opt = getopt(argc, argv, "f:o:m:a:v:bng")) != -1) {
		switch (opt) {
			case 'f':
				additionalFrames = atoi(optarg);
				break;
			case 'o':
				mencoderOptions = optarg;
				break;
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

	if (optind+!rawAudioFile >= argc) {
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

	FifoInit(audiofifo);
	FifoInit(videofifo);

	MastInit();
	MastLoadRom(argv[optind], &rom, &romlength);
	MastSetRom(rom,romlength);
	MastHardReset();

	MsndRate=44100;
	MsndInit();

	if (movieFile != 0) {
		frames = MvidStart(movieFile, PLAYBACK_MODE, 0);
	}

	MsndLen=(MsndRate+(framerate>>1))/framerate;
	pMsndOut=malloc(MsndLen*2*2);

	if (rawAudioFile && rawVideoFile) {
		videofd = open(rawVideoFile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (videofd == -1) {
			perror("open");
			exit(1);
		}

		audiofd = open(rawAudioFile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (audiofd == -1) {
			perror("open");
			exit(1);
		}

		fcntl(videofd, F_SETFL, O_NONBLOCK);
		fcntl(audiofd, F_SETFL, O_NONBLOCK);
	} else {
		mencoder_pid = spawn_mencoder(mencoderOptions, argv[optind+1]);
	}

	for (i = 0; i < frames+additionalFrames; i++) {
		int progress;
		pMsndOut = FifoPush(audiofifo, MsndLen*2*2);
		videodata = FifoPush(videofifo, xwid*yhei*3);

		MastFrame();

		do {
			fd_set fds;
			progress = 0;

			FD_ZERO(&fds);
			FD_SET(videofd, &fds);
			FD_SET(audiofd, &fds);
			select(videofd>audiofd ? videofd+1 : audiofd+1, 0, &fds, 0, 0);

			if (FD_ISSET(videofd, &fds)) {
				if (FifoFlush(videofifo, videofd) != 0) {
					progress = 1;
				}
			}
			
			if (FD_ISSET(audiofd, &fds)) {
				if (FifoFlush(audiofifo, audiofd) != 0) {
					progress = 1;
				}
			}
		} while (progress);
	}

	/* Flush any remaining fifos and close them */

	if (videofifo[0].DataCur == 0) {
		close(videofd);
	}

	if (audiofifo[0].DataCur == 0) {
		close(audiofd);
	}

	while (videofifo[0].DataCur != 0 || audiofifo[0].DataCur != 0) {
		fd_set fds;
		struct timeval timeout;

		int max = 0;
		FD_ZERO(&fds);
		if (videofifo[0].DataCur != 0) {
			FD_SET(videofd, &fds);
			max = videofd;
		}
		if (audiofifo[0].DataCur != 0) {
			FD_SET(audiofd, &fds);
			if (max < audiofd) max = audiofd;
		}

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		if (select(max+1, 0, &fds, 0, &timeout) == 0) {
			break; /* timeout */
		}

		if (FD_ISSET(videofd, &fds)) {
			FifoFlush(videofifo, videofd);
			if (videofifo[0].DataCur == 0) {
				close(videofd);
			}
		}

		if (FD_ISSET(audiofd, &fds)) {
			FifoFlush(audiofifo, audiofd);
			if (audiofifo[0].DataCur == 0) {
				close(audiofd);
			}
		}
	}

	if (videofifo[0].DataCur != 0) {
		close(videofd);
	}

	if (audiofifo[0].DataCur != 0) {
		close(audiofd);
	}

	waitpid(mencoder_pid, &status, 0);
	return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

