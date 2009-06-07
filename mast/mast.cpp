// Mast - main module
#include "mastint.h"
#include "md5.h"

#ifdef EMU_DOZE
#define EMU_VERSION 0x0000
#elif defined(EMU_Z80JB)
#define EMU_VERSION 0x4000
#endif

unsigned int MastVer=0x1160 | EMU_VERSION; // Version number of the library (bits 30+31 indicate which Z80 simulator is in use)

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

#ifdef EMU_Z80JB
  MastSetMemHandlers();
  Z80Init();
#endif

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

void MastFlagsFromHeader()
{
  unsigned int i;
  unsigned short offsets[] = {0x1ff0, 0x3ff0, 0x7ff0}; // possible header locations
  unsigned char signature[] = "TMR SEGA"; // the header's signature
  for (i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++)
  {
    unsigned short ofs = offsets[i];
    if (ofs+15 >= Mastz.RomLen) break;
    if (memcmp(Mastz.Rom+ofs, signature, 8) == 0) // did we find a header?
    {
      unsigned char region = Mastz.Rom[ofs+15] >> 4;
      if (region == 3 || region == 4) // 0x03 -> Japan, 0x04 -> Export
      { // SMS
        MastEx &= ~MX_GG;
        if (region == 3)
          MastEx |= MX_JAPAN;
        else
          MastEx &= ~MX_JAPAN;
      }
      else if (region >= 5 && region <= 7) // 0x05 -> Japan, 0x06 Export, 0x07 International
      { // GG
        MastEx |= MX_GG;
        if (region == 5)
          MastEx |= MX_JAPAN;
        else
          MastEx &= ~MX_JAPAN;
      } // I haven't seen any ROM that actually had a header that didn't fit any of those five values, so that should be about it
      break; // We found a header, no need to check at any of the other possible locations
    }
  }
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

#ifdef EMU_DOZE
  DozeReset();
  Doze.sp=0xdff0; // bios sets
#elif defined(EMU_Z80JB)
  Z80Reset();
  Z80.sp.w.l=0xdff0; // bios sets
#endif

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

