/***************************************************************************
 * Copyright (c) 2008-2009 University of Guelph.
 *                         All rights reserved.
 *
 * This file is part of the Pilot software package.  For license
 * information, see the LICENSE file in the top level directory of the
 * Pilot source distribution.
 **************************************************************************/

/*!
********************************************************************************
\file pilot.h
\brief Public header file for Pilot
\author John Douglas Carter
\author Bill Gardner

Provides prototypes for all functions used by Pilot application programmers.
*******************************************************************************/

#ifndef PILOT_H
#define PILOT_H

#ifndef PI_NO_OPAQUE
/*!
********************************************************************************
Function prototype for user defined worker functions.

Functions to be assigned to processes must match this prototype.
The work function's return value will be output in the log, but has no
other operational significance.
The two arguments are supplied via PI_CreateProcess.
*******************************************************************************/
typedef int(*PI_WORK_FUNC)(int,void*);

/* Opaque datatypes for function declarations below */
typedef struct OPAQUE PI_PROCESS;
typedef struct OPAQUE PI_CHANNEL;
typedef struct OPAQUE PI_BUNDLE;
#endif

#include "pilot_limits.h"

// define NULL
#include <stddef.h>


/*** Pilot global variables ***/

/*!
********************************************************************************
Specifies whether Pilot will print a banner and configuration info on stdout
when it starts up.
 - 0 = normal print
 - non-0 = quiet
Error messages will print on stderr regardless.
*******************************************************************************/
extern int PI_QuietMode;

/*!
********************************************************************************
Specifies level of error checking to be done by library functions.  The variable
may be set directly by the programmer before calling PI_Configure, or indirectly
by means of command line options (future).  Normal value is 0, causing default error
checking to be done.  If set to N, additional checks up to and including level N
will be done at the expense of performance degradation.

Level 0:
 - validates many function preconditions (detects user errors)

Level 1:
 - validates internal tables (detects system errors, could be user-caused)
 - more time-consuming checks (read/write formats match arg list)
 - (future) check all returns from MPI function calls (default behaviour
   gives no info on location of failed call within library or which func
   user called); MPI errors would likely be Pilot system errors

Level 2: (future)
 - check that read args are likely pointers (&arg) not data (arg)
 - send/check headers with every write/read to verify that no. and types of
   data match (adds message traffic up to double)
*******************************************************************************/
extern int PI_CheckLevel;

/*!
********************************************************************************
Specifies whether or not an error detected by a library function should abort the
program.  The variable must be set directly by the programmer, and should only be used
by Pilot developers in order to test error-checking mechanisms.  It is not for use by
Pilot application programmers.  Normal value is 0, causing any error to abort the
program via MPI_Abort.  If set to non-zero, library functions will return after setting
PI_Errno.
*******************************************************************************/
extern int PI_OnErrorReturn;

/*!
********************************************************************************
The last error encountered by the library.

If PI_OnErrorReturn is non-zero and an error was detected by a library
function, then after the function returns, an error code > 0 (from pilate_error.h) will
be here, and further calls to library functions will be undefined.  If no error
occurred, this variable will be zero (=PI_NO_ERROR).
*******************************************************************************/
extern int PI_Errno;

/*! Filename of current library caller. */
extern const char *PI_CallerFile;

/*! Line number of current library caller. */
extern int PI_CallerLine;


/*!
********************************************************************************
\def PI_MAIN
\brief Alias for the 'master' process, whose behavior is found in main().
*******************************************************************************/
#define PI_MAIN 0


/*** functions to call in configuration phase ***/

/*!
********************************************************************************
Initializes the Pilot library.

Must be called before any other Pilot calls, passing argc,argv from main().
This function removes Pilot arguments from argc/argv, and the MPI implementation
may do likewise for its own arguments.  Therefore, PI_Configure() can be called
before the application looks for its own arguments.

Optional command line arguments (all start with "-pi"):

- -picheck=\<level number\>
  - 0: minimum level (default)
  - 1: additional checks
  - 2: reserved for future use

- -pisvc=\<runtime services\>
  - c: make log of API calls
  - d: perform deadlock detection (uses one additional MPI process)

- -pilog=\<filename\>

\c -picheck overrides any programmer setting of the PI_CheckLevel global variable
made prior to calling \c PI_Configure(). Level N includes all levels below it.

\c -pisvc only causes relevant data to be dumped to the log file. Another program
is needed to analyze and print/visualize the results. Other services are planned
for future versions.

\c -pilog allows the name of the log file to be changed from the default "pilot.log"

\note Only specifying -pilog=fname does not by itself create a log. Some
logging service (presently only "c") must also be selected.

\param argc Number of arguments, argument to main.
\param argv Array of strings, containing arguments.

\return The number of MPI processes available for Pilot process creation. This
number is a total that includes main (which is running and does not need to be
explicitly created) and deadlock detection (if selected).  If N is returned,
then PI_CreateProcess can be called at most N-1 times.

\pre argc/argv are unmodified.
\post MPI and Pilot are initialized. Pilot calls can be made. Pilot
(and possibly MPI) arguments have been removed from argc/argv by shuffling
up the argv array and decrementing argc.

\note Must be called in your program's main().
\note For special purposes like running benchmarks, Pilot can be put into
"bench mode" by calling MPI_Init prior to PI_Configure.
*******************************************************************************/
int PI_Configure_( int *argc, char ***argv );
#define PI_Configure( argc, argv ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_Configure_( argc, argv ))

