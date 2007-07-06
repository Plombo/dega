// Doze - Dave's optimized Z80 emulator
// Code Maker
#include "dam.h"

static char *OutName="dozea.asm";
static FILE *Out=NULL;
char DamPc[]="si";
char DamCycles[]="dword [_nDozeCycles]";

int ot(char *Format,...)
{
  va_list Arg; va_start(Arg,Format);
  if (Out!=NULL) vfprintf(Out,Format,Arg);
  va_end(Arg);
  return 0;
}

void DamAlign() { ot("times ($$-$) & 3 db 0\n\n"); }

static int DamVariables()
{
  DamAlign();
  ot("; Z80 Registers\n");
  ot("global _Doze\n");
  ot("_Doze:\n");
  ot("DozeAF  dw 0\n");
  ot("DozeBC  dw 0\n");
  ot("DozeDE  dw 0\n");
  ot("DozeHL  dw 0\n");
  ot("DozeIX  dw 0\n");
  ot("DozeIY  dw 0\n");
  ot("DozePC  dw 0\n");
  ot("DozeSP  dw 0\n");
  ot("DozeAF2 dw 0\n");
  ot("DozeBC2 dw 0\n");
  ot("DozeDE2 dw 0\n");
  ot("DozeHL2 dw 0\n");
  ot("DozeIR  dw 0\n");
  ot("DozeIFF dw 0\n");
  ot("DozeIM  db 0\n");
  ot("\n");
  DamAlign();
  ot("global _nDozeCycles\n");
  ot("_nDozeCycles: dd 0 ; Cycles left (in T-states)\n");
  ot("global _nDozeEi\n");
  ot("_nDozeEi: dd 0 ; 1 = assembler quit on EI, 2 = assembler did quit on EI\n");
  ot("SaveReg: times 6 dd 0\n");
  ot("Tmp16: dw 0\n");
  ot("TmpFlag: db 0\n");
  DamAlign();
  ot("global _DozeMemFetch\n");
  ot("_DozeMemFetch: times 0x100 dd 0\n");
  ot("global _DozeMemRead\n");
  ot("_DozeMemRead:  times 0x100 dd 0\n");
  ot("global _DozeMemWrite\n");
  ot("_DozeMemWrite: times 0x100 dd 0\n");
  return 0;
}

int DamOpStart(unsigned int op)
{
  ot(";****************************************************************\n");

  DamAlign();

  ot("Op%-6.2X: INC_R\n\n",op);
  return 0;
}

int DamOpDone(int nCycles,int bDontEnd)
{
  ot("\n  sub %s,%d\n",DamCycles,nCycles);
  if (bDontEnd==0)
  {
    ot("  jle near DozeRunEnd\n");
    ot("  FETCH_OP\n\n");
  }
  return 0;
}

