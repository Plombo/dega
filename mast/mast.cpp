// Mast - main module
#include "mastint.h"
#include "md5.h"

int MastVer=0x1140; // Version number of the library

unsigned char MastInput[2]={0,0}; // Joypads
unsigned int MastEx=0; // Extra options

int MastDrawDo=0; // 1 to draw image

struct Masta Masta={0};
struct Mastb *pMastb=NULL;
struct Mastz Mastz={0};

char MastRomName[128] = "";

int MastInit()
{
  MdrawInit();
  // Init space for the machine state
  memset(&Masta,0,sizeof(Masta));

  pMastb=(struct Mastb *)malloc(sizeof(*pMastb));
  if (pMastb==NULL) return 1;
  
  memset(&Mastz,0,sizeof(Mastz));
  MastSetRom(NULL,0);

  return 0;
}

int MastExit()
{
  MastSetRom(NULL,0);

  // Exit space for the machine state
  memset(&Masta,0,sizeof(Masta));
  if (pMastb!=NULL) free(pMastb);  pMastb=NULL;
  memset(&Mastz,0,sizeof(Mastz));

  return 0;
}

int MastSetRom(unsigned char *Rom,int RomLen)
{
  memset(&Mastz,0,sizeof(Mastz));
  Mastz.Rom=Rom; Mastz.RomLen=RomLen;
  MastMapMemory(); // Map memory
  MastHardReset(); // Start from empty state and battery
  return 0;
}

void MastSetRomName(char *name)
{
  memset(MastRomName, 0, sizeof(MastRomName));
  strncpy(MastRomName, name, sizeof(MastRomName)); MastRomName[sizeof(MastRomName)-1] = '\0';
}

void MastGetRomDigest(unsigned char *digest)
{
  Md5_t md5;
  int RealLen=Mastz.RomLen;
  if (RealLen&2) RealLen-=2; // account for overrun if present

  Md5Initialize(&md5);
  Md5Update(&md5, Mastz.Rom, RealLen);
  Md5Final(&md5, digest);
}

int MastReset()
{
  // Reset banks
  Masta.Bank[0]=0; Masta.Bank[1]=0; Masta.Bank[2]=1; Masta.Bank[3]=0;
  MastMapPage0(); MastMapPage1(); MastMapPage2();
  // Reset vdp
  memset(&Masta.v,0,sizeof(Masta.v));
  Masta.v.Reg[10]=0xff;
  pMastb->ThreeD=0;

  memset(pMastb->CRam,0,sizeof(pMastb->CRam));
  // Update the colors in Mdraw
  MdrawCramChangeAll();

  // Reset sound
  memset(&Masta.p,0,sizeof(Masta.p));
  DpsgRecalc();

  DozeReset();
  Doze.sp=0xdff0; // bios sets

  return 0;
}

int MastHardReset()
{
  // Hard reset all memory, including battery
  memset(&Masta,0,sizeof(Masta));
  memset(pMastb,0,sizeof(*pMastb));
  MsndRefresh(); // Reset sound
  return MastReset();
}

