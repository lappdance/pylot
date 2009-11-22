/**
 @file pylot.h
 @author Jason Lapp 0652137
 @date 22 Nov 2009
 
 Defines extra interop methods used to massag Python data to C data.
**/

#ifndef PYLOT_INCLUDE_PYLOT_H
#define PYLOT_INCLUDE_PYLOT_H

#include<Python.h>
#include<pilot.h>

/**
 Wrapper func for PI_Configure. @c argv is not modified.
 @param [in] argv Command-line arguments. This array is a copy of @c sys.argv,
                  and so the original is not modified.
 @return The number of MPI processes available.
**/
int wrap_PI_Configure(char** argv);

/**
 Write a single int to a channel.
 @param [in] c The channel to write to.
 @param [in] i The int
**/
void PI_WriteInt(PI_CHANNEL* c, int i);
/**
 Write an array of ints to a channel.
 @param [in] array The array to write.
 @param [in] n The length of the array
**/
void PI_WriteIntArray(PI_CHANNEL* c, int* array, int n);
/** @overload */
void PI_WriteDouble(PI_CHANNEL* c, double d);
/** @overload */
void PI_WriteDoubleArray(PI_CHANNEL* c, double* array, int n);
/** Write a null-terminated string to a channel. */
void PI_WriteString(PI_CHANNEL* c, char* text);

/**
 Read one Python object from a channel.
 @param [in] c The channel to read from
 @return A variable type. 1 instance of int, double, or string
**/
PyObject* PI_ReadItem(PI_CHANNEL* c);
/**
 Read a list of objects from a channel. The method will block until all objects
 have been read successfully.
 @param [in] c The channel to read from.
 @param [in] n The number of objects to read.
 @return A list of ints, doubles, or strings.
**/
PyObject* PI_ReadArray(PI_CHANNEL* c, int n);

#endif //PYLOT_INCLUDE_PYLOT_H

