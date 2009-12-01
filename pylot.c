#include"pylot.h"
#include<stdarg.h>
#include<mpi.h>

bool_type enterBenchMode(char** argv, int* rank, int* N) {
	int initialized = 0;
	
	MPI_Initialized(&initialized);
	if(!initialized) {
		int i = 0;
		while(argv[i])
			++i;
	
		MPI_Init(&i, &argv);
		MPI_Comm_rank(MPI_COMM_WORLD, rank);
		MPI_Comm_size(MPI_COMM_WORLD, N);
	} else {
		PyErr_SetString(PyExc_EnvironmentError, "you cannot try to enter bench"
			"mode more than once, or enter it after Pilot has been configured.");
	}
	
	return !initialized;
}

void exitBenchMode() {
	MPI_Finalize();
}

void wrap_PI_Configure(char** argv, int* rank, int* N) {
	int i = 0;
	while(argv[i])
		++i;
	
	*N = PI_Configure_(&i, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, rank);
}

struct Context {
	PyObject* func;
	PyObject* data;
};

int work_func(int index, void* pv) {
	struct Context* context = (struct Context*)pv;
	
	PyObject* tuple = PyTuple_Pack(2, PyInt_FromLong(index), context->data);
	if(!tuple) {
		PyErr_Print();
		return 0;
	}
	
	PyObject* result = PyObject_Call(context->func, tuple, 0L);
	if(!result)
		PyErr_Print();
	
	free(context);
	
	Py_XDECREF(result);
	Py_XDECREF(tuple);
	return 0;
}

PI_PROCESS* wrap_PI_CreateProcess(PyObject* function, int index, PyObject* data) {
	struct Context* c = 0L;
	PI_PROCESS* proc = 0L;
	
	c = malloc(sizeof(struct Context));
	c->func = function;
	c->data = data;
	
	Py_INCREF(c->func);
	Py_INCREF(c->data);
	
	proc = PI_CreateProcess_(work_func, index, c);
	
	if(!proc) {
		Py_DECREF(c->func);
		Py_DECREF(c->data);
		free(c);
	}
	
	return proc;
}

enum type {
	STRING = 's',
	INT = 'i',
	FLOAT = 'd',
	NONE = 'v',
	LIST = 'l',
	TUPLE = 't',
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
	else if(PyList_Check(o))
		return LIST;
	else if(PyTuple_Check(o))
		return TUPLE;
	else
		return UNKNOWN;
}

bool_type writeArg(PI_CHANNEL* channel, PyObject* arg) {	
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
		case LIST:
		case TUPLE: {
			int i = 0;
			size_t length = PySequence_Length(arg);
			PI_Write(channel, "%c%lu", type, length);
			
			for(i=0; i<length; ++i) {
				int success = writeArg(channel, PySequence_GetItem(arg, i));
				if(!success)
					return 0;
			}
			
			} break;
		default:
			PyErr_SetString(PyExc_TypeError, "unknown type in send list");
			return 0;
			break;
	}

	return 1;
}

bool_type PI_WriteVarArgs(PI_CHANNEL* c, ...) {
	va_list list;
	PyObject* args = 0L;
	int i = 0;
	size_t numArgs = 0;
	
	va_start(list, c);
	args = va_arg(list, PyObject*);
	va_end(list);
	
	numArgs = PyTuple_Size(args);
	
	/* @c args contains every python object provided after @c c. */
	if(numArgs < 1 || (int)numArgs == -1) {
		PyErr_SetString(PyExc_ValueError, "you must write at least one object");
		return 0;
	}
	
	for(i=0; i<numArgs; ++i) {
		int success = writeArg(c, PyTuple_GetItem(args, i));
		if(!success)
			return 0;
	}

	return 1;
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
		case LIST:
		case TUPLE: {
			size_t length = 0;
			PI_Read(c, "%lu", &length);
			
			PyObject* list = PI_ReadArray(c, (int)length);
			
			if(type == LIST)
				obj = list;
			else {
				PyObject* tuple = PyList_AsTuple(list);
				Py_XDECREF(list);
				obj = tuple;
			}
			
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
	}
	
	return list;
abort:
	Py_DECREF(list);
	return 0L;
}

