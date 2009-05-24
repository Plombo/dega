// Graphics Rendering
#include "app.h"

static unsigned int DispWholePal[0x1000];
static unsigned int DispRedPal  [0x1000];
static unsigned int DispCyanPal [0x1000];
static unsigned char *pLine=NULL; int Len=256;
static unsigned char *ps=NULL;
int RedBlue3D=1;

static int ColAdjust(int& r,int& g,int& b,int Filter)
{
#define CLIPA(x) if (x<0) x=0; if (x>255) x=255;

  if (Filter==0) return 0;
  // Red data will be lost, carry a bit over to green and blue
  g=(r+(g<<2))/5; b=(r+b)>>1;
  // Copy red brightness from blue/green
  r=((g<<1)+b)/3;
  // Green and blue are too bright (and show through red lens), so darken them
  g>>=1;  b*=3; b>>=2;
       if (Filter==1) { g=0; b=0; }
  else if (Filter==2) { r=0; }

  CLIPA(r) CLIPA(g) CLIPA(b)
  return 0;
}

static void RgbToYuv(int& r,int& g,int& b,int& y,int& u,int& v)
{
  y=r*73+g*149+b*34; y>>=8;
  u=-36*r - 74*g + 110*b; u>>=8; u+=0x80;
  v= 92*r - 74*g -  18*b; v>>=8; v+=0x80;
}

static int WholePalMake(unsigned int *pw,int Filter)
{
  int i;
  // Precalc the whole 4096 color palette
  for (i=0; i<0x1000; i++,pw++)
  {
    int r,g,b,c; r=i&15; g=(i>>4)&15; b=(i>>8)&15;
    // Convert to maximum colors
    r*=0xff; g*=0xff; b*=0xff;
    if (MastEx&MX_GG) { r/=15; g/=15; b/=15; }
    else { r/=12; g/=12; b/=12; } // Colors e.g. 13,13,13 will overflow, but are unused

    ColAdjust(r,g,b,Filter);

    c=i;
    switch (DispFormat)
    {
      int y,u,v;
      default: break;
      case 15: c=(r&0xf8)<<7; c|=(g&0xf8)<<2; c|=b>>3; break;
      case 16: c=(r&0xf8)<<8; c|=(g&0xfc)<<3; c|=b>>3; break;
      case 24: case 32:  c=r<<16; c|=g<<8; c|=b; break;
      case 'U'|('Y'<<8)|('V'<<16)|('Y'<<24):

        RgbToYuv(r,g,b,y,u,v); c= u |(y<<8)|(v<<16)|(y<<24);
      break;
      case 'Y'|('U'<<8)|('Y'<<16)|('2'<<24):
        RgbToYuv(r,g,b,y,u,v); c= y |(u<<8)|(y<<16)|(v<<24);
      break;
    }
    *pw=c;
  }
  return 0;
}

int RenderInit()
{
  WholePalMake(DispWholePal,0);
  WholePalMake(DispRedPal,  1);
  WholePalMake(DispCyanPal, 2);
  return 0;
}

static void DrawLine1()
{
  // Draw line at 1 byte per pixel
  unsigned char *pd,*pe; unsigned int *pwp;
  
  pd=pLine; pe=pd+Len;
  pwp=DispWholePal;
  do { *pd=(unsigned char)pwp[Mdraw.Pal[*ps]]; pd++; ps++; }
  while (pd<pe);
}

static void DrawLine2()
{
  // Draw line at 2 bytes per pixel
  unsigned short *pd,*pe; unsigned int *pwp;
  pd=(unsigned short *)pLine; pe=pd+Len;
      
  if (DispFormat>16)
  {
    // YUV format    
    pwp=DispWholePal;
    do
    {
      *pd=(unsigned short) pwp[Mdraw.Pal[*ps]]     ; pd++; ps++;
      *pd=(unsigned short)(pwp[Mdraw.Pal[*ps]]>>16); pd++; ps++;
    }
    while (pd<pe);
    return;
  }

  if (RedBlue3D==0 || Mdraw.ThreeD<2)
  {
    // Normal
    pwp=DispWholePal;
    do { *pd=(unsigned short)pwp[Mdraw.Pal[*ps]]; pd++; ps++; } while (pd<pe);
    return;
  }

  if (Mdraw.ThreeD==3)
  {
    // Left = red
    unsigned short OnlyCyan=0x07ff;
    pwp=DispRedPal;
    if (DispFormat==15) OnlyCyan=0x03ff;

    do
    { unsigned short Col,Val;
      Val=*pd; Col=(unsigned short)pwp[Mdraw.Pal[*ps]];
      Val&=OnlyCyan; Val|=Col; *pd=Val; pd++; ps++;
    } while (pd<pe);
    return;
  }

  {
    // Right = cyan
    unsigned short OnlyRed=0xf800;
    pwp=DispCyanPal;
    if (DispFormat==15) OnlyRed=0x7c00;
    do
    { unsigned short Col,Val;
      Val=*pd; Col=(unsigned short)pwp[Mdraw.Pal[*ps]];
      Val&=OnlyRed; Val|=Col; *pd=Val; pd++; ps++;
    } while (pd<pe);
    return;
  }
}

