#define APPNAME "dega"
#define APPNAME_LONG "Dega/SDL"
#define VERSION "1.15"

#include <stdio.h>
#include <unistd.h>
#include <mast.h>
#include <SDL.h>
#include <malloc.h>
#include <getopt.h>
#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <time.h>

#include "../python/embed.h"

SDL_Surface *thescreen;
unsigned short themap[256];

int width, height;

static int audio_len=0;

int framerate=60;
int mult=0;

int readonly;

int python;
int vidflags=0;

int scrlock()
{
        if(SDL_MUSTLOCK(thescreen))
        {
                if ( SDL_LockSurface(thescreen) < 0 )
                {
                        fprintf(stderr, "Couldn't lock display surface: %s\n",
                                                                SDL_GetError());
                        return -1;
                }
        }
        return 0;
}
void scrunlock(void)
{
        if(SDL_MUSTLOCK(thescreen))
                SDL_UnlockSurface(thescreen);
        SDL_UpdateRect(thescreen, 0, 0, 0, 0);
}

void MsndCall(void* data, Uint8* stream, int len)
{
	if(audio_len < len)
	{
		memcpy(stream,data,audio_len);
		audio_len=0;
		// printf("resetting audio_len\n");
		return;
	}
	//printf("audio_len=%d\n",audio_len);
	memcpy(stream,data,len);
	audio_len-=len;
	memcpy(data,(unsigned char*)data+len,audio_len);
}

static char *chompgets(char *buf, int len, FILE *fh) {
	char *ret;
	if ((ret = fgets(buf, len, fh))) {
		char *nl = strchr(buf, '\n');
		if (nl != NULL) {
			*nl = '\0';
		}
	}
	return ret;
}

static FILE *sf;

static int StateLoadAcb(struct MastArea *pma)
{
  if (sf!=NULL) return fread(pma->Data,1,pma->Len,sf);
  return 0;
}

static int StateLoad(char *StateName)
{
  if (StateName[0]==0) return 1;
  
  sf=fopen (StateName,"rb");
  if (sf==NULL) return 1;

  // Scan state
  MastAcb=StateLoadAcb;
  MastAreaDega();
  MvidPostLoadState(readonly);
  MastAcb=MastAcbNull;

  fclose(sf); sf=NULL;

  return 0;
}

static int StateSaveAcb(struct MastArea *pma)
{
  if (sf!=NULL) fwrite(pma->Data,1,pma->Len,sf);
  return 0;
}

static int StateSave(char *StateName)
{
  if (StateName[0]==0) return 1;

  sf=fopen (StateName,"wb");
  if (sf==NULL) return 1;

  // Scan state
  MastAcb=StateSaveAcb;
  MastAreaDega();
  MvidPostSaveState();
  MastAcb=MastAcbNull;

  fclose(sf); sf=NULL;
  return 0;
}

static void LeaveFullScreen() {
	if (vidflags&SDL_FULLSCREEN) {
		thescreen = SDL_SetVideoMode(width, height, 15, SDL_SWSURFACE|(vidflags&~SDL_FULLSCREEN));
	}
}

static void EnterFullScreen() {
	if (vidflags&SDL_FULLSCREEN) {
		thescreen = SDL_SetVideoMode(width, height, 15, SDL_SWSURFACE|vidflags);
	}
}

void HandleSaveState() {
	char buffer[64];
	LeaveFullScreen();
	puts("Enter name of state to save:");
	chompgets(buffer, sizeof(buffer), stdin);
	StateSave(buffer);
	EnterFullScreen();
}

void HandleLoadState() {
	char buffer[64];
	LeaveFullScreen();
	puts("Enter name of state to load:");
	chompgets(buffer, sizeof(buffer), stdin);
	StateLoad(buffer);
	EnterFullScreen();
}

void HandleRecordMovie(int reset) {
	char buffer[64];
	LeaveFullScreen();
	printf("Enter name of movie to begin recording%s:\n", reset ? " from reset" : "");
	chompgets(buffer, sizeof(buffer), stdin);
	MvidStart(buffer, RECORD_MODE, reset, 0);
	EnterFullScreen();
}

