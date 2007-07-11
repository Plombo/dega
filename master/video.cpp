#include "app.h"

char VideoName[256] = "";

static int VideoReset;
static char VideoAuthor[64];

static void HexDigest(char *out, unsigned char *digest) {
	snprintf(out, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	  digest[0], digest[1], digest[2], digest[3],
	  digest[4], digest[5], digest[6], digest[7],
	  digest[8], digest[9], digest[10], digest[11],
	  digest[12], digest[13], digest[14], digest[15]);
}

static UINT_PTR CALLBACK MenuVideoPlaybackHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  /*
  char buf[32];
  snprintf(buf, 32, "uiMsg = %d", uiMsg);
  MessageBox(0, buf, "foo", 0);
  */
  switch (uiMsg) {
  	case WM_INITDIALOG:
	{
		unsigned char digest[16];
		char buf[33];
		SetDlgItemText(hdlg, IDT_PI_ROM, MastRomName);

		MastGetRomDigest(digest);
		HexDigest(buf, digest);
		SetDlgItemText(hdlg, IDT_PI_DIGEST, buf);

		break;
	}
	case WM_NOTIFY:
	{
		LPOFNOTIFY nt = (LPOFNOTIFY)lParam;
		/*
		snprintf(buf, 32, "nt->hdr.code = %d", nt->hdr.code);
		MessageBox(0, buf, "foo", 0);
		*/
		switch (nt->hdr.code) {
			case CDN_SELCHANGE:
			{
				char buf[256];
				HWND hparent = GetParent(hdlg);
				FILE *file;
				struct MvidHeader header;
				CommDlg_OpenSave_GetFilePath(hparent, (LPARAM)buf, sizeof(buf));

				if ((file = fopen(buf, "rb"))
				  && MvidReadHeader(file, &header)) {
				  	WCHAR bufWide[128];

					snprintf(buf, sizeof(buf), "%d", header.vidFrameCount);
					SetDlgItemText(hdlg, IDT_PI_FRAMECOUNT, buf);

					snprintf(buf, sizeof(buf), "%d", header.rerecordCount);
					SetDlgItemText(hdlg, IDT_PI_RERECORDCOUNT, buf);

					MultiByteToWideChar(CP_UTF8, 0, header.videoAuthor, -1, bufWide, sizeof(bufWide));
					SetDlgItemTextW(hdlg, IDT_PI_AUTHOR, bufWide);

					SetDlgItemText(hdlg, IDT_PI_RESET, header.beginReset ? "yes" : "no");

					SetDlgItemText(hdlg, IDT_PI_MOVIE_ROM, header.vidRomName);

					HexDigest(buf, header.vidDigest);
					SetDlgItemText(hdlg, IDT_PI_MOVIE_DIGEST, buf);
				} else {
					SetDlgItemText(hdlg, IDT_PI_FRAMECOUNT, "");
					SetDlgItemText(hdlg, IDT_PI_RERECORDCOUNT, "");
					SetDlgItemText(hdlg, IDT_PI_AUTHOR, "");
					SetDlgItemText(hdlg, IDT_PI_RESET, "");
					SetDlgItemText(hdlg, IDT_PI_MOVIE_ROM, "");
					SetDlgItemText(hdlg, IDT_PI_MOVIE_DIGEST, "");
				}

				if (file) fclose(file);
				// SetDlgItemText(hdlg, IDT_RI_FILENAME, buf);
				break;
			}
			default:
				return 0;
		}
		break;
	}
	default:
		return 0;
  }
  return 1;
}

UINT_PTR CALLBACK MenuVideoRecordHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uiMsg) {
  	case WM_COMMAND:
	{
		int cmd = wParam&0xffff;
		switch (cmd) {
			case IDT_RI_RESET:
				/* goddamn monkeys over at microsoft... why do I have to change the state myself? */
				VideoReset=!VideoReset;
				SendDlgItemMessage(hdlg, IDT_RI_RESET, BM_SETCHECK, VideoReset?BST_CHECKED:BST_UNCHECKED, 0);
				break;
			default:
				return 0;
		}
		break;
	}
	case WM_NOTIFY:
	{
		LPOFNOTIFY nt = (LPOFNOTIFY)lParam;
		switch (nt->hdr.code) {
			case CDN_FILEOK:
			{
				WCHAR authorNameWide[64];

				GetDlgItemTextW(hdlg, IDT_RI_AUTHOR, authorNameWide, sizeof(authorNameWide));
				WideCharToMultiByte(CP_UTF8, 0, authorNameWide, -1, VideoAuthor, sizeof(VideoAuthor), 0, 0);
				break;
			}
			default:
				return 0;
		}
		break;
	}
  	default:
		return 0;
  }
  return 1;
}

