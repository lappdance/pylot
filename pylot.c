#include"pylot.h"

int wrap_PI_Configure(char** argv) {
	int i = 0;
	while(argv[i])
		++i;
	
	return PI_Configure_(&i, &argv);
}

void PI_WriteInt(PI_CHANNEL* c, int i) {

}

void PI_WriteIntArray(PI_CHANNEL* c, int* array, int n) {

}

void PI_WriteDouble(PI_CHANNEL* c, double d) {

}

void PI_WriteDoubleArray(PI_CHANNEL* c, double* array, int n) {

}

void PI_WriteString(PI_CHANNEL* c, char* text) {

}

PyObject* PI_ReadItem(PI_CHANNEL* c) {
	return 0L;
}

PyObject* PI_ReadArray(PI_CHANNEL* c, int n) {
	return 0L;
}