static void DrawLine3()
{
  // Draw line at 3 bytes per pixel
  unsigned char *pd,*pe; unsigned int *pwp;
  pd=pLine; pe=pd+(Len<<1)+Len;
  
  if (RedBlue3D==0 || Mdraw.ThreeD<2)
  {
    // Normal
    pwp=DispWholePal;
    do
    { unsigned int Col; Col=pwp[Mdraw.Pal[*ps]];
      *pd++=(unsigned char)Col; Col>>=8;
      *pd++=(unsigned char)Col; Col>>=8;
      *pd++=(unsigned char)Col; ps++;
    } while (pd<pe);
    return;
  }

  if (Mdraw.ThreeD==3)
  {
    // Left = red
    pwp=DispRedPal;
    do
    { unsigned char *pCol; pCol=(unsigned char *)(pwp+Mdraw.Pal[*ps]);
      pd+=2; *pd++=pCol[2]; // Copy Red byte only
      ps++;
    } while (pd<pe);
    return;
  }

  // Right = cyan
  pwp=DispCyanPal;
  do
  { unsigned char *pCol; pCol=(unsigned char *)(pwp+Mdraw.Pal[*ps]);
    *pd++=pCol[0]; *pd++=pCol[1]; pd++; ps++; // Copy Blue and Green bytes
  } while (pd<pe);
  return;
}

static void DrawLine4()
{
  // Draw line at 4 bytes per pixel
  unsigned int *pd,*pe; unsigned int *pwp;
  pd=(unsigned int *)pLine; pe=pd+Len;

  if (RedBlue3D==0 || Mdraw.ThreeD<2)
  {
    // Normal
    pwp=DispWholePal;
    do { *pd=pwp[Mdraw.Pal[*ps]]; pd++; ps++; } while (pd<pe);
    return;
  }

  if (Mdraw.ThreeD==3)
  {
    // Left = red
    pwp=DispRedPal;
    do
    {
      unsigned char *pCol; pCol=(unsigned char *)(pwp+Mdraw.Pal[*ps]);
      ((unsigned char *)pd)[2]=pCol[2]; // Copy Red byte only
      pd++; ps++;
    } while (pd<pe);
    return;
  }

  // Right = cyan
  pwp=DispCyanPal;
  do
  {
    unsigned char *pCol; pCol=(unsigned char *)(pwp+Mdraw.Pal[*ps]);
    ((unsigned char *)pd)[0]=pCol[0]; // Copy Blue byte
    ((unsigned char *)pd)[1]=pCol[1]; // Copy Green byte
    pd++; ps++;
  } while (pd<pe);
  return;
}

void MdrawCall()
{
  int y;
  ShotLine(); // Pass onto screenshot

  // Callback with scanline in Mdraw
  if (DispMem==NULL) return;

  // Find destination line
  y=Mdraw.Line; if (MastEx&MX_GG) y-=24;
  if (y<0) return; if (y>=ScrnHeight) return;
  pLine=DispMem+y*DispMemPitch;

  // Find source pixels
  ps=Mdraw.Data+16; Len=256;
  if (MastEx&MX_GG) { ps+=48; Len=160; }

  // Draw the line in the relevant bytes per pixel
  switch (DispBpp)
  {
    default: DrawLine1(); return;
    case 2:  DrawLine2(); return;
    case 3:  DrawLine3(); return;
    case 4:  DrawLine4(); return;
  }
}
