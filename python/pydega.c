#include <Python.h>
#include <mast.h>

static PyObject *pydega_hello(PyObject *self, PyObject *args) {
	printf("hello world! %d\n", 42);
	Py_RETURN_NONE;
}

static PyObject *pydega_load_rom(PyObject *self, PyObject *args) {
	const char *file;
	unsigned char *rom;
	int romlength;
	int rv;

	if (!PyArg_ParseTuple(args, "s", &file))
		return NULL;
	
	rv = MastLoadRom(file, &rom, &romlength);
	if (rv != 0)
		return PyErr_SetFromErrno(PyExc_IOError);
	
	MastSetRom(rom, romlength);

	Py_RETURN_NONE;
}

static PyMethodDef PydegaMethods[] = {
	{ "hello", pydega_hello, METH_VARARGS, "Say hello" },
	{ "load_rom", pydega_load_rom, METH_VARARGS, "Load a rom" },
	{ NULL, NULL, 0, NULL }
};

void MdrawCall() {}
void MvidModeChanged() {}
void MvidMovieStopped() {}

PyMODINIT_FUNC initpydega(void) {
	MastInit();
	Py_InitModule("pydega", PydegaMethods);
}