static int DamCall()
{
  DamAlign();
  ot(
  "extern _DozeRead\n"
  "Read:\n"
  "  REG_TO_DOZE\n"
  "  push edx\n  push edi\n"
  "  push edi\n"
  "  call _DozeRead\n"
  "  add esp,4\n"
  "  pop edi\n  pop edx\n\n"
  "  mov dl,al\n"
  "  DOZE_TO_REG\n"
  "  ret\n\n"
  );

  DamAlign();
  ot(
  "extern _DozeWrite\n"
  "Write:\n"
  "  REG_TO_DOZE\n"
  "  push edx\n  push edi\n"
  "  push edx\n"
  "  push edi\n"
  "  call _DozeWrite\n"
  "  add esp,8\n"
  "  pop edi\n  pop edx\n\n"
  "  DOZE_TO_REG\n"
  "  ret\n\n"
  );

  DamAlign();
  ot(
  "extern _DozeIn\n"
  "PortIn:\n"
  "  REG_TO_DOZE\n"
  "  push edx\n  push edi\n"
  "  push edi\n"
  "  call _DozeIn\n"
  "  add esp,4\n"
  "  pop edi\n  pop edx\n\n"
  "  mov dl,al\n"
  "  DOZE_TO_REG\n"
  "  ret\n\n"
  );

  DamAlign();
  ot(
  "extern _DozeOut\n"
  "PortOut:\n"
  "  REG_TO_DOZE\n"
  "  push edx\n  push edi\n"
  "  push edx\n"
  "  push edi\n"
  "  call _DozeOut\n"
  "  add esp,8\n"
  "  pop edi\n  pop edx\n\n"
  "  DOZE_TO_REG\n"
  "  ret\n\n"
  );

  DamAlign();
  ot(
  "; Call a routine\n"
  "global _DozeAsmCall\n"
  "_DozeAsmCall:\n"
  "  mov ax,word[esp+4] ; Get address\n"
  "  mov word[Tmp16],ax\n"
  "  SAVE_REGS\n"
  "  REG_BLANK\n"
  "  DOZE_TO_REG\n"
  "  INC_R\n"
  "  sub word [DozeSP],2\n"
  "  mov dx,si\n"
  "  mov di,word [DozeSP]\n"
  "  DAM_WRITE16\n"
  "  mov dx,word[Tmp16]\n"
  "  mov si,dx\n"
  "  REG_TO_DOZE\n"
  "  RESTORE_REGS\n"
  "  ret\n\n"
  );

  DamAlign();
  ot(
  "; Read a byte from memory\n"
  "global _DozeAsmRead\n"
  "_DozeAsmRead:\n"
  "  mov ax,word[esp+4] ; Get address\n"
  "  mov word[Tmp16],ax\n"
  "  SAVE_REGS\n"
  "  REG_BLANK\n"
  "  DOZE_TO_REG\n"
  "  mov di,word [Tmp16]\n"
  "  DAM_READ8\n"
  "  REG_TO_DOZE\n"
  "  xor eax,eax\n"
  "  mov al,dl\n"
  "  RESTORE_REGS\n"
  "  ret\n\n"
  );

  return 0;
}

static int DamMain()
{
  int i=0;
  ot("; Doze - Dave's Z80 Emulator - Assembler output\n\n");
  ot("bits 32\n\n");

  ot("section .data\n\n");
  DamVariables();

  ot("section .text\n\n");
  DamMacros(); // make macros
  DamCall();

  DamAlign();
  ot("global _DozeAsmRun\n");
  ot("_DozeAsmRun:\n");
  ot("\n");
  ot("  SAVE_REGS\n");
  ot("  REG_BLANK\n");
  ot("  DOZE_TO_REG\n");

  ot("  FETCH_OP ; Fetch first opcode\n\n"); 

  ot("DozeRunEnd: ; After cycles have run out, we come back here\n\n");
  ot("  REG_TO_DOZE\n");
  ot("  RESTORE_REGS\n");
  ot("  ret\n\n\n");


  memset(DamJump  ,0,sizeof(DamJump));
  
  // Fill in jump table if the instruction is available
  for (i=0x00;i<0x100;i++) DamJump[      i]=DamaOp  (         i);
  for (i=0x00;i<0x100;i++) DamJump[0x100+i]=DamcOpCb(  0xCB00+i);
  for (i=0x00;i<0x100;i++) DamJump[0x200+i]=DameOpEd(  0xED00+i);
  for (i=0x00;i<0x100;i++) DamJump[0x300+i]=DamaOp  (  0xDD00+i);
  for (i=0x00;i<0x100;i++) DamJump[0x400+i]=DamaOp  (  0xFD00+i);
  // Note that DDCB__xx is represented as DDCBxx here
  for (i=0x00;i<0x100;i++) DamJump[0x500+i]=DamcOpCb(0xDDCB00+i);
  for (i=0x00;i<0x100;i++) DamJump[0x600+i]=DamcOpCb(0xFDCB00+i);

  DamJumpTab(); // Jump table
  DamTables();  // Other tables
  return 0;
}

int main()
{
  Out=fopen(OutName,"wt");
  if (Out==NULL) return 1;
  DamMain();
  fclose(Out); Out=NULL;
  return 0;
}