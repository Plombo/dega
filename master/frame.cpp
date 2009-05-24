#include "app.h"

static char *ClassName="Frame";
HWND hFrameWnd=NULL; // Frame - Window handle
HMENU hFrameMenu=NULL;
HWND hFrameStatus=NULL; // Frame - status window
DWORD MoveOK=0;

// The window procedure
static LRESULT CALLBACK WindowProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
  if (Msg==WM_CREATE)
  {
    hFrameStatus=CreateStatusWindow(WS_CHILD,"",hWnd,0);
    if (StatusMode==STATUS_SHOW) ShowWindow(hFrameStatus,SW_NORMAL);
    return 0;
  }

  if (Msg==WM_SIZE)
  {
    RECT Rect={0,0,0,0};
    DWORD time = timeGetTime();
    if (time-MoveOK > 2000) /* This is a stupid way to handle this,
                             * but is the only way I can think of given
                             * the seemingly random number of WM_SIZE
                             * messages Windows sends us.
                             * Thanks Microsoft!
                             */
    {
      SizeMultiplier=0;
    }
    GetWindowRect(hWnd,&Rect);
    MoveWindow(hFrameStatus,Rect.left,Rect.bottom,Rect.right,Rect.bottom,1);
    PostMessage(NULL,WMU_SIZE,0,0);
    return 0;
  }

  if (Msg==WM_PAINT) { DispPaint(); }
  if (Msg==WM_MOVE) { DispPaint(); }

  if (Msg==WM_KEYDOWN && wParam==VK_ESCAPE) { PostMessage(NULL,WMU_TOGGLEMENU,0,0); return 0; }
  if (Msg==WM_RBUTTONDOWN)                  { PostMessage(NULL,WMU_TOGGLEMENU,0,0); return 0; }

  if (Msg==WM_COMMAND)
  {
    int Item=0;
    Item=wParam&0xffff;
    if (Item==ID_FILE_LOADROM) { MenuLoadRom(); return 0; }
    if (Item==ID_FILE_FREEROM)
    {
      EmuRomName[0]=0;
      PostMessage(NULL,WMU_CHANGEDROM,0,0);
      return 0;
    }
    if (Item==ID_STATE_LOADSTATE) { MenuStateLoad(0); return 0; }
    if (Item==ID_STATE_SAVESTATE) { MenuStateLoad(1); return 0; }
    if (Item==ID_STATE_IMPORT) { MenuStatePort(0); return 0; }
    if (Item==ID_STATE_EXPORT) { MenuStatePort(1); return 0; }
    if (Item==ID_SOUND_VGMLOG_START) { MenuVgmStart(); return 0; }
    if (Item==ID_VIDEO_PLAYBACK || Item==ID_VIDEO_RECORD)
      { MenuVideo(Item); return 0; }
    if (Item==ID_VIDEO_PROPERTIES) { VideoProperties(); return 0; }
    if (Item==ID_INPUT_KEYMAPPING) { KeyMapping(); return 0; }
    if (Item==ID_PYTHON_LOAD) { MenuPython(0); return 0; }
    if (Item==ID_PYTHON_LOAD_THREAD) { MenuPython(1); return 0; }
    if (Item==ID_PYTHON_MEMORY) { MenuPythonFixed("memory.py", 1); return 0; }
    PostMessage(NULL,WMU_COMMAND,Item,0);
  }

  if (Msg==WM_CLOSE) { PostQuitMessage(0); return 0; }
  if (Msg==WM_DESTROY) { hFrameStatus=NULL; hFrameWnd=NULL; return 0; }

  return DefWindowProc(hWnd,Msg,wParam,lParam);
}

int FrameInit()
{
  WNDCLASSEX wc;
  ATOM Atom=0;

  // Register the window class
  memset(&wc,0,sizeof(wc));
  wc.cbSize=sizeof(wc);
  wc.style=CS_HREDRAW|CS_VREDRAW;
  wc.lpfnWndProc=WindowProc;
  wc.hInstance=hAppInst;
  wc.hIcon=LoadIcon(hAppInst,MAKEINTRESOURCE(IDI_ICON1));
  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
  wc.lpszClassName=ClassName;
  Atom=RegisterClassEx(&wc);
  if (Atom==0) return 1;

  hFrameWnd=CreateWindow(ClassName,"",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
    100,100,0,0,NULL,NULL,hAppInst,NULL);
  if (hFrameWnd==NULL) return 1;

  // Load the menu from the resource
  hFrameMenu=LoadMenu(hAppInst,MAKEINTRESOURCE(IDR_MENU1));
  return 0;
}

#if 0
int FrameSize()
{
  RECT WorkArea={0,0,640,480}; // Work area on the desktop
  int x=0,y=0,w=0,h=0;
  if (hFrameWnd==NULL) return 1;

  // Get the desktop work area
  SystemParametersInfo(SPI_GETWORKAREA,0,&WorkArea,0);
  // Find midpoint
  x=WorkArea.left  +WorkArea.right; x>>=1;
  y=WorkArea.bottom+WorkArea.top;   y>>=1;

  w=WorkArea.right -WorkArea.left;
  h=WorkArea.bottom-WorkArea.top;

  w=w*3/4; h=h*3/4;

  x-=w>>1;
  y-=h>>1;

  MoveWindow(hFrameWnd,x,y,w,h,1);
  return 0;
}
#else
int SizeMultiplier = 2;

int FrameSize()
{
  RECT size, client;
  int diffx=-42, diffy=-42, newdiffx, newdiffy;
  if (SizeMultiplier == 0) return 0;

  /* The loop is necessary because the size of the non-client area
   * may change when we resize (because of the menu)
   */
  while (1)
  {
    GetWindowRect(hFrameWnd,&size);
    GetClientRect(hFrameWnd,&client);

    newdiffx = (size.right-size.left) - client.right;
    newdiffy = (size.bottom-size.top) - client.bottom;

    if (diffx==newdiffx && diffy==newdiffy) break;

    diffx = newdiffx;
    diffy = newdiffy;

    MoveOK = timeGetTime();
    MoveWindow(hFrameWnd,
        size.left,
        size.top,
        diffx + ScrnWidth*SizeMultiplier,
        diffy + ScrnHeight*SizeMultiplier+StatusHeight(),
    1);
  }
  return 0;
}
#endif

int FrameExit()
{
  SetMenu(hFrameWnd,NULL);
  if (hFrameMenu!=NULL) DestroyMenu(hFrameMenu);  hFrameMenu=NULL;

  // Destroy window if not already destroyed
  if (hFrameWnd!=NULL) DestroyWindow(hFrameWnd);  hFrameWnd=NULL;

  // Unregister the window class
  UnregisterClass(ClassName,hAppInst);
  return 0;
}
