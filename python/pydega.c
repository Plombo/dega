#include <Python.h>
#include <structmember.h>

#include <mastint.h>

typedef struct {
	PyObject_HEAD
	unsigned char *data;
	size_t datalen;
} UCharArray;

static PyTypeObject uchararray_type;

static PyObject *uchararray_create(unsigned char *data, size_t datalen);

static void uchararray_dealloc(PyObject *self) {
	self->ob_type->tp_free(self);
}

static int uchararray_length(PyObject *self) {
	UCharArray *uself = (UCharArray *)self;
	return uself->datalen;
}

static PyObject *uchararray_item(PyObject *self, int index) {
	UCharArray *uself = (UCharArray *)self;

	if (index < 0 || index >= uself->datalen) {
		PyErr_SetString(PyExc_IndexError, "index out of bounds");
		return NULL;
	}

	return Py_BuildValue("i", uself->data[index]);
}

static PyObject *uchararray_slice(PyObject *self, int lo, int hi) {
	UCharArray *uself = (UCharArray *)self;

	if (hi == INT_MAX) hi = uself->datalen;

	if (lo < 0 || hi > uself->datalen || lo > hi) {
		PyErr_SetString(PyExc_IndexError, "index out of bounds");
		return NULL;
	}

	return uchararray_create(uself->data+lo, hi-lo);
}

static int uchararray_ass_item(PyObject *self, int index, PyObject *value) {
	UCharArray *uself = (UCharArray *)self;

	if (index < 0 || index >= uself->datalen) {
		PyErr_SetString(PyExc_IndexError, "index out of bounds");
		return -1;
	}

	if (!PyArg_Parse(value, "B", &uself->data[index])) {
		return -1;
	}

	return 0;
}

static int uchararray_ass_slice(PyObject *self, int lo, int hi, PyObject *other) {
	UCharArray *uself = (UCharArray *)self;
	unsigned char *ptr;
	int t, size=hi-lo;

	if (hi == INT_MAX) hi = uself->datalen;

	if (lo < 0 || hi > uself->datalen || lo > hi) {
		PyErr_SetString(PyExc_IndexError, "index out of bounds");
		return -1;
	}

	if (!PySequence_Check(other)) {
		PyErr_SetString(PyExc_TypeError, "replacement must be a sequence");
		return -1;
	}

	if (PySequence_Size(other) != size) {
		PyErr_SetString(PyExc_IndexError, "sequence has incorrect size");
		return -1;
	}

	if ((t = PyObject_TypeCheck(other, &uchararray_type))) {
		UCharArray *uother = (UCharArray *)other;
		ptr = uother->data;
	} else {
		int i;
		ptr = malloc(size);

		for (i = 0; i < size; i++) {
			PyObject *value = PySequence_GetItem(other, i);
			if (!value) {
				free(ptr);
				return -1;
			}

			if (!PyArg_Parse(value, "B", &ptr[i])) {
				free(ptr);
				return -1;
			}
		}
	}

	memmove(uself->data+lo, ptr, size);

	if (!t) {
		free(ptr);
	}

	return 0;
}

static int uchararray_contains(PyObject *self, PyObject *value) {
	unsigned char ch;
	UCharArray *uself = (UCharArray *)self;
	unsigned char *data = uself->data;
	int len = uself->datalen, i;

	if (!PyArg_Parse(value, "B", &ch)) {
		return -1;
	}

	for (i = 0; i < len; i++) {
		if (data[i] == ch) {
			return 1;
		}
	}

	return 0;
}

static PyObject *uchararray_repr(PyObject *self) {
	UCharArray *uself = (UCharArray *)self;
	unsigned char *data = uself->data;
	size_t size = uself->datalen, i;
	char *str = malloc(size*5 + 20);
	char *strpos = str;
	PyObject *strobj;

#define STRCONST(c) strcpy(strpos, c), strpos += sizeof(c)-1

	STRCONST("<uchararray> [");
	if (size > 0) {
		strpos += sprintf(strpos, "%d", data[0]);
	}
	for (i = 1; i < size; i++) {
		strpos += sprintf(strpos, ", %d", data[i]);
	}
	STRCONST("]");

#undef STRCONST

	strobj = PyString_FromString(str);
	free(str);
	return strobj;
}

static PySequenceMethods uchararray_as_sequence = {
	uchararray_length,         /* sq_length */
	0,                         /* sq_concat */
	0,                         /* sq_repeat */
	uchararray_item,           /* sq_item */
	uchararray_slice,          /* sq_slice */
	uchararray_ass_item,       /* sq_ass_item */
	uchararray_ass_slice,      /* sq_ass_slice */
	uchararray_contains,       /* sq_contains */
};

static PyTypeObject uchararray_type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"pydega.uchararray",       /*tp_name*/
	sizeof(UCharArray),        /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	uchararray_dealloc,        /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	uchararray_repr,           /*tp_repr*/
	0,                         /*tp_as_number*/
	&uchararray_as_sequence,   /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Array of unsigned chars", /* tp_doc */
#if 0
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	pydega_methods,            /* tp_methods */
	0,                         /* tp_members */
	pydega_getset,             /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	pydega_init,               /* tp_init */
	0,                         /* tp_alloc */
	pydega_new,                /* tp_new */
