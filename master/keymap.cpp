#include "app.h"

unsigned short KeyMappings[KMAPCOUNT] = {
  'Z',          /* "1" */
  'X',          /* "2" */
  VK_UP,        /* Up */
  VK_DOWN,      /* Down */
  VK_LEFT,      /* Left */
  VK_RIGHT,     /* Right */
  'C',          /* START */
  'P',          /* Pause */
  'O',          /* Frame Advance */
  VK_F5,        /* Quick Load */
  VK_F6,        /* Quick Save */
  VK_OEM_PLUS,  /* Speed Up */
  VK_OEM_MINUS, /* Slow Down */
  VK_F8,        /* Fast Forward */
  '0',          /* Save Slot 0 */
  '1',          /* Save Slot 1 */
  '2',          /* Save Slot 2 */
  '3',          /* Save Slot 3 */
  '4',          /* Save Slot 4 */
  '5',          /* Save Slot 5 */
  '6',          /* Save Slot 6 */
  '7',          /* Save Slot 7 */
  '8',          /* Save Slot 8 */
  '9',          /* Save Slot 9 */
};

static unsigned short NewKeyMappings[KMAPCOUNT];
static int receivingKmap;

static HHOOK hook;

static HWND hwndKeyDlg;

static void EnableAll(int enable) {
	int i;
	for (i = 0; i < KMAPCOUNT; i++) {
		EnableWindow(GetDlgItem(hwndKeyDlg, ID_KMAP(i)), enable);
	}
}

static LRESULT CALLBACK KeyMappingHook(int code, WPARAM wParam, LPARAM lParam) {
	if (code < 0) {
		return CallNextHookEx(hook, code, wParam, lParam);
	}

	char keyNameBuf[16];
	unsigned short key = wParam;

	NewKeyMappings[receivingKmap] = key;

	snprintf(keyNameBuf, 16, "0x%02x", key);
	SetDlgItemText(hwndKeyDlg, ID_LAB_KMAP(receivingKmap), keyNameBuf);

	EnableAll(TRUE);

	UnhookWindowsHookEx(hook);
	hook = 0;

	return 1;
}

static BOOL CALLBACK KeyMappingProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
	case WM_INITDIALOG: {
		int i;
		hwndKeyDlg = hwndDlg;
		memcpy(NewKeyMappings, KeyMappings, sizeof(KeyMappings));

		for (i = 0; i < KMAPCOUNT; i++) {
			char keyNameBuf[16];
			snprintf(keyNameBuf, 16, "0x%02x", KeyMappings[i]);
			SetDlgItemText(hwndKeyDlg, ID_LAB_KMAP(i), keyNameBuf);
		}
			
		break;
	}

	case WM_COMMAND:
		if (IS_ID_KMAP(wParam & 0xffff)) {
			receivingKmap = KMAP_ID(wParam & 0xffff);
			hook = SetWindowsHookEx(WH_KEYBOARD, KeyMappingHook, 0, GetCurrentThreadId());

			EnableAll(FALSE);

			break;
		} else switch (wParam & 0xffff) {
			case IDOK:
				memcpy(KeyMappings, NewKeyMappings, sizeof(KeyMappings));
			case IDCANCEL:
				if (hook != 0) {
					UnhookWindowsHookEx(hook);
					hook = 0;
				}

				EndDialog(hwndDlg, 0);
				break;
		}
		break;

	default:
		return FALSE;
  }
  return TRUE;
}


void KeyMapping() {
  DialogBox(hAppInst, MAKEINTRESOURCE(IDD_HOTKEYMAP), hFrameWnd, KeyMappingProc);
}
