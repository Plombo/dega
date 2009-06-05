// Mast - frame module
#include "mastint.h"

int MastY=0; // 0-261
static int Hint=0; // Hint counter
static int LineCyc=0,TotalCyc=0;
static int FrameCyc=0; // Total cycles done this frame, (apart from current call)

static int CpuBase=0; // Value to subtract nDozeCycles from in order to get midcycle count

#ifdef EMU_DOZE
#define CyclesLeft nDozeCycles
#elif defined(EMU_Z80JB)
#define CyclesLeft z80_ICount
#endif

int CpuMid() // Returns how many cycles the z80 has done inside the run call
{
  return CpuBase-CyclesLeft;
}

// Run the z80 cpu for Cycles more cycles
static INLINE void CpuRun(int Cycles)
{
  int Done=0;

  CyclesLeft+=Cycles; CpuBase=CyclesLeft;
#ifdef EMU_DOZE
  DozeRun();
#elif defined(EMU_Z80JB)
  Z80Execute(CpuBase);
#endif

  Done=CpuMid(); // Find out number of cycles actually done
  CpuBase=CyclesLeft; // Reset CpuBase, so CpuMid() will return 0

  FrameCyc+=Done; // Add cycles done to frame total
  VgmCycleDone(Done); // Add cycles done to VGM total
}

static void RunLine()
{
  if (MastY<=0) Hint=Masta.v.Reg[10];

  if (MastY<=192)
  {
    Hint--;
    if (Hint<0)
    {
      Masta.v.Stat|=0x40;
      if (Masta.v.Reg[0]&0x10) // Do hint
      { 
#ifdef EMU_DOZE
        nDozeInterrupt=0xff;
#elif defined(EMU_Z80JB)
        Z80Vector=0x38;
        Z80SetIrqLine(0x38, 1);
#endif
      }
      Hint=Masta.v.Reg[10];
    }
  }

  if (MastY==193)
  {
    if (Masta.v.Reg[1]&0x20) // Do vint
    { 
#ifdef EMU_DOZE
      nDozeInterrupt=0xff;
#elif defined(EMU_Z80JB)
      Z80Vector=0x38;
      Z80SetIrqLine(0x38, 1);
#endif
    }
  }

  CpuRun(LineCyc);
}

int MastFrame()
{
//printf("frame PC=0x%x\n", Doze.pc);

  MvidPreFrame();

#ifdef EMU_DOZE
  nDozeInterrupt=Masta.Irq ? 0xff : -1; // Load IRQ latch
#elif defined(EMU_Z80JB)
  Z80Vector=Masta.Irq;
#endif

  if (MastEx&MX_PAL) LineCyc=273; // PAL timings (but not really: not enough lines)
  else               LineCyc=228; // NTSC timings

  TotalCyc=LineCyc*262; // For sound

  // Start counter and sound
  MsndDone=0; FrameCyc=0; CyclesLeft=0; CpuBase=0;

  // V-Int:
  Masta.v.Stat|=0x80; MastY=192; RunLine();
  for (MastY=193;MastY<262;MastY++) { RunLine(); }

  if (MastInput[0]&0x40) // Cause nmi if pause pressed
#ifdef EMU_DOZE
    DozeNmi();
#elif defined(EMU_Z80JB)
    Z80SetIrqLine(Z80_INPUT_LINE_NMI, 1);
#endif

  // Active scan
  for (MastY=0;MastY<192;MastY++) { Mdraw.Line=MastY; MdrawDo(); RunLine(); }
  // Finish sound
  MsndTo(MsndLen);

#ifdef EMU_DOZE
  Masta.Irq = (unsigned char)(nDozeInterrupt==0xff); // Save IRQ latch
#elif defined(EMU_Z80JB)
  Masta.Irq = Z80Vector;
#endif

  TotalCyc=0; // Don't update sound outside of a frame
  return 0;
}

void MastSoundUpdate()
{
  int Now;
  if (TotalCyc<=0) return;
  Now=FrameCyc+CpuMid();
  MsndTo(Now*MsndLen/TotalCyc);
}
