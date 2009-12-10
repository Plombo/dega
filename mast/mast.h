//---------------------------------------------------------------------------
// Mast - Master System emulator library
// Copyright (c) 2004 Dave (www.finalburn.com), all rights reserved.

// This refers to all the code except where stated otherwise
// (e.g. ym2413 emulator)

// You can use, modify and redistribute this code freely as long as you
// don't do so commercially. This copyright notice must remain with the code.
// You must state if your program uses this code.

// Dave
// Homepage: www.finalburn.com
// E-mail:  dave@finalburn.com
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

// mast.cpp
extern unsigned int MastVer; // Version number of the library (lower 16 bits) and Z80 core (upper 16 bits)
#define MAST_CORE_MASK    0xFFFF0000
#define MAST_VERSION_MASK 0x0000FFFF
#define MAST_CORE_DOZE    (0 << 16)
#define MAST_CORE_Z80JB   (1 << 16)

extern unsigned char MastInput[2]; // Joypads
extern unsigned int MastEx; // Extra options
#define MX_GG     (1) // Run as Game Gear
#define MX_PAL    (2) // Run as PAL timing
#define MX_JAPAN  (4) // Return Japan as Region
#define MX_FMCHIP (8) // Emulate FM chip
#define MX_SRAM  (16) // Save/load SRAM in save state

extern int MastDrawDo; // 1 to draw image
int MastInit();
int MastExit();
int MastSetRom(unsigned char *Rom,int RomLen);
void MastFlagsFromHeader();
extern char MastRomName[128];
void MastSetRomName(char *romname);
void MastGetRomDigest(unsigned char *digest);
int MastReset();
int MastHardReset();

// snd.cpp
extern int MsndRate; // sample rate of sound
extern int MsndLen;  // length in samples per frame
extern short *pMsndOut; // pointer to sound output buffer or NULL for no sound
int MsndInit();
int MsndExit();

// frame.cpp
int MastFrame();

// area.cpp
struct MastArea { void *Data; int Len; };
extern int MastAcbNull (struct MastArea *pba);
extern int (*MastAcb) (struct MastArea *pma); // Area callback
int MastAreaBattery();
int MastAreaMeka();
int MastAreaDega();

// load.cpp
int MastLoadRom(char *Name,unsigned char **pRom,int *pRomLen);

// draw.cpp
// Master system scanline
struct Mdraw
{
  unsigned short Pal[0x100]; // Palette (0000rrrr ggggbbbb) (0x1000 used)
  unsigned char Data[0x120]; // Pixel values
  unsigned char PalChange;
  int Line; // Image line
  unsigned char ThreeD; // 0/1=normal, 2=probably 3D right image, 3=probably 3D left image
};
extern struct Mdraw Mdraw;
void MdrawCall();

// samp.cpp
extern int DpsgEnhance;

// vgm.cpp
int VgmStart(char *VgmName);
int VgmStop(unsigned short *Gd3Text);
extern FILE *VgmFile;
extern int VgmAccurate; // 1=Sample accurate

// video.cpp
struct MvidHeader {
  int mastVer;
  int vidFrameCount;
  int rerecordCount;
  int beginReset;
  int stateOffset;
  int inputOffset;
  int packetSize;
  char videoAuthor[64];
  int vidFlags;
  char vidRomName[128];
  unsigned char vidDigest[16];
};
#define PLAYBACK_MODE 0
#define RECORD_MODE 1
#define VIDFLAG_PAL   (1<<1)
#define VIDFLAG_JAPAN (1<<2)
#define VIDFLAG_GG    (1<<3)
int MvidReadHeader(FILE *file, struct MvidHeader *header);
int MvidStart(char *videoFilename, int mode, int reset, char *author);
void MvidStop();
void MvidPostLoadState(int readonly);
void MvidPostSaveState();
int MvidSetAuthor(char *author);
void MvidGetAuthor(char *author, int len);
int MvidGetFrameCount();
int MvidGetRerecordCount();
int MvidGotProperties();
int MvidInVideo();
// defined by implementation
void MvidModeChanged();
void MvidMovieStopped();

// osd.cpp
#define OSD_BUTTONS 1
#define OSD_FRAMECOUNT 2
extern int MdrawOsdOptions;

// implementation
void MimplFrame(int ReadInput);

#ifdef __cplusplus
} // End of extern "C"
#endif
