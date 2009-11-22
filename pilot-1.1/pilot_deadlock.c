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
\file pilot_deadlock.c
\brief Implementation file for Pilot deadlock detector.
\author Bill Gardner

This online process is invoked by OnlineProcessFunc in 3 phases:
 -# start: called to allow for setup
 -# event: called every time a log event is received
 -# end: called after all user processes have terminated

Uses PI_OLP_ASSERT to check for malloc failures.
*******************************************************************************/

#include "pilot_deadlock.h"

#include "pilot_error.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// comment out to activate print statements for debugging
#define printf(...) ;

/*! Environment of OnlineProcessFunc. */
static const PI_PROCENVT *olpe;


/*!
********************************************************************************
event queue "EQ"

This is needed because events may arrive out of order, globally speaking.
If P sends Q 3 messages, those 3 will arrive at Q in order.  But if P, then
Q, and finally R, send messages to S, they may not arrive in that order.
The practical impact is that the deadlock detector can get events from a
process that should apparently be blocked.  It has been unblocked, but the
message about the latter event has not yet arrived.  Such events need to be
queued, and handled when their processes have been unblocked.

saveEvent: This string needs to be freed at some point.  When the event is
handled by calling makeDepend(), it may be transferred to process[i].lastEvent
(in which case saveEvent will get NULL, and a subsequent makeDepend() that
unblocks process[i] will free lastEvent).  Otherwise, handle() will free it.
*******************************************************************************/
typedef struct EQevent_t {
    struct EQevent_t *next;	/*!< -> next event in queue (NULL => end of queue) */
    int proc;		/*!< ID of this event's process (-1 => handled) */
    char *event;	/*!< copy of event (modified by strtok_r) */
    char *strtokLast;	/*!< so strtok_r can continue parsing event */
    const char *saveEvent;	/*!< another copy for deadlock traceback */
    char ecode[5];	/*!< 1-byte event type + 3-byte event code + NUL */
} EQevent;

static EQevent EQdummy = { .next=NULL },   /*!< dummy first event (helps pointer juggling) */
	*EQhead = &EQdummy,	/*!< head pointer doesn't change */
	*EQtail = &EQdummy,	/*!< head==tail => empty queue */
	*EQthis = &EQdummy;	/*!< for use in scanning queue */


/*!
********************************************************************************
Dependency matrix: [p][q] records p's dependency on q:
  -  0	no dependency
  - +1	p wrote to q, awaiting q's read
  - -1	p read from q, awaiting q's write
  - -2	p selected on q (in selector bundle), awaiting some write
*******************************************************************************/
static signed char *depends;
#define DEPENDS(p,q) depends[p*olpe->allocated_processes + q]


/*!
********************************************************************************
Process state array-of-struct, indexed by process ID (up to worldsize).

When an event is handled which implies the process will block, its state
is updated here and the event string is saved.  State 0 = RUN so that
calloc'ing the array results in initial RUN state.  Blocking increments the
state, unblocking decrements, since collective operations (including select)
block on multiple processes.
*******************************************************************************/
static struct {
    enum {DEAD=-1, RUN=0} state; /*!< process state; >0 = blocked */
    const char *lastEvent;	/*!< relevant if process blocked */
} *process;


/*!
********************************************************************************
Channel used-by-process array, indexed by channel ID (up to allocated_channels).

When a dependency is added from process P to Q via channel C, we record that
P is using channel C.  Later, when the dependency from Q to P via channel X
is added, we lookup chanproc[X] to confirm that P is using that channel.  If
not, it means that P and Q are deadlocked due to using different channels.
*******************************************************************************/
static int *chanproc;   /*!< -1 = channel not in use. */


/*!
********************************************************************************
Recognized event codes, made up of event type (1st byte) + 3-byte event,
stored in a single string.  Order of codes matches switch cases in handle().
*******************************************************************************/
static char eventCodes[] = {
	// CALLS events
	"CWri" "CRea" "CSel" "CHas" "CTry" "CBro" "CGat"
	// PILOT events
	"PFIN" };

/******** Event queue functions **********/

