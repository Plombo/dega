// DirectSound module
#include "app.h"
#define DIRECTSOUND_VERSION  0x0300  // Only need version from DirectX 3
#include <dsound.h>

// Sound is split into a series of 'segs', one seg for each frame
// (typically 367 samples long).
// The Loop buffer is a multiple of this seg length.

static IDirectSound *pds=NULL;   // DirectSound interface
static IDirectSoundBuffer *pdsbPrim=NULL; // Primary buffer
static IDirectSoundBuffer *pdsbLoop=NULL; // (Secondary) Loop buffer
int DSoundSamRate=44100;         // sample rate
int DSoundSegCount=5;           // Segs in the pdsbLoop buffer
static int cbLoopLen=0;          // Loop length (in bytes) calculated

int FramesPerSecond=60;          // Application fps
int FrameMult=0;                 // Frame multiplier
int RealFramesPerSecond=60;      // Application fps taking into account speedup/slowdown
int DSoundSegLen=0;             // Seg length in samples (calculated from Rate/Fps)
short *DSoundNextSound=NULL;     // The next sound seg we will add to the sample loop
static unsigned char DSoundOkay=0;     // True if DSound was initted okay
unsigned char DSoundPlaying=0;  // True if the Loop buffer is playing


static int BlankSound()
{
  void *pData=NULL,*pData2=NULL; DWORD Len=0,Len2=0;
  int Ret=0;
  // Lock the Loop buffer
  Ret=pdsbLoop->Lock(0,cbLoopLen,&pData,&Len,
    &pData2,&Len2,0);
  if (Ret<0) return 1;
  if (pData!=NULL) memset(pData,0,Len);

  // Unlock (2nd 0 is because we wrote nothing to second part)
  Ret=pdsbLoop->Unlock(pData,Len,pData2,0);

  // Also blank the DSoundNextSound buffer
  if (DSoundNextSound!=NULL) memset(DSoundNextSound,0,DSoundSegLen<<2);
  return 0;
}

#define WRAP_INC(x) { x++; if (x>=DSoundSegCount) x=0; }

static int DSoundNextSeg=0; // We have filled the sound in the loop up to the beginning of 'nNextSeg'

static int DSoundGetNextSoundFiller(int Draw)
{
  (void)Draw;
  if (DSoundNextSound==NULL) return 1;
  memset(DSoundNextSound,0,DSoundSegLen<<2); // Write silence into the buffer
  return 0;
}

int (*DSoundGetNextSound) (int Draw) = DSoundGetNextSoundFiller; // Callback used to request more sound

// This function checks the DSound loop, and if necessary gets some more sound
// and a picture.
int DSoundCheck()
{
  int PlaySeg=0,FollowingSeg=0;
  DWORD Play=0,Write=0;
  int Ret=0; int RetVal=0;
  if (pdsbLoop==NULL) { RetVal=1; goto End; }
  // We should do nothing until Play has left DSoundNextSeg
  Ret=pdsbLoop->GetCurrentPosition(&Play,&Write);

  PlaySeg=Play/(DSoundSegLen<<2);
  
  if (PlaySeg>DSoundSegCount-1) PlaySeg=DSoundSegCount-1;
  if (PlaySeg<0) PlaySeg=0; // important to ensure PlaySeg clipped for below
  
  if (DSoundNextSeg==PlaySeg)
  {
    // Don't need to do anything for a bit
    return 4;
  }

  // work out which seg we will fill next
  FollowingSeg=DSoundNextSeg; WRAP_INC(FollowingSeg)
  while (DSoundNextSeg!=PlaySeg)
  {
    void *pData=NULL,*pData2=NULL; DWORD Len=0,Len2=0;
    int Draw;
    // fill nNextSeg
    // Lock the relevant seg of the loop buffer
    Ret=pdsbLoop->Lock(DSoundNextSeg*(DSoundSegLen<<2),DSoundSegLen<<2,&pData,&Len,&pData2,&Len2,0);

    if (Ret>=0 && pData!=NULL)
    {
      // Locked the seg okay - write the sound we calculated last time
      memcpy(pData,DSoundNextSound,DSoundSegLen<<2);
    }
    // Unlock (2nd 0 is because we wrote nothing to second part)
    if (Ret>=0) pdsbLoop->Unlock(pData,Len,pData2,0); 

    Draw=(FollowingSeg==PlaySeg); // If this is the last seg of sound, flag Draw (to draw the graphics)

    DSoundGetNextSound(Draw); // get more sound into DSoundNextSound

    DSoundNextSeg=FollowingSeg;
    WRAP_INC(FollowingSeg)
  }
End:
  return 0;
}

