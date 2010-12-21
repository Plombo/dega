// Updated by Bryan Cain
// Date of update: August 21, 2010
// SDL backend stuff.

// Dega was originally written by Dave from finalburn.com.  It was extended by 
// Peter Collingbourne before I (Bryan) picked it up.

#define APPNAME "degaplus"
#define APPNAME_LONG "Dega Plus"
#define VERSION "1.15"

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <getopt.h>
#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <time.h>

#include "mast.h"
#include "logwav.h"
#include "sdl.h"
#include "hankaku.h"
#include "../python/embed.h"

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_framerate.h"

#if GTK
#include "gtksdl.h"
#else
#define SetVideoMode SDL_SetVideoMode
#undef main
#endif

#define JS_AXIS_LIMIT 1500

SDL_Surface *thescreen, *dispscreen;
unsigned short themap[256];

SDL_AudioSpec aspec;
unsigned char* audiobuf;

int width, height;
int scaledwidth, scaledheight;
int nativewidth, nativeheight;
int framerate=60;
int mult=0;

int readonly;

int python;
int vidflags=0;

int done=0;
int rom_open = 0;

int paused2=0, frameadvance=0;

// options
int autodetect=1;
int sound=1;
int joystick=1;
int scale = 1;
int opengl = 1;

// misc.
GLuint gltexture;
char message[1024];
int messageTime = 0;
static int audio_len=0;

int scrlock()
{
    if(SDL_MUSTLOCK(thescreen))
    {
        if (SDL_LockSurface(thescreen) < 0)
        {
            fprintf(stderr, "Couldn't lock display surface: %s\n", SDL_GetError());
            return -1;
        }
    }
    return 0;
}
void scrunlock(void)
{
        if(SDL_MUSTLOCK(thescreen))
            SDL_UnlockSurface(thescreen);
        
        if(opengl)
        {
        	glBindTexture(GL_TEXTURE_2D, gltexture);
        	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, thescreen->pixels);
        	glBegin(GL_QUADS);
				glTexCoord2f(0.0, 1.0);
				glVertex2i(0, 0);
				glTexCoord2f(1.0, 1.0);
				glVertex2i(scaledwidth-1, 0);
				glTexCoord2f(1.0, 0.0);
				glVertex2i(scaledwidth-1, scaledheight-1);
				glTexCoord2f(0.0, 0.0);
				glVertex2i(0, scaledheight-1);
			glEnd();
			SDL_GL_SwapBuffers();
        }
        else if(dispscreen)
        {
        	SDL_SoftStretch(thescreen, NULL, dispscreen, NULL);
        	SDL_UpdateRect(dispscreen, 0, 0, 0, 0);
        }
        else SDL_UpdateRect(thescreen, 0, 0, 0, 0);
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
	if(logwav) WavLogAppend(data, len);
	audio_len-=len;
	memcpy(data,(unsigned char*)data+len,audio_len);
}

