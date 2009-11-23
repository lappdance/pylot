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

%typemap(in) (int* array, int n) {
	/* Check if is a list */
  if (PyList_Check($input)) {
    $2 = PyList_Size($input);
    int i = 0;
    $1 = (int*) malloc(($2)*sizeof(int));
    for (i = 0; i < $2; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyInt_Check(o))
        $1[i] = PyInt_AsLong(PyList_GetItem($input,i));
      else {
        PyErr_SetString(PyExc_TypeError,"list must contain ints");
        free($1);
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(in) (double* array, int n) {
	/* Check if is a list */
  if (PyList_Check($input)) {
    $2 = PyList_Size($input);
    int i = 0;
    $1 = (double*) malloc(($2)*sizeof(double));
    for (i = 0; i < $2; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyFloat_Check(o))
        $1[i] = PyFloat_AsDouble(PyList_GetItem($input,i));
      else {
        PyErr_SetString(PyExc_TypeError,"list must contain floats");
        free($1);
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(freearg) (double* array, int n) {
	free($1);
}

%typemap(freearg) (int* array, int n) {
	free($1);
}

%rename(PI_Configure_) wrap_PI_Configure;
%rename(PI_Write_) PI_WriteVarArgs;
%rename(PI_Read_) PI_ReadItem;
%rename(PI_Read_) PI_ReadArray;
%rename(PI_Broadcast_) PI_BroadcastVarArgs;
%rename(PI_Gather_) PI_GatherItem;
%rename(PI_Gather_) PI_GatherArray;

%ignore PI_Configure_;
%ignore PI_Read_;
%ignore PI_Write_;
%ignore PI_Broadcast_;
%ignore PI_Gather_;

%{
#include "pilot.h"
#include "pylot.h"
%}

%include "pilot.h"
%include "pylot.h"