/*!
********************************************************************************
Reset prior to EQevent calls
*******************************************************************************/
static void EQreset()
{
    EQthis = EQhead;
}

/*!
********************************************************************************
Return next event, advance "this" pointer
*******************************************************************************/
static EQevent *EQnext()
{
    // shouldn't be calling after EQnext returns NULL
    if ( EQthis == NULL ) PI_OLP_ASSERT( 0, PI_SYSTEM_ERROR )

    return (EQthis = EQthis->next);
}

/*!
********************************************************************************
Copy event, generic parse, append
*******************************************************************************/
static void EQappend( const char *evt )
{
    EQevent *ep = malloc( sizeof(EQevent) );
    PI_OLP_ASSERT( ep, PI_MALLOC_ERROR );

    EQtail->next = ep;		// enqueue
    EQtail = ep;
    ep->next = NULL;

    ep->event = strdup( evt );	// make copy that strtok can modify
    PI_OLP_ASSERT( ep->event, PI_MALLOC_ERROR );
    ep->saveEvent = strdup( evt );	// make another copy for use in traceback
    PI_OLP_ASSERT( ep->event, PI_MALLOC_ERROR );
    ep->proc = -1;		// initialize process ID to invalid no.

    // first token should be event type
    const char* tok = strtok_r( ep->event, PI_LOGSEP, &ep->strtokLast );
    PI_OLP_ASSERT( tok, PI_SYSTEM_ERROR )
    ep->ecode[0] = tok[0];	// store event type

    // next token should be process ID
    tok = strtok_r( NULL, PI_LOGSEP, &ep->strtokLast );
    PI_OLP_ASSERT( tok, PI_SYSTEM_ERROR )
    ep->proc = atoi( tok );
    PI_OLP_ASSERT( ep->proc >= 0, PI_SYSTEM_ERROR )

    // next token should be 3-char code
    tok = strtok_r( NULL, PI_LOGSEP, &ep->strtokLast );
    PI_OLP_ASSERT( tok, PI_SYSTEM_ERROR )
    strncpy( &ep->ecode[1], tok, sizeof(ep->ecode)-1 );	// append to event type

    // further event-specific parsing happens when event is handled
printf( "$DL$ after append { " );
for( ep=EQhead->next; ep; ep=ep->next)
  printf( "%d:%4s ", ep->proc, ep->ecode );
printf( "}\n" );
}

/*!
********************************************************************************
Delete handled events, return number remaining in queue
*******************************************************************************/
static int EQcompact()
{
    int active = 0;
    EQevent *tmp;
    EQthis = EQhead;

    while( EQthis->next ) {
	if ( (tmp = EQthis->next)->proc < 0 ) {	// this one's inactive
	    EQthis->next = tmp->next;
	    if ( tmp == EQtail ) EQtail = EQthis;	// removing tail
	    free(tmp);
	}
	else {
	    EQthis = tmp;
	    active++;
	}
    }
printf( "$DL$ after compact { " );
for( tmp=EQhead->next; tmp; tmp=tmp->next)
  printf( "%d:%4s ", tmp->proc, tmp->ecode );
printf( "}\n" );
    return active;
}


/******** Reporting deadlock and aborting *********/

void abortDL( int procID, const char* event, const char *reason )
{
    fprintf( stderr,
        "\n\n*** Deadlock detected from Pilot process '%s'(%d); refer: %s\n"
        "*** Reason: %s\n",
                olpe->processes[procID].name,
                olpe->processes[procID].argument,
                event,
                reason );
    PI_OLP_ASSERT( 0, PI_DEADLOCK )
}


/******** Dependency matrix functions **********/

