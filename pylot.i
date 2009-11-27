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

%typemap(out) (bool_type) {
	$result = $1 ? SWIG_Py_Void() : 0L;
}

//for some reason, SWIG won't break the varargs apart properly
//it leaves the first arg in @c args instead of putting it in @c varargs
//so i need to construct my own @c varargs variable
%typemap(in) (...) {
	PyObject* firstArg = PyTuple_Pack(1, PySequence_GetItem(args, $argnum-1));
	$1 = PySequence_Concat(firstArg, varargs);
}

%typemap(freearg) (...) {
	Py_XDECREF((PyObject*)$1);
}

%rename(PI_Configure_) wrap_PI_Configure;
%rename(PI_Write_) PI_WriteVarArgs;
%rename(PI_Read_) PI_ReadItem;
%rename(PI_Read_) PI_ReadArray;
//%rename(PI_Broadcast_) PI_BroadcastVarArgs;
//%rename(PI_Gather_) PI_GatherItem;
//%rename(PI_Gather_) PI_GatherArray;
%rename(PI_CreateProcess_) wrap_PI_CreateProcess;

%ignore PI_Configure_;
%ignore PI_Read_;
%ignore PI_Write_;
//%ignore PI_Broadcast_;
//%ignore PI_Gather_;
%ignore PI_CreateProcess_;

%{
#include "pilot.h"
#include "pylot.h"
%}

%include "pilot.h"
%include "pylot.h"

%constant PI_PROCESS* mainProc = 0L;