void HandlePlaybackMovie(void) {
	char buffer[64];
	LeaveFullScreen();
	puts("Enter name of movie to begin playback:");
	chompgets(buffer, sizeof(buffer), stdin);
	MvidStart(buffer, PLAYBACK_MODE, 0, 0);
	EnterFullScreen();
}

void HandleSetAuthor(void) {
	char buffer[64], buffer_utf8[64];
	char *pbuffer = buffer, *pbuffer_utf8 = buffer_utf8;
	size_t buffersiz, buffersiz_utf8 = sizeof(buffer_utf8), bytes;
	iconv_t cd;
	LeaveFullScreen();
	puts("Enter name of author:");
	chompgets(buffer, sizeof(buffer), stdin);
	buffersiz = strlen(buffer);

	cd = iconv_open("UTF-8", nl_langinfo(CODESET));
	bytes = iconv(cd, &pbuffer, &buffersiz, &pbuffer_utf8, &buffersiz_utf8);
	if (bytes == (size_t)(-1)) {
		perror("iconv");
		iconv_close(cd);
		return;
	}
	iconv_close(cd);

	*pbuffer_utf8 = '\0';

	MvidSetAuthor(buffer_utf8);

	EnterFullScreen();
}

void HandlePython(void) {
	char buffer[64];
	LeaveFullScreen();
	
	if (!python) {
		puts("Python not available!");
		return;
	}

	puts("Enter name of Python control script to execute:");
	chompgets(buffer, sizeof(buffer), stdin);

	EnterFullScreen();

	MPyEmbed_Run(buffer);
}

void HandlePythonREPL(void) {
	if (!python) {
		puts("Python not available!");
		return;
	}
	LeaveFullScreen();
	MPyEmbed_Repl();
	EnterFullScreen();
}

void *PythonThreadRun(void *pbuf) {
	char *buffer = pbuf;
	MPyEmbed_RunThread(buffer);
	free(buffer);
	return 0;
}

void HandlePythonThread(void) {
	char *buffer = malloc(64);
	pthread_t pythread;
	LeaveFullScreen();
	
	if (!python) {
		puts("Python not available!");
		return;
	}

	puts("Enter name of Python viewer script to execute:");
	chompgets(buffer, 64, stdin);

	EnterFullScreen();

	pthread_create(&pythread, 0, PythonThreadRun, buffer);
}

void SetRateMult() {
	int newrate = mult>0 ? framerate<<mult : framerate>>-mult;
	if (newrate < 1) newrate = 1;

	free(pMsndOut);
	MsndLen=(MsndRate+(newrate>>1))/newrate; 
	pMsndOut = malloc(MsndLen*2*2);
}

void MvidModeChanged() {
	framerate = (MastEx & MX_PAL) ? 50 : 60;
	SetRateMult();
}

void MvidMovieStopped() {}