/*!
********************************************************************************
Check for cycle in dependency matrix from p to q, and if detected, print
traceback of involved events if print arg is true (non-0). Returns true/false.

\note Recursive search!
*******************************************************************************/
static int isCycle( int p, int q, int print )
{
    if ( p == q ) return 1;             // trivially true (base case)

    // loop through p's row looking for dependencies
    int r,
	firstDep = -1;	// initialize to no deps found

    for ( r=0; r<olpe->allocated_processes; r++ ) {
        switch( DEPENDS(p,r) ) {
	    case 0:       	// p not waiting on r
		continue;	// loop through rest of row

	    case +1:		// read/write dependency
	    case -1:
		firstDep = r;	// where to start checking cycles
		break;		// break out of switch & loop

	    /* select dependency:
		If we find ANY producer process r that's running, it
		means that process p is not necessarily part of a
		circular wait, because there is potentially a way
		to satisfy the select.  In that case, just return 0.
		But if we drop out of the loop, it means that ALL
		parties to the select are blocked (or dead) and must
		therefore be investigated for cycles, starting from
		the first party found.
	    */
	    case -2:
		if ( process[r].state == RUN ) {
printf( "$DL$ +++ Select by P%d, P%d is running\n", p, r );
		    return 0;
		}
		if ( firstDep < 0 ) firstDep = r;
		continue;	// loop through rest of row

	    default:		// bogus value in dependency matrix
		PI_OLP_ASSERT( 0, PI_SYSTEM_ERROR )
	}
	break;	// this makes case +1/-1's break also break out of loop
    }

    if ( firstDep < 0 ) return 0;	// found no dependencies

    // loop through row again, starting from firstDep, now checking cycles
    for ( r=firstDep; r<olpe->allocated_processes; r++ ) {
	int thisDep = DEPENDS(p,r);
        if ( thisDep == 0 ) continue;       // p not waiting on r

        // p is waiting on r, so see if cycle from r to q
	DEPENDS(p,r) = 0;		// zero this dep. to prevent inf. rec.
        if ( isCycle( r, q, print ) ) {
            if (print) fprintf( stderr, "*** Process '%s'(%d) doing: %s\n",
                      olpe->processes[p].name,
                      olpe->processes[p].argument,
                      process[p].lastEvent );
	    DEPENDS(p,r) = thisDep;	// restore dep.
            return 1;
        }
	DEPENDS(p,r) = thisDep;		// restore dep.
    }
    return 0;				// found no cycles
}

