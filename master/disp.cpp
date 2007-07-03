#include "app.h"

static IDirectDraw *pDD=NULL;
static IDirectDrawSurface *DispPrim=NULL,*DispBuff=NULL,*DispOver=NULL,*DispBack=NULL;
static IDirectDrawClipper *pClipper=NULL;
unsigned char *DispMem=NULL;
int DispMemPitch=0;
unsigned int DispFormat=0; // Either a FourCC code or a color depth
int DispBpp=0; // Bytes per pixel
int TryOverlay=2;
static int OverlayColor=0;
static RECT LastOver={0,0,0,0}; // Last overlay rectangle

// Create the primary surface
static int MakePrimarySurface()
{
  int Ret=0; unsigned int PrimFormat=0; int PrimBpp=0;
  DDSURFACEDESC ddsd;
  DispPrim=NULL;
  memset(&ddsd,0,sizeof(ddsd));
  ddsd.dwSize=sizeof(ddsd);
  ddsd.dwFlags=DDSD_CAPS;
  ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;
  Ret=pDD->CreateSurface(&ddsd,&DispPrim,NULL);
  if (Ret<0) return 1; if (DispPrim==NULL) return 1; 

  // Find out the color to use to paint the overlay
  GetSurfaceFormat(DispPrim,&PrimFormat,&PrimBpp);
  switch (PrimFormat)
  {
    default: OverlayColor=0; break;
    case 15: OverlayColor=0x0401; break;
    case 16: OverlayColor=0x0801; break;
    case 24: case 32: OverlayColor=0x080008; break;
  }

  // Create a clipper
  Ret=DirectDrawCreateClipper(0,&pClipper,NULL);
  if (Ret==DD_OK)
  {
    Ret=pClipper->SetHWnd(0,hFrameWnd);
    if (Ret==DD_OK) DispPrim->SetClipper(pClipper);
  }

  return 0;
}

static int OverlayPut()
{
  // Show the overlay
  RECT Dest={0,0,0,0};
  DDOVERLAYFX ofx;
  if (DispOver==NULL) return 1;

  GetClientScreenRect(hFrameWnd,&Dest);
  Dest.bottom-=StatusHeight();

  if (memcmp(&LastOver,&Dest,sizeof(Dest))==0) return 0; // Overlay already in the right place

  memset(&ofx,0,sizeof(ofx));
  ofx.dwSize=sizeof(ofx);
  ofx.dckDestColorkey.dwColorSpaceLowValue =OverlayColor;
  ofx.dckDestColorkey.dwColorSpaceHighValue=OverlayColor;

  DispOver->UpdateOverlay(NULL,DispPrim,&Dest,
    DDOVER_SHOW|DDOVER_DDFX|DDOVER_KEYDESTOVERRIDE,&ofx);

  LastOver=Dest;
  return 0;
}

static int NormalPut()
{
  int Ret=0;
  RECT Dest={0,0,0,0};
  if (DispPrim==NULL) return 1;
  if (DispBuff==NULL) return 1;
  // buffer surface --> primary surface
  GetClientScreenRect(hFrameWnd,&Dest);
  Dest.bottom-=StatusHeight();

  // Normal
  Ret=DispPrim->Blt(&Dest,DispBuff,NULL,DDBLT_WAIT,NULL);
  if (Ret<0) return 1;
  return 0;
}

static int DoPaint()
{
  // As called by WM_PAINT commands
  DDBLTFX bfx; RECT Dest={0,0,0,0};
  if (DispPrim==NULL) return 1;
  if (DispBuff!=NULL) { return NormalPut(); }
  if (DispOver==NULL) return 0; // Don't draw anything

  memset(&bfx,0,sizeof(bfx));
  bfx.dwSize=sizeof(bfx);

  // Paint the color in the overlay key color
  bfx.dwFillColor=OverlayColor;
  GetClientScreenRect(hFrameWnd,&Dest);
  DispPrim->Blt(&Dest,NULL,NULL,DDBLT_COLORFILL,&bfx);

  OverlayPut();
  return 0;
}