/*!
********************************************************************************
Creates a new process.

Assigns a function pointer as behavior to the new process, and returns a
process pointer. Its default name will be "Pn" where n is its integer process ID.

\param f Pointer to the function this process 'runs'.
\param index An integer used for configuring the work function.
\param opt_pointer A pointer which can be used to supply data to the work function.

\return A pointer to the process created, or NULL if an error occured.

\pre PI_Configure() has been called.
\post Process has been created, stored in process table.
*******************************************************************************/
PI_PROCESS *PI_CreateProcess_( PI_WORK_FUNC f, int index, void *opt_pointer );
#define PI_CreateProcess( f, index, opt_pointer ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateProcess_( f, index, opt_pointer ))

/*!
********************************************************************************
Creates a new channel between the specified processes.

Opens a channel between the specified processes, with the specified alias.
Returns a channel pointer.  Its default name will be "Cn:Pf>Pt" where n is its
integer channel ID, and f and t are the from/to process IDs, respectively.
Multiple channels can exist between the same processes.

\param from A pointer to the 'write-end' of the channel.
\param to A pointer to the 'read-end' of the channel.

\return A pointer to the newly created channel, or NULL if an error occured.

\pre None
\post Channel created, entered into channel table.

\note If PI_MAIN/NULL is specified for either process, it represents the
master/main process (rank 0).
*******************************************************************************/
PI_CHANNEL *PI_CreateChannel_( PI_PROCESS *from, PI_PROCESS *to );
#define PI_CreateChannel( from, to ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateChannel_( from, to ))

/*!
********************************************************************************
Specifies which type of bundle to create.
\see PI_CreateBundle
*******************************************************************************/
enum PI_BUNUSE { PI_BROADCAST, PI_GATHER, PI_SELECT };

/*!
********************************************************************************
Creates a channel grouping (bundle) for a particular collective use.

Creates a bundle of channels, such that all the channels must have one
endpoint in common.  Its default name will be "Bn@Pc" where n is its integer
bundle ID, and e is the process ID of the common endpoint.  Returns a bundle
pointer.

\param usage Symbol denoting how the bundle will be used. If PI_Broadcast
 will be called, then code PI_BROADCAST.
\param array Channels to store in selector.
\param size Number of channels in array.

\return Returns the newly created bundle.

\post All channels in array are marked as being in this bundle.

\note The non-common end of all channels must be unique.
\note A channel can be in only one bundle.
*******************************************************************************/
PI_BUNDLE *PI_CreateBundle_( enum PI_BUNUSE usage, PI_CHANNEL *const array[], int size );
#define PI_CreateBundle( usage, array, size ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateBundle_( usage, array, size ))

/*!
********************************************************************************
Specifies which direction the channels should point after a copy operation.
\see PI_CopyChannels
*******************************************************************************/
enum PI_COPYDIR { PI_SAME, PI_REVERSE };

/*!
********************************************************************************
Copies an array of channels.

Given an array of channel pointers, this function duplicates the channels (i.e.,
for each CHANNEL* in the array, issues PI_CreateChannel on the same pair of
endpoints) and returns their pointers in a same-size array. The order of channel
pointers in the input and output arrays will be the same. If specified, the
endpoints will be reversed (i.e., each channel from P to Q will be copied as a
channel from Q to P). This function makes it convenient to create multiple
channels between, say, a master process and a set of workers, where one set is
for PI_Write usage, one set for PI_Broadcast, and another for PI_Select/Read or
PI_Gather.

\param direction Symbol denoting the direction of the copy. PI_SAME will
preserve the current endpoints; PI_REVERSE will flip the direction.
\param array Channels to be copied.
\param size Number of channels in array.

\return Returns a pointer to the new array of CHANNEL*, same dimension as
the size input.

\post 'size' new channels were created, each having its default name.

\note This function does not copy a bundle. You can copy the array used to
create a bundle, and then create a new bundle -- of same or different usage
-- from the function's output.
\note The main program should call free() on the returned array, since it is
obtained via malloc.
*******************************************************************************/
PI_CHANNEL **PI_CopyChannels_( enum PI_COPYDIR direction, PI_CHANNEL *const array[], int size );
#define PI_CopyChannels( direction, array, size ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CopyChannels_( direction, array, size ))