int MenuVideo(int Action)
{
  char Name[256];
  OPENFILENAME Ofn; int Ret=0; VideoReset=0;

  SetCurrentDirectory(StateFolder); // Point to the state folder

  memset(Name,0,sizeof(Name));
  // Copy the name of the rom without the extension
  if (EmuTitle!=NULL) strncpy(Name,EmuTitle,sizeof(Name)-1);

  memset(&Ofn,0,sizeof(Ofn));
  Ofn.lStructSize=sizeof(Ofn);
  Ofn.hInstance=hAppInst;
  Ofn.hwndOwner=hFrameWnd;
  Ofn.lpstrFilter="Dega video files (.mmv)\0*.mmv\0All Files (*.*)\0*.*\0\0";
  Ofn.lpstrFile=Name;
  Ofn.nMaxFile=sizeof(Name);
  Ofn.lpstrInitialDir=".";
  Ofn.Flags=OFN_OVERWRITEPROMPT|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE|OFN_EXPLORER;
  Ofn.lpstrDefExt="mmv";

  switch (Action)
  {
    case ID_VIDEO_PLAYBACK:
      Ofn.lpstrTitle="Play movie";
      if (VideoReadOnly) Ofn.Flags|=OFN_READONLY;
      Ofn.lpfnHook=MenuVideoPlaybackHook;
      Ofn.lpTemplateName=MAKEINTRESOURCE(IDD_PLAYBACK_INFO);
      Ret=GetOpenFileName(&Ofn);
      VideoReadOnly=(Ofn.Flags&OFN_READONLY)!=0;
      break;
    case ID_VIDEO_RECORD:
      Ofn.lpstrTitle="Record movie";
      Ofn.Flags|=OFN_HIDEREADONLY;
      Ofn.lpfnHook=MenuVideoRecordHook;
      Ofn.lpTemplateName=MAKEINTRESOURCE(IDD_RECORD_INFO);
      Ret=GetSaveFileName(&Ofn);
      break;
  }

  // Get the state folder
  memset(StateFolder,0,sizeof(StateFolder));
  GetCurrentDirectory(sizeof(StateFolder)-1,StateFolder);
  
  // Return to the application folder
  AppDirectory();

  if (Ret==0) return 1; // Cancel/error

  // Remember the Video name
  memcpy(VideoName,Name,sizeof(Name));
  // Post to main message loop
  switch (Action)
  {
    case ID_VIDEO_PLAYBACK:    PostMessage(NULL,WMU_VIDEOPLAYBACK,0,0); break;
    case ID_VIDEO_RECORD:      PostMessage(NULL,WMU_VIDEORECORD,0,0); break;
  }
  return 0;
}

void VideoRecord()
{
  MvidStart(VideoName, RECORD_MODE, VideoReset);
  MvidSetAuthor(VideoAuthor);
  RunText("Movie record", 2*60);
}

void VideoPlayback()
{
  MvidStart(VideoName, PLAYBACK_MODE, 0);
  SetupPal = (MastEx & MX_PAL) ? 1 : 0;
  RunText("Movie playback", 2*60);
}

void MvidModeChanged() {}

void MvidMovieStopped() {
	EnableMenuItem(hFrameMenu,ID_VIDEO_STOP, MF_GRAYED);
	RunText("Movie stopped", 2*60);
}

static BOOL CALLBACK VideoPropertiesProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
	case WM_INITDIALOG: {
		int frameCount = MvidGetFrameCount(), rerecordCount = MvidGetRerecordCount();
		char frameCountStr[16], rerecordCountStr[16];

		char *authorName = MvidGetAuthor();
		WCHAR authorNameWide[128];

		snprintf(frameCountStr, sizeof(frameCountStr), "%d", frameCount);
		SetDlgItemText(hwndDlg, IDT_MP_FRAMECOUNT, frameCountStr);

		snprintf(rerecordCountStr, sizeof(rerecordCountStr), "%d", rerecordCount);
		SetDlgItemText(hwndDlg, IDT_MP_RERECORDCOUNT, rerecordCountStr);

		MultiByteToWideChar(CP_UTF8, 0, authorName, -1, authorNameWide, sizeof(authorNameWide));
		SetDlgItemTextW(hwndDlg, IDT_MP_AUTHOR, authorNameWide);

		break;
	}
	case WM_COMMAND:
		switch (wParam & 0xffff) {
			case IDOK: {
				WCHAR authorNameWide[128];
				char authorName[64];

				GetDlgItemTextW(hwndDlg, IDT_MP_AUTHOR, authorNameWide, sizeof(authorNameWide));
				WideCharToMultiByte(CP_UTF8, 0, authorNameWide, -1, authorName, sizeof(authorName), 0, 0);

				if (MvidSetAuthor(authorName) == 0) {
					MessageBox(hwndDlg, "For some strange reason, I could not set the author name.", "Error", MB_ICONEXCLAMATION | MB_OK);
				}
			}

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				break;
		}
		break;
	default:
		return FALSE;
  }
  return TRUE;
}

void VideoProperties()
{
  DialogBox(hAppInst, MAKEINTRESOURCE(IDD_MOVIEPROPERTIES), hFrameWnd, VideoPropertiesProc);
}