/*!
********************************************************************************
Make a dependency in the matrix from process p (in EQevent) to process q via
channel c.

This is only important when making a select dependency, since caller needs to
know whether any q is alive to write a response.

This function can detect some deadlocks itself:
- operation is doomed because q has exited
- select is doomed because no possible producers are running
- deadly embrace between p & q (e.g., both try to write to each other)

If the new dependency could possibly create a circular wait, it calls
isCycle() to investigate.

\param pevt
\param q
\param c
\param dep Gives the nature of the dependency (see Dependency matrix #depends).

\retval 1 is good status, the dependency was created.
\retval 0 means it was not created because the other process has exited, or
was doing something inconsistent (but this could be valid due to a select).
\retval -1 means that a matching select/write pair was found, and the earlier
dependency was removed.
*******************************************************************************/
static int makeDepend( EQevent *pevt, int q, int c, signed char dep )
{
    int p = pevt->proc;

printf( "$DL$ +++ makeDepend %d->%d @C%d %d; ", p, q, c, dep );

    // case where q has exited => DEADLOCK
    if ( process[q].state == DEAD ) {
printf( "dead target!\n" );
	if ( dep == -2 ) return 0;	// could be other selection producers
	abortDL( pevt->proc, pevt->saveEvent,
	        "Process at other end of channel has exited" );
    }

    // case where q->p has no dependency, so just make the new one
    if ( DEPENDS(q,p) == 0 ) {
	DEPENDS(p,q) = dep;
	chanproc[c] = p;	// note that p is using channel c
printf( "added!\n" );

	// increment state to show blocked
        if ( (process[p].state)++ == RUN ) { // if was running...
	    process[p].lastEvent = pevt->saveEvent;
	    pevt->saveEvent = NULL;
        }

	/* Deadlock checking:
	    Just added dependency from p to q; see if there's a
	    cycle from q back to p!  If this is a select dependency (-2),
	    don't abort here, because another producer process (q) may
	    not have a cycle, so just return status to caller.  If all the
	    statuses add to 0, the caller will abort.
	*/
printf( "$DL$ +++ checking cycle\n" );
	if ( dep == -2 )
	    return isCycle( q, p, 0 ) ? 0 : 1;	// suppress printing

	if ( isCycle( q, p, 1 ) ) {
	    // will have printed all the waiting events up to p
	    abortDL( p, process[p].lastEvent,
		    "Operation creates circular wait with above processes" );
	}
	else return 1;
    }

    // case where q->p dependency exists; check whether it's the same channel
    if ( chanproc[c] == q ) {
        int ch, sel;

	// combine with p->q's dependency and handle according to result
	switch( DEPENDS(q,p) + dep ) {

	    case 0:		// dependencies cancel each other
printf( "cancels!\n" );
		DEPENDS(q,p) = 0;
		chanproc[c] = -1;	// no one using channel c now

		// decrement state to show unblocked (from q, anyway)
        	if ( --(process[q].state) == RUN ) {	// if now running...
		    free( (char*)process[q].lastEvent ); // discard saved event
	        // [future] emit a trace
		}
		// no need to record new dependency, we're done
		return 1;

	    case -1:        // select matches write
printf( "select complete!\n" );
                sel = (dep == -2 ) ? p : q;     // who's doing select?

                // clear selecting process's row
                memset( &DEPENDS(sel,0), 0, olpe->allocated_processes );

                // clear its channel(s) usage
                for ( ch=1; ch<=olpe->allocated_channels; ch++ )
                    if ( chanproc[ch] == sel ) chanproc[ch] = -1;

                // p doing select, which succeeded on q's write
        	if ( sel == p ) {
		    if ( process[p].state > 0 ) {	// p might be blocked
        		process[p].state = RUN; // unblock p and discard saved event
        		free( (char*)process[p].lastEvent );
		    }
		    return -1;  // indicate p's select write match found
		}

		// q doing select, which succeeded on p'q write
        	process[q].state = RUN;     // unblock q's select and discard saved event
        	free( (char*)process[q].lastEvent );
        	return makeDepend( pevt, q, c, dep );  // add the write dependency

	    default:	// any other result should not occur
		PI_OLP_ASSERT( 0, PI_SYSTEM_ERROR )
	}
    }

    /* case where q->p dependency exists on different channel; this can only
       be legitimate if p and/or q is doing select (because another process
       could fulfill the select; this process cannot, because the two are
       doing operations on different channels)
    */

    // if p is doing the select, don't insert the dependency
    if ( dep == -2 ) return 0;

    // if q is doing the select...
    if ( DEPENDS(q,p) == -2 ) {
	DEPENDS(q,p) = 0;	// clear the dependency and decrement q's state

	/* Strictly speaking, we should clear chanproc[ch] where ch is the
	   channel in the selector bundle coming from p, but this information
	   would have to be looked up.  It does no harm to leave the channel
	   status as "used by q" since p is not going to write on it (p's
	   doing something else with q).  When q's select completes, the
	   channel status will be cleaned up.
	*/

	if ( --(process[q].state) == RUN )	// if now running...
	    abortDL( pevt->proc, pevt->saveEvent,
			"Earlier select cannot be fulfilled" );
	// reinsert the dependency, which will now go in because q->p been cleared
	return makeDepend( pevt, q, c, dep );
    }

    // neither party is selecting, so they're acting at cross purposes
    abortDL( pevt->proc, pevt->saveEvent,
		"Conflicting channels create deadly embrace" );
    return 0;		// make compiler not warn
}

