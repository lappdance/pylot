#include"pylot.h"

int wrap_PI_Configure(char** argv) {
	int i = 0;
	while(argv[i])
		++i;
	
	return PI_Configure_(&i, &argv);
}

enum type {
	STRING = 's',
	INT = 'i',
	FLOAT = 'd',
	NONE = 'v',
	SEQUENCE = 'l',
	UNKNOWN = '?'
};

enum type typeForObject(PyObject* o) {
	if(o == Py_None)
		return NONE;
	else if(PyString_Check(o))
		return STRING;
	else if(PyInt_Check(o))
		return INT;
	else if(PyFloat_Check(o))
		return FLOAT;
	else if(PyList_Check(o) || PyTuple_Check(o))
		return SEQUENCE;
	else
		return UNKNOWN;
}

void writeArg(PI_CHANNEL* channel, PyObject* arg) {	
	enum type type = typeForObject(arg);
	
	switch(type) {
		case INT: {
			long l = PyInt_AsLong(arg);
			PI_Write(channel, "%c%ld", type, l);
			} break;
		case FLOAT: {
			double d = PyFloat_AsDouble(arg);
			PI_Write(channel, "%c%lf", type, d);
			} break;
		case NONE:
			PI_Write(channel, "%c", type);
			break;
		case STRING: {
			char* string = PyString_AsString(arg);
			size_t length = strlen(string);
			PI_Write(channel, "%c%lu%*c", type, length, length, string);
			} break;
		case SEQUENCE:
			PyErr_SetString(PyExc_TypeError, "you cannot write lists, dicts or tuples to a channel");
			break;
		default:
			PyErr_SetString(PyExc_TypeError, "unknown type in send list");
			break;
	}

}

void PI_WriteVarArgs(PI_CHANNEL* c, PyObject* args) {
	int i = 0;
	size_t numArgs = PyList_Size(args);
	
	/* @c args contains every python object provided after @c c. */
	if(numArgs < 1) {
		PyErr_SetString(PyExc_ValueError, "you must write at least one object");
	}
	
	for(i=0; i<numArgs; ++i) {
		//should recurse if arg is a sequence?
		writeArg(c, PyList_GetItem(args, i));
	}

}

PyObject* PI_ReadItem(PI_CHANNEL* c) {
	PyObject* obj = 0L;
	enum type type = 0;
	
	PI_Read(c, "%c", &type);
	
	switch(type) {
		case INT: {
			long value = 0;
			PI_Read(c, "%ld", &value);
			obj = PyInt_FromLong(value);
			} break;
		case FLOAT: {
			double value = 0.0;
			PI_Read(c, "%lf", &value);
			obj = PyFloat_FromDouble(value);
			} break;
		case STRING: {
			size_t length = 0;
			char* buffer = 0L;
			
			PI_Read(c, "%lu", &length);
			buffer = (char*)malloc((length + 1) * sizeof(char));
			buffer[length] = 0;			
			PI_Read(c, "%*c", length, &buffer[0]);
			
			obj = PyString_FromString(buffer);

			free(buffer);
			} break;
		case NONE: {
			Py_INCREF(Py_None);
			obj = Py_None;
			} break;
		default:
			PyErr_SetString(PyExc_TypeError, "unknown data type in channel");
			break;
	}
	
	return obj;
}

PyObject* PI_ReadArray(PI_CHANNEL* c, int n) {
	PyObject* list = 0L;
	int i = 0;
	
	if(n < 1) {
		PyErr_SetString(PyExc_ValueError, "you must read at least one item from a channel");
		return 0L;
	}
	
	list = PyList_New(n);
	for(i=0; i<n; ++i) {
		PyObject* item = PI_ReadItem(c);
		if(!item)
			goto abort;
		
		PyList_SetItem(list, i, item);
		Py_DECREF(item);
	}
	
	return list;
abort:
	Py_DECREF(list);
	return 0L;
}