/*!
********************************************************************************
Set the friendly name of a process, channel, or bundle.

When created, each object has a default name. If the user wishes to change
them, say for log or error message readability, this function is used.

\param object The PROCESS*, CHANNEL*, or BUNDLE* whose name is to be set.
\param name Friendly name for object. A copy is made of this string up to
PI_MAX_NAMELEN characters. If NULL is supplied, the name is set to "".
*******************************************************************************/
void PI_SetName_( void *object, const char *name );
#define PI_SetName( object, name ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_SetName_( object, name ))

/*!
********************************************************************************
Kicks off parallel processing.

All processes call their assigned functions, and the primary process continues as main.
Other processes DO NOT RETURN from PI_StartAll (unless in "bench mode", see below).

\return MPI rank of this process.  Normally this is of no interest because
only the main process (rank 0) returns.  But if Pilot has been put into
"bench mode" by calling MPI_Init in advance, then all processes will return.
In that case, the user will want to check this value.

\pre PI_Configure must have been called.
\post Parallel execution has begun.

\warning No channels, processes, bundles or selectors may be created once
this has been called.
*******************************************************************************/
int PI_StartAll_(void);
#define PI_StartAll() \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_StartAll_())

/*** functions to call after PI_StartAll ***/

/*!
********************************************************************************
Returns the friendly name of a process, channel, or bundle.

Returns a string containing the friendly name of the given object, or that of
the caller's process if given NULL. This is the name set by the last call to
PI_SetName(), or its default name if none was ever set.

\param object The PROCESS*, CHANNEL*, or BUNDLE* whose name is to be
returned, or NULL to indicate the caller's process.
\return String containing the name of the given object (will not be NULL).
*******************************************************************************/
const char *PI_GetName_( const void *object );
#define PI_GetName( object ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_GetName_( object ))

/*!
********************************************************************************
Writes a number of values to the specified channel.

The format string specifies the types of each variable.  Uses control codes
similar to stdio.h's scanf:
- %d or %i - for integer
- %ld or %li - for long int
- %lld or %lli - for long long int
- %u - for unsigned int
- %lu - for unsigned long
- %llu - for unsigned long long
- %hd or %hi - for short
- %hu - for unsigned short
- %c - for character (printable, may get code-converted)
- %hhu - for unsigned character
- %b - for byte (uninterpreted 8 bits, i.e., unsigned char)
- %f - for float
- %lf - for double
- %Lf - for long double
Arrays are specified by inserting the size between % and the type, e.g.,
%25d = int[25].  If the size is specified as "*", it is obtained from the
next argument, e.g., ("%*d", 25, intarray).

\param c Channel to write to.
\param format Format string specifying the type of each variable.

\pre Channel must be open.
\post Channel now contains the variables written to it.

\note %1 (one) is not allowed since it looks too similar to %l (ell).
\note Variables after the format string are passed by value, except for arrays,
following C's normal argument-passing practice.
\note Format strings on read/write ends must match!
*******************************************************************************/
void PI_Write_( PI_CHANNEL *c, const char *format, ... );
#define PI_Write( c, format, ... ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_Write_( c, format, __VA_ARGS__, PI_END1, PI_END2 ))

/*!
********************************************************************************
Reads a number of values from the specified channel.

The format string specifies the types of each variable (see PI_Write).

\param c Channel to read from.
\param format Format string specifying the type of each variable.

\pre Channel must be open, should contain variables to read.
\post Channel no longer contains variables read from it.

\note Variables after the format string must be given by reference (&arg),
not by value, just as with scanf.
\note Format strings on read/write ends must match!
*******************************************************************************/
void PI_Read_( PI_CHANNEL *c, const char *format, ... );
#define PI_Read( c, format, ... ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_Read_( c, format, __VA_ARGS__, PI_END1, PI_END2 ))

/*!
********************************************************************************
Returns the index of a channel in the bundle that has data to read.

\param b Bundle to select from.
\return Index of Channel to be read.

\pre Bundle has been created.
\post The channel selected is the next to be read from those in the bundle.

\see PI_GetBundleChannel
*******************************************************************************/
int PI_Select_( PI_BUNDLE *b );
#define PI_Select( b ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_Select_( b ))

/*!
********************************************************************************
Indicates whether the specified channel can be read.

Can be used to test whether or not a read operation would block.
\param c Channel to test for a queued read.
\retval 1 if Channel has a queued read.
\retval 0 if Channel does not have a queued read.
\pre Channel \p c has been created.
*******************************************************************************/
int PI_ChannelHasData_( PI_CHANNEL *c );
#define PI_ChannelHasData( c ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_ChannelHasData_( c ))

