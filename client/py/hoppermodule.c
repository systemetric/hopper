#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stddef.h>

#include "hopper/hopper.h"
#include "hoppermodule.h"

// clang-format off
Hopper_Pipe_GET(name)
Hopper_Pipe_GET(endpoint)
Hopper_Pipe_GET(hopper)
Hopper_Pipe_SETSTR(name)
Hopper_Pipe_SETSTR(endpoint)
Hopper_Pipe_SETSTR(hopper);
// clang-format on

static PyObject *hopper_pipe_new(PyTypeObject *type, PyObject *args,
                                 PyObject *kwds) {
    struct py_hopper_pipe *self =
        (struct py_hopper_pipe *)type->tp_alloc(type, 0);
    if (!self)
        return NULL;

    // Create each field with default values
    // This is __new__ in Python
    self->name = PyUnicode_FromString("");
    self->endpoint = PyUnicode_FromString("");
    self->hopper = PyUnicode_FromString("");

    if (!self->name || !self->endpoint || !self->hopper) {
        Py_XDECREF(self->name);
        Py_XDECREF(self->endpoint);
        Py_XDECREF(self->hopper);
        return NULL;
    }

    self->fd = -1;
    self->flags = 0;

    return (PyObject *)self;
}

static int hopper_pipe_init(struct py_hopper_pipe *self, PyObject *args,
                            PyObject *kwds) {
    static char *kwlist[] = {"name", "endpoint", "hopper", "fd", "flags", NULL};
    PyObject *name = NULL, *endpoint = NULL, *hopper = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|UUUii", kwlist, &name,
                                     &endpoint, &hopper, &self->fd,
                                     &self->flags))
        return -1;

    // After parsing, copy the provided value into the struct
    if (name)
        Py_SETREF(self->name, Py_NewRef(name));
    if (endpoint)
        Py_SETREF(self->endpoint, Py_NewRef(endpoint));
    if (hopper)
        Py_SETREF(self->hopper, Py_NewRef(hopper));

    return 0;
}

static void hopper_pipe_dealloc(struct py_hopper_pipe *self) {
    assert(self->name);
    assert(self->endpoint);
    assert(self->hopper);

    Py_XDECREF(self->name);
    Py_XDECREF(self->endpoint);
    Py_XDECREF(self->hopper);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *hopper_pipe_open(PyObject *self, PyObject *args) {}

static PyObject *hopper_pipe_close(PyObject *self, PyObject *args) {
    Hopper_Pipe_CONVERT(self, pipe);
    hopper_close(&pipe);
    Hopper_Pipe_UPDATE(pipe, self);
    Py_RETURN_NONE;
}

static PyObject *hopper_pipe_read(PyObject *self, PyObject *args) {}
static PyObject *hopper_pipe_write(PyObject *self, PyObject *args) {}

static PyMemberDef hopper_pipe_members[] = {
    {"fd", Py_T_INT, offsetof(struct py_hopper_pipe, fd), 0,
     "pipe file descriptor"},
    {"flags", Py_T_INT, offsetof(struct py_hopper_pipe, flags), 0,
     "pipe flags"},
    {NULL},
};

static PyGetSetDef hopper_pipe_get_set[] = {
    {"name", (getter)_hopper_pipe_getname, (setter)_hopper_pipe_setname,
     "pipe name", NULL},
    {"endpoint", (getter)_hopper_pipe_getendpoint,
     (setter)_hopper_pipe_setendpoint, "pipe endpoint", NULL},
    {"hopper", (getter)_hopper_pipe_gethopper, (setter)_hopper_pipe_sethopper,
     "pipe hopper", NULL},
    {NULL},
};

static PyMethodDef hopper_pipe_methods[] = {
    {"open", hopper_pipe_open, METH_VARARGS, ""},
    {"close", hopper_pipe_close, METH_VARARGS, ""},
    {"read", hopper_pipe_read, METH_VARARGS, ""},
    {"write", hopper_pipe_write, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL},
};

// clang-format off
static PyTypeObject hopper_pipe_type = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "hopper.HopperPipe",
    .tp_doc = PyDoc_STR("A object representing a Hopper pipe"),
    .tp_basicsize = sizeof(struct py_hopper_pipe),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = hopper_pipe_new,
    .tp_init = (initproc)hopper_pipe_init,
    .tp_dealloc = (destructor)hopper_pipe_dealloc,
    .tp_members = hopper_pipe_members,
    .tp_getset = hopper_pipe_get_set,
    .tp_methods = hopper_pipe_methods,
};
// clang-format on

static int hopper_module_exec(PyObject *m) {
    if (PyType_Ready(&hopper_pipe_type) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "HopperPipe", (PyObject *)&hopper_pipe_type) <
        0)
        return -1;

    return 0;
}

static PyModuleDef_Slot hopper_slots[] = {
    {Py_mod_exec, hopper_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL},
};

static struct PyModuleDef hoppermodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "hopper",
    .m_doc = NULL,
    .m_slots = hopper_slots,
};

PyMODINIT_FUNC PyInit_hopper(void) { return PyModuleDef_Init(&hoppermodule); }

