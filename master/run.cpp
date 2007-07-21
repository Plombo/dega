// Run module
#include "app.h"

static unsigned int LastSecond=0;
static int FramesDone=0;

static HANDLE hRunThread=NULL;
static DWORD RunId=0,MainId=0;

int StatusMode=STATUS_AUTO;
static int StatusCount=-1;
static int NoInput=0;

// Run a frame (with or without sound)
int RunFrame(int Draw,short *pSound)
{
  if (!NoInput) InputGet();
  MastDrawDo=Draw; pMsndOut=pSound;

  // Run frame
  EmuFrame();

  // Update status window
  StatusCount--;
  if (StatusCount==0)
  {
    if (StatusMode==STATUS_AUTO) ShowWindow(hFrameStatus,SW_HIDE);
    SetWindowText(hFrameStatus,"");
    StatusCount=-1;
  }

  if (Draw) DispDraw();

  PythonPostFrame();

  return 0;
}

void MimplFrame(int ReadInput)
{
  if (!ReadInput) NoInput=1;
  RunFrame(1,NULL);
  NoInput=0;
}

// Display text for a period of time in the status bar
int RunText(char *Text,int Len)
{
  SetWindowText(hFrameStatus,Text);
  if (StatusMode==STATUS_AUTO) ShowWindow(hFrameStatus,SW_NORMAL);
  StatusCount=Len;
  return 0;
}

void SetStatusMode(int NewMode)
{
       if (NewMode==STATUS_HIDE) ShowWindow(hFrameStatus,SW_HIDE);
  else if (NewMode==STATUS_SHOW) ShowWindow(hFrameStatus,SW_NORMAL);
  else if (NewMode==STATUS_AUTO) ShowWindow(hFrameStatus,StatusCount>0?SW_NORMAL:SW_HIDE);
  StatusMode=NewMode;
}

int StatusHeight()
{
  static int height = -1;
  if (height==-1)
  {
    RECT rect;
    GetClientRect(hFrameStatus,&rect);
    height=rect.bottom;
  }
  if (StatusMode==STATUS_HIDE)
  {
    return 0;
  } else if (StatusMode==STATUS_AUTO)
  {
    return StatusCount>0 ? height : 0;
  } else /* StatusMode==STATUS_SHOW */
  {
    return height;
  }
}

static void RunIdle()
{
#if 1
  int Time=0,Frame=0;
  int Do=0,i=0;
#else
  int NewTime;
  int MsPerFrame=1000/RealFramesPerSecond;
  static int SkipCount=0;
#endif

  if (GetActiveWindow()==hFrameWnd && GetAsyncKeyState(KeyMappings[KMAP_FASTFORWARD])&0x8000)
  {
    // Fast forward
    int i=0; for (i=0;i<5;i++) RunFrame(0,NULL);
  }

  // Try to run with sound
  if (DSoundPlaying) { DSoundCheck(); return; }

#if 1
  Time=timeGetTime()-LastSecond; // This will be about 0-1000 ms
  Frame=Time*RealFramesPerSecond/1000;
  Do=Frame-FramesDone;
  FramesDone=Frame;

  if (FramesPerSecond>0)
  {
    // Carry over any >60 frame count to one second
    while (FramesDone>=RealFramesPerSecond) { FramesDone-=RealFramesPerSecond; LastSecond+=1000; }
  }

  if (Do<=0) { Sleep(4); return; }
  if (Do>10) Do=10; // Limit frame skipping

  // Exec frames
  pMsndOut=NULL;
  for (i=0;i<Do;i++)
  {
    if (i>=Do-1) RunFrame(1,NULL);
    else         RunFrame(0,NULL);
  }
#else
  NewTime = timeGetTime();
  printf("N-L = %d, M = %d, S = %d\n", NewTime-LastSecond, MsPerFrame, SkipCount);
  if (NewTime-LastSecond>MsPerFrame && SkipCount<10) {
    RunFrame(0,NULL);
    SkipCount++;
  } else {
    SkipCount=0;
    if (MsPerFrame>NewTime-LastSecond) Sleep(MsPerFrame-(NewTime-LastSecond));
    RunFrame(1,NULL);
  }
  LastSecond=NewTime;
#endif
}

// Callback from DSoundCheck()
static int FrameWithSound(int bDraw) { return RunFrame(bDraw,DSoundNextSound); }

// The Run Thread
static DWORD WINAPI RunThreadProc(void *pParam)
{
  MSG Msg; int Ret=0;
  (void)pParam;
  PeekMessage(&Msg,NULL,0,0,PM_NOREMOVE); // Make message queue
  AttachThreadInput(RunId,MainId,1); // Attach to main thread (for Input/GetActiveWindow)

  LastSecond=timeGetTime(); FramesDone=0; // Remember start time
  DSoundGetNextSound=FrameWithSound; // Use our callback to make sound

  if (PythonRunning)
  {
    PythonRun();
  }

  if (LoopPause==0)
  {
    for (;;)
    {
      Ret=PeekMessage(&Msg,NULL,0,0,PM_REMOVE);
      if (Ret!=0)
      {
        // A message is waiting to be processed
        if (Msg.message==WM_QUIT) break; // Quit thread
      }
      else
      {
        // No messages are waiting
        RunIdle();
      }
    }
  }
  AttachThreadInput(RunId,MainId,0); // Detach from main thread
  ExitThread(0); return 0;
}


int RunStart()
{
  // Get the main thread id
  MainId=GetCurrentThreadId();

  RunId=0;
  hRunThread=CreateThread(NULL,0,RunThreadProc,NULL,0,&RunId);
  if (hRunThread==NULL) return 1;

  // Set thread priority higher
  SetThreadPriority(hRunThread,THREAD_PRIORITY_ABOVE_NORMAL);

  return 0;
}

int RunStop()
{
  int Ret=0;

  if (PythonRunning)
  {
    MessageBox(0, "Uh oh.  You're in deep shit now.", "Error", 0);
  }

  if (hRunThread!=NULL)
  {
    // Exit the thread
    PostThreadMessage(RunId,WM_QUIT,0,0); // try to exit cleanly
    Ret=WaitForSingleObject(hRunThread,250);
    if (Ret!=WAIT_OBJECT_0) TerminateThread(hRunThread,0); // else kill the thread
    CloseHandle(hRunThread);
  }
  hRunThread=NULL;
  RunId=0;
  MainId=0;
  return 0;
}