void MimplFrame(int input) {
	if (input) {
		SDL_Event event;
		int key;

                while(SDL_PollEvent(&event))
                {
		switch (event.type)
                        {
                        case SDL_KEYDOWN:
                                key=event.key.keysym.sym;
                                if(key==SDLK_UP) {MastInput[0]|=0x01;break;}
                                if(key==SDLK_DOWN) {MastInput[0]|=0x02;break;}
                                if(key==SDLK_LEFT) {MastInput[0]|=0x04;break;}
                                if(key==SDLK_RIGHT) {MastInput[0]|=0x08;break;}
                                if(key==SDLK_z || key==SDLK_y) {MastInput[0]|=0x10;break;}
                                if(key==SDLK_x) {MastInput[0]|=0x20;break;}
                                if(key==SDLK_c) {
				  MastInput[0]|=0x80;
				  if ((MastEx&MX_GG)==0)
				    MastInput[0]|=0x40;
				  break;}

                                if(key==SDLK_u) {MastInput[1]|=0x01;break;}
                                if(key==SDLK_j) {MastInput[1]|=0x02;break;}
                                if(key==SDLK_h) {MastInput[1]|=0x04;break;}
                                if(key==SDLK_k) {MastInput[1]|=0x08;break;}
                                if(key==SDLK_f) {MastInput[1]|=0x10;break;}
                                if(key==SDLK_g) {MastInput[1]|=0x20;break;}

                                break;
                        case SDL_KEYUP:
                                key=event.key.keysym.sym;
                                if(key==SDLK_UP) {MastInput[0]&=0xfe;break;}
                                if(key==SDLK_DOWN) {MastInput[0]&=0xfd;break;}
                                if(key==SDLK_LEFT) {MastInput[0]&=0xfb;break;}
                                if(key==SDLK_RIGHT) {MastInput[0]&=0xf7;break;}
                                if(key==SDLK_z || key==SDLK_y) {MastInput[0]&=0xef;break;}
                                if(key==SDLK_x) {MastInput[0]&=0xdf;break;}
                                if(key==SDLK_c) {MastInput[0]&=0x3f;break;}

                                if(key==SDLK_u) {MastInput[1]&=0xfe;break;}
                                if(key==SDLK_j) {MastInput[1]&=0xfd;break;}
                                if(key==SDLK_h) {MastInput[1]&=0xfb;break;}
                                if(key==SDLK_k) {MastInput[1]&=0xf7;break;}
                                if(key==SDLK_f) {MastInput[1]&=0xef;break;}
                                if(key==SDLK_g) {MastInput[1]&=0xdf;break;}
                                break;
                        default:
                                break;
                        }
		}
	}

	scrlock();
	MastFrame();
	scrunlock();

#if 0
	pydega_cbpostframe(mainstate);
#else
	MPyEmbed_CBPostFrame();
#endif

	if (input) {
		MastInput[0]&=~0x40;
	}
}

void usage(void)
{
	printf("\nUsage: %s [OPTION]... [ROM file]\n",APPNAME);
	printf("\nOptions:\n");
	printf("     --help\tprint what you're reading now\n");
	printf("  -v --version\tprint version no.\n");
	printf("  -g --gamegear\tforce Game Gear emulation (default autodetect)\n");
	printf("  -m --sms\tforce Master System emulation (default autodetect)\n");
	printf("  -p --pal\tuse PAL mode (default NTSC)\n");
	printf("  -j --japan\tJapanese region\n");
	printf("  -s --nosound\tdisable sound\n");
	printf("  -f --fullscreen\tfullscreen display\n");
	printf("  -r --readonly\tmovies are readonly\n");
	printf("\n" APPNAME_LONG " version " VERSION " by Ulrich Hecht <uli@emulinks.de>\n");
	printf("extended by Peter Collingbourne <peter@peter.uk.to>\n");
	printf("based on Win32 version by Dave <dave@finalburn.com>\n");
	exit (0);
}

