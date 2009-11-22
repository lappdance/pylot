/***************************************************************************
 * Copyright (c) 2008-2009 University of Guelph.
 *                         All rights reserved.
 *
 * This file is part of the Pilot software package.  For license
 * information, see the LICENSE file in the top level directory of the
 * Pilot source distribution.
 **************************************************************************/

/*! \file pilot_old_api.h
 *  \brief Public header file for Pilot, old API
 *  \author John Douglas Carter
 *  \author Bill Gardner
 *
 *  Include this header *instead of* (or after) "pilot.h" in order to compile
 *  applications written with the old API, meaning versions 0.1-0.2.
 */

#ifndef PILOT_OLD_API_H
#define PILOT_OLD_API_H

// make sure new API is defined first
#include "pilot.h"

// old typedefs, new names
#define PROCESS PI_PROCESS
#define CHANNEL PI_CHANNEL
#define FUNNEL PI_BUNDLE

// old function prototypes, new names, same arguments
#define PI_Initialize PI_Configure
#define PI_Go PI_StartAll
#define PI_GetFunnelChannel PI_GetBundleChannel
#define PI_GetFunnelSize PI_GetBundleSize

// same names, different arguments; just drop the name/alias
#undef PI_CreateProcess
#define PI_CreateProcess( f, alias, index, opt_pointer ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateProcess_( f, index, opt_pointer ))

#undef PI_CreateChannel
#define PI_CreateChannel( from, to, alias ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateChannel_( from, to ))

// replaced by PI_CreateBundle
#undef PI_CreateSelector
#define PI_CreateSelector( array, size, name ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateBundle_( PI_SELECT, array, size ))

#undef PI_CreateGatherer
#define PI_CreateGatherer( array, size, name ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateBundle_( PI_GATHER, array, size ))

#undef CreateBroadcaster
#define PI_CreateBroadcaster( array, size, name ) \
	(PI_CallerFile = __FILE__, PI_CallerLine = __LINE__ , \
	PI_CreateBundle_( PI_BROADCAST, array, size ))


// different names & arguments
#define PI_GetMyName() PI_GetName(NULL)

#define PI_Finalize() PI_StopMain(0)

#endif
