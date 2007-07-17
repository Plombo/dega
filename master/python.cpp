#include "../python/linkage.h"
#include <Python.h>
#include "../python/linkage.h"

#include "app.h"

int PythonLoaded=0, PythonRunning=0;
static int argc;
static char **argv;

void PythonLoad(int _argc, char **_argv) {
	PythonLoaded = initlinkage();
	if (PythonLoaded) {
		argc = _argc;
		argv = _argv;
	}
}

void PythonInit() {
	if (PythonLoaded) {
		Py_Initialize();
		PySys_SetArgv(argc, argv);
		initpydega();
	}
}

void PythonExit() {
	if (PythonLoaded) {
		Py_Finalize();
	}
}

void PythonRun() {
	if (PythonLoaded) {
		FILE *fp = fopen(PythonScriptName, "r");
		PyRun_SimpleFile(fp, PythonScriptName);
		fclose(fp);
	}
	PythonRunning = 0;
}