int main(int argc, char** argv)
{
	unsigned char* rom;
	int romlength;
	int done=0;
	SDL_Event event;
	int key;
	SDL_AudioSpec aspec;
	unsigned char* audiobuf;
	int paused=0, frameadvance=0;

	// options
	int autodetect=1;
	int sound=1;

	setlocale(LC_CTYPE, "");

	readonly = 0;

	MPyEmbed_SetArgv(argc, argv);

	while(1)
	{
		int option_index=0;
		int copt;
		
		static struct option long_options[] = {
			{"help",0,0,0},
			{"version",no_argument,NULL,'v'},
			{"gamegear",no_argument,NULL,'g'},
			{"sms",no_argument,NULL,'m'},
			{"pal",no_argument,NULL,'p'},
			{"japan",no_argument,NULL,'j'},
			{"nosound",no_argument,NULL,'s'},
			{"fullscreen",no_argument,NULL,'f'},
			{"readonly",no_argument,NULL,'r'},
			{0,0,0,0}
		};
		
		copt=getopt_long(argc,argv,"vgmpjsfr",long_options,&option_index);
		if(copt==-1) break;
		switch(copt)
		{
			case 0:
				if(strcmp(long_options[option_index].name,"help")==0) usage();
				break;
			
			case 'v':
				printf("%s",VERSION "\n");
				exit(0);
			
			case 'g':
				autodetect=0;
				MastEx |= MX_GG;
				break;
			
			case 'm':
				autodetect=0;
				MastEx &= ~MX_GG;
				break;
			
			case 'p':
				MastEx |= MX_PAL;
				framerate=50;
				break;
			
			case 'j':
				MastEx |= MX_JAPAN;
				break;
			
			case 's':
				sound=0;
				break;

			case 'f':
				vidflags |= SDL_FULLSCREEN;
				break;
				
			case 'r':
				readonly = 1;
				break;

			case '?':
				usage();
				break;
		}
	}
	
	if(optind==argc)
	{
		fprintf(stderr,APPNAME ": no ROM image specified.\n");
		exit(1);
	}

	atexit(SDL_Quit);

	if(sound)
		SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	else
		SDL_Init(SDL_INIT_VIDEO);

	MastInit();
	MastLoadRom(argv[optind], &rom, &romlength);
	MastSetRom(rom,romlength);
	if (autodetect)
		MastFlagsFromHeader();
	MastHardReset();
	memset(&MastInput,0,sizeof(MastInput));
	
	width=MastEx&MX_GG?160:256;
	height=MastEx&MX_GG?144:192;

	thescreen=SDL_SetVideoMode(width, height, 15, SDL_SWSURFACE|vidflags);

	if(sound)
	{
		MsndRate=44100; MsndLen=(MsndRate+(framerate>>1))/framerate; //guess
		aspec.freq=MsndRate;
		aspec.format=AUDIO_S16;
		aspec.channels=2;
		aspec.samples=1024;
		audiobuf=malloc(aspec.samples*aspec.channels*2*64);
		memset(audiobuf,0,aspec.samples*aspec.channels*2);
		aspec.callback=MsndCall;
		pMsndOut=malloc(MsndLen*aspec.channels*2);
		aspec.userdata=audiobuf;
		if(SDL_OpenAudio(&aspec,NULL)) {
			fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
			sound=0;
		}
		SDL_PauseAudio(0);
		MsndInit();
	}
	else
	{
		pMsndOut=NULL;
	}

	MPyEmbed_Init();
	python = MPyEmbed_Available();

	MastDrawDo=1;
	while(!done)
	{
		if (!paused || frameadvance)
		{
			scrlock();
			MastFrame();
			scrunlock();

#if 0
			clock_gettime(CLOCK_REALTIME, &t1);
			pydega_cbpostframe(mainstate);
			clock_gettime(CLOCK_REALTIME, &t2);
			printf("postframe took %d ns\n", t2.tv_nsec-t1.tv_nsec);
#else
			MPyEmbed_CBPostFrame();
#endif

			MastInput[0]&=~0x40;
			if(sound)
			{
				SDL_LockAudio();
				memcpy(audiobuf+audio_len,pMsndOut,MsndLen*aspec.channels*2);
				audio_len+=MsndLen*aspec.channels*2;
				//printf("audio_len %d\n",audio_len);
				SDL_UnlockAudio();
			}
		}
		frameadvance = 0;
		if (paused)
		{
			SDL_WaitEvent(&event);
			goto Handler;
		}
                while(SDL_PollEvent(&event))
                {
Handler:		switch (event.type)
                        {
                        case SDL_KEYDOWN:
                                key=event.key.keysym.sym;
                                if(key==SDLK_ESCAPE) {done=1;break;}
                                if(key==SDLK_UP) {MastInput[0]|=0x01;break;}
                                if(key==SDLK_DOWN) {MastInput[0]|=0x02;break;}
                                if(key==SDLK_LEFT) {MastInput[0]|=0x04;break;}
                                if(key==SDLK_RIGHT) {MastInput[0]|=0x08;break;}
                                if(key==SDLK_z || key==SDLK_y) {MastInput[0]|=0x10;break;}
                                if(key==SDLK_x) {MastInput[0]|=0x20;break;}
                                if(key==SDLK_c) {
				  MastInput[0]|=0x80;
				  if ((MastEx&MX_GG)==0)
				    MastInput[0]|=0x40;
				  break;}

                                if(key==SDLK_u) {MastInput[1]|=0x01;break;}
                                if(key==SDLK_j) {MastInput[1]|=0x02;break;}
                                if(key==SDLK_h) {MastInput[1]|=0x04;break;}
                                if(key==SDLK_k) {MastInput[1]|=0x08;break;}
                                if(key==SDLK_f) {MastInput[1]|=0x10;break;}
                                if(key==SDLK_g) {MastInput[1]|=0x20;break;}

				if(key==SDLK_p) {paused=!paused;break;}
				if(key==SDLK_o) {paused=1;frameadvance=1;break;}
				if(key==SDLK_r) {HandleRecordMovie(1);break;}
				if(key==SDLK_e) {HandleRecordMovie(0);break;}
				if(key==SDLK_t) {HandlePlaybackMovie();break;}
				if(key==SDLK_w) {MvidStop();break;}
				if(key==SDLK_s) {HandleSaveState();break;}
				if(key==SDLK_l) {HandleLoadState();break;}
				if(key==SDLK_a) {HandleSetAuthor();break;}
				if(key==SDLK_n) {HandlePython();break;}
				if(key==SDLK_m) {HandlePythonREPL();break;}
				if(key==SDLK_i) {HandlePythonThread();break;}
				if(key==SDLK_b) {MdrawOsdOptions^=OSD_BUTTONS;break;}
				if(key==SDLK_f) {MdrawOsdOptions^=OSD_FRAMECOUNT;break;}
				if(key==SDLK_EQUALS) {mult++;SetRateMult();break;}
				if(key==SDLK_MINUS) {mult--;SetRateMult();break;}
                                break;
                        case SDL_KEYUP:
                                key=event.key.keysym.sym;
                                if(key==SDLK_ESCAPE) {done=1;break;}
                                if(key==SDLK_UP) {MastInput[0]&=0xfe;break;}
                                if(key==SDLK_DOWN) {MastInput[0]&=0xfd;break;}
                                if(key==SDLK_LEFT) {MastInput[0]&=0xfb;break;}
                                if(key==SDLK_RIGHT) {MastInput[0]&=0xf7;break;}
                                if(key==SDLK_z || key==SDLK_y) {MastInput[0]&=0xef;break;}
                                if(key==SDLK_x) {MastInput[0]&=0xdf;break;}
                                if(key==SDLK_c) {MastInput[0]&=0x3f;break;}

                                if(key==SDLK_u) {MastInput[1]&=0xfe;break;}
                                if(key==SDLK_j) {MastInput[1]&=0xfd;break;}
                                if(key==SDLK_h) {MastInput[1]&=0xfb;break;}
                                if(key==SDLK_k) {MastInput[1]&=0xf7;break;}
                                if(key==SDLK_f) {MastInput[1]&=0xef;break;}
                                if(key==SDLK_g) {MastInput[1]&=0xdf;break;}
                                break;
                        case SDL_QUIT:
                                done = 1;
                                break;
                        default:
                                break;
                        }
                }
		if (!paused || frameadvance)
		{
			if(sound) while(audio_len>aspec.samples*aspec.channels*2*4) usleep(5);
		}
	}
	if (python) {
		MPyEmbed_Fini();
	}
	return 0;
}

void MdrawCall()
{
	int i,xoff,yoff=0;
	unsigned short *line;
	if(Mdraw.Data[0]) printf("MdrawCall called, line %d, first pixel %d\n",Mdraw.Line,Mdraw.Data[0]);
	if(Mdraw.PalChange)
	{
		Mdraw.PalChange=0;
#define p(x) Mdraw.Pal[x]
		for(i=0;i<0x100;i++)
		{
			themap[i] = ((p(i)&0xf00)>>7) | ((p(i)&0xf0)<<2) | ((p(i)&0xf)<<11);
		}
	}
    	if(MastEx&MX_GG) {xoff=64; yoff=24;}
    		else	 {xoff=16; }
    	if(Mdraw.Line-yoff<0 || Mdraw.Line-yoff>=height) return;
	line = thescreen->pixels+(Mdraw.Line-yoff)*thescreen->pitch;
	for (i=0; i < width; i++) {
		line[i] = themap[Mdraw.Data[xoff+i]];
	}
}

