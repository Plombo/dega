// Mast - map memory module
#include "mastint.h"

#ifdef EMU_DOZE
#define MemFetch DozeMemFetch
#define MemRead DozeMemRead
#define MemWrite DozeMemWrite
#elif defined(EMU_Z80JB)
unsigned char *MemFetch[0x100], *MemRead[0x100], *MemWrite[0x100];
#endif

int MastMapMemory()
{
  // Map in framework
  int i=0;

  memset(&MemFetch,0,sizeof(MemFetch));
  memset(&MemRead, 0,sizeof(MemRead));
  memset(&MemWrite,0,sizeof(MemWrite));

  // 0000-03ff Fixed Rom view
  for (i=0x00;i<0x04;i++)
  { MemFetch[i]=MemRead[i]=Mastz.Rom; MemWrite[i]=0; }

  // c000-dfff Ram
  for (i=0xc0;i<0xe0;i++)
  { MemFetch[i]=MemRead[i]=MemWrite[i]=pMastb->Ram-0xc000; }
  // e000-ffff Ram mirror
  for (i=0xe0;i<0x100;i++)
  { MemFetch[i]=MemRead[i]=MemWrite[i]=pMastb->Ram-0xe000; }

  // For bank writes ff00-ffff callback Doze*
  MemWrite[0xff]=0;
  // Map in pages
  MastMapPage0(); MastMapPage1(); MastMapPage2();
  return 0;
}

static INLINE void CalcRomPage(int n)
{
  // Point to the rom page selected for page n
  int b; int PageOff; int Fold=0xff;
  b=Masta.Bank[1+n];
TryLower:
  PageOff=(b&Fold)<<14;
  if (PageOff+0x4000>Mastz.RomLen) // Set if the page exceeds the rom length
  {
    PageOff=0;
    if (Fold) { Fold>>=1; goto TryLower; } // (32k games, spellcaster, jungle book)
  }

  Mastz.RomPage[n]=PageOff; // Store in the Mastz structure
}

static INLINE unsigned char *GetRomPage(int n)
{
  CalcRomPage(n); // Recalc the rom page
  return Mastz.Rom+Mastz.RomPage[n]; // Get the direct memory pointer
}

// 0400-3fff Page 0
void MastMapPage0()
{
  unsigned char *Page; Page=GetRomPage(0);
  // Map Rom Page
  {
    int i=0;
    for (i=0x04;i<0x40;i++)
    {
      MemFetch[i]=MemRead[i]=Page;
      MemWrite[i]=0;
    }
  }
}

// 4000-7fff Page 1
void MastMapPage1()
{
  unsigned char *Page; Page=GetRomPage(1);
  // Map Rom Page
  {
    int i=0;
    Page-=0x4000;
    for (i=0x40;i<0x80;i++)
    {
      MemFetch[i]=MemRead[i]=Page;
      MemWrite[i]=0;
    }
  }
}

// 8000-bfff Page 2
void MastMapPage2()
{
  unsigned char *Page=0; int i=0;
  if (Masta.Bank[0]&0x08)
  {
    // Map Battery Ram
    Page=pMastb->Sram;
    Page+=(Masta.Bank[0]&4)<<11; // Page -> 0000 or 2000
    Page-=0x8000;
    for (i=0x80;i<0xc0;i++)
    {
      MemFetch[i]=Page;
      MemRead [i]=Page;
      MemWrite[i]=Page;
    }
  }
  else
  {
    // Map normal Rom Page
    Page=GetRomPage(2);
    Page-=0x8000;
    for (i=0x80;i<0xc0;i++)
    {
      MemFetch[i]=Page;
      MemRead [i]=Page;
      MemWrite[i]=0;
    }
  }
}
