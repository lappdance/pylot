%module pylot
%include "typemaps.i"

%typemap(in) (char** argv) {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
        $1[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
        PyErr_SetString(PyExc_TypeError,"list must contain strings");
        free($1);
        return NULL;
      }
    }
    $1[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(freearg) (char** argv) {
	free($1);
}

%typemap(check) (PyObject* function) {
	if(!PyCallable_Check($1))
		SWIG_exception_fail(SWIG_ValueError, "callback function must be a functor");
}

%typemap(check) (PI_CHANNEL*) {
	if(!$1)
		SWIG_exception_fail(SWIG_ValueError, "channel cannot be null: did you forget to call it global?");
}

%typemap(in) (PyObject* varargs) {
	assert(PySequence_Check(args));
	$1 = PySequence_GetSlice(args, $argnum-1, PySequence_Length(args));
}

%typemap(freearg) (PyObject* varargs) {
	Py_XDECREF($1);
}

%typemap(out) (bool_type) {
	$result = $1 ? SWIG_Py_Void() : 0L;
}

%rename(PI_Configure_) wrap_PI_Configure;
%rename(PI_Write_) PI_WriteVarArgs;
%rename(PI_Read_) PI_ReadItem;
%rename(PI_Read_) PI_ReadArray;
#%rename(PI_Broadcast_) PI_BroadcastVarArgs;
#%rename(PI_Gather_) PI_GatherItem;
#%rename(PI_Gather_) PI_GatherArray;
%rename(PI_CreateProcess_) wrap_PI_CreateProcess;

%ignore PI_Configure_;
%ignore PI_Read_;
%ignore PI_Write_;
#%ignore PI_Broadcast_;
#%ignore PI_Gather_;
%ignore PI_CreateProcess_;

%ignore PI_MAIN;

%{
#include "pilot.h"
#include "pylot.h"
%}

%include "pilot.h"
%include "pylot.h"

%constant PI_PROCESS* mainProc = 0L;

