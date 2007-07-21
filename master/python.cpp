#include "../python/embed.h"

#include "app.h"

int PythonLoaded=0, PythonRunning=0, PythonThread;

void PythonLoad(int _argc, char **_argv) {
	MPyEmbed_SetArgv(_argc, _argv);
}

void PythonInit() {
	MPyEmbed_Init();
	PythonLoaded = MPyEmbed_Available();
}

void PythonExit() {
	MPyEmbed_Fini();
}

void PythonRun() {
	MPyEmbed_Run(PythonScriptName);
	PythonRunning = 0;
}

static DWORD WINAPI PythonThreadProc(void *pParam) {
	MPyEmbed_RunThread(PythonScriptName);
	ExitThread(0);
	return 0;
}

void PythonRunThread() {
	DWORD RunId;
	CreateThread(NULL,0,PythonThreadProc,NULL,0,&RunId);
}

void PythonPostFrame() {
	MPyEmbed_CBPostFrame();
}
