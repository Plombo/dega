// Loop module
#include "app.h"

static int ShowMenu=1;
int MenuHideOnLoad=1;
int SetupPal=0;
int LoopPause=0;
static MSG Msg;

static void MediaShowMenu() {
    RECT oldclient, newclient;

    MoveOK=timeGetTime();
    GetClientRect(hFrameWnd,&oldclient);
    if (ShowMenu) SetMenu(hFrameWnd,hFrameMenu);
    else          SetMenu(hFrameWnd,NULL);
    GetClientRect(hFrameWnd,&newclient);

    if (oldclient.bottom != newclient.bottom)
    {
      RECT winrect;
      GetWindowRect(hFrameWnd,&winrect);
      winrect.top -= oldclient.bottom-newclient.bottom;
      SetWindowRect(hFrameWnd,&winrect);
    }
}

// Reinit the media, going down to a certain level
static int MediaInit(int Level)
{
  int Ret=0;

  if (Level<=10) // Frame and DirectInput
  {
    Ret=FrameInit(); if (Ret!=0) { AppError("FrameInit Failed",0); return 1; }
    FrameSize();
    ShowWindow(hFrameWnd,SW_SHOWDEFAULT);
    DirInputInit(hAppInst,hFrameWnd); // not critical if it fails
  }

  if (Level<=15) // Mast library and Python
  {
    Ret=EmuInit(); if (Ret!=0) { AppError("EmuInit Failed",0); return 1; }
    PythonInit();
    EnableMenuItem(hFrameMenu, 7, (PythonLoaded ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION); // XXX hardcoded (7 = Python)
  }

  if (Level<=20) // Game load
  {
    unsigned int Enabled=MF_GRAYED;
    char WinName[128]="";
    Ret=EmuLoad();
    if (Ret==0) { if (MenuHideOnLoad) ShowMenu=0; Enabled=MF_ENABLED; LoopPause=0; }
    else EmuFree();
    EnableMenuItem(hFrameMenu,ID_FILE_RESET    ,Enabled);
    EnableMenuItem(hFrameMenu,ID_FILE_FREEROM  ,Enabled);
    EnableMenuItem(hFrameMenu,ID_VIDEO_RECORD  ,Enabled);
    EnableMenuItem(hFrameMenu,ID_VIDEO_PLAYBACK,Enabled);

    StateAuto(0); // Auto load the state
    // Size and show the frame window

    // Make frame window title
    if (EmuTitle!=NULL)
    {
      sprintf (WinName,"%.60s - %s",EmuTitle,AppName(MastVer));
    }
    else
    { sprintf (WinName,"%s",AppName(MastVer)); }

    SetWindowText(hFrameWnd,WinName);

    if (MastEx&MX_GG) { ScrnWidth=160; ScrnHeight=144; } // Game gear
    else              { ScrnWidth=256; ScrnHeight=192; } // Master System

  }

  if (Level<=30) // DirectDraw
  {
    Ret=DispInit(); if (Ret!=0) { AppError("DispInit Failed",0); return 1; }
    // Repaint the Frame Window
    InvalidateRect(hFrameWnd,NULL,1);
    UpdateWindow(hFrameWnd);
  }

  if (Level<=35) // Window size
  {
    if (Fullscreen)
    {
      SetWindowLong(hFrameWnd,GWL_STYLE,WS_POPUP|WS_CLIPCHILDREN);
      ShowWindow(hFrameWnd,SW_MAXIMIZE);
    }
    else
    {
      ShowWindow(hFrameWnd,SW_HIDE);
      SetWindowLong(hFrameWnd,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN);
      FrameSize();
      ShowWindow(hFrameWnd,SW_NORMAL);
    }
  }

  if (Level<=50) // Sound
  {
    if (SetupPal==0 || MastEx&MX_GG) { MastEx&=~MX_PAL; FramesPerSecond=60; } 
    else                             { MastEx|= MX_PAL; FramesPerSecond=50; }
  
    RealFramesPerSecond=FrameMult>0 ? FramesPerSecond<<FrameMult : FramesPerSecond>>-FrameMult;
    if (RealFramesPerSecond<1) RealFramesPerSecond = 1;

    DSoundInit(hFrameWnd);
  }

  if (Level<=55) // Emu Sound
  {
    MsndRate=DSoundSamRate; MsndLen=DSoundSegLen;
    MsndInit();
  }

  if (Level<=60) // Run thread
  {
    ShotExit(); // Get rid of previous screenshots

    // Make sure we have a screenshot image in situations where the image isn't in motion
    // (this is done so that the user takes a screenshot of the paused frame)
    if (Msg.wParam==ID_SETUP_ONEFRAME || Msg.wParam==ID_SETUP_PAUSE)
    {
      if (LoopPause) { ShotStart(); RunFrame(1,NULL); }
    }

    // Start the Run thread
    if (LoopPause==0 || PythonRunning)
    {
      RunStart();
      if (LoopPause==0) DSoundPlay();
    }
  }

  if (Level<=70) // Menu
  {
    int i;
   
#define CHK(a,b) CheckMenuItem(hFrameMenu,a,(b)?MF_CHECKED:MF_UNCHECKED);

    CHK(ID_SETUP_PAL,SetupPal)
    // Disable the NTSC/PAL switch if it's the game gear
    EnableMenuItem(hFrameMenu,ID_SETUP_PAL ,MastEx&MX_GG ? MF_GRAYED : MF_ENABLED);

    CHK(ID_SETUP_JAPAN,MastEx&MX_JAPAN)
    CHK(ID_SETUP_FMCHIP,MastEx&MX_FMCHIP)
    CHK(ID_SETUP_OVERLAY_NONE,TryOverlay==0)
    CHK(ID_SETUP_OVERLAY_YUV, TryOverlay==1)
    CHK(ID_SETUP_OVERLAY_RGB, TryOverlay==2)
    CHK(ID_SETUP_FULLSCREEN, Fullscreen)
    CHK(ID_SETUP_REDBLUE3D, RedBlue3D)
    CHK(ID_SETUP_PAUSE, LoopPause)
    CHK(ID_SETUP_STATUSHIDE, StatusMode==STATUS_HIDE)
    CHK(ID_SETUP_STATUSAUTO, StatusMode==STATUS_AUTO)
    CHK(ID_SETUP_STATUSSHOW, StatusMode==STATUS_SHOW)
    CHK(ID_SETUP_MENUHIDEONLOAD, MenuHideOnLoad)
    CHK(ID_INPUT_KEYBOARD,UseJoystick==0)
    CHK(ID_INPUT_JOYSTICK,UseJoystick!=0)
    CHK(ID_STATE_AUTOLOADSAVE,AutoLoadSave!=0)
    CHK(ID_STATE_SRAM,MastEx&MX_SRAM)
    CHK(ID_SOUND_ENHANCEPSG,DpsgEnhance)
    CHK(ID_SOUND_QUALITY_OFF,DSoundSamRate==0)
    CHK(ID_SOUND_QUALITY_12000HZ,DSoundSamRate==12000)
    CHK(ID_SOUND_QUALITY_44100HZ,DSoundSamRate==44100)
    EnableMenuItem(hFrameMenu,ID_SOUND_VGMLOG_START,VgmFile==NULL ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hFrameMenu,ID_SOUND_VGMLOG_STOP, VgmFile!=NULL ? MF_ENABLED : MF_GRAYED);
    CHK(ID_SOUND_VGMLOG_SAMPLEACCURATE,VgmAccurate)
    CHK(ID_VIDEO_READONLY,VideoReadOnly)
    EnableMenuItem(hFrameMenu,ID_VIDEO_STOP, MvidInVideo() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hFrameMenu,ID_VIDEO_PROPERTIES, MvidGotProperties() ? MF_ENABLED : MF_GRAYED);
    CHK(ID_VIDEO_FRAMECOUNTER,MdrawOsdOptions&OSD_FRAMECOUNT)
    CHK(ID_INPUT_BUTTONS,MdrawOsdOptions&OSD_BUTTONS)
    for (i = 0; i < 10; i++) CHK(ID_STATE_SLOT(i), SaveSlot==i)
    CHK(ID_SETUP_SCALE_1X,SizeMultiplier==1)
    CHK(ID_SETUP_SCALE_2X,SizeMultiplier==2)
    CHK(ID_SETUP_SCALE_3X,SizeMultiplier==3)
    CHK(ID_SETUP_SCALE_4X,SizeMultiplier==4)

    MediaShowMenu();

    if (Fullscreen && ShowMenu==0) { while (ShowCursor(0)>=0) ; } // Hide mouse
  }
  return 0;
}

// Exit the media, going down to a certain level
static int MediaExit(int Level)
{
  if (Level<=70)
  {
    while (ShowCursor(1)< 0) ; // Show mouse
  }

  if (Level<=60)
  {
    DSoundStop();
    RunStop();
  }

  if (Level<=55)
  {
    MsndExit();
    MsndLen=0; MsndRate=0;
  }

  if (Level<=50)
  {
    DSoundStop();
    DSoundExit();
    MastEx&=~MX_PAL;
  }

  if (Level<=30)
  {
    DispExit();
  }
 
  if (Level<=20)
  {
    StateAuto(1); // Auto save the state
    EmuFree();
  }

  if (Level<=15)
  {
    VgmStop(NULL);
    PythonExit();
    EmuExit();
  }

  if (Level<=10)
  {
    DirInputExit();
    FrameExit();
  }

  if (Level<=0)
  {
    ShotExit(); // Deallocate screenshot
  }
  return 0;
}

struct CustomKeyMap {
	int key;
	WPARAM command;
};

static void TranslateCustomKeys(HWND hwnd, struct CustomKeyMap *map, MSG *msg) {
	if (msg->hwnd != hwnd || msg->message != WM_KEYDOWN) return;
	while (map->key != 0 || map->command != 0) {
		if (msg->wParam == KeyMappings[map->key]) {
			SendMessage(hwnd, WM_COMMAND, map->command, 0);
			return;
		}
		map++;
	}
}

struct CustomKeyMap mymap[] = {
 { KMAP_PAUSE, ID_SETUP_PAUSE },
 { KMAP_FRAMEADVANCE, ID_SETUP_ONEFRAME },
 { KMAP_QUICKLOAD, ID_STATE_QUICKLOAD },
 { KMAP_QUICKSAVE, ID_STATE_QUICKSAVE },
 { KMAP_SPEEDUP, ID_SETUP_SPEEDUP },
 { KMAP_SLOWDOWN, ID_SETUP_SLOWDOWN },
 { KMAP_NORMALSPEED, ID_SETUP_NORMALSPEED },
 { KMAP_SAVESLOT(0), ID_STATE_SLOT(0) },
 { KMAP_SAVESLOT(1), ID_STATE_SLOT(1) },
 { KMAP_SAVESLOT(2), ID_STATE_SLOT(2) },
 { KMAP_SAVESLOT(3), ID_STATE_SLOT(3) },
 { KMAP_SAVESLOT(4), ID_STATE_SLOT(4) },
 { KMAP_SAVESLOT(5), ID_STATE_SLOT(5) },
 { KMAP_SAVESLOT(6), ID_STATE_SLOT(6) },
 { KMAP_SAVESLOT(7), ID_STATE_SLOT(7) },
 { KMAP_SAVESLOT(8), ID_STATE_SLOT(8) },
 { KMAP_SAVESLOT(9), ID_STATE_SLOT(9) },
 { KMAP_READONLY, ID_VIDEO_READONLY },
 { KMAP_HOLD_1,     ID_INPUT_HOLD_1 },
 { KMAP_HOLD_2,     ID_INPUT_HOLD_2 },
 { KMAP_HOLD_UP,    ID_INPUT_HOLD_UP },
 { KMAP_HOLD_DOWN,  ID_INPUT_HOLD_DOWN },
 { KMAP_HOLD_LEFT,  ID_INPUT_HOLD_LEFT },
 { KMAP_HOLD_RIGHT, ID_INPUT_HOLD_RIGHT },
 { KMAP_HOLD_START, ID_INPUT_HOLD_START },
 { KMAP_P2_HOLD_1,     ID_INPUT_P2_HOLD_1 },
 { KMAP_P2_HOLD_2,     ID_INPUT_P2_HOLD_2 },
 { KMAP_P2_HOLD_UP,    ID_INPUT_P2_HOLD_UP },
 { KMAP_P2_HOLD_DOWN,  ID_INPUT_P2_HOLD_DOWN },
 { KMAP_P2_HOLD_LEFT,  ID_INPUT_P2_HOLD_LEFT },
 { KMAP_P2_HOLD_RIGHT, ID_INPUT_P2_HOLD_RIGHT },
 { KMAP_BUTTONSTATE,  ID_INPUT_BUTTONS },
 { KMAP_FRAMECOUNTER, ID_VIDEO_FRAMECOUNTER },
 { KMAP_SAVE_TO_SLOT(0), ID_STATE_SAVE_SLOT(0) },
 { KMAP_SAVE_TO_SLOT(1), ID_STATE_SAVE_SLOT(1) },
 { KMAP_SAVE_TO_SLOT(2), ID_STATE_SAVE_SLOT(2) },
 { KMAP_SAVE_TO_SLOT(3), ID_STATE_SAVE_SLOT(3) },
 { KMAP_SAVE_TO_SLOT(4), ID_STATE_SAVE_SLOT(4) },
 { KMAP_SAVE_TO_SLOT(5), ID_STATE_SAVE_SLOT(5) },
 { KMAP_SAVE_TO_SLOT(6), ID_STATE_SAVE_SLOT(6) },
 { KMAP_SAVE_TO_SLOT(7), ID_STATE_SAVE_SLOT(7) },
 { KMAP_SAVE_TO_SLOT(8), ID_STATE_SAVE_SLOT(8) },
 { KMAP_SAVE_TO_SLOT(9), ID_STATE_SAVE_SLOT(9) },
 { KMAP_LOAD_FROM_SLOT(0), ID_STATE_LOAD_SLOT(0) },
 { KMAP_LOAD_FROM_SLOT(1), ID_STATE_LOAD_SLOT(1) },
 { KMAP_LOAD_FROM_SLOT(2), ID_STATE_LOAD_SLOT(2) },
 { KMAP_LOAD_FROM_SLOT(3), ID_STATE_LOAD_SLOT(3) },
 { KMAP_LOAD_FROM_SLOT(4), ID_STATE_LOAD_SLOT(4) },
 { KMAP_LOAD_FROM_SLOT(5), ID_STATE_LOAD_SLOT(5) },
 { KMAP_LOAD_FROM_SLOT(6), ID_STATE_LOAD_SLOT(6) },
 { KMAP_LOAD_FROM_SLOT(7), ID_STATE_LOAD_SLOT(7) },
 { KMAP_LOAD_FROM_SLOT(8), ID_STATE_LOAD_SLOT(8) },
 { KMAP_LOAD_FROM_SLOT(9), ID_STATE_LOAD_SLOT(9) },
 { 0, 0 }
};

// Main program loop
int LoopDo()
{
  int Ret=0; int InitLevel=10;
  memset(&Msg,0,sizeof(Msg));

  for (;;)
  {
    // Perform any changes which are needed
    if (Msg.message==WMU_COMMAND)
    {
      if (Msg.wParam==ID_FILE_RESET) MastReset();

      if (Msg.wParam==ID_SETUP_PAL) SetupPal=!SetupPal;
      if (Msg.wParam==ID_SETUP_JAPAN) MastEx^=MX_JAPAN;
      if (Msg.wParam==ID_SETUP_FMCHIP) MastEx^=MX_FMCHIP;
      if (Msg.wParam==ID_SETUP_OVERLAY_NONE) TryOverlay=0;
      if (Msg.wParam==ID_SETUP_OVERLAY_YUV)  TryOverlay=1;
      if (Msg.wParam==ID_SETUP_OVERLAY_RGB)  TryOverlay=2;
      if (Msg.wParam==ID_SETUP_FULLSCREEN)
      {
        Fullscreen=!Fullscreen;
        if (Fullscreen) ShowMenu=0;
      }
      if (Msg.wParam==ID_SETUP_REDBLUE3D) RedBlue3D=!RedBlue3D;
      if (Msg.wParam==ID_OPTIONS_SCREENSHOT)
      {
        if (LoopPause==0) { ShotStart(); RunFrame(1,NULL); }
        ShotSave();
      }

      if (Msg.wParam==ID_SETUP_PAUSE)
      {
        LoopPause=!LoopPause;
        if (LoopPause) RunText("Emulator Paused",2*60);
        else           RunText("Emulator Unpaused",2*60);
      }

      if (Msg.wParam==ID_SETUP_ONEFRAME)
      {
        RunText("One Frame Step",2*60);
	LoopPause=1;
        // RunFrame(1,NULL);
      }
      
      if (Msg.wParam==ID_INPUT_KEYBOARD) UseJoystick=0;
      if (Msg.wParam==ID_INPUT_JOYSTICK) UseJoystick=1;
      if (Msg.wParam==ID_STATE_AUTOLOADSAVE) AutoLoadSave=!AutoLoadSave;
      if (Msg.wParam==ID_STATE_SRAM) MastEx^=MX_SRAM;
      if (Msg.wParam==ID_SOUND_ENHANCEPSG) DpsgEnhance=!DpsgEnhance;
      if (Msg.wParam==ID_SOUND_QUALITY_OFF) DSoundSamRate=0;
      if (Msg.wParam==ID_SOUND_QUALITY_12000HZ) DSoundSamRate=12000;
      if (Msg.wParam==ID_SOUND_QUALITY_44100HZ) DSoundSamRate=44100;
      if (Msg.wParam==ID_SOUND_VGMLOG_STOP) { VgmStop(NULL); }
      if (Msg.wParam==ID_SOUND_VGMLOG_SAMPLEACCURATE) { VgmAccurate=!VgmAccurate; }
      if (Msg.wParam==ID_VIDEO_STOP) { MvidStop(); }
      if (Msg.wParam==ID_VIDEO_READONLY)
      { 
        VideoReadOnly=!VideoReadOnly;
	if (VideoReadOnly) RunText("Read Only Mode", 2*60);
	             else  RunText("Read Write Mode", 2*60);
      }
      if (Msg.wParam==ID_STATE_QUICKLOAD)
      {
        char msg[20];
	snprintf(msg, sizeof(msg), "Quick Load %d", SaveSlot);
	RunText(msg, 2*60);
        StateAutoState(0, SaveSlot);
      }
      if (Msg.wParam==ID_STATE_QUICKSAVE)
      {
        char msg[20];
	snprintf(msg, sizeof(msg), "Quick Save %d", SaveSlot);
	RunText(msg, 2*60);
        StateAutoState(1, SaveSlot);
      }
      if (Msg.wParam>=ID_STATE_LOAD_SLOT(0) && Msg.wParam<=ID_STATE_LOAD_SLOT(9))
      {
	int Slot = Msg.wParam-ID_STATE_LOAD_SLOT_START;
        char msg[20];
	snprintf(msg, sizeof(msg), "Quick Load %d", Slot);
	RunText(msg, 2*60);
        StateAutoState(0, Slot);
      }
      if (Msg.wParam>=ID_STATE_SAVE_SLOT(0) && Msg.wParam<=ID_STATE_SAVE_SLOT(9))
      {
	int Slot = Msg.wParam-ID_STATE_SAVE_SLOT_START;
        char msg[20];
	snprintf(msg, sizeof(msg), "Quick Save %d", Slot);
	RunText(msg, 2*60);
        StateAutoState(1, Slot);
      }
      if (Msg.wParam==ID_SETUP_SPEEDUP) { FrameMult++; }
      if (Msg.wParam==ID_SETUP_SLOWDOWN) { FrameMult--; }
      if (Msg.wParam==ID_SETUP_NORMALSPEED) { FrameMult=0; }
      if (Msg.wParam==ID_VIDEO_FRAMECOUNTER) { MdrawOsdOptions^=OSD_FRAMECOUNT; }
      if (Msg.wParam==ID_INPUT_BUTTONS) { MdrawOsdOptions^=OSD_BUTTONS; }
      if (Msg.wParam>=ID_STATE_SLOT(0) && Msg.wParam<=ID_STATE_SLOT(9))
      {
        char msg[20];
        SaveSlot=Msg.wParam-ID_STATE_SLOT_START;
        snprintf(msg, sizeof(msg), "Save Slot %d", SaveSlot);
	RunText(msg, 2*60);
      }
      if (Msg.wParam==ID_SETUP_STATUSHIDE) { SetStatusMode(STATUS_HIDE); }
      if (Msg.wParam==ID_SETUP_STATUSAUTO) { SetStatusMode(STATUS_AUTO); }
      if (Msg.wParam==ID_SETUP_STATUSSHOW) { SetStatusMode(STATUS_SHOW); }
      if (Msg.wParam==ID_SETUP_MENUHIDEONLOAD) { MenuHideOnLoad=!MenuHideOnLoad; }
      if (Msg.wParam>=ID_SETUP_SCALE_1X && Msg.wParam<=ID_SETUP_SCALE_4X)
      {
        SizeMultiplier=Msg.wParam-ID_SETUP_SCALE_1X+1;
	FrameSize();
      }
    }
    if (Msg.message==WMU_STATELOAD)   { StateLoad(0); }
    if (Msg.message==WMU_STATESAVE)   { StateSave(0); }
    if (Msg.message==WMU_STATEIMPORT) { StateLoad(1); }
    if (Msg.message==WMU_STATEEXPORT) { StateSave(1); }
    if (Msg.message==WMU_VGMSTART) { VgmStart(VgmName); }

    if (Msg.message==WMU_VIDEORECORD)      { VideoRecord(); }
    if (Msg.message==WMU_VIDEOPLAYBACK)    { VideoPlayback(); }
    if (Msg.message==WMU_PYTHON) { PythonRunning=1; }
    if (Msg.message==WMU_PYTHON_THREAD) { PythonRunThread(); }

    Ret=MediaInit(InitLevel); if (Ret!=0) { InitLevel=0; goto Error; }

MainLoop:
    for (;;)
    {
      int Ret=0;

      InitLevel=80;
      Ret=GetMessage(&Msg,NULL,0,0);

      // Check for changes which mean we need to re-init down to a certain level
      if (Msg.message==WMU_TOGGLEMENU) { ShowMenu=!ShowMenu; InitLevel=70; break; }
      if (Msg.message==WMU_CHANGEDROM) {                     InitLevel=20; break; }
      if (Msg.message==WM_QUIT)        {                     InitLevel=0 ; break; }
      if (Msg.message==WMU_COMMAND)
      {
        if (Msg.wParam==ID_FILE_RESET)         { InitLevel=60; break; }
        if (Msg.wParam==ID_FILE_EXIT)          { InitLevel=0 ; break; }
        if (Msg.wParam==ID_SETUP_PAL)          { InitLevel=50; break; }
        if (Msg.wParam==ID_SETUP_JAPAN)        { InitLevel=70; break; }
        if (Msg.wParam==ID_SETUP_FMCHIP)       { InitLevel=70; break; }
        if (Msg.wParam==ID_SETUP_OVERLAY_NONE) { InitLevel=30; break; }
        if (Msg.wParam==ID_SETUP_OVERLAY_YUV)  { InitLevel=30; break; }
        if (Msg.wParam==ID_SETUP_OVERLAY_RGB)  { InitLevel=30; break; }
        if (Msg.wParam==ID_SETUP_FULLSCREEN)   { InitLevel=35; break; }
        if (Msg.wParam==ID_SETUP_REDBLUE3D)    { InitLevel=70; break; }
        if (Msg.wParam==ID_SETUP_PAUSE)        { InitLevel=60; break; }
        if (Msg.wParam==ID_SETUP_ONEFRAME)     { InitLevel=60; break; }
        if (Msg.wParam==ID_OPTIONS_SCREENSHOT) { InitLevel=60; break; }
        if (Msg.wParam==ID_INPUT_KEYBOARD)     { InitLevel=60; break; }
        if (Msg.wParam==ID_INPUT_JOYSTICK)     { InitLevel=60; break; }
        if (Msg.wParam==ID_STATE_AUTOLOADSAVE) { InitLevel=70; break; }
        if (Msg.wParam==ID_SOUND_ENHANCEPSG)   { InitLevel=50; break; }
        if (Msg.wParam==ID_SOUND_QUALITY_OFF)     { InitLevel=50; break; }
        if (Msg.wParam==ID_SOUND_QUALITY_12000HZ) { InitLevel=50; break; }
        if (Msg.wParam==ID_SOUND_QUALITY_44100HZ) { InitLevel=50; break; }
        if (Msg.wParam==ID_SOUND_VGMLOG_STOP)  { InitLevel=60; break; }
        if (Msg.wParam==ID_SOUND_VGMLOG_SAMPLEACCURATE) { InitLevel=60; break; }
        if (Msg.wParam==ID_VIDEO_STOP)         { InitLevel=60; break; }
        if (Msg.wParam==ID_VIDEO_READONLY)     { InitLevel=70; break; }
        if (Msg.wParam==ID_STATE_QUICKLOAD)    { InitLevel=60; break; }
        if (Msg.wParam==ID_STATE_QUICKSAVE)    { InitLevel=60; break; }
        if (Msg.wParam>=ID_STATE_LOAD_SLOT(0) && Msg.wParam<=ID_STATE_LOAD_SLOT(9)) { InitLevel=60; break; }
        if (Msg.wParam>=ID_STATE_SAVE_SLOT(0) && Msg.wParam<=ID_STATE_SAVE_SLOT(9)) { InitLevel=60; break; }
        if (Msg.wParam==ID_STATE_QUICKSAVE)    { InitLevel=60; break; }
        if (Msg.wParam==ID_SETUP_SPEEDUP)      { InitLevel=50; break; }
        if (Msg.wParam==ID_SETUP_SLOWDOWN)     { InitLevel=50; break; }
        if (Msg.wParam==ID_SETUP_NORMALSPEED)  { InitLevel=50; break; }
	if (Msg.wParam==ID_VIDEO_FRAMECOUNTER) { InitLevel=70; break; }
	if (Msg.wParam==ID_INPUT_BUTTONS)      { InitLevel=70; break; }
	if (Msg.wParam>=ID_STATE_SLOT(0) && Msg.wParam<=ID_STATE_SLOT(9)) { InitLevel=70; break; }
        if (Msg.wParam==ID_SETUP_STATUSHIDE)   { InitLevel=70; break; }
        if (Msg.wParam==ID_SETUP_STATUSAUTO)   { InitLevel=70; break; }
        if (Msg.wParam==ID_SETUP_STATUSSHOW)   { InitLevel=70; break; }
        if (Msg.wParam==ID_SETUP_MENUHIDEONLOAD) { InitLevel=70; break; }

        if (Msg.wParam>=ID_SETUP_SCALE_1X && Msg.wParam<=ID_SETUP_SCALE_4X)
          { InitLevel=70; break; }

        if (Msg.wParam==ID_INPUT_HOLD_UP   ) AutoHold[0]^=0x01;
        if (Msg.wParam==ID_INPUT_HOLD_DOWN ) AutoHold[0]^=0x02;
        if (Msg.wParam==ID_INPUT_HOLD_LEFT ) AutoHold[0]^=0x04;
        if (Msg.wParam==ID_INPUT_HOLD_RIGHT) AutoHold[0]^=0x08;
        if (Msg.wParam==ID_INPUT_HOLD_1    ) AutoHold[0]^=0x10;
        if (Msg.wParam==ID_INPUT_HOLD_2    ) AutoHold[0]^=0x20;
        if (Msg.wParam==ID_INPUT_HOLD_START) AutoHold[0]^=0x80;

        if (Msg.wParam==ID_INPUT_P2_HOLD_UP   ) AutoHold[1]^=0x01;
        if (Msg.wParam==ID_INPUT_P2_HOLD_DOWN ) AutoHold[1]^=0x02;
        if (Msg.wParam==ID_INPUT_P2_HOLD_LEFT ) AutoHold[1]^=0x04;
        if (Msg.wParam==ID_INPUT_P2_HOLD_RIGHT) AutoHold[1]^=0x08;
        if (Msg.wParam==ID_INPUT_P2_HOLD_1    ) AutoHold[1]^=0x10;
        if (Msg.wParam==ID_INPUT_P2_HOLD_2    ) AutoHold[1]^=0x20;
      }
      if (Msg.message==WMU_STATELOAD) { InitLevel=60; break; }
      if (Msg.message==WMU_STATESAVE) { InitLevel=60; break; }
      if (Msg.message==WMU_STATEIMPORT) { InitLevel=60; break; }
      if (Msg.message==WMU_STATEEXPORT) { InitLevel=60; break; }
      if (Msg.message==WMU_VGMSTART)    { InitLevel=60; break; }

      if (Msg.message==WMU_VIDEORECORD)      { InitLevel=60; break; }
      if (Msg.message==WMU_VIDEOPLAYBACK)    { InitLevel=50; break; }
      if (Msg.message==WMU_PYTHON)           { InitLevel=60; break; }
      if (Msg.message==WMU_PYTHON_THREAD)    { InitLevel=70; break; }
      if (Msg.message==WMU_SIZE)             { InitLevel=70; break; }

      if (hAccel!=NULL) TranslateAccelerator(hFrameWnd,hAccel,&Msg);
      TranslateCustomKeys(hFrameWnd,mymap,&Msg);
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
    }

  Error:
    if (InitLevel<=60 && PythonRunning) {
       MessageBox(hFrameWnd, "You cannot perform this action because a Python script is running.", "Error", MB_OK | MB_ICONERROR);
       goto MainLoop;
    } else {
      if (InitLevel<=0) MoveOK = timeGetTime();
      // Exit everything
      MediaExit(InitLevel);

      if (InitLevel<=0) break; // Quit program
    }
  }
  return 0;
}
