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
 Write items to a channel. This method does not yet support extracting items from
 lists or tuples, so each arg must be given as a seperate object.
 Thus,
 @code
 	pylot.write(channel, arg0, arg1, arg2)
 @endcode
 and not
 @code
 	l = [arg0, arg1, arg2]
 	pylot.write(l)
 @endcode
 .
 @param [in] c The channel to write to.
 @param [in] varargs A sequence of Python objects.
**/
void PI_WriteVarArgs(PI_CHANNEL* c, PyObject* varargs);

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

void PI_BroadcastVarArgs(PI_BUNDLE* bundle, PyObject* varargs);

#endif //PYLOT_INCLUDE_PYLOT_H

