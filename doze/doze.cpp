#include "dozeint.h"

int nDozeVer=0x1001; // Version number of the library
int nDozeInterrupt=-1; // Interrupt latch

static inline int Interrupt(int nVal)
{
  if ((Doze.iff&0xff)==0) return 0; // not enabled

  // Get out of any halt opcodes
  if (DozeAsmRead(Doze.pc)==0x76) Doze.pc++;

  Doze.iff=0;
  if (Doze.im==0)
  {
    DozeAsmCall((unsigned short)(nVal&0x38)); // rst nn
    return 13; // cycles done
  }
  else if (Doze.im==2)
  {
    int nTabAddr=0,nIntAddr=0;
    // Get interrupt address from table (I points to the table)
    nTabAddr=(Doze.ir&0xff00)+nVal;

    // Read 16-bit table value
    nIntAddr =DozeAsmRead((unsigned short)(nTabAddr+1))<<8;
    nIntAddr|=DozeAsmRead((unsigned short)(nTabAddr));

    DozeAsmCall((unsigned short)(nIntAddr));
    return 19; // cycles done
  }
  else
  {
    DozeAsmCall(0x38); // rst 38h
    return 13; // cycles done
  }
}

// Try to take the latched interrupt
static inline void TryInt()
{
  int nDid=0;
  if (nDozeInterrupt<0) return;

  nDid=Interrupt(nDozeInterrupt);
  if (nDid>0) nDozeInterrupt=-1; // Success! we did some cycles, and took the interrupt
  nDozeCycles-=nDid;
}

void DozeRun()
{
  TryInt(); // Try the interrupt before we begin
  if (nDozeCycles<0) return;

  if (DozeAsmRead(Doze.pc)==0x76)
  {
    // cpu is halted (repeatedly doing halt inst.)
    int nDid=0; nDid=(nDozeCycles>>2)+1;
    Doze.ir= (unsigned short)( ((Doze.ir+nDid)&0x7f) | (Doze.ir&0xff80) ); // Increase R register
    nDozeCycles-=nDid;
    return;
  }

  if (nDozeInterrupt==0)
  {
    // Run as normal
    nDozeEi=0; DozeAsmRun();
    return;
  }

  // Find out about mid-exec EIs
  nDozeEi=1; DozeAsmRun();
  if (nDozeEi!=2) return; // there were none

  // Just enabled interrupts
  {
    int nTodo=0;
    // (do one more instruction before interrupt)
    nTodo=nDozeCycles; nDozeCycles=0;
    nDozeEi=0; DozeAsmRun();
    nDozeCycles+=nTodo;
  }

  TryInt();

  // And continue the rest of the exec
  DozeAsmRun();
}

int DozeReset()
{
  // Reset z80
  memset(&Doze,0,sizeof(Doze));
  Doze.af=0x0040;
  Doze.ix=0xffff;
  Doze.iy=0xffff;
  return 0;
}

int DozeNmi()
{
  Doze.iff&=0xff00; // reset iff1
  DozeAsmCall((unsigned short)0x66); // Do nmi
  return 12;
}
