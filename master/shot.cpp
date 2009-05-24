#include "app.h"
#include "zlib.h"
// Screen Shots

static unsigned char *Scr=NULL;
static int ScrLen=0;
// Grab screenshots as true color, and leave a zero byte at the start of each line
// (= no filtering in PNG)

// Called before rendering a frame
int ShotStart()
{
  ShotExit(); // Deallocate last screenshot

  if (MastEx&MX_GG) ScrLen=481*144;
  else              ScrLen=769*192;

  Scr=(unsigned char *)malloc(ScrLen);
  if (Scr==NULL) return 1;
  memset(Scr,0,ScrLen);
  return 0;
}

// Called to deallocate the screen shot
int ShotExit()
{
  if (Scr!=NULL) free(Scr);  Scr=NULL; ScrLen=0;
  return 0;
}

// Called to get a scanline from Mdraw
void ShotLine()
{
  unsigned char *pd,*pEnd,*ps;
  int Divb=12;

  if (Scr==NULL) return;

  // Find out the areas to convert across
  if (MastEx&MX_GG)
  {
    if (Mdraw.Line< 24)     return;
    if (Mdraw.Line>=24+144) return;
    pd=Scr+1+(Mdraw.Line-24)*481; pEnd=pd+480;
    ps=Mdraw.Data+16+48;
    Divb=15;
  }
  else
  {
    if (Mdraw.Line<   0) return;
    if (Mdraw.Line>=192) return;
    pd=Scr+1+Mdraw.Line*769; pEnd=pd+768;
    ps=Mdraw.Data+16;
  }

  // Get the scanline into our buffer as bb,gg,rr
  do
  {
    int v;
    v=Mdraw.Pal[*ps]; // (0000rrrr ggggbbbb)
    pd[0]=(unsigned char)((( v    &Divb)*255)/Divb);
    pd[1]=(unsigned char)((((v>>4)&Divb)*255)/Divb);
    pd[2]=(unsigned char)((((v>>8)&Divb)*255)/Divb);
    pd+=3; ps++;
  }
  while (pd<pEnd);
}

static void WriteInt(unsigned char *d,unsigned int v)
{
  d[0]=(unsigned char)(v>>24);
  d[1]=(unsigned char)(v>>16);
  d[2]=(unsigned char)(v>> 8);
  d[3]=(unsigned char) v     ;
}

// Save the screenshot to a PNG file
static int ShotSaveAs(char *Name)
{
  FILE *f=NULL;
  unsigned char *Comp=NULL; unsigned long CompLen=0;
  static unsigned char PngSig[]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
  static unsigned char PngEnd[]={0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82};
  unsigned char Buf[0x1c];
  unsigned int ScrX=256,ScrY=192,Crc=0;

  if (Scr==NULL) return 1;
  f=fopen(Name,"wb"); if (f==NULL) return 1;

  // Write PNG signature
  fwrite(PngSig,1,sizeof(PngSig),f);

  // Write IHDR chunk
  if (MastEx&MX_GG) { ScrX=160; ScrY=144; }
  memset(Buf,0,sizeof(Buf));
  Buf[3]=0x0d;
  memcpy(Buf+4,"IHDR",4);
  WriteInt(Buf+0x08,ScrX);
  WriteInt(Buf+0x0c,ScrY);
  Buf[0x10]=8; Buf[0x11]=2;
  Crc=crc32(0,Buf+0x04,0x0d+4);
  WriteInt(Buf+0x15,Crc);
  fwrite(Buf,1,0x19,f);

  // Write IDAT chunk
  CompLen=ScrLen+(ScrLen>>9)+12 +12; // Create a buffer big enough (plus 12 for length,IDAT,crc)
  Comp=(unsigned char *)malloc(CompLen);
  if (Comp==NULL) { fclose(f); return 1; }
  memset(Comp,0,CompLen);
  CompLen-=12;
  compress(Comp+8,&CompLen,Scr,ScrLen);

  WriteInt(Comp,CompLen);
  memcpy(Comp+4,"IDAT",4);
  Crc=crc32(0,Comp+4,CompLen+4); // Do a crc of IDAT plus data
  WriteInt(Comp+8+CompLen,Crc); // Write crc after

  fwrite(Comp,1,CompLen+12,f);
  free(Comp);

  // Write IEND chunk
  fwrite(PngEnd,1,sizeof(PngEnd),f);
  fclose(f);
  return 0;
}

static char *ShotName(unsigned int Num)
{
  char *Name=NULL; int Len=0;
  char NumString[16];

  if (EmuTitle==NULL) return NULL;
  Len=strlen(EmuTitle);
  Name=(char *)malloc(Len+15); // shots\ and _nnn .png\0
  if (Name==NULL) return NULL;
  memcpy(Name,"shots\\",6);
  memcpy(Name+6,EmuTitle,Len);
  sprintf (NumString,"_%.3d",Num);
  memcpy(Name+6+Len,NumString,4);
  memcpy(Name+6+Len+4,".png",5);
  return Name;
}

int ShotSave()
{
  char *Name=NULL;
  unsigned int i=0;
  int Ret=0;
  char Text[128]="";

  // Make the next save number
  for (i=0;i<1000;i++)
  {
    FILE *f=NULL;
    Name=ShotName(i); if (Name==NULL) return 1;
    // See if it exists
    f=fopen(Name,"rb"); if (f==NULL) break; // success - doesn't exist
    fclose(f);
    free(Name); Name=NULL;
  }
  if (Name==NULL) return 1;

  CreateDirectory("shots",NULL); // Make sure there is a directory
  Ret=ShotSaveAs(Name); if (Ret!=0) { free(Name); return 1; }

  sprintf (Text,"Saved screenshot to %.100s",Name);
  RunText(Text,2*60);
    
  free(Name);
  return 0;
}