static char *chompgets(char *buf, int len, FILE *fh) { // TODO remove after transitioning from stdin to GTK
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

int StateLoad(char *StateName)
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

int StateSave(char *StateName)
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

void HandleSetAuthor(void) { // TODO use GTK instead of stdin/stdout
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

void HandlePython(void) { // TODO use GTK instead of stdin/stdout
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

void HandlePythonREPL(void) { // TODO use GTK instead of stdout
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

void HandlePythonThread(void) { // TODO use GTK instead of stdin
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

int MainLoopIteration(void)
{
	SDL_Event event;
	int key;
	
	if (!paused2 || frameadvance)
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
		if(sound && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING))
		{
			SDL_LockAudio();
			memcpy(audiobuf+audio_len,pMsndOut,MsndLen*aspec.channels*2);
			audio_len+=MsndLen*aspec.channels*2;
			//printf("audio_len %d\n",audio_len);
			SDL_UnlockAudio();
		}
	}
	frameadvance = 0;
	if (paused2)
	{
		SDL_WaitEvent(&event);
		goto Handler;
	}

	while(SDL_PollEvent(&event))
	{
	Handler:
		switch (event.type)
		{
			case SDL_KEYDOWN:
				    key=event.key.keysym.sym;
				    if(key==SDLK_ESCAPE) {done=1;break;} // TODO remove
				    if(key==SDLK_UP) {MastInput[0]|=0x01;break;}
				    if(key==SDLK_DOWN) {MastInput[0]|=0x02;break;}
				    if(key==SDLK_LEFT) {MastInput[0]|=0x04;break;}
				    if(key==SDLK_RIGHT) {MastInput[0]|=0x08;break;}
				    if(key==SDLK_z || key==SDLK_y) {MastInput[0]|=0x10;break;}
				    if(key==SDLK_x) {MastInput[0]|=0x20;break;}
				    // if(key==SDLK_c) {MastInput[0]|=0x40;break;} // [from my Dega/SDL 1.12 source]
				    if(key==SDLK_c) {MastInput[0]|=((MastEx&MX_GG)==0)?0x40:0xc0;break;}
				    // if(key==SDLK_v) {MastInput[0]|=0x80;break;}
				    
				    if(key==SDLK_u) {MastInput[1]|=0x01;break;}
				    if(key==SDLK_j) {MastInput[1]|=0x02;break;}
				    if(key==SDLK_h) {MastInput[1]|=0x04;break;}
				    if(key==SDLK_k) {MastInput[1]|=0x08;break;}
				    if(key==SDLK_f) {MastInput[1]|=0x10;break;}
				    if(key==SDLK_g) {MastInput[1]|=0x20;break;}
				    
				    // [from Dega/SDL 1.16 source]
				    if(key==SDLK_p) {paused2=!paused2;break;}
					if(key==SDLK_o) {paused2=1;frameadvance=1;break;}
					if(key==SDLK_a) {HandleSetAuthor();break;}
					if(key==SDLK_n) {HandlePython();break;}
					if(key==SDLK_m) {HandlePythonREPL();break;}
					if(key==SDLK_i) {HandlePythonThread();break;}
					if(key==SDLK_b) {MdrawOsdOptions^=OSD_BUTTONS;break;}
					// if(key==SDLK_f) {MdrawOsdOptions^=OSD_FRAMECOUNT;break;} // FIXME this is the same as one of P2's controls!!
					if(key==SDLK_EQUALS) {mult++;SetRateMult();break;}
					if(key==SDLK_MINUS) {mult--;SetRateMult();break;}
					
					if(key==SDLK_q) {opengl=!opengl;InitVideo();break;}
				    break;
			case SDL_KEYUP:
				    key=event.key.keysym.sym;
				    if(key==SDLK_ESCAPE) {done=1;break;} // TODO remove
				    if(key==SDLK_UP) {MastInput[0]&=0xfe;break;}
				    if(key==SDLK_DOWN) {MastInput[0]&=0xfd;break;}
				    if(key==SDLK_LEFT) {MastInput[0]&=0xfb;break;}
				    if(key==SDLK_RIGHT) {MastInput[0]&=0xf7;break;}
				    if(key==SDLK_z || key==SDLK_y) {MastInput[0]&=0xef;break;}
				    if(key==SDLK_x) {MastInput[0]&=0xdf;break;}
				    if(key==SDLK_c) {MastInput[0]&=0x3f;break;}
				    //if(key==SDLK_v) {MastInput[0]&=0x7f;break;}
				    
				    if(key==SDLK_u) {MastInput[1]&=0xfe;break;}
				    if(key==SDLK_j) {MastInput[1]&=0xfd;break;}
				    if(key==SDLK_h) {MastInput[1]&=0xfb;break;}
				    if(key==SDLK_k) {MastInput[1]&=0xf7;break;}
				    if(key==SDLK_f) {MastInput[1]&=0xef;break;}
				    if(key==SDLK_g) {MastInput[1]&=0xdf;break;}
				    
				    if(key==SDLK_1 || key==SDLK_2) { // TODO restore scaling in some form
				    	scale = (key==SDLK_1) ? 0 : 1;
				    	InitVideo();
				    }
				    if(key==SDLK_w) {vidflags^=SDL_FULLSCREEN;InitVideo();break;}
				    
				    break;
			case SDL_QUIT:
				    done = 1;
				    break;
			case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */ // FIXME only P1 can use a joystick
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
	}

	if (!paused2 || frameadvance)
	{
		if(sound && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING))
		{
			while(audio_len>aspec.samples*aspec.channels*2*4)
			{
				usleep(5);
				//printf("%i\t\t%i\n", audio_len, aspec.samples*aspec.channels*2*4);
				gtk_main_iteration_do(FALSE);
			}
		}
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
	//memset(themap, 0, 256*sizeof(SDL_Color));
	//SDL_SetColors(thescreen, themap, 0, 256);
	scrlock();
	memset(thescreen->pixels, 0, thescreen->w * thescreen->h * thescreen->format->BytesPerPixel);
	scrunlock();
	
	// No audio stream, so close the sound output.
	CloseSound();
	
	rom_open = 0;
}

void OpenROM(char* filename)
{
	unsigned char* rom;
	int romlength;
	
	if(rom_open) CloseROM();
	rom_open = 1;
	done = 0;

	//detect Game Gear ROM
	if(strstr(filename, ".gg") || strstr(filename, ".GG"))
		MastEx |= MX_GG;
	
	// Set the video mode
	InitVideo();
	
	// Start sound playback
	InitSound();
	
	// Load and play the ROM
	MastInit();
	MastLoadRom(filename, &rom, &romlength);
	MastSetRom(rom, romlength);
	if (autodetect)
		MastFlagsFromHeader();
	MastHardReset();
	memset(&MastInput,0,sizeof(MastInput));
	
	// Notify user that ROM was loaded
	SetMessage("ROM loaded: %s\n", filename);
}

void InitVideo()
{
	width = (MastEx & MX_GG) ? 160 : 256;
	height = (MastEx & MX_GG) ? 144 : 192;
	
	if(thescreen) SDL_FreeSurface(thescreen);
	if(dispscreen) SDL_FreeSurface(dispscreen);
	
	if(opengl)
	{
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
		
		scaledwidth = width * (scale+1);
		scaledheight = height * (scale+1);
		thescreen = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16, 0,0,0,0);
		dispscreen = SetVideoMode(scaledwidth, scaledheight, 16, SDL_OPENGL | vidflags);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);
		glDisable(GL_DITHER);
		glViewport(0, 0, scaledwidth, scaledheight);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, scaledwidth-1, 0, scaledheight-1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDeleteTextures(1, &gltexture);
		glGenTextures(1, &gltexture);
		glBindTexture(GL_TEXTURE_2D, gltexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, thescreen->pixels);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else if(scale == 1)
	{
		thescreen = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16, 0,0,0,0);
		dispscreen = SetVideoMode(width*2, height*2, 16, SDL_SWSURFACE | vidflags);
	}
	else
	{
		thescreen = SetVideoMode(width, height, 16, SDL_SWSURFACE | vidflags);
		dispscreen = NULL;
	}
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
		if(!audiobuf) audiobuf = malloc(aspec.samples*aspec.channels*2*64);
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

/**
 * Initializes the backend, i.e. everything not GUI-related.  This includes 
 * initializing SDL and Python.
 */
void SDL_backend_init()
{
	char* pyargv = {"dega"};
	const SDL_VideoInfo* videoinfo;
	
	setlocale(LC_CTYPE, "");
	readonly = 0;
	MPyEmbed_SetArgv(1, &pyargv);
	
	// Initialize SDL.
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0)
		appExit(1);
	videoinfo = SDL_GetVideoInfo();
	nativewidth = videoinfo->current_w;
	nativeheight = videoinfo->current_h;
	
	// Scan for joysticks.
	JoystickScan();
	
	// Other miscellaneous initialization.
	memset(&MastInput, 0, sizeof(MastInput));
	SDL_ShowCursor(SDL_DISABLE);
	MastDrawDo = 1;
}

/**
 * Uninitializes the backend, i.e. everything not GUI-related.
 */
void SDL_backend_exit()
{
	if (python) MPyEmbed_Fini();
}

void MdrawCall()
{
	int i,xoff,yoff=0;
	static int lastticks = 0;
	unsigned short *line;
	if(Mdraw.Data[0]) printf("MdrawCall called, line %d, first pixel %d\n",Mdraw.Line,Mdraw.Data[0]);
	if(Mdraw.PalChange)
	{
		Mdraw.PalChange=0;
#define p(x) Mdraw.Pal[x]
		for(i=0;i<0x100;i++)
		{
			themap[i] = ((p(i)&0xf00)>>7) | ((p(i)&0xf0)<<3) | ((p(i)&0xf)<<12);
		}
	}
    	if(MastEx&MX_GG) {xoff=64; yoff=24;}
    		else	 {xoff=16; }
    	if(Mdraw.Line-yoff<0 || Mdraw.Line-yoff>=height) return;
	line = thescreen->pixels+(Mdraw.Line-yoff)*thescreen->pitch;
	for (i=0; i < width; i++) {
		line[i] = themap[Mdraw.Data[xoff+i]];
	}
	
	if(messageTime > 0)
	{
		PrintText(5, thescreen->h - 15, 0xffff, message);
		messageTime -= SDL_GetTicks() - lastticks;
	}
	lastticks = SDL_GetTicks();
}

void PrintText(int x, int y, int color, char *buf)
{
	int x1, y1, i;
	int bpp = 16;
	unsigned long data;
	unsigned short *line16 = NULL;
	unsigned int   *line32 = NULL;
	unsigned char *font;
	unsigned char ch = 0;

	// if(factor > 1){ y += 5; }

	for(i=0; i<strlen(buf); i++)
	{
		ch = buf[i];
		// mapping
		if (ch<0x20) ch = 0;
		else if (ch<0x80) { ch -= 0x20; }
		else if (ch<0xa0) {	ch = 0;	}
		else ch -= 0x40;
		font = (unsigned char *)&hankaku_font10[ch*10];
		// draw
		if (bpp == 16) line16 = (unsigned short *)thescreen->pixels + x + y * thescreen->w;
        else           line32 = (unsigned int   *)thescreen->pixels + x + y * thescreen->w;

		for (y1=0; y1<10; y1++)
		{
			data = *font++;
			for (x1=0; x1<5; x1++)
			{
				if (data & 1)
				{
					if (bpp == 16) *line16 = color;
				    else           *line32 = color;
				}

				if (bpp == 16) line16++;
				else           line32++;

				data = data >> 1;
			}
			if (bpp == 16) line16 += thescreen->w-5;
			else           line32 += thescreen->w-5;
		}
		x+=5;
	}
}

void SetMessage(char* format, ...)
{
	va_list arglist;
    va_start(arglist, format);
    vsprintf(message, format, arglist);
    va_end(arglist);
    messageTime = 5000;
}


