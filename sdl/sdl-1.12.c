// Updated by Bryan Cain
// Date of update: August 21, 2010
// SDL backend stuff.

#define APPNAME "dega"
#define APPNAME_LONG "Dega/SDL"
#define VERSION "1.12"

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <getopt.h>

#include "mast.h"
#include "logwav.h"
#include "sdl.h"

#include "SDL.h"
#include "SDL_framerate.h"

#if GTK
#include "gtksdl.h"
#else
#define SetVideoMode SDL_SetVideoMode
#undef main
#endif

#define JS_AXIS_LIMIT 1500

SDL_Surface *thescreen;
SDL_Color themap[256];

SDL_AudioSpec aspec;
unsigned char* audiobuf;

int width, height;
int scale=0, count=0;

int done=0;
int rom_open = 0;

// options
int framerate=60;
int autodetect=1;
int sound=1;
int joystick=1;
int vidflags=0;

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
	if(logwav) WavLogAppend(data, len);
	audio_len-=len;
	memcpy(data,(unsigned char*)data+len,audio_len);
}

int MainLoopIteration(void)
{
	SDL_Event event;
	int key;
	
	scrlock();
	MastFrame();
	scrunlock();
	if(sound && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING))
	{
		SDL_LockAudio();
		memcpy(audiobuf+audio_len, pMsndOut, MsndLen*aspec.channels*2);
		audio_len += MsndLen*aspec.channels*2;
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
                            if(key==SDLK_c) {MastInput[0]|=0x40;break;}
                            if(key==SDLK_v) {MastInput[0]|=0x80;break;}
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
                            if(key==SDLK_c) {MastInput[0]&=0xbf;break;}
                            if(key==SDLK_v) {MastInput[0]&=0x7f;break;}
                            if(key==SDLK_1 || key==SDLK_2) {
                            	scale = (key==SDLK_1) ? 0 : 1;
                            	thescreen = SetVideoMode(width * (scale + 1), height * (scale + 1), 8, SDL_HWSURFACE|vidflags);
                            	Mdraw.PalChange = 1;
                            	MdrawCall();
                            }
                            if(key==SDLK_f) {
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
	
	if(sound && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING))
		while(audio_len>aspec.samples*aspec.channels*2*4)
		{
			usleep(5);
			//printf("%i\t\t%i\n", audio_len, aspec.samples*aspec.channels*2*4);
			gtk_main_iteration_do(FALSE);
		}
	return done;
}

void MainLoop(void)
{
	done = 0;
	
	while(1)
	{
		if(rom_open && MainLoopIteration()) CloseROM();
#ifdef GTK
		// Wait for GTK+ to catch up.
		while (gtk_events_pending())
			gtk_main_iteration_do(FALSE);
#endif
	}
	
	done = 0;
}


void CloseROM()
{
	done = 1;
	
	// Exit the Master System emulator.
	MastExit();
	
	// Clear the screen when closing the ROM.
	memset(themap, 0, 256*sizeof(SDL_Color));
	SDL_SetColors(thescreen, themap, 0, 256);
	scrlock();
	memset(thescreen->pixels, 0, thescreen->w * thescreen->h * thescreen->format->BytesPerPixel);
	scrunlock();
	
	// No audio stream, so close the sound output.
	CloseSound();
	
	rom_open = 0;
}

void OpenROM(const char* filename)
{
	unsigned char* rom;
	int romlength;
	
	if(rom_open) CloseROM();
	rom_open = 1;
	done = 0;

	//detect Game Gear ROM
	if(strstr(filename, ".gg") || strstr(filename, ".GG"))
		MastEx |= MX_GG;
		
	width = (MastEx & MX_GG) ? 160 : 256;
	height = (MastEx & MX_GG) ? 144 : 192;

	if(scale == 1)
		thescreen = SetVideoMode(width * 2, height * 2, 8, SDL_HWSURFACE | vidflags);
	else
		thescreen = SetVideoMode(width, height, 8, SDL_HWSURFACE | vidflags);
	
	// Start sound playback
	InitSound();
	
	// Load and play the ROM
	MastInit();
	MastLoadRom(filename, &rom, &romlength);
	MastSetRom(rom, romlength);
	MastHardReset();
}

void InitSound()
{
	if(sound)
	{
		MsndRate = 44100; MsndLen = (MsndRate + (framerate>>1)) / framerate; //guess
		aspec.freq = MsndRate;
		aspec.format = AUDIO_S16;
		aspec.channels = 2;
		aspec.samples = 1024;
		if(!audiobuf) audiobuf = malloc(aspec.samples*aspec.channels*2*4);
		memset(audiobuf, 0, aspec.samples*aspec.channels*2);
		aspec.callback = MsndCall;
		if(!pMsndOut) pMsndOut = malloc(MsndLen*aspec.channels*2);
		aspec.userdata = audiobuf;
		if(SDL_OpenAudio(&aspec, NULL)) {
			fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
			sound = 0;
		}
		SDL_PauseAudio(0);
		MsndInit();
	}
	else
	{
		pMsndOut = NULL;
	}
}

void CloseSound()
{
	if(sound)
	{
		MsndExit();
		SDL_CloseAudio();
	}
}

void JoystickScan()
{
	SDL_Joystick *js;
	int i;
	
	if ( (joystick = SDL_NumJoysticks()) != 0 )
	{
		fprintf(stderr,"%d joystick(s) found\n", joystick);
    	for( i=0; i < SDL_NumJoysticks(); i++ )  
   			printf("%d    %s\n",i, SDL_JoystickName(i));
		SDL_JoystickEventState(SDL_ENABLE);
    	js = SDL_JoystickOpen(0);
	}
}
	                
void SDL_backend_init()
{
	// Initialize SDL.
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0)
		appExit(1);
	
	// Scan for joysticks.
	JoystickScan();
	
	// Other miscellaneous initialization.
	memset(&MastInput, 0, sizeof(MastInput));
	SDL_ShowCursor(SDL_DISABLE);
	MastDrawDo = 1;
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