/*!
********************************************************************************
Remove all dependencies for an exited process
*******************************************************************************/
static void removeDepends( int q )
{
printf( "$DL$ +++ removeDepend %d\n", q );

    int p, r;		// process IDs
    int selectOK;	// true if not removing last possible selection producer

    process[q].state = DEAD;

    // if this was an "extra" MPI process, nothing more to do
    if ( q >= olpe->allocated_processes ) return;

    // check process's column (who's depending on q)
    for ( p = 0; p<olpe->allocated_processes; p++ ) {

	switch( DEPENDS(p,q) ) {
	    case 0:	// no dependency, pass on
		break;

	    case -2:	// selection on q
		DEPENDS(p,q) = 0;	// remove dependency
		// better be some p->r -2 left, o'wise cannot complete
		selectOK = 0;
		for ( r = 0; r<olpe->allocated_processes; r++ ) {
		    if ( DEPENDS(p,r) == -2 ) {
			selectOK = 1;	// found a selection producer
			break;          // out of inner for loop
		    }
		}
		if ( selectOK ) break;  // out of switch to outer for loop
                // else fall through to default and abort

	    default:	// read/write from/to p cannot complete
	    	abortDL( q, process[p].lastEvent,
	    	        "Process exiting leaves earlier operation hung" );
	}
    }
}


/*!
********************************************************************************
Event handling function
*******************************************************************************/
static void handle( EQevent *ev )
{
    int object = -1;		// object of call: channel or bundle ID
    int q;			// process corresponding to object

    // we get here with some generic parsing already done
printf( "$DL$ * handling %d '%4s'\n", ev->proc, ev->ecode );

    // lookup event code
    char *found = strstr( eventCodes, ev->ecode );
    PI_OLP_ASSERT( found, PI_SYSTEM_ERROR )	// unrecognized code!

    // if CALLS event type, parse object number
    if ( ev->ecode[0] == 'C' ) {
	const char* tok = strtok_r( NULL, PI_LOGSEP, &ev->strtokLast );
	PI_OLP_ASSERT( tok, PI_SYSTEM_ERROR )
	object = atoi( tok );
	PI_OLP_ASSERT( object>=0, PI_SYSTEM_ERROR )
    }

    // switch on index of event code
    switch( (found-eventCodes)/(sizeof(ev->ecode)-1) ) {
	int bundsize, countdeps, i;
	PI_CHANNEL **bundchan;

	case 0:	// PI_Write; make write dependency ev->q via channel
	    q = olpe->channels[object-1]->consumer;
	    makeDepend( ev, q, olpe->channels[object-1]->chan_id, +1 );
	    break;

	case 1: // PI_Read; make read dependency ev->q via channel
	    q = olpe->channels[object-1]->producer;
	    makeDepend( ev, q, olpe->channels[object-1]->chan_id, -1 );
	    break;

	case 2: // PI_Select
	    bundsize = olpe->bundles[object-1]->size;
	    bundchan = olpe->bundles[object-1]->channels;

	    /* Make selection dependencies p->{producers of Selector bundle}:
	       Count the successful insertions (makeDepend() returns 1).  A -1
	       returns means we can stop, because the select hit a corresponding
	       write.
	    */
	    for ( countdeps = i = 0; i<bundsize; i++ ) {
		int d = makeDepend( ev, bundchan[i]->producer, bundchan[i]->chan_id, -2 );
		if ( d < 0 ) {
		    countdeps = 1;
		    break;		// break out of for loop
		}
		countdeps += d;
            }

	    /* Count could be 0 for combo of 2 reasons:
		- all producers were dead (so event still in saveEvent)
		- some/all producers were part of cycle (event in lastEvent)
	    */
	    if ( countdeps == 0 )
		abortDL( ev->proc,
		  ev->saveEvent ? ev->saveEvent : process[ev->proc].lastEvent,
		  "Select cannot be fulfilled" );
	    break;

	case 3: // PI_ChannelHasData ... no deadlock implications
	    break;

	case 4: // PI_TrySelect ... no deadlock implications
	    break;

	case 5: // PI_Broadcast
	    bundsize = olpe->bundles[object-1]->size;
	    bundchan = olpe->bundles[object-1]->channels;

	    // make write dependencies p->{consumers of Broadcaster bundle}
	    for ( i = 0; i<bundsize; i++ )
		makeDepend( ev, bundchan[i]->consumer, bundchan[i]->chan_id, +1 );
	    break;

	case 6: // PI_Gather
	    bundsize = olpe->bundles[object-1]->size;
	    bundchan = olpe->bundles[object-1]->channels;

	    // make read dependencies p->{producers of Broadcaster bundle}
	    for ( i = 0; i<bundsize; i++ )
		makeDepend( ev, bundchan[i]->producer, bundchan[i]->chan_id, -1 );
	    break;

	case 7: // process exited
	    removeDepends( ev->proc );
	    break;

	default:	// not expecting any other event type
	    PI_OLP_ASSERT( found, PI_SYSTEM_ERROR )
    }

    // end of handling
    free( ev->event );	// free copy used for parsing
    if ( ev->saveEvent ) free( (char*)ev->saveEvent ); // copy wasn't saved by makeDepend()
    ev->proc = -1;	// flag inactive status in queue for compacting

}