int DSoundInit(HWND hWnd)
{
  int Ret=0;
  DSBUFFERDESC dsbd;
  WAVEFORMATEX wfx;

  if (DSoundSamRate<=0) return 1;
  if (FramesPerSecond<=0) return 1;
  if (FramesPerSecond>500) return 1;

  // Calculate the Seg Length and Loop length
  // (round to nearest sample)
  DSoundSegLen=(DSoundSamRate+(RealFramesPerSecond>>1))/RealFramesPerSecond;
  cbLoopLen=(DSoundSegLen*DSoundSegCount)<<2;

  // Make the format of the sound
  memset(&wfx,0,sizeof(wfx));
  wfx.cbSize=sizeof(wfx);
  wfx.wFormatTag=WAVE_FORMAT_PCM;
  wfx.nChannels=2; // stereo
  wfx.nSamplesPerSec=DSoundSamRate; // sample rate
  wfx.wBitsPerSample=16; // 16-bit
  wfx.nBlockAlign=4; // bytes per sample
  wfx.nAvgBytesPerSec=wfx.nSamplesPerSec*wfx.nBlockAlign;

  // Create the DirectSound interface
  Ret=DirectSoundCreate(NULL,&pds,NULL);
  if (Ret<0 || pds==NULL) return 1;

  // Set the coop level
  Ret=pds->SetCooperativeLevel(hWnd,DSSCL_PRIORITY);

  // Make the primary sound buffer
  memset(&dsbd,0,sizeof(dsbd));
  dsbd.dwSize=sizeof(dsbd);
  dsbd.dwFlags=DSBCAPS_PRIMARYBUFFER;
  Ret=pds->CreateSoundBuffer(&dsbd,&pdsbPrim,NULL);
  if (Ret<0 || pdsbPrim==NULL) { DSoundExit(); return 1; }

  // Set the format of the primary sound buffer (not critical if it fails)
  if (DSoundSamRate<44100) wfx.nSamplesPerSec=44100;
  Ret=pdsbPrim->SetFormat(&wfx);
  wfx.nSamplesPerSec=DSoundSamRate;

  // Make the loop sound buffer
  memset(&dsbd,0,sizeof(dsbd));
  dsbd.dwSize=sizeof(dsbd);
  // A standard secondary buffer (accurate position and plays in the background).
  dsbd.dwFlags=DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  dsbd.dwBufferBytes=cbLoopLen;
  dsbd.lpwfxFormat=&wfx; // Same format as the primary buffer
  Ret=pds->CreateSoundBuffer(&dsbd,&pdsbLoop,NULL);
  if (Ret<0 || pdsbLoop==NULL) { DSoundExit(); return 1; }
  
  DSoundNextSound=(short *)malloc(DSoundSegLen<<2); // The next sound block to put in the stream
  if (DSoundNextSound==NULL) { DSoundExit(); return 1; }

  DSoundOkay=1; // This module was initted okay

  return 0;
}

int DSoundPlay()
{
  if (DSoundOkay==0) return 1;
  BlankSound();
  // Play the looping buffer
  if (pdsbLoop->Play(0,0,DSBPLAY_LOOPING)<0) return 1;
  DSoundPlaying=1;
  return 0;
}

int DSoundStop()
{
  DSoundPlaying=0;
  if (DSoundOkay==0) return 1;
  // Stop the looping buffer
  pdsbLoop->Stop();
  return 0;
}

int DSoundExit()
{
  DSoundOkay=0; // This module is no longer okay
  if (DSoundNextSound!=NULL) free(DSoundNextSound);
  DSoundNextSound=NULL;

  // Release the (Secondary) Loop Sound Buffer
  if (pdsbLoop!=NULL) pdsbLoop->Release();
  pdsbLoop=NULL;

  // Release the Primary Sound Buffer
  if (pdsbPrim!=NULL) pdsbPrim->Release();
  pdsbPrim=NULL;

  // Release the DirectSound interface
  if (pds!=NULL) pds->Release();
  pds=NULL;
  return 0;
}
