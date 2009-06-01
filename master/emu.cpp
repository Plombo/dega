// Emu module
#include "app.h"
#include <ctype.h>

char EmuRomName[256]="";
char EmuStateName[256]="";

char *EmuTitle=NULL;

static unsigned char *EmuRom=NULL;
static int EmuRomLen=0;

int PortMemicmp(char *s1, char *s2, size_t len) {
	while (len > 0) {
		int c1 = tolower(*s1), c2 = tolower(*s2);
		if (c1 != c2) return c1-c2;
		s1++, s2++, len--;
	}
	return 0;
}

int EmuInit()
{
  int Ret=0;
  Ret=MastInit(); if (Ret!=0) { AppError("MastInit Failed",0); return 1; }

  EmuTitle=NULL;
  return 0;
}

int EmuExit()
{
  MastExit();
  return 0;
}

// Change all upper case titles to lower case
static int NoUpper(char *Title)
{
  int i=0,Len=0;
  // Change to lower case
  if (Title==NULL) return 1;
  Len=strlen(EmuTitle);
  
  for (i=0;i<Len;i++)
  {
    int c=EmuTitle[i];
    if (c>='a' && c<='z') return 0;
  }
  // No lower case letters

  for (i=0;i<Len;i++) { EmuTitle[i]=(char)tolower(EmuTitle[i]); }
  return 0;
}

// Adjusts an allocated rom so it will work with the Mast library
static int AdjustRom(unsigned char *& Rom,int& RomLen)
{
  int Len=0;
  void *NewMem=NULL;
  if (Rom==NULL) return 1;
  if (RomLen<=0) return 1;

  Len=RomLen;
  // Check for headers and knock them off:
  if ((Len&0x3fff)==0x0200) { memmove(Rom,Rom+0x200,Len-0x200); Len-=0x200; }
  Len+=0x3fff; Len&=0x7fffc000; Len+=2; // Round up to a page (+ overrun)

  // Expand the rom to the correct size
  NewMem=realloc(Rom,Len);
  if (NewMem==NULL) return 1;
  Rom=(unsigned char *)NewMem;
  if (Len>RomLen) memset(Rom+RomLen,0,Len-RomLen); // Fill up with zeros
  RomLen=Len;
  return 0;
}

int EmuLoad()
{
  int Ret=0;
  char *EmuRomFileName;
  if (EmuRomName==NULL) return 1;
  if (EmuRomName[0]==0) return 1;

  EmuFree(); // make sure nothing is loaded

  Ret=ZipOpen(EmuRomName);
  if (Ret==0)
  {
    // Successfully opened as a zip file - load it
    ZipRead(&EmuRom,&EmuRomLen);
    ZipClose();
  }
  else
  {
    // If zip open fails, load up the rom normally
    FILE *f=NULL;
    f=fopen(EmuRomName,"rb"); if (f==NULL) return 1;
    fseek(f,0,SEEK_END); EmuRomLen=ftell(f);
    fseek(f,0,SEEK_SET);
    EmuRom=(unsigned char *)malloc(EmuRomLen);
    if (EmuRom==NULL) { fclose(f); return 1; }
    memset(EmuRom,0,EmuRomLen);
    fread(EmuRom,1,EmuRomLen,f);
    fclose(f);

    EmuRomFileName=strrchr(EmuRomName,'\\');
    if (EmuRomFileName) { EmuRomFileName++; }
                   else { EmuRomFileName=EmuRomName; }
    MastSetRomName(EmuRomFileName);
  }

  if (EmuTitle!=NULL) free(EmuTitle);
  EmuTitle=GetStubName(EmuRomName);
  NoUpper(EmuTitle);

  AdjustRom(EmuRom,EmuRomLen);

  MastSetRom(EmuRom,EmuRomLen); // Plug in the rom
  MastFlagsFromHeader();
  return 0;
}

// Free the loaded rom
int EmuFree()
{
  MastSetRom(NULL,0); // Unplug the rom
  MastEx&=~MX_GG;

  if (EmuTitle!=NULL) free(EmuTitle);  EmuTitle=NULL;

  if (EmuRom!=NULL) free(EmuRom);  EmuRom=NULL; EmuRomLen=0;
  return 0;
}

int EmuFrame()
{
  if (EmuRom==NULL) return 1;
  return MastFrame();
}
