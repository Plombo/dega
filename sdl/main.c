#define APPNAME "dega"
#define APPNAME_LONG "Dega/SDL"
#define VERSION "1.07"

#include <stdio.h>
#include <unistd.h>
#include <mast.h>
#include <SDL.h>
#include <malloc.h>
#include <getopt.h>

SDL_Surface *thescreen;
SDL_Color themap[256];

int width, height;

static int audio_len=0;

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
		printf("resetting audio_len\n");
		return;
	}
	//printf("audio_len=%d\n",audio_len);
	memcpy(stream,data,len);
	audio_len-=len;
	memcpy(data,(unsigned char*)data+len,audio_len);
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
	printf("  -s --nosound\tdisable sound\n");
	printf("  -f --fullscreen\tfullscreen display\n");
	printf("\n" APPNAME_LONG " version " VERSION " by Ulrich Hecht <uli@emulinks.de>\n");
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

	// options
	int framerate=60;
	int autodetect=1;
	int sound=1;
	int vidflags=0;

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
			{"nosound",no_argument,NULL,'s'},
			{"fullscreen",no_argument,NULL,'f'},
			{0,0,0,0}
		};
		
		copt=getopt_long(argc,argv,"vgmpsf",long_options,&option_index);
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
				MastEx &= !MX_GG;
				break;
			
			case 'p':
				MastEx |= MX_PAL;
				framerate=50;
				break;
			
			case 's':
				sound=0;
				break;

			case 'f':
				vidflags |= SDL_FULLSCREEN;
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

	//detect GG ROM
	if(strstr(argv[optind],".gg") || strstr(argv[optind],".GG"))
		MastEx |= MX_GG;
				
	atexit(SDL_Quit);

	if(sound)
		SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	else
		SDL_Init(SDL_INIT_VIDEO);

	width=MastEx&MX_GG?160:256;
	height=MastEx&MX_GG?144:192;

	thescreen=SDL_SetVideoMode(width, height, 8, SDL_SWSURFACE|vidflags);
		
	MastInit();
	MastLoadRom(argv[optind], &rom, &romlength);
	MastSetRom(rom,romlength);
	MastHardReset();
	memset(&MastInput,0,sizeof(MastInput));
	
	if(sound)
	{
		MsndRate=44100; MsndLen=(MsndRate+(framerate>>1))/framerate; //guess
		aspec.freq=MsndRate;
		aspec.format=AUDIO_S16;
		aspec.channels=2;
		aspec.samples=1024;
		audiobuf=malloc(aspec.samples*aspec.channels*2*4);
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

	MastDrawDo=1;
	while(!done)
	{
		scrlock();
		MastFrame();
		scrunlock();
		if(sound)
		{
			SDL_LockAudio();
			memcpy(audiobuf+audio_len,pMsndOut,MsndLen*aspec.channels*2);
			audio_len+=MsndLen*aspec.channels*2;
			//printf("audio_len %d\n",audio_len);
			SDL_UnlockAudio();
		}
                while(SDL_PollEvent(&event))
                {
                        switch (event.type)
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
                                if(key==SDLK_c) {MastInput[0]|=0x80;break;}
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
                                if(key==SDLK_c) {MastInput[0]&=0x7f;break;}
                                break;
                        case SDL_QUIT:
                                done = 1;
                                break;
                        default:
                                break;
                        }
                }
		if(sound) while(audio_len>aspec.samples*aspec.channels*2*4) usleep(5);
	}
}

void MdrawCall()
{
	int i,len,yoff=0;
	if(Mdraw.Data[0]) printf("MdrawCall called, line %d, first pixel %d\n",Mdraw.Line,Mdraw.Data[0]);
	if(Mdraw.PalChange)
	{
		Mdraw.PalChange=0;
#define p(x) Mdraw.Pal[x]
		for(i=0;i<0x100;i++)
		{
			themap[i].r=(p(i)&7)<<5;
			themap[i].g=(p(i)&56)<<2;
			themap[i].b=(p(i)&448)>>1;
		}
		SDL_SetColors(thescreen, themap, 0, 256);
	}
    	if(MastEx&MX_GG) {i=64; yoff=24;}
    		else	 {i=16; }
    	if(Mdraw.Line-yoff<0 || Mdraw.Line-yoff>=height) return;
	memcpy(thescreen->pixels+(Mdraw.Line-yoff)*thescreen->pitch,Mdraw.Data+i,width);
}

