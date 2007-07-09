#include "app.h"

char VideoName[256] = "";

void VideoRecord(int reset)
{
  MvidStart(VideoName, RECORD_MODE, reset);
}

void VideoPlayback()
{
  MvidStart(VideoName, PLAYBACK_MODE, 0);
  SetupPal = (MastEx & MX_PAL) ? 1 : 0;
}

void MvidModeChanged() {}

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
