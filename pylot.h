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

typedef int bool_type;

/**
 Put Pilot into "bench mode".
 In bench mode, Pilot can be stopped and restarted without reinitializing the
 entire program.
 @param [in] argv Commandline arguments. This array is a copy of @c sys.argv,
                  and so the original is not modified.
 @param [out] rank This process's rank
 @param [out] worldsize The total number of available processes
**/
bool_type enterBenchMode(char** argv, int* rank, int* worldsize);
/** Exit "bench mode" and return to normal behaviour. */
void exitBenchMode();

/**
 Wrapper func for PI_Configure. @c argv is not modified.
 @param [in] argv Command-line arguments. This array is a copy of @c sys.argv,
                  and so the original is not modified.
  @param [out] rank This process's rank
 @param [out] worldsize The total number of available processes
**/
void wrap_PI_Configure(char** argv, int* rank, int* worldsize);

/**
 Wrapper func to allow Python objects to be used as callbacks. SWIG won't do this
 for me automatically.
 @param [in] function Function callback. This object must be callable, and have
             a signature that matches @code def foo(index, data) @endcode.
 @param [in] index The index of the process. This value is passed to the process
             when it is spawned.
 @param [in] data User data. Anything is valid.
 @return A handle to the process. The process will not start until @c PI_StartAll
         is called.
**/
PI_PROCESS* wrap_PI_CreateProcess(PyObject* callback, int index, PyObject* data);

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
 @return @c true if the write was successful, @c false if it was not
**/
bool_type PI_WriteVarArgs(PI_CHANNEL* c, ...);

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

/**
 Broadcast arguments to multiple channels at once.
 @param [in] bundle The bundle of channels to write to
 @param [in] ... A sequence of objects to write
**/
bool_type PI_BroadcastVarArgs(PI_BUNDLE* bundle, ...);

/**
 Read from multiple channels at once.
 At the moment, only numbers and @c None can be gathered this way; strings and
 lists must be read individually from each channel.
 @param [in] bundle The bundle to read from.
 @return A 1-D list.
**/
PyObject* PI_GatherItem(PI_BUNDLE* bundle);

/**
 Read from multiple channels at once.
 At the moment, only numbers and @c None can be gathered this way; strings and
 lists must be read individually from each channel.
 Each element in the returned array matches a call to @c PI_GatherItem.
 @param [in] bundle The bundle to read from.
 @param [in] n The number of items to read from each channel
 @return A 2-D list
**/
PyObject* PI_GatherArray(PI_BUNDLE* bundle, int n);

#endif //PYLOT_INCLUDE_PYLOT_H

