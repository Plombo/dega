#define APPNAME "dega"
#define APPNAME_LONG "Dega/SDL"
#define VERSION "1.12"

#include <stdio.h>
#include <unistd.h>
#include <mast.h>
#include <SDL.h>
#include <malloc.h>
#include <getopt.h>

#define JS_AXIS_LIMIT 1500

SDL_Surface *thescreen;
SDL_Color themap[256];

int width, height;
int scale=0, count=0;

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
	printf("     --help\t\tprint what you're reading now\n");
	printf("  -v --version\t\tprint version no.\n");
	printf("  -g --gamegear\t\tforce Game Gear emulation (default autodetect)\n");
	printf("  -m --sms\t\tforce Master System emulation (default autodetect)\n");
	printf("  -p --pal\t\tuse PAL mode (default NTSC)\n");
	printf("  -s --nosound\t\tdisable sound\n");
	printf("  -f --fullscreen\tfullscreen display\n");
	printf("  -j --joy\t\tenable joystick\n");
	printf("  -2 --scale\tquadruple window size\n");
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

	Uint32 SDL_init_flags = SDL_INIT_VIDEO;
	int i;
	SDL_Joystick *js;

	// options
	int framerate=60;
	int autodetect=1;
	int sound=1;
	int joystick=0;
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
			{"joy",no_argument,NULL,'j'},
			{"scale", no_argument, NULL, '2'},
			{0,0,0,0}
		};
		
		copt=getopt_long(argc,argv,"vgmpsfj2",long_options,&option_index);
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

			case 'j':
				fprintf(stderr,"Joystick enabled.\n");
				joystick = 1;
				break;

			case '2':
				scale=1;
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

	if(joystick) SDL_init_flags |= SDL_INIT_JOYSTICK;
	if(sound) SDL_init_flags |= SDL_INIT_AUDIO;
	if( SDL_Init(SDL_init_flags) != 0) exit(-1);

	if ( (joystick = SDL_NumJoysticks()) != 0 ) {
		fprintf(stderr,"%d joystick(s) found\n",joystick);
	    	for( i=0; i < SDL_NumJoysticks(); i++ )  
       			printf("%d    %s\n",i, SDL_JoystickName(i));
		SDL_JoystickEventState(SDL_ENABLE);
    		js = SDL_JoystickOpen(0);
	} else
		joystick = 0;
	
		
	width=MastEx&MX_GG?160:256;
	height=MastEx&MX_GG?144:192;

	if(scale == 1)
		thescreen=SDL_SetVideoMode(width * 2, height * 2, 8, SDL_HWSURFACE|vidflags);
	else
		thescreen=SDL_SetVideoMode(width, height, 8, SDL_HWSURFACE|vidflags);

	SDL_WM_SetCaption(APPNAME_LONG " " VERSION, APPNAME_LONG);
	
	MastInit();
	MastLoadRom(argv[optind], &rom, &romlength);
	MastSetRom(rom,romlength);
	MastHardReset();
	memset(&MastInput,0,sizeof(MastInput));

	SDL_ShowCursor(SDL_DISABLE);
	
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
                                if(key==SDLK_1 || key==SDLK_2) {
                                	scale = (key==SDLK_1) ? 0 : 1;
                                	thescreen = SDL_SetVideoMode(width * (scale + 1), height * (scale + 1), 8, SDL_HWSURFACE|vidflags);
                                	Mdraw.PalChange = 1;
                                	MdrawCall();
                                }
                                if(key==SDLK_f) {
                                	if(vidflags & SDL_FULLSCREEN) vidflags &= ~SDL_FULLSCREEN;
                                	else vidflags |= SDL_FULLSCREEN;
                                	SDL_WM_ToggleFullScreen(thescreen);
				}
                                break;
                        case SDL_QUIT:
                                done = 1;
                                break;
			case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
				if ( event.jaxis.axis == 0)  {	/* Left/Right Axis */
					MastInput[0] &= ~0x0c;	/* Unset this axis first */
					if ( event.jaxis.value < -JS_AXIS_LIMIT ) 
						MastInput[0]|=0x04;	/* Left */
					else if ( event.jaxis.value > JS_AXIS_LIMIT )
						MastInput[0]|=0x08;	/* Right */
				} else if ( event.jaxis.axis == 1) {	/* Up/Down Axis */
					MastInput[0] &= ~0x03;
					if ( event.jaxis.value < -JS_AXIS_LIMIT )
						MastInput[0]|=0x01;	/* Up */
					else if ( event.jaxis.value > JS_AXIS_LIMIT )
						MastInput[0]|=0x02;	/* Down */
				} 
				break;
			case SDL_JOYBUTTONDOWN:  /* Handle Joystick Button Press */
   				if ( event.jbutton.button == 0 )  {MastInput[0]|=0x10;break;}
				if ( event.jbutton.button == 1 )  {MastInput[0]|=0x20;break;}
    				break;
			case SDL_JOYBUTTONUP:  	/* Handle Joystick Button Depress */
   				if ( event.jbutton.button == 0 )  {MastInput[0]&=~0x10;break;}
				if ( event.jbutton.button == 1 )  {MastInput[0]&=~0x20;break;}
    				break;

                        default:
                                break;
                        }
			//fprintf(stderr,"%x: %2x\n",event.type,MastInput[0] );
		}
		
		if(sound) while(audio_len>aspec.samples*aspec.channels*2*4) usleep(5);
	}
}

void MdrawCall()
{
	int i,len,yoff=0;
	int j;
	Uint8* line;
	Uint8 pixel;
	
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

	// If we're doing a tall window, draw each line once then once lower,
	// incrementing "count" (added on each time) so we go down the whole screen
	line = thescreen->pixels+(Mdraw.Line-yoff)*thescreen->pitch * (scale? 2 : 1);
	if(scale==1) {
		for(j = 0; j < width; j++) {
			pixel = *(Mdraw.Data + i + j);
			*(line + j * 2) = *(line + j * 2 + 1) = pixel;
			*(line + j * 2 + thescreen->pitch) = *(line + j * 2 + thescreen->pitch + 1) = pixel;
		}
	} else
		memcpy(line,Mdraw.Data+i,width);


}

