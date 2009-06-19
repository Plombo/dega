#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include <windowsx.h>

#include "avioutput.h"
#include "libvfw.h"

#include "degavirc.h"

#include <commctrl.h>

int stopFlag;
FILE *rawa = 0, *rawv = 0;
VFW enc;
VFWOptions encOpts;

HINSTANCE hAppInst;
HANDLE encodeThread = 0;

#define ENC_OK 0
#define ENC_OPENERROR 1
#define ENC_ERROR 2
#define ENC_CANCELLED 3

#undef FAKE_ENCODE

struct EncodeSettings {
	HWND hwndDlg;
	int additionalFrames;
	char *romFile, *movieFile;
};

static int EncInit(int _width, int _height, int _framerate, int frameCount, void *data) {
	struct EncodeSettings *es = data;
	HWND pb;
#ifndef FAKE_ENCODE
	enc.width = _width;
	enc.height = _height;
	enc.fps = _framerate;

	if (VFWOpen(&enc) != 0) {
		perror("VFWOpen");
		return ENC_OPENERROR;
	}
#endif

	pb = GetDlgItem(es->hwndDlg, IDC_ENCODE_PROGRESS);
	PostMessage(pb, PBM_SETRANGE32, 0, frameCount);
	PostMessage(pb, PBM_SETPOS, 0, 0);
	PostMessage(pb, PBM_SETSTEP, 1, 0);

	return ENC_OK;
}

#define CHECKRV(fn) \
	if (rv != 0) { \
		puts(fn ": unable to write data"); \
		return ENC_ERROR; \
	}

static int EncVideoFrame(unsigned char *videoData, int videoDataSize, void *data) {
	struct EncodeSettings *es = data;
#ifndef FAKE_ENCODE
	int rv;
#endif
	if (stopFlag) return ENC_CANCELLED;
#ifndef FAKE_ENCODE
	rv = VFWVideoData(&enc, videoData);
	CHECKRV("VFWVideoData")
#endif

	PostMessage(GetDlgItem(es->hwndDlg, IDC_ENCODE_PROGRESS), PBM_STEPIT, 0, 0);

	return ENC_OK;
}

static int EncAudioFrame(short *audioData, int audioDataSize, void *data) {
#ifndef FAKE_ENCODE
	int rv;
#endif
	if (stopFlag) return ENC_CANCELLED;
#ifndef FAKE_ENCODE
	rv = VFWAudioData(&enc, audioData);
	CHECKRV("VFWAudioData")
#endif

	return ENC_OK;
}

static void UpdateDisplay(HWND hwndDlg) {
	wchar_t codecName[128];
	BOOL addFramesOK;
	GetDlgItemInt(hwndDlg, IDT_EXTRA_FRAMES, &addFramesOK, 0);

	EnableWindow(GetDlgItem(hwndDlg, IDOK),
		   VFWOptionsValid(&encOpts) && addFramesOK
		&& Edit_GetTextLength(GetDlgItem(hwndDlg, IDT_ROM))
		&& Edit_GetTextLength(GetDlgItem(hwndDlg, IDT_OUTPUT)));

	if (VFWOptionsDescribeAudioCodec(&encOpts, codecName, 128) == 0) {
		SetDlgItemTextW(hwndDlg, IDT_AUDIOCODEC, codecName);
	} else {
		SetDlgItemText(hwndDlg, IDT_AUDIOCODEC, "???");
	}

	if (VFWOptionsDescribeVideoCodec(&encOpts, codecName, 128) == 0) {
		SetDlgItemTextW(hwndDlg, IDT_VIDEOCODEC, codecName);
	} else {
		SetDlgItemText(hwndDlg, IDT_VIDEOCODEC, "???");
	}
}

#define PROMPT_OPEN 0
#define PROMPT_SAVE 1

void PromptFileToDlg(HWND hwndDlg, int widgetId, char *title, char *fileTypes, char *defaultFileType, int promptType) {
	OPENFILENAME ofn;
	char fileName[1024];
	int rv = 0;

	GetDlgItemText(hwndDlg, widgetId, fileName, sizeof(fileName));

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = hAppInst;
	ofn.hwndOwner = hwndDlg;
	ofn.lpstrTitle = title;
	ofn.lpstrFilter = fileTypes;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = sizeof(fileName);
	ofn.lpstrInitialDir = ".";
	ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = defaultFileType;
	if (promptType == PROMPT_OPEN) {
		rv = GetOpenFileName(&ofn);
	} else if (promptType == PROMPT_SAVE) {
		rv = GetSaveFileName(&ofn);
	}

	if (rv != 0) {
		SetDlgItemText(hwndDlg, widgetId, fileName);
	}
}

static void ResetControls(HWND hwndDlg, int encoding) {
#define ABLE(id) EnableWindow(GetDlgItem(hwndDlg, id), !encoding)
	ABLE(IDT_ROM);          ABLE(IDB_CHOOSE_ROM);
	ABLE(IDT_MOVIE);        ABLE(IDB_CHOOSE_MOVIE);
	ABLE(IDT_EXTRA_FRAMES);
	ABLE(IDT_OUTPUT);       ABLE(IDB_CHOOSE_OUTPUT);
	ABLE(IDT_AUDIOCODEC);   ABLE(IDB_CHOOSE_AUDIOCODEC);
	ABLE(IDT_VIDEOCODEC);   ABLE(IDB_CHOOSE_VIDEOCODEC);
	ABLE(IDOK);
#undef ABLE
	SetDlgItemText(hwndDlg, IDCANCEL, encoding ? "Cancel" : "Close");
}