/*!
********************************************************************************
Start the deadlock detector.

\param e the environment of \c OnlineProcessFunc, which has the same tables
as all processes.
*******************************************************************************/
void PI_DetectDL_start_( const PI_PROCENVT *e )
{
    int i;

    printf( "$DL$ Worldsize = %d; P %d, C %d, B %d\n",
        e->worldsize, e->allocated_processes, e->allocated_channels,
        e->allocated_bundles );

    /* allocate the dependency matrix for N processes, initially zero entries */
    depends = calloc( e->allocated_processes*e->allocated_processes,
			sizeof(*depends) );
    PI_OLP_ASSERT( depends, PI_SYSTEM_ERROR )

    /* allocate process state array, initially all RUN state; this array has
       to be up to worldsize, since there may be "extra" MPI processes which
       report exiting and nothing more; they don't affect the dependency
       matrix
    */
    process = calloc( e->worldsize, sizeof(*process) );
    PI_OLP_ASSERT( process, PI_SYSTEM_ERROR )

    /* allocate channel used-by-process array, initially all not-in-use.
       Channel IDs run from 1 to allocated_channels, so make array one larger. */
    chanproc = malloc( (1+e->allocated_channels) * sizeof(*chanproc) );
    PI_OLP_ASSERT( chanproc, PI_SYSTEM_ERROR )
    for ( i=1; i <= e->allocated_channels; i++ )
	chanproc[i] = -1;

    olpe = e;			// needed by event_ func
}

/*!
********************************************************************************
If a deadlock is detected, print all available information on stderr
concerning the deadlocked processes, then call PI_OLP_ASSERT(0, PI_DEADLOCK).

\param event is in form "E_\#_text" where E is the event type, # is the
reporting process, '_' is the field separator PI_LOGSEP, and text depends on
event type.  This detector gets events of type PILOT and CALLS.
*******************************************************************************/
void PI_DetectDL_event_( const char* event )
{
printf( "$DL$ Recd & queued: [%s]\n", event );

    EQappend( event );		// copy, parse, and enqueue event

    /* Repeatedly scan event queue, because handling any event could unblock
       others before/after it in queue.  Scan till no work was found, or queue
       now empty.

	!!!! below: do we want to finish a scan and then go round again? (no
	break)  or must we start a fresh scan anytime an event was handled?
	if there are multiple events from (blocked) P in queue, and handling
	an event of Q unblocks P, it is possible that we would handle chrono.
	later events of P before rescanning the (now unblocked) earlier ones.
	Is that a problem?  Would there really be multiple events from the
	same blocked process?  For now, we'll leave the break; this queue
	shouldn't get very long, one wouldn't think.
    */
    int foundWork;
    EQevent *ev;

    do {
	foundWork = 0;		// assume no work to do (all events blocked)
	EQreset();
	while( (ev = EQnext()) ) {	// scan till end of queue
	    if ( process[ev->proc].state == RUN ) {
		handle( ev );
		foundWork = 1;
		break;		// break out to do/while loop condition !!!!
	    }
	}
    } while( foundWork && EQcompact()>0 );
}

/*!
********************************************************************************
End the deadlock detector.
*******************************************************************************/
void PI_DetectDL_end_()
{
    // make sure event queue is empty, o'wise something wrong!
    PI_OLP_ASSERT( EQcompact()==0, PI_SYSTEM_ERROR )

    free( depends );
    free( process );
    free( chanproc );
printf( "$DL$ signing off\n" );
}
