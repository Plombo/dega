// Doze - Dave's optimized Z80 emulator
// internal code
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "doze.h"

// Make the INLINE macro
#undef INLINE
#define INLINE inline

extern "C" {

void DozeAsmRun();
extern int nDozeEi;
void DozeAsmCall(unsigned short nAddr);
}