static DWORD WINAPI EncodeThreadProc(void *param) {
	struct EncodeSettings *es = param;
	int rv;

	rv = AVIOutputRun(es->romFile, *(es->movieFile) ? es->movieFile : 0, es->additionalFrames, 0, 44100, VFORMAT_DIB, EncInit, es, EncVideoFrame, es, EncAudioFrame, es);
#ifndef FAKE_ENCODE
	if (rv != ENC_OPENERROR) VFWClose(&enc);
	if (rv != ENC_OK) remove(enc.output);
#endif
	if (rv == ENC_OPENERROR) {
		MessageBox(es->hwndDlg, "An error occurred while opening a file.", "Error", MB_ICONERROR | MB_OK);
	} else if (rv == -1) {
		MessageBox(es->hwndDlg, "An error occurred while opening the movie file.", "Error", MB_ICONERROR | MB_OK);
	} else if (rv == ENC_ERROR) {
		MessageBox(es->hwndDlg, "An error occurred during encoding.", "Error", MB_ICONERROR | MB_OK);
	}

	PostMessage(GetDlgItem(es->hwndDlg, IDC_ENCODE_PROGRESS), PBM_SETPOS, 0, 0);

	encodeThread = 0;
	ResetControls(es->hwndDlg, 0);

	free(es->romFile); free(es->movieFile); free(enc.output);
	free(es);

	return rv;
}

INT_PTR CALLBACK EncodeOptionsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG: {
			enc.channels = 2;
			enc.rate = 44100;
			enc.samplesize = 2;
			enc.width = 256; /* We don't know yet what the actual size of the movie will be, but we guess here that it is SMS size */
			enc.height = 192;

			VFWOptionsInit(&encOpts, &enc);
			encOpts.parent = hwndDlg;
			enc.opts = &encOpts;

			SetDlgItemInt(hwndDlg, IDT_EXTRA_FRAMES, 0, 0);
			UpdateDisplay(hwndDlg);

			break;
		}
		case WM_COMMAND: {
			switch (wParam & 0xffff) {
				case IDB_CHOOSE_AUDIOCODEC: {
					VFWOptionsChooseAudioCodec(&encOpts);
					UpdateDisplay(hwndDlg);
					break;
				}
				case IDB_CHOOSE_VIDEOCODEC: {
					VFWOptionsChooseVideoCodec(&encOpts);
					UpdateDisplay(hwndDlg);
					break;
				}
				case IDB_CHOOSE_ROM: {
					PromptFileToDlg(hwndDlg, IDT_ROM, "Select ROM", "SMS/Game Gear ROMs (*.sms, *.gg)\0*.sms;*.gg\0All files\0*.*\0\0", "sms", PROMPT_OPEN);
					break;
				}
				case IDB_CHOOSE_MOVIE: {
					PromptFileToDlg(hwndDlg, IDT_MOVIE, "Select movie", "Dega movie files (*.mmv)\0*.mmv\0All files\0*.*\0\0", "mmv", PROMPT_OPEN);
					break;
				}
				case IDB_CHOOSE_OUTPUT: {
					PromptFileToDlg(hwndDlg, IDT_OUTPUT, "Output file", "AVI files (*.avi)\0*.avi\0All files\0*.*\0\0", "avi", PROMPT_SAVE);
					break;
				}
				case IDT_ROM:
				case IDT_OUTPUT: {
					if (wParam >> 16 == EN_CHANGE) UpdateDisplay(hwndDlg);
					break;
				}
				case IDOK: {
					int romFileLen, movieFileLen, outputFileLen, additionalFrames;
					char *romFile, *movieFile, *outputFile;
					struct EncodeSettings *es;

					additionalFrames = GetDlgItemInt(hwndDlg, IDT_EXTRA_FRAMES, 0, 0);

					romFile = malloc(romFileLen = Edit_GetTextLength(GetDlgItem(hwndDlg, IDT_ROM))+1);
					movieFile = malloc(movieFileLen = Edit_GetTextLength(GetDlgItem(hwndDlg, IDT_MOVIE))+1);
					outputFile = malloc(outputFileLen = Edit_GetTextLength(GetDlgItem(hwndDlg, IDT_OUTPUT))+1);

					GetDlgItemText(hwndDlg, IDT_ROM, romFile, romFileLen);
					GetDlgItemText(hwndDlg, IDT_MOVIE, movieFile, movieFileLen);
					GetDlgItemText(hwndDlg, IDT_OUTPUT, outputFile, outputFileLen);

					enc.output = outputFile;

					es = malloc(sizeof(*es));
					es->hwndDlg = hwndDlg;
					es->additionalFrames = additionalFrames;
					es->romFile = romFile;
					es->movieFile = movieFile;
					
					ResetControls(hwndDlg, 1);

					stopFlag = 0;
					encodeThread = CreateThread(0, 0, EncodeThreadProc, es, 0, 0);
					if (!encodeThread) {
						MessageBox(hwndDlg, "Unable to start encode thread.", "Error", MB_OK | MB_ICONERROR);

						free(es->romFile); free(es->movieFile); free(enc.output);
						free(es);

						ResetControls(hwndDlg, 0);
					}

					break;
				}
				case IDCANCEL: {
					if (encodeThread) {
						stopFlag = 1;
					} else {
						EndDialog(hwndDlg, 0);
					}
					break;
				}
				default: return FALSE;
			}
			break;
		}
		default: return FALSE;
	}
	return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	hAppInst = hInstance;
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_ENCODEOPTIONS), 0, EncodeOptionsDialogProc);
}
