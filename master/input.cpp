// Input module
#include "app.h"

int UseJoystick=0;
unsigned char AutoHold=0;

int InputGet()
{
  static unsigned int LastInput=0;
  static unsigned int AltFrame=0;
  memset(&MastInput,0,sizeof(MastInput));
  AltFrame=!AltFrame;
  if (GetActiveWindow()!=hFrameWnd) { LastInput=0; return 0; } // No window focus

  DirInputStart();
  if (UseJoystick==0)
  {
    // Keyboard
    if (DirInputState(KeyMappings[KMAP_UP]   )) MastInput[0]|=0x01;
    if (DirInputState(KeyMappings[KMAP_DOWN] )) MastInput[0]|=0x02;
    if (DirInputState(KeyMappings[KMAP_LEFT] )) MastInput[0]|=0x04;
    if (DirInputState(KeyMappings[KMAP_RIGHT])) MastInput[0]|=0x08;
    if (DirInputState(KeyMappings[KMAP_1]    )) MastInput[0]|=0x10;
    if (DirInputState(KeyMappings[KMAP_2]    )) MastInput[0]|=0x20;
    if (DirInputState(KeyMappings[KMAP_START])) MastInput[0]|=0x80;

    if (AltFrame)
    {
      if (DirInputState(KeyMappings[KMAP_AUTO_UP]   )) MastInput[0]|=0x01;
      if (DirInputState(KeyMappings[KMAP_AUTO_DOWN] )) MastInput[0]|=0x02;
      if (DirInputState(KeyMappings[KMAP_AUTO_LEFT] )) MastInput[0]|=0x04;
      if (DirInputState(KeyMappings[KMAP_AUTO_RIGHT])) MastInput[0]|=0x08;
      if (DirInputState(KeyMappings[KMAP_AUTO_1]    )) MastInput[0]|=0x10;
      if (DirInputState(KeyMappings[KMAP_AUTO_2]    )) MastInput[0]|=0x20;
      if (DirInputState(KeyMappings[KMAP_AUTO_START])) MastInput[0]|=0x80;
    }

    MastInput[0]|=AutoHold;
  }
  else
  {
    // Joypad
    if (DirInputState(0x4001)) MastInput[0]|=0x01;
    if (DirInputState(0x4002)) MastInput[0]|=0x02;
    if (DirInputState(0x4003)) MastInput[0]|=0x04;
    if (DirInputState(0x4004)) MastInput[0]|=0x08;
    if (DirInputState(0x4010)) MastInput[0]|=0x10;
    if (DirInputState(0x4011)) MastInput[0]|=0x20;
    if (DirInputState(0x4012)) MastInput[0]|=0x80;
  }

  // Check start button
  if ((MastEx&MX_GG)==0)
  {
    if ((MastInput[0]&0x80) && (LastInput&0x80)==0) { MastInput[0]|=0x40; } // On master system cause nmi
  }
  LastInput=MastInput[0];
  return 0;
}
