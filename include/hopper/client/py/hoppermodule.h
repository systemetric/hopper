#ifndef hoppermodule_h_INCLUDED
#define hoppermodule_h_INCLUDED

#define PY_SSIZE_T_CLEAN
#include <Python.h>

// clang-format off
struct _hopper_pipe {
    PyObject_HEAD
    PyObject *name;
    PyObject *endpoint;
    PyObject *hopper;
    int fd;
    int flags;
};
// clang-format on

// These macros are horrible, but hard to replace :(
#define Hopper_Pipe_GET(field)                                                 \
    static PyObject *_hopper_pipe_get##field(struct _hopper_pipe *self,        \
                                             void *closure) {                  \
        return Py_NewRef(self->field);                                         \
    }

#define Hopper_Pipe_SETSTR(field)                                              \
    static int _hopper_pipe_set##field(struct _hopper_pipe *self,              \
                                       PyObject *value, void *closure) {       \
        if (!value) {                                                          \
            PyErr_SetString(PyExc_TypeError, "cannot delete " #field);         \
            return -1;                                                         \
        }                                                                      \
                                                                               \
        if (!PyUnicode_Check(value)) {                                         \
            PyErr_SetString(PyExc_TypeError,                                   \
                            #field " must be a Unicode string");               \
            return -1;                                                         \
        }                                                                      \
                                                                               \
        Py_SETREF(self->field, Py_NewRef(value));                              \
        return 0;                                                              \
    }

#endif // hoppermodule_h_INCLUDED