/*!
********************************************************************************
Indicates whether any of the Selector's channels can be read.

Same as PI_Select, except that if none of the Selector's channels is ready
to be read (i.e., PI_ChannelHasData would return 0 for every channel), this
function returns -1.

\param b Selector bundle to test for a queued read.
\return Index of Channel to be read, or -1 if no channel has data.

\pre Selector \p b has been created.
*******************************************************************************/
int PI_TrySelect_( PI_BUNDLE *b );
#define PI_TrySelect( b ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_TrySelect_( b ))

/*!
********************************************************************************
Returns the specified channel from a bundle.

Given an index, returns that channel from the bundle.

\param b Bundle containing the desired channel.
\param index index of the channel to return.
\return Requested channel, or NULL if index was invalid.

\pre Bundle \p b has been created.
*******************************************************************************/
PI_CHANNEL* PI_GetBundleChannel_( const PI_BUNDLE *b, int index );
#define PI_GetBundleChannel( b, index ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_GetBundleChannel_( b, index ))

/*!
********************************************************************************
Returns the size of a bundle.

Provides the number of channels in the specified bundle.

\param b Bundle to return the size for.
\return Number of channels in this bundle.

\pre Bundle \p b has been created.
*******************************************************************************/
int PI_GetBundleSize_( const PI_BUNDLE *b );
#define PI_GetBundleSize( b ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_GetBundleSize_( b ))

/*!
********************************************************************************
Writes to all channels in the specified bundle.

Simultaneously writes the format string and values to every channel
contained in the bundle.

\param b Broadcaster bundle to write to.
\param format Format string and values to write to the bundle.
\pre Bundle must be a broadcaster bundle.
*******************************************************************************/
void PI_Broadcast_( PI_BUNDLE *b, const char *format, ... );
#define PI_Broadcast( f, format, ... ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_Broadcast_( f, format, __VA_ARGS__, PI_END1, PI_END2 ))

/*!
********************************************************************************
Read from all channels in the specified bundle.

Simultaneously reads the format string and values from every channel
contained in the bundle.

\param b Broadcaster bundle to read from.
\param format Format string and values to write to the bundle.
\pre Bundle must be a gatherer bundle.
*******************************************************************************/
void PI_Gather_( PI_BUNDLE *b, const char *format, ... );
#define PI_Gather( f, format, ... ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_Gather_( f, format, __VA_ARGS__, PI_END1, PI_END2 ))

/*!
********************************************************************************
Starts an internal timer.  Creates a fixed point in time -- the time between
now and another point in time is reported by PI_EndTime().
*******************************************************************************/
void PI_StartTime(void);

/*!
********************************************************************************
Gives the time elapsed since the previous fixed point.

\return The wall-clock time elapsed in seconds sinced PI_StartTime() was called.
*******************************************************************************/
double PI_EndTime(void);

/*!
********************************************************************************
Logs a time-stamped event to the log file.

Allow a user program to make entries in the log file.  If logging to file is
not enabled, the call is a no-op.  The user may wish to check PI_IsLogging()
first.  Each entry will record a time stamp and caller's process number.
*******************************************************************************/
void PI_Log_( const char *text );
#define PI_Log( text ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_Log_( text ))

/*!
********************************************************************************
Returns true if logging to a file is enabled.
*******************************************************************************/
int PI_IsLogging(void);

/*!
********************************************************************************
Aborts execution of Pilot application.

Normally called by library functions when error detected, but can be
called by user program that wants to exit abruptly.  Prints message:

<tt>*** PI_Abort *** (MPI process \#N) Pilot process 'name'(arg), \<file\>:\<line\>:
\<errmsg\>\<text\></tt>

where errmsg is derived from errcode if it is in range for Pilot codes, otherwise
"" (e.g., errcode 0 will suppress errmsg).

\param errcode The Pilot error code or 0 (zero).
\param text Extra message to display with the error message.
\param file Should be filled in with \c __FILE__ .
\param line Should be filled in with \c __LINE__ .

\post The entire application is aborted.
*******************************************************************************/
void PI_Abort( const int errcode, const char *text, const char *file, const int line );

/*!
********************************************************************************
Performs clean up for the Pilot library.

Finalizes the underlying MPI library, de-allocates all internal structures.

\param status If logging, value will appear in log, otherwise has no effect.

\pre PI_StartAll was called.
\post No additional Pilot calls may be made after function returns.

\warning Should be called only once, at the end of your program's main().

\note PI_StopMain_ is also called internally by PI_StartAll when each work
function returns, in which case it calls exit().
\note Need to say something about "bench mode."
*******************************************************************************/
void PI_StopMain_( int status );
#define PI_StopMain( status ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_StopMain_( status ))

#endif
