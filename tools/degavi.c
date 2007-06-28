#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <mast.h>
#include <getopt.h>

unsigned char paldata[256][3];

int mencoder_pid;
int videofd, audiofd;

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
	int size, newsize;
	
	if (fifo[0].DataCur == 0) {
		return 0;
	}

	size = fifo[0].Remaining;

	while (1) {
		int newsize = write(fd, fifo[0].DataCur, size);
		if (newsize == 0 && errno == EAGAIN) {
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

	for (i = 0; i < sizeof(Mdraw.Data); i++) {
		memcpy(videodata+(Mdraw.Line*sizeof(Mdraw.Data)*3)+i*3, paldata[Mdraw.Data[i]], 3);
	}
}

int spawn_mencoder(char *params, char *output) {
	int videofds[2], audiofds[2], mencoder_pid;

	pipe(videofds);
	pipe(audiofds);

	if ((mencoder_pid = fork()) == 0) {
		close(videofds[1]);
		close(audiofds[1]);

		char cmdline[512];
		snprintf(cmdline, 512, "mencoder -demuxer rawvideo -rawvideo format=0x52474218:w=288:h=192:fps=60 -audio-demuxer rawaudio -rawaudio channels=2:rate=44100:samplesize=2 -audiofile /dev/fd/%d %s -o %s /dev/fd/%d", audiofds[0], params, output, videofds[0]);

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
	       "%s [-f frames] [-o options] [-m movie.mmv] rom.sms movie.avi\n"
	       "  -f frames     encode specified number of frames after movie end (default: 0)\n"
	       "  -o options    supply the given options to mencoder (default: low quality avi)\n"
	       "  -m movie.mmv  use the given movie file (without this option, encode a movie\n"
	       "                with no input and the number of frames given by -f)\n"
	       "  rom.sms       the ROM file to load\n"
	       "  movie.avi     the name of the encoded movie (needs not be AVI)\n",
	name);
}

int main(int argc, char **argv) {
	int framerate = 60, i, frames = 0;

	unsigned char *rom;
	int romlength;

	int additionalFrames = 0;
	char *mencoderOptions = "-oac mp3lame -ovc xvid -xvidencopts \"bitrate=200:me_quality=1:gmc:aspect=4/3\" -lameopts preset=30:aq=9 -noskip -mc 0", *movieFile = 0;
	int opt;

	while ((opt = getopt(argc, argv, "f:o:m:")) != -1) {
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
			default:
				usage(argv[0]);
				exit(1);
		}
	}

	if (optind+1 >= argc) {
		usage(argv[0]);
		exit(1);
	}

	mencoder_pid = spawn_mencoder(mencoderOptions, argv[optind+1]);

	FifoInit(audiofifo);
	FifoInit(videofifo);

	MastInit();
	MastLoadRom(argv[optind], &rom, &romlength);
	MastSetRom(rom,romlength);
	MastHardReset();

	MsndRate=44100; MsndLen=(MsndRate+(framerate>>1))/framerate;
	pMsndOut=malloc(MsndLen*2*2);
	MsndInit();

	if (movieFile != 0) {
		frames = MvidStart(movieFile, PLAYBACK_MODE, 0);
	}

	for (i = 0; i < frames+additionalFrames; i++) {
		fd_set fds;
		int progress;
		pMsndOut = FifoPush(audiofifo, MsndLen*2*2);
		videodata = FifoPush(videofifo, 288*192*3);

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

	waitpid(mencoder_pid, 0, 0);
}

