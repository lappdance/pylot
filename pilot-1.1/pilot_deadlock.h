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
\file pilot_deadlock.h
\brief Public header file for Pilot deadlock detector.
\author Bill Gardner
*******************************************************************************/
#ifndef PILOT_DEADLOCK_H
#define PILOT_DEADLOCK_H

#include "pilot_private.h"	// include these typedefs first

#define PI_NO_OPAQUE		// suppress typedefs in public pilot.h
#include "pilot.h"


/* e-> the environment of OnlineProcessFunc, which has the same tables as
   all processes
*/
void PI_DetectDL_start_( const PI_PROCENVT *e );

void PI_DetectDL_event_( const char *event );

void PI_DetectDL_end_( );

#endif
