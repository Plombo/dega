#include "linkage.h"
#include <Python.h>
#include <pythread.h>
#include "linkage.h"

#include "embed.h"

static int PythonAvailable;
static PyThreadState *mainstate;
static int argc;
static char **argv;

unsigned int MPyEmbed_Exiting = 0;

#define THREADS 8

struct MPyEmbed_ThreadData {
	long thread_id;
	PyThreadState *mainstate, *threadstate;
	PyThread_type_lock exitlock;
	PyObject *postframe;
} threaddata[THREADS];
PyThread_type_lock threaddatalock;

#define VALID(td) ((td).mainstate != 0)

void MPyEmbed_SetArgv(int _argc, char **_argv) {
	argc = _argc;
	argv = _argv;
}

void MPyEmbed_Init(void) {
	if (!PythonAvailable) {
		PythonAvailable = initlinkage();
	}
	if (!PythonAvailable) {
		return;
	}
	PyEval_InitThreads();
	Py_Initialize();

	initpydega();

	PySys_SetArgv(argc, argv);
	mainstate = PyEval_SaveThread();
	memset(threaddata, 0, sizeof(threaddata));
	threaddatalock = PyThread_allocate_lock();
}

void MPyEmbed_Fini(void) {
	int i;
	if (!PythonAvailable) {
		return;
	}
	MPyEmbed_Exiting = 1;
	for (i = 0; i < THREADS; i++) {
		if (VALID(threaddata[i])) {
			PyThread_acquire_lock(threaddata[i].exitlock, WAIT_LOCK);
			PyThread_free_lock(threaddata[i].exitlock);
		} else if (threaddata[i].exitlock != 0) {
			PyThread_free_lock(threaddata[i].exitlock);
		}

	}
	memset(threaddata, 0, sizeof(threaddata));

	PyEval_RestoreThread(mainstate);
	Py_Finalize();
	MPyEmbed_Exiting = 0;
}
 
int MPyEmbed_Available(void) {
	return PythonAvailable;
}

int MPyEmbed_Run(char *file) {
	FILE *fp;
	if (!PythonAvailable) {
		return 0;
	}
	fp = fopen(file, "r");
	if (!fp) {
		perror("fopen");
		return 0;
	}
	mainstate->thread_id = PyThread_get_thread_ident();
	PyEval_AcquireThread(mainstate);
	PyRun_SimpleFile(fp, file);
	PyEval_ReleaseThread(mainstate);
	fclose(fp);
	return 1;
}

int MPyEmbed_Repl(void) {
	if (!PythonAvailable) {
		return 0;
	}
	mainstate->thread_id = PyThread_get_thread_ident();
	PyEval_AcquireThread(mainstate);
	PyRun_AnyFile(stdin, "<stdin>");
	PyEval_ReleaseThread(mainstate);
	return 1;
}

int MPyEmbed_RunThread(char *file) {
	int i;
	FILE *fp;
	if (!PythonAvailable) {
		return 0;
	}
	fp = fopen(file, "r");
	if (!fp) {
		perror("fopen");
		return 0;
	}
	PyThread_acquire_lock(threaddatalock, WAIT_LOCK);
	
	for (i = 0; i < THREADS; i++) {
		if (!VALID(threaddata[i])) {
			if (threaddata[i].exitlock != 0)
				PyThread_free_lock(threaddata[i].exitlock);

			threaddata[i].thread_id = PyThread_get_thread_ident();

			PyEval_AcquireLock();
			threaddata[i].threadstate = Py_NewInterpreter();

			initpydega();
			PySys_SetArgv(argc, argv);

			PyEval_ReleaseThread(threaddata[i].threadstate);
			
			threaddata[i].mainstate = PyThreadState_New(threaddata[i].threadstate->interp);
			threaddata[i].mainstate->thread_id = mainstate->thread_id;

			threaddata[i].exitlock = PyThread_allocate_lock();
			PyThread_acquire_lock(threaddata[i].exitlock, WAIT_LOCK);
			break;
		}
	}

	PyThread_release_lock(threaddatalock);

	if (i == THREADS) {
		fclose(fp);
		return 0;
	}

	PyEval_AcquireThread(threaddata[i].threadstate);
	PyRun_SimpleFile(fp, file);
	PyEval_ReleaseThread(threaddata[i].threadstate);
	fclose(fp);

	PyThread_acquire_lock(threaddatalock, WAIT_LOCK);
	
	PyEval_AcquireThread(threaddata[i].threadstate);
	PyThreadState_Clear(threaddata[i].mainstate);
	PyThreadState_Delete(threaddata[i].mainstate);
	Py_EndInterpreter(threaddata[i].threadstate);
	PyEval_ReleaseLock();

	threaddata[i].mainstate = threaddata[i].threadstate = 0;
	PyThread_release_lock(threaddatalock);

	PyThread_release_lock(threaddata[i].exitlock);

	return 1;
}

static struct MPyEmbed_ThreadData *get_current_data(void) {
	PyThreadState *ts = PyThreadState_Get();
	int i;

	for (i = 0; i < THREADS; i++) {
		if (VALID(threaddata[i]) &&
			(ts == threaddata[i].mainstate || ts == threaddata[i].threadstate)) {
			return &threaddata[i];
		}
	}

	return 0;
}

PyObject *MPyEmbed_GetPostFrame(void) {
	struct MPyEmbed_ThreadData *td;
	PyObject *postframe;

	PyThread_acquire_lock(threaddatalock, WAIT_LOCK);

	td = get_current_data();
	if (!td) {
		PyThread_release_lock(threaddatalock);
		return 0;
	}
	postframe = td->postframe;
	PyThread_release_lock(threaddatalock);
	Py_XINCREF(postframe);
	return postframe;
}

int MPyEmbed_SetPostFrame(PyObject *postframe) {
	struct MPyEmbed_ThreadData *td;
	PyObject *old_postframe;

	PyThread_acquire_lock(threaddatalock, WAIT_LOCK);

	td = get_current_data();
	if (!td) {
		PyThread_release_lock(threaddatalock);
		return 0;
	}
	old_postframe = td->postframe;
	td->postframe = postframe;
	Py_XINCREF(postframe);
	PyThread_release_lock(threaddatalock);
	Py_XDECREF(old_postframe);
	return 1;
}

void MPyEmbed_CBPostFrame(void) {
	int i;
	if (!PythonAvailable) {
		return;
	}
	PyThread_acquire_lock(threaddatalock, WAIT_LOCK);
	for (i = 0; i < THREADS; i++) {
		if (VALID(threaddata[i])) {
			PyObject *cb = threaddata[i].postframe;
			PyThreadState *ts = threaddata[i].mainstate;
			PyObject *rv;

			PyThread_release_lock(threaddatalock);
			ts->thread_id = PyThread_get_thread_ident();
			PyEval_AcquireThread(ts);
			if (cb != NULL && PyCallable_Check(cb)) {
				rv = PyObject_CallObject(cb, NULL);
				Py_XDECREF(rv);
				if (rv == NULL) {
					PyErr_Print();
				}
			}
			PyEval_ReleaseThread(ts);
			PyThread_acquire_lock(threaddatalock, WAIT_LOCK);
		}
	}
	PyThread_release_lock(threaddatalock);
}

