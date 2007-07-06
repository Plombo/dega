// Mast - memory module
#include "mastint.h"

// --------------------------  Video chip access  -----------------------------

static INLINE void VidCtrlWrite(unsigned char d)
{
  int Cmd=0;
  if (Masta.v.Wait==0) { Masta.v.Low=d; Masta.v.Wait=1; return; } // low byte

  // high byte: do video command
  Cmd=d<<8; Cmd|=Masta.v.Low;
  Masta.v.Addr=(unsigned short)(Cmd&0x3fff);
  Masta.v.Mode=(unsigned char)((Cmd>>14)&3); // 0-2=VRAM read/write 3=CRAM write

  if ((Cmd&0xf000)==0x8000)
  {
    // Video register set
    int i;
    i=(Cmd>>8)&0x3f;
    if (i<0x10) Masta.v.Reg[i]=(unsigned char)(Cmd&0xff);
  }
  
  Masta.v.Wait=0; nDozeInterrupt=-1;
}

static INLINE unsigned char VidCtrlRead()
{
  unsigned char d=0;
  d=Masta.v.Stat; d|=0x20;

  Masta.v.Wait=0; Masta.v.Stat&=0x3f; nDozeInterrupt=-1;
  return d;
}

// -----------------------------------------------------------------------------

static INLINE void VidDataWrite(unsigned char d)
{
  if (Masta.v.Mode==3)
  {
    // CRam Write
    unsigned char *pc;
    pc=pMastb->CRam+(Masta.v.Addr&0x3f);
    if (pc[0]!=d) { pc[0]=d; MdrawCramChange(Masta.v.Addr); }  // CRam byte change
  }
  else
  {
    pMastb->VRam[Masta.v.Addr&0x3fff]=d;
  }
  Masta.v.Addr++; // auto increment address
  Masta.v.Wait=0;
}

static INLINE unsigned char VidDataRead()
{
  unsigned char d=0;
  d=pMastb->VRam[Masta.v.Addr&0x3fff];
  Masta.v.Addr++; // auto increment address
  Masta.v.Wait=0;
  return d;
}

// =============================================================================
static INLINE unsigned char SysIn(unsigned short a)
{
  unsigned char d=0xff;
  a&=0xff; // 8-bit ports
  if (a==0x00)
  {
    d=0x7f; if ((MastInput[0]&0x80)==0) d|=0x80; // Game gear start button
    goto End;
  }
  if (a==0x05) { d=0; goto End; } // Link-up
  if (a==0x7e)
  {
    // V-Counter read
    if (MastY>0xda) d=(unsigned char)(MastY-6);
    else            d=(unsigned char) MastY;
    goto End;
  }
  if (a==0x7f)
  {
    // H-Counter read: return about the middle
    d=0x40;
    goto End;
  }
  if (a==0xbe) { d=VidDataRead(); goto End; }
  if (a==0xbf) { d=VidCtrlRead(); goto End; }
  if (a==0xdc || a==0xc0)
  {
    // Input
    d=MastInput[0]; d&=0x3f;
    d=(unsigned char)(~d);
    goto End;
  }
  if (a==0xdd || a==0xc1)
  {
    // Region detect:
    d=0x3f;
    d|=pMastb->Out3F&0x80; // bit 7->7
    d|=(pMastb->Out3F<<1)&0x40; // bit 5->6
    if (MastEx&MX_JAPAN) d^=0xc0; //select japanese
    goto End;
  }
  if (a==0xf2)
  {
    // Fm Detect
    d=0xff;
    if (MastEx&MX_FMCHIP) { d=pMastb->FmDetect; d&=1; }
    goto End;
  }
End:
//printf("read 0x%x (PC=0x%x) -> 0x%x\n", a, Doze.pc, d);
  return d;
}

static INLINE void SysOut(unsigned short a,unsigned char d)
{
  a&=0xff; // 8-bit ports
//printf("write 0x%x (PC=0x%x) -> 0x%x\n", a, Doze.pc, d);
  if ( a      ==0x06) { DpsgStereo(d);   goto End; } // Psg Stereo
  if ( a      ==0x3f) { pMastb->Out3F=d; goto End; } // Region detect
  if ((a&0xfe)==0x7e) { DpsgWrite(d); goto End; } // Psg Write
  if ( a      ==0xbe) { VidDataWrite(d); goto End; }
  if ( a      ==0xbf) { VidCtrlWrite(d); goto End; }
  if ( a      ==0xf0) { pMastb->FmSel=d; goto End; }
  if ( a      ==0xf1) { MsndFm(pMastb->FmSel,d); goto End; }
  if ( a      ==0xf2) { pMastb->FmDetect=d; goto End; }
End:
  return;
}

// -----------------------------------------------------------------------------

static INLINE unsigned char SysRead(unsigned short a)
{
  (void)a; return 0;
}

static INLINE void SysWrite(unsigned short a,unsigned char d)
{
  if (a==0x8000) { Masta.Bank[3]=d; MastMapPage2(); goto End; } // Codemasters mapper

  if ((a&0xc000)==0xc000) pMastb->Ram[a&0x1fff]=d; // Ram is always written to
  if ((a&0xfffc)==0xfffc)
  {
    // bank select write
    int b; b=a&3;
    if (d==Masta.Bank[b]) goto End; // No change
    Masta.Bank[b]=d;
    if (b==0) MastMapPage2();
    if (b==1) MastMapPage0();
    if (b==2) MastMapPage1();
    if (b==3) MastMapPage2();
    goto End;
  }

  if ((a&0xfffc)==0xfff8)
  {
    int e;
    // Wonderboy 2 writes to this even though it's 2D
    e=pMastb->ThreeD&1; pMastb->ThreeD&=2;
    pMastb->ThreeD|=d&1;
    if (d!=e) pMastb->ThreeD|=2; // A toggle: looks like it's probably a 3D game
    goto End;
  }

End:
  return;
}

#ifdef EMU_DOZE
unsigned char DozeIn(unsigned short a)            { return SysIn(a); }
void DozeOut(unsigned short a, unsigned char d)   { SysOut(a,d); }
unsigned char DozeRead(unsigned short a)          { return SysRead(a); }
void DozeWrite(unsigned short a, unsigned char d) { SysWrite(a,d); }
#endif