bool_type broadcastArg(PI_BUNDLE* bundle, PyObject* arg) {	
	enum type type = typeForObject(arg);
	
	switch(type) {
		case INT: {
			long l = PyInt_AsLong(arg);
			PI_Broadcast(bundle, "%c%ld", type, l);
			} break;
		case FLOAT: {
			double d = PyFloat_AsDouble(arg);
			PI_Broadcast(bundle, "%c%lf", type, d);
			} break;
		case NONE:
			PI_Broadcast(bundle, "%c", type);
			break;
		case STRING: {
			char* string = PyString_AsString(arg);
			size_t length = strlen(string);
			PI_Broadcast(bundle, "%c%lu%*c", type, length, length, string);
			} break;
		case LIST:
		case TUPLE: {
			int i = 0;
			size_t length = PySequence_Length(arg);
			PI_Broadcast(bundle, "%c%lu", type, length);
			
			for(i=0; i<length; ++i) {
				int success = broadcastArg(bundle, PySequence_GetItem(arg, i));
				if(!success)
					return 0;
			}
			
			} break;
		default:
			PyErr_SetString(PyExc_TypeError, "unknown type in send list");
			return 0;
			break;
	}

	return 1;
}

bool_type PI_BroadcastVarArgs(PI_BUNDLE* bundle, ...) {
	va_list list;
	PyObject* args = 0L;
	int i = 0;
	size_t numArgs = 0;
	
	va_start(list, bundle);
	args = va_arg(list, PyObject*);
	va_end(list);
	
	numArgs = PyTuple_Size(args);
	
	/* @c args contains every python object provided after @c c. */
	if(numArgs < 1 || (int)numArgs == -1) {
		PyErr_SetString(PyExc_ValueError, "you must write at least one object");
		return 0;
	}
	
	for(i=0; i<numArgs; ++i) {
		int success = broadcastArg(bundle, PyTuple_GetItem(args, i));
		if(!success)
			return 0;
	}

	return 1;
}

PyObject* PI_GatherItem(PI_BUNDLE* bundle) {
	int i = 0;
	int bundleSize = PI_GetBundleSize(bundle);
	enum type* types = malloc(bundleSize * sizeof(enum type));
	PyObject* obj = 0L;
	
	PI_Gather(bundle, "%c", &types[0]);
	for(i=1; i<bundleSize; ++i) {
		if(types[i] != types[i-1]) {
			PyErr_SetString(PyExc_ValueError, "can't gather bundle because channels have different incoming datatypes");
			return 0L;
		}
	}
	
	obj = PyList_New(bundleSize);
	switch(types[0]) {
		case INT: {
			long* values = malloc(bundleSize * sizeof(long));
			PI_Gather(bundle, "%ld", &values[0]);
			for(i=0; i<bundleSize; ++i) {
				PyList_SetItem(obj, i, PyInt_FromLong(values[i]));
			}
			free(values);
			} break;
		case FLOAT: {
			double* values = malloc(bundleSize * sizeof(double));
			PI_Gather(bundle, "%lf", &values[0]);
			for(i=0; i<bundleSize; ++i) {
				PyList_SetItem(obj, i, PyFloat_FromDouble(values[i]));
			}
			free(values);
			} break;
		case NONE: {
			for(i=0; i<bundleSize; ++i) {
				Py_INCREF(Py_None);
				PyList_SetItem(obj, i, Py_None);
			}
			} break;
		case STRING:
		case LIST:
		case TUPLE: 
			PyErr_SetString(PyExc_TypeError, "only numbers, bools, and None may be gathered.");
			break;
		default:
			PyErr_SetString(PyExc_TypeError, "unknown data type in channel");
			break;
	}
	
	return obj;
}