int DispPaint()
{
  // Stop function from being reentered
  static unsigned char ReEntry=0;
  int Ret=0;
  if (ReEntry) return 1;
  ReEntry=1; Ret=DoPaint(); ReEntry=0;
  return Ret;
}

// Create an Overlay surface
static int MakeOverlaySurface()
{
  static DDPIXELFORMAT PixelFormat[4]=
  {
    {sizeof(DDPIXELFORMAT),DDPF_RGB,0,16,0x7c00,0x03e0,0x001f,0},
    {sizeof(DDPIXELFORMAT),DDPF_RGB,0,16,0xf800,0x07e0,0x001f,0},
    {sizeof(DDPIXELFORMAT),DDPF_FOURCC,MAKEFOURCC('U','Y','V','Y'),0,0,0,0,0},
    {sizeof(DDPIXELFORMAT),DDPF_FOURCC,MAKEFOURCC('Y','U','Y','2'),0,0,0,0,0}
  };
  int Ret=0,Type=0;
  DDSURFACEDESC ddsd;

  memset(&ddsd,0,sizeof(ddsd));
  ddsd.dwSize=sizeof(ddsd);
  ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_BACKBUFFERCOUNT;
  ddsd.ddsCaps.dwCaps=DDSCAPS_OVERLAY|DDSCAPS_FLIP|DDSCAPS_COMPLEX;
  ddsd.dwWidth=ScrnWidth;
  ddsd.dwHeight=ScrnHeight;
  ddsd.dwBackBufferCount=2;

  // Try several pixelformats
  DispOver=NULL;
  Type=2; // Try YUV overlays
  if (TryOverlay==2) Type=0; // Try RGB overlays too
  for (;Type<4;Type++)
  {
    ddsd.ddpfPixelFormat=PixelFormat[Type];
    Ret=pDD->CreateSurface(&ddsd,&DispOver,NULL);
    if (Ret<0) DispOver=NULL;
    if (DispOver!=NULL) break;
  }
 
  if (DispOver==NULL) return 1;

  // Get the back buffer
  memset(&ddsd.ddsCaps,0,sizeof(ddsd.ddsCaps));
  ddsd.ddsCaps.dwCaps=DDSCAPS_BACKBUFFER; 
  DispBack=NULL; DispOver->GetAttachedSurface(&ddsd.ddsCaps,&DispBack);

  // Find out the color depth
  GetSurfaceFormat(DispOver,&DispFormat,&DispBpp);

  // Clear it
  SurfaceClear(DispOver,0);

  // Show and size it
  memset(&LastOver,0,sizeof(LastOver));
  OverlayPut();
  return 0;
}

// Create the buffer surface
static int MakeBufferSurface()
{
  int Ret=0;
  int UseSys=0;
  DDSURFACEDESC ddsd;
  UseSys=AutodetectUseSys(pDD);
  DispBuff=NULL;
  memset(&ddsd,0,sizeof(ddsd));
  ddsd.dwSize=sizeof(ddsd);
  ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
  ddsd.dwWidth=ScrnWidth;
  ddsd.dwHeight=ScrnHeight;
TryAgain:
  ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
  if (UseSys) ddsd.ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY;
  else         ddsd.ddsCaps.dwCaps|=DDSCAPS_VIDEOMEMORY;
  Ret=pDD->CreateSurface(&ddsd,&DispBuff,NULL);
  if (Ret<0 && UseSys!=1) { UseSys=1; goto TryAgain; } // Try again in system memory
  if (Ret<0) return 1; if (DispBuff==NULL) return 1; 

  // Find out the color depth
  GetSurfaceFormat(DispBuff,&DispFormat,&DispBpp);

  // Clear it
  SurfaceClear(DispBuff,0);
  return 0;
}