#endif
};

static PyObject *uchararray_create(unsigned char *data, size_t datalen) {
	UCharArray *uchararray = (UCharArray *)uchararray_type.tp_alloc(&uchararray_type, 0);
	uchararray->data = data;
	uchararray->datalen = datalen;
	return (PyObject *)uchararray;
}

typedef struct {
	PyObject_HEAD
	PyObject *input, *ram;
} Dega;

static PyObject *pydega_hello(PyObject *self, PyObject *args) {
	printf("hello world! %d\n", 42);
	Py_RETURN_NONE;
}

#if 0
static PyObject *pydega_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	Dega *self = (Dega *)type->tp_alloc(type, 0);
	return (PyObject *)self;
}

static int pydega_init(PyObject *self, PyObject *args, PyObject *kwds) {
	Dega *dself = (Dega *)self;
	return 0;
}
#endif

static void pydega_dealloc(PyObject *self) {
	self->ob_type->tp_free(self);
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

static PyObject *pydega_frame_advance(PyObject *self, PyObject *args) {
	MastFrame();
	Py_RETURN_NONE;
}

/*
static PyObject *pydega_getattr(PyObject *self, PyObject *args) {
	char *attr;

	if (!PyArg_ParseTuple(args, "s", &attr))
		return NULL;

	printf("getattr %s\n", attr);

	Py_RETURN_NONE;
}
*/

static PyObject *getter_global_ui(PyObject *self, void *data) {
	long int data_l = *(unsigned int *)data;

	return Py_BuildValue("l", data_l);
}

static int setter_global_ui(PyObject *self, PyObject *val, void *data) {
	if (!PyArg_Parse(val, "I", (unsigned int *)data))
		return -1;
	
	return 0;
}

static PyMethodDef pydega_methods[] = {
	{ "load_rom", pydega_load_rom, METH_VARARGS, "Load a rom" },
	{ "frame_advance", pydega_frame_advance, METH_NOARGS, "Compute the next frame" },
	{ NULL, NULL, 0, NULL }
};

static PyMemberDef pydega_members[] = {
	{ "input", T_OBJECT, offsetof(Dega, input), READONLY, "Joypad input data" },
	{ "ram", T_OBJECT, offsetof(Dega, ram), READONLY, "RAM data" },
	{ NULL }
};

static PyGetSetDef pydega_getset[] = {
	{ "flags", getter_global_ui, setter_global_ui, "Emulation flags", &MastEx },
	{ NULL }
};

static PyTypeObject pydega_type = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"pydega.pydega",           /*tp_name*/
	sizeof(Dega),              /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	pydega_dealloc,            /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"Dega emulator instances", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	pydega_methods,            /* tp_methods */
	pydega_members,            /* tp_members */
	pydega_getset,             /* tp_getset */
#if 0
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	pydega_init,               /* tp_init */
	0,                         /* tp_alloc */
	pydega_new,                /* tp_new */
#endif
};

static PyObject *pydega_create(void) {
	Dega *dega = (Dega *)pydega_type.tp_alloc(&pydega_type, 0);
	dega->input = uchararray_create(MastInput, sizeof(MastInput));
	dega->ram = uchararray_create(pMastb->Ram, sizeof(pMastb->Ram));
	return (PyObject *)dega;
}

static PyMethodDef pydega_mod_methods[] = {
	{ "hello", pydega_hello, METH_NOARGS, "Say hello" },
	{ NULL, NULL, 0, NULL }
};

void MdrawCall() {}
void MvidModeChanged() {}
void MvidMovieStopped() {}

PyMODINIT_FUNC initpydega(void) {
	PyObject *mod, *dega;
	MastInit();

	if (PyType_Ready(&pydega_type) < 0)
		return;

	if (PyType_Ready(&uchararray_type) < 0)
		return;

	mod = Py_InitModule("pydega", pydega_mod_methods);
	if (mod == NULL)
		return;

	PyModule_AddIntConstant(mod, "MX_GG", MX_GG);
	PyModule_AddIntConstant(mod, "MX_PAL", MX_PAL);
	PyModule_AddIntConstant(mod, "MX_JAPAN", MX_JAPAN);
	PyModule_AddIntConstant(mod, "MX_FMCHIP", MX_FMCHIP);

	PyModule_AddIntConstant(mod, "BTN_UP", 0x01);
	PyModule_AddIntConstant(mod, "BTN_DOWN", 0x02);
	PyModule_AddIntConstant(mod, "BTN_LEFT", 0x04);
	PyModule_AddIntConstant(mod, "BTN_RIGHT", 0x08);
	PyModule_AddIntConstant(mod, "BTN_1", 0x10);
	PyModule_AddIntConstant(mod, "BTN_2", 0x20);
	PyModule_AddIntConstant(mod, "BTN_MS_START", 0x40);
	PyModule_AddIntConstant(mod, "BTN_GG_START", 0x80);

	Py_INCREF(&pydega_type);
	PyModule_AddObject(mod, "pydega", (PyObject *)&pydega_type);

	Py_INCREF(&uchararray_type);
	PyModule_AddObject(mod, "uchararray", (PyObject *)&uchararray_type);

	dega = pydega_create();
	PyModule_AddObject(mod, "dega", dega);
}

