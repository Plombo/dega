//---------------------------------------------------------------------------
// Master - Master System emulator interface
// Copyright (c) 2004 Dave (www.finalburn.com), all rights reserved.

// This refers to all code except where stated otherwise
// (e.g. zlib code)

// You can use, modify and redistribute this code freely as long as you
// don't do so commercially. This copyright notice must remain with the code.
// You must state if your program uses this code.

// Dave
// Homepage: www.finalburn.com
// E-mail:  dave@finalburn.com
//---------------------------------------------------------------------------

// Main module
#include "app.h"
#ifdef _DEBUG
#include <crtdbg.h>
#endif

HINSTANCE hAppInst=NULL;
int ScrnWidth=0,ScrnHeight=0;
int Fullscreen=0;
HACCEL hAccel=NULL; // Key accelerators
int StartInFullscreen=0;

static FILE *DebugFile=NULL;
// Debug printf to a file
extern "C" int dprintf (char *Format,...)
{
  va_list Arg; va_start(Arg,Format);
  if (DebugFile!=NULL) { vfprintf(DebugFile,Format,Arg); fflush(DebugFile); }
  va_end(Arg); return 0;
}

// Main program entry point
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE,LPSTR pCmdLine,INT)
{
  char *CmdRom=NULL;
  char *argv[] = { "dega", pCmdLine, 0 }; // for python
  hAppInst=hInstance;
  InitCommonControls();

  AppDirectory(); // Set current directory to be the applications directory

//  DebugFile=fopen("zzd.txt","wt"); // Log debug to a file

  hAccel=LoadAccelerators(hAppInst,MAKEINTRESOURCE(IDR_ACCELERATOR1));

  ConfLoad(); // Load config

  PythonLoad(2, argv);

  // Get rom name from command line:
  EmuRomName[0]=0;
  CmdRom=UnwrapString(pCmdLine,0);
  if (CmdRom!=NULL)
  {
    if (CmdRom[0]) { strncpy(EmuRomName,CmdRom,255); EmuRomName[255]=0; }
    free(CmdRom); CmdRom=NULL;
  }

  if (StartInFullscreen) Fullscreen=1;

  // Main loop
  LoopDo();

  ConfSave(); // Save config
  if (DebugFile!=NULL) fclose(DebugFile);  DebugFile=NULL; // Stop logging
  
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF ); // Check for memory leaks
#endif
  return 0;
}
