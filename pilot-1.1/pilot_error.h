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
\file pilot_error.h
\brief Error codes and error messages for Pilot.

This file can be included in any .c file, but in the one where it is
desired to define the error text storage, \#define DEF_ERROR_TEXT before
including it.
*******************************************************************************/

#ifndef PILOT_ERROR_H
#define PILOT_ERROR_H


/*! Error codes defined as enumerated values */
enum {
PI_NO_ERROR,            // 0
PI_LOG_OPEN,
PI_MALLOC_ERROR,
PI_BUNDLE_INDEX,
PI_NULL_CHANNEL,

PI_ENDPOINT_WRITER,     // 5
PI_ENDPOINT_READER,
PI_NULL_FUNCTION,
PI_ENDPOINT_DUPLICATE,
PI_INSUFFICIENT_MPIPROCS,

PI_BUNDLED_CHANNEL,        // 10
PI_ZERO_MEMBERS,
PI_FORMAT_ARGS,
PI_WRONG_PHASE,
PI_NULL_FORMAT,

PI_BUNDLE_READEND,    	// 15
PI_NULL_BUNDLE,
PI_BUNDLE_DUPLICATE,
PI_BUNDLE_USAGE,
PI_BUNDLE_WRITEEND,

PI_SYSTEM_ERROR,        // 20
PI_MAX_TAGS,
PI_THREAD_SUPPORT,
PI_START_THREAD,
PI_DEADLOCK,

PI_INVALID_OBJ		// 25
};

/*! First defined error code. */
#define PI_MIN_ERROR 1

/*! Last defined error code. */
#define PI_MAX_ERROR PI_INVALID_OBJ

/*!
********************************************************************************
\brief Array of error strings.
Error text at position [x] is for error x.
*******************************************************************************/
#ifdef DEF_ERROR_TEXT
static char *ErrorText[] = {
    "No Error Occured",
    "Cannot open log file",
    "Malloc() Failed -- Insufficient Memory",
    "Bundle index is out of range",
    "Channel pointer is NULL",

    "Process does not have write access to channel or bundle",
    "Process does not have read access to channel or bundle",
    "Function pointer is NULL",
    "Channel cannot have the same from/to endpoints",
    "Not enough MPI processes to allocate a Pilot process",

    "Operation not allowed on common end of bundled channel",
    "Creating bundle with zero member channels",
    "Format doesn't match type or number of arguments",
    "Function called during wrong phase of execution",
    "Format argument is NULL",

    "Channels in this type bundle do not have a single read end",
    "Bundle pointer is NULL",
    "Bundle contains channels from the same process",
    "Bundle does not allow this usage",
    "Channels in this type bundle do not have a single write end",

    "System error, check library code for reason",
    "MPI tags exhausted; cannot create channel",
    "This MPI does not support the thread needed for file logging.\n"
        "Try again with -pilog=p.",
    "Cannot start online thread",
    "Program is deadlocked",

    "Object is not a valid process, channel, or bundle"
};
#endif

#endif
