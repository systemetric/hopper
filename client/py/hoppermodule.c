#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "hopper/hopper.h"

// clang-format off
struct _hopper_pipe {
    PyObject_HEAD
    PyObject *name;
    PyObject *endpoint;
    PyObject *hopper;
    int fd;
    int flags;
};

static PyTypeObject _py_hopper_pipe = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "hopper.HopperPipe",
    .tp_doc = PyDoc_STR("A object representing a Hopper pipe"),
    .tp_basicsize = sizeof(struct _hopper_pipe),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};
// clang-format on

static PyObject *hopper_open_pipe(PyObject *self, PyObject *args) {}
static PyObject *hopper_close_pipe(PyObject *self, PyObject *args) {}
static PyObject *hopper_read_pipe(PyObject *self, PyObject *args) {}
static PyObject *hopper_write_pipe(PyObject *self, PyObject *args) {}

static int hopper_module_exec(PyObject *m) {
    if (PyType_Ready(&_py_hopper_pipe) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "HopperPipe", (PyObject *)&_py_hopper_pipe) <
        0)
        return -1;

    return 0;
}

static PyMethodDef hopper_methods[] = {
    {"open", hopper_open_pipe, METH_VARARGS, ""},
    {"close", hopper_close_pipe, METH_VARARGS, ""},
    {"read", hopper_read_pipe, METH_VARARGS, ""},
    {"write", hopper_write_pipe, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL},
};

static PyModuleDef_Slot hopper_slots[] = {
    {Py_mod_exec, (void *)hopper_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL},
};

static struct PyModuleDef hoppermodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "hopper",
    .m_doc = NULL,
    .m_methods = hopper_methods,
    .m_slots = hopper_slots,
};

PyMODINIT_FUNC PyInit_hopper(void) { return PyModuleDef_Init(&hoppermodule); }

