#ifndef hoppermodule_h_INCLUDED
#define hoppermodule_h_INCLUDED

#define PY_SSIZE_T_CLEAN
#include <Python.h>

// clang-format off
struct py_hopper_pipe {
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
    static PyObject *_hopper_pipe_get##field(struct py_hopper_pipe *self,      \
                                             void *closure) {                  \
        return Py_NewRef(self->field);                                         \
    }

#define Hopper_Pipe_SETSTR(field)                                              \
    static int _hopper_pipe_set##field(struct py_hopper_pipe *self,            \
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

#define Hopper_Pipe_CONVERT(in, out)                                           \
    struct py_hopper_pipe *_self = (struct py_hopper_pipe *)in;                \
                                                                               \
    const char *name = PyUnicode_AsUTF8(_self->name);                          \
    const char *endpoint = PyUnicode_AsUTF8(_self->endpoint);                  \
    const char *hopper = PyUnicode_AsUTF8(_self->hopper);                      \
                                                                               \
    struct hopper_pipe out = {                                                 \
        .name = name,                                                          \
        .endpoint = endpoint,                                                  \
        .hopper = hopper,                                                      \
        .fd = _self->fd,                                                       \
        .flags = _self->flags,                                                 \
    };

#endif // hoppermodule_h_INCLUDED