int DispInit()
{
  int Ret=0,MemLen=0;

  // Create the DirectDraw interface
  pDD=NULL; Ret=DirectDrawCreate(NULL,&pDD,NULL);
  if (Ret<0) return 1; if (pDD==NULL) return 1; 

  // Set cooperation level
  pDD->SetCooperativeLevel(hFrameWnd,DDSCL_NORMAL);

  // Make primary surface
  Ret=MakePrimarySurface(); if (Ret!=0) return 1;
  
  Ret=1;
  if (TryOverlay) Ret=MakeOverlaySurface(); // Try overlay first
  if (Ret!=0)
  {
    // Else make buffer surface
    Ret=MakeBufferSurface(); if (Ret!=0) return 1;
  }

  // Render Init (precalc all colors)
  RenderInit();

  // Create the memory buffer
  DispMemPitch=ScrnWidth*DispBpp;
  MemLen=ScrnHeight*DispMemPitch;
  DispMem=(unsigned char *)malloc(MemLen); if (DispMem==NULL) return 1;
  memset(DispMem,0,MemLen);

  return 0;
}

int DispExit()
{
  // Release everything
  if (DispMem!=NULL) free(DispMem);  DispMem=NULL;
  DispMemPitch=0;
  DispFormat=0; DispBpp=0;

  if (DispOver!=NULL && DispPrim!=NULL)
  {
    // Hide overlay
    DispOver->UpdateOverlay(NULL,DispPrim,NULL,DDOVER_HIDE,NULL);
  }
  if (DispOver!=NULL) DispOver->Release();  DispOver=NULL;
  DispBack=NULL; // A single call releases all surfaces

  memset(&LastOver,0,sizeof(LastOver));

  if (DispBuff!=NULL) DispBuff->Release();  DispBuff=NULL;
  if (DispPrim!=NULL) DispPrim->SetClipper(NULL); // Detach clipper
  if (pClipper!=NULL) pClipper->Release();  pClipper=NULL;
  if (DispPrim!=NULL) DispPrim->Release();  DispPrim=NULL;
  if (pDD     !=NULL) pDD->Release();       pDD=NULL;
  return 0;
}

// Copy DispMem to DestSurf
static int MemToSurf(IDirectDrawSurface *DestSurf)
{
  DDSURFACEDESC ddsd; int Ret=0;
  unsigned char *pd=NULL,*ps=NULL; int y=0;
  unsigned char *Surf=NULL; int Pitch=0;
  if (DestSurf==NULL) return 1;
  if (DispMem==NULL) return 1;

  // Lock the surface so we can write to it
  memset(&ddsd,0,sizeof(ddsd));
  ddsd.dwSize=sizeof(ddsd);

  Ret=DestSurf->Lock(NULL,&ddsd,DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT,NULL);
  if (Ret<0) return 1;
  if (ddsd.lpSurface==NULL) { DestSurf->Unlock(NULL); return 1; }

  Surf=(unsigned char *)ddsd.lpSurface;
  Pitch=ddsd.lPitch;

  pd=Surf; ps=DispMem;
  for (y=0;y<ScrnHeight; y++,pd+=Pitch,ps+=DispMemPitch)
  {
    memcpy(pd,ps,DispMemPitch);
  }

  Ret=DestSurf->Unlock(NULL);

  return 0;
}

// Draw the buffer memory onto the screen using the overlay
static int DispDrawOverlay()
{
  if (DispBack!=NULL)
  {
    MemToSurf(DispBack);
    DispOver->Flip(NULL,DDFLIP_WAIT);
  }
  else
  {
    MemToSurf(DispOver);
  }
  // Resize the overlay if needed
  OverlayPut();
  return 0;
}

// Draw the buffer memory onto the primary surface
int DispDraw()
{
  int Ret=0;
  if (DispPrim==NULL) return 1;
  if (DispPrim->IsLost()) { DispPrim->Restore(); } // Restore surface if lost

  if (DispOver!=NULL) { return DispDrawOverlay(); } // Use overlay if present

  // Else use normal Buffer

  if (DispBuff==NULL) return 1;
  if (DispBuff->IsLost()) { DispBuff->Restore(); } // Restore surface if lost

  // buffer memory --> buffer surface
  Ret=MemToSurf(DispBuff); if (Ret!=0) return 1;

  return NormalPut();
}
