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
\file pilot.c
\author John Douglas Carter
\author Bill Gardner
\author Emmanuel MacCaull

\brief Implementation file for Pilot

[9-Mar-09] Fixed bug #21: PI_Gather triggers stack out-of-bounds seg fault
	with large array. Switched to MPI_Gatherv. V0.1 (BG)
[30-May-09] Fixed bug #25: PI_Read/Write doesn't realize when channel part
	of non-Selector funnel. V1.0 (BG)
[10-July-09] Fixed bug #22,#23: Too-long format string can overflow
buffer; ParseFormatStrings not picky enough. V1.0 (EM)
*******************************************************************************/

#include "pilot_private.h"	// include these typedefs first

#define PI_NO_OPAQUE		// suppress typedefs in public pilot.h
#include "pilot.h"

#define DEF_ERROR_TEXT      // generate storage for error text array
#include "pilot_error.h"

/* headers for online processes */
#include "pilot_deadlock.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/*** Pilot global variables ***/

int PI_QuietMode = 0;		// quiet mode off
int PI_CheckLevel = 1;		// default level of checking
int PI_OnErrorReturn = 0;	// on any error, abort program
int PI_Errno = PI_NO_ERROR;	// error code returned here (if no abort)
const char *PI_CallerFile;	// filename of caller (set by macro)
int PI_CallerLine;		// line no. of caller (set by macro)

/*** Forward declarations of internal-use functions ***/
static void HandleMPIErrors( MPI_Comm *comm, int *code, ... );
static char *ParseArgs( int *argc, char ***argv );
static void *OnlineThreadFunc( void *arg );
static int OnlineProcessFunc( int a1, void *a2 );
static int ParseFormatString( IO_DIRECTION readOrWrite, PI_MPI_RTTI meta[], const char *fmt, va_list ap );

/*** Logging facility ***/
typedef enum { PILOT='P', USER='U', TABLES='T', CALLS='C', STATS='S' } LOGEVENT;
static void LogEvent( LOGEVENT ev, const char *event );


#define LOUD if( !PI_QuietMode )


/*!
********************************************************************************
\brief Copy of the environment for this Pilot process.

Holds processes, channels and internal variables.  Each process
will possess the same environment at PI_StartAll(), then local updates will
occur for bookkeeping purposes as reads/writes take place.
*******************************************************************************/
static PI_PROCENVT thisproc = { .phase=PREINIT };

static int MPICallLine;	/*!< line number of last MPI library call (PI_CALLMPI macro) */
static int MPIMaxTag;	/*!< max tag number allowed by this MPI implementation */
static int MPIPreInit;	/*!< non-0 if MPI already initialized when Pilot invoked */

static pthread_t OnlineThreadID; /*!< ID of online thread, if any */

/* Command-line options:
These variables are only meaningful on node 0 (and we assume that only
node 0 can write files).  The resulting service flag settings are broadcast
to other processes to store in their PI_PROCENVT, where they will be checked
during PI_ API calls.  These variables are not used after PI_Configure.
*/
static char *LogFilename;	/*!< Path to log file. NULL = no log file needed. */
static enum {OLP_NONE, OLP_THREAD, OLP_PILOT} OnlineProcess;
enum {OPT_CALLS=0, OPT_STATS, OPT_TOPO, OPT_TRACE, OPT_DEADLOCK, OPT_END};
static unsigned char Option[OPT_END];	/*!< List of command-line options. 1/0 = flag set/clear */


/*** Public API; each PI_Foo_ is called via PI_Foo wrapper macro ***/

/* Setup functions, to call before StartAll */

int PI_Configure_( int *argc, char ***argv )
{
    int i /******, j, k*********/;
    int provided;	// level of thread support provided by MPI

    PI_ON_ERROR_RETURN( 0 )
    PI_ASSERT( , thisproc.phase==PREINIT, PI_WRONG_PHASE )

    thisproc.phase = CONFIG;
    thisproc.start_time = -1.0;

    /* Process command line arguments into LogFilename, OnlineProcess, and
       Option array.  May override caller's setting of PI_CheckLevel.
       This removes any recognized by Pilot.  MPI_Init will do the same with
       whatever are left.  The user's app can scan anything remaining.  This
       may change *argc and shuffle the array of char* in *argv.  If we're not
       running on node 0, there may be no arguments.  We'll only use the results
       on node 0 below.  (We don't know which node we are since MPI isn't
       initialized yet!)
    */
    char *badargs = ParseArgs( argc, argv );	// return needs to be freed

    /* Unless we're in "bench mode" (MPI already initialized), we next init MPI.
       If we need the thread-based online process (because of writing a log
       file), we have to specify the threaded version for node 0.  Unfortunately,
       if we're not node and the arguments _were_ conveyed and parsed, this will
       result in (needlessly) starting the threaded version on ALL nodes; there's
       no way around this.  If the user knows that files can be written from
       non-0 nodes, then the Pilot-based online process can be selected, which
       will avoid starting the threaded version.
    */
    MPI_Initialized( &MPIPreInit );	/* did user already initialize MPI? */
    if ( !MPIPreInit ) {
        MPI_Init_thread( argc, argv,
                         OnlineProcess==OLP_THREAD ? MPI_THREAD_MULTIPLE : MPI_THREAD_SINGLE,
                         &provided );		/* starts MPI */
    }
    else
        MPI_Query_thread( &provided );

    /* set up handler for any MPI error so Pilot can provide details then abort */
    MPI_Errhandler errhandler;
    MPI_Comm_create_errhandler( (MPI_Comm_errhandler_fn*)HandleMPIErrors, &errhandler );
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, errhandler ); // inherited by all communicators

    MPI_Comm_rank( MPI_COMM_WORLD, &thisproc.rank );	/* get current process id */
    MPI_Comm_size( MPI_COMM_WORLD, &thisproc.worldsize );	/* get number of processes */

    /* find out what the max. tag no. can be (15 bits by standard, up to 31) */
    int *tagub, flag;
    MPI_Attr_get( MPI_COMM_WORLD, MPI_TAG_UB, &tagub, &flag );
    MPIMaxTag = flag ? *tagub : 32767;		// was attrib. defined?

    if ( thisproc.rank == 0 ) {
        if ( badargs )
            fprintf( stderr, "Pilot: unrecognized arguments: %s\n", badargs );

/////////////// should following be done in ParseArgs, so worked-over value of
/////////////// OnlineProcess is available for MPI_Init?
        /* go through the options and determine their runtime implications,
           setting appropriate service flags */
        int i, anyopts = 0;
        for ( i=0; i<OPT_END; i++ )
            anyopts = anyopts || Option[i];

        /* determine which kinds of data must be logged to supply the service */
        thisproc.svc_flag[LOG_TABLES] = Option[OPT_CALLS] ||
            Option[OPT_STATS] || Option[OPT_TOPO] || Option[OPT_TRACE] ? 1 : 0;
        thisproc.svc_flag[LOG_CALLS] = Option[OPT_CALLS] ||
            Option[OPT_TRACE] || Option[OPT_DEADLOCK] ? 1 : 0;
        thisproc.svc_flag[LOG_STATS] = Option[OPT_STATS] ? 1 : 0;

        /* services needing an online process/thread */
        thisproc.svc_flag[OLP_DEADLOCK] = Option[OPT_DEADLOCK] ? 1 : 0;

        /* log file needed? use default 'pilot.log' if not specified; if
           file specified but no services turned on logging earlier, turn
           on log file after all */
        thisproc.svc_flag[OLP_LOGFILE] = Option[OPT_CALLS] ||
            Option[OPT_STATS] || Option[OPT_TOPO] || Option[OPT_TRACE] ||
            OnlineProcess != OLP_NONE ? 1 : 0;
        if ( thisproc.svc_flag[OLP_LOGFILE] ) {
            if ( NULL==LogFilename ) LogFilename = strdup( "pilot.log" );
        }
        //else if ( LogFilename ) thisproc.svc_flag[OLP_LOGFILE] = 1;

        /* determine which, if any, style of online process is needed;
           default for logfile and for deadlock is Pilot process,
           since thread support in MPIs seems to be generally lacking */
        if ( thisproc.svc_flag[OLP_LOGFILE] ) {
            if ( OnlineProcess == OLP_NONE ) OnlineProcess = OLP_PILOT;
        }
        else OnlineProcess = Option[OPT_DEADLOCK] ? OLP_PILOT : OLP_NONE;

        /* assign the OLP to rank 0 if thread, or 1 if Pilot process */
        thisproc.svc_flag[OLP_RANK] = OnlineProcess==OLP_PILOT ? 1 : 0;

        /* this is the "one stop shop" to find out if logging is enabled */
        thisproc.svc_flag[LOGGING] =
                anyopts || thisproc.svc_flag[OLP_LOGFILE] ? 1 : 0;

/* TESTING: print summary of args
        for ( i=0; i<OPT_END; i++) printf( " %d", Option[i] );
        printf( "; any = %d; olp = %d; fname = %s\n", anyopts, OnlineProcess,
                LogFilename ? LogFilename : "NULL" );
        for ( i=0; i<SVC_END; i++) printf( " %d", thisproc.svc_flag[i] );
        printf( "\n===Thread support = %d\n", provided );
********************************/

        /* say hello; suppress this via PI_QuietMode */
        LOUD {
            printf( "\n*** %s\n", PI_HELLO );
            printf( "*** Available MPI processes: %d; tags for channels: %d\n",
                    thisproc.worldsize, MPIMaxTag );
            printf( "*** Running with error checking at Level %d\n", PI_CheckLevel );

            /* print the options that are in effect */
            printf( "*** Command-line options:\n***  " );
            if ( Option[OPT_CALLS] ) printf( " Call_logging" );
            if ( Option[OPT_STATS] ) printf( " Statistics" );
            if ( Option[OPT_TOPO] ) printf( " Topology" );
            if ( Option[OPT_TRACE] ) printf( " Tracing" );
            if ( Option[OPT_DEADLOCK] ) printf( " Deadlock_detection" );
            printf( "\n" );
            if ( LogFilename )
                printf( "*** Logging to file: %s\n", LogFilename );
            if ( OnlineProcess==OLP_THREAD )
                printf( "*** Online process running on node 0 thread\n" );
            if ( OnlineProcess==OLP_PILOT )
                printf( "*** Online process running as P1\n" );
        }

        /* check to make sure that threading support is available if we need it */
        PI_ASSERT( , OnlineProcess!=OLP_THREAD || provided==MPI_THREAD_MULTIPLE,
                                               PI_THREAD_SUPPORT )
    }

    free( badargs );

    /* broadcast the options to everybody, since we can't assume they parsed
       the args for themselves */

    PI_CALLMPI( MPI_Bcast( thisproc.svc_flag, SVC_END, MPI_UNSIGNED_CHAR, PI_MAIN,
                           MPI_COMM_WORLD ) )

    /* initialize table of processes */
    thisproc.processes =
        ( PI_PROCESS * ) malloc( sizeof( PI_PROCESS ) * thisproc.worldsize );
    PI_ASSERT( , thisproc.processes, PI_MALLOC_ERROR )

    /* initialize table of process aliases: default is Pn */
    for ( i = 0; i < thisproc.worldsize; i++ ) {
        sprintf( thisproc.processes[i].name, "P%d", i );
    }

    /* initialize table of function pointers */
    for ( i = 0; i < thisproc.worldsize; i++ ) {
        thisproc.processes[i].run = NULL;
    }

    thisproc.channels = NULL;	// grow using realloc on demand

    /* allocate bundle table */
    thisproc.bundles = malloc( sizeof( PI_BUNDLE * ) * PI_MAX_BUNDLES );

    for ( i = 0; i < PI_MAX_BUNDLES; i++ ) {
        thisproc.bundles[i] = NULL;
    }

    /* initialize process, channel, bundle counts */
    thisproc.allocated_processes = 0;
    thisproc.allocated_channels = 0;
    thisproc.allocated_bundles = 0;

    /* create a place holder for rank 0 and set name */
    PI_SetName( PI_CreateProcess_( NULL, 0, NULL ), "main" );

    /* If we need to start an online process, create it now, so it gets
       rank 1; this will abort if there aren't at least 2 MPI processes
       available.  Filename has to be sent from PI_MAIN in PI_StartAll, since
       P1 doesn't know it.
    */
    i = thisproc.worldsize;
    if ( thisproc.svc_flag[OLP_RANK] == 1 ) {
        i--;	/* decr. worldsize since OLP absorbs 1 */
        PI_SetName( PI_CreateProcess_( OnlineProcessFunc, 0, NULL ),
			"Pilot Online Process" );
    }

	return thisproc.rank;
//    return i;	/* no. of processes available to user, including PI_MAIN */
}

/* Note: Process IDs (=MPI rank) run from 0 */
PI_PROCESS *PI_CreateProcess_( PI_WORK_FUNC f, int index, void *opt_pointer )
{
    PI_ON_ERROR_RETURN( NULL )
    PI_ASSERT( , thisproc.phase==CONFIG, PI_WRONG_PHASE )

    /* assign the new process the next available MPI process rank */
    int r = thisproc.allocated_processes++;
    PI_ASSERT( , r<thisproc.worldsize, PI_INSUFFICIENT_MPIPROCS )

    /* must supply function unless it's the zero main process */
    PI_ASSERT( , r==0 || f!=NULL, PI_NULL_FUNCTION )
    thisproc.processes[r].run = f;


    snprintf( thisproc.processes[r].name, PI_MAX_NAMELEN, "P%d", r ); // default name "Pn"
    thisproc.processes[r].argument = index;
    thisproc.processes[r].argument2 = opt_pointer;
    thisproc.processes[r].rank = r;
    thisproc.processes[r].magic = PI_PROC;
    return &thisproc.processes[r];
}

/* Note: Channel tags run from 1, with 0 reserved for special use, i.e.,
   log messages */
PI_CHANNEL *PI_CreateChannel_( PI_PROCESS *from, PI_PROCESS *to )
{
    PI_ON_ERROR_RETURN( NULL )
    PI_ASSERT( , thisproc.phase==CONFIG, PI_WRONG_PHASE )

    int f, t;		// MPI ranks of from/to processes

    if ( from == NULL ) {
        f = PI_MAIN;
    } else {
        PI_ASSERT( LEVEL(1), ISVALID(PI_PROC,from), PI_SYSTEM_ERROR )
        f = from->rank;
    }

    if ( to == NULL ) {
        t = PI_MAIN;
    } else {
        PI_ASSERT( LEVEL(1), ISVALID(PI_PROC,to), PI_SYSTEM_ERROR )
        t = to->rank;
    }

    PI_ASSERT( , t!=f, PI_ENDPOINT_DUPLICATE )

    PI_CHANNEL *pc = malloc( sizeof( PI_CHANNEL ) );
    PI_ASSERT( , pc, PI_MALLOC_ERROR )

    // expand array of PI_CHANNEL* pointers
    thisproc.channels = realloc( thisproc.channels,
			(1+thisproc.allocated_channels)*sizeof(PI_CHANNEL*) );
    PI_ASSERT( , thisproc.channels, PI_MALLOC_ERROR )
    thisproc.channels[thisproc.allocated_channels] = pc;

    /* channel ID & initial tag is just 1+no. allocated so far */
    pc->chan_id = pc->chan_tag = ++thisproc.allocated_channels;

    /* it's unlikely that the user will blow past this limit, but better check */
    PI_ASSERT( , pc->chan_tag<MPIMaxTag, PI_MAX_TAGS )

    pc->producer = f;
    pc->consumer = t;

    snprintf( pc->name, PI_MAX_NAMELEN,
		"C%d:P%d>P%d", pc->chan_id, f, t ); // default name "Cn:Pf>Pt"

    pc->bundle = NULL;		/* initially not part of bundle */
    pc->write_count = 0;
    pc->magic = PI_CHAN;

    return pc;
}

PI_BUNDLE *PI_CreateBundle_( enum PI_BUNUSE usage, PI_CHANNEL *const array[], int size )
{
    PI_ON_ERROR_RETURN( NULL )	/* makes caller func return NULL to user */
    PI_ASSERT( , thisproc.phase==CONFIG, PI_WRONG_PHASE )
    PI_ASSERT( , array, PI_NULL_CHANNEL )
    PI_ASSERT( , size>0, PI_ZERO_MEMBERS )
    PI_ASSERT( , thisproc.allocated_bundles<PI_MAX_BUNDLES, PI_MAX_BUNDLES )

    PI_BUNDLE *b = malloc( sizeof( PI_BUNDLE ) );
    PI_ASSERT( , b, PI_MALLOC_ERROR )

    /* Depending on the bundle usage, we'll extract some properties from the
       first channel and propagate them to the others in the bundle:
       - all have a common endpoint, either the producer or consumer
       - a Selector has a common tag; collective bundles don't use tags
    */
    PI_ASSERT( LEVEL(1), ISVALID(PI_CHAN,array[0]), PI_SYSTEM_ERROR )
    int commonEnd = usage==PI_BROADCAST ? array[0]->producer : array[0]->consumer;
    int commonTag = usage==PI_SELECT ? array[0]->chan_tag : 0;

    b->usage = usage;
    b->size = size;
    b->channels = malloc( sizeof( PI_CHANNEL * ) * size );
    PI_ASSERT( , b->channels, PI_MALLOC_ERROR )

    /* copy array of channels into bundle, checking/setting properties */
    int i, j;
    for ( i = 0; i < size; i++ ) {
        PI_ASSERT( LEVEL(1), ISVALID(PI_CHAN,array[i]), PI_SYSTEM_ERROR )

        /* verify that each member channel has the required common end */
        switch ( usage ) {
        case PI_SELECT:
	case PI_GATHER:
            PI_ASSERT( , array[i]->consumer==commonEnd, PI_BUNDLE_READEND )
            break;
        case PI_BROADCAST:
            PI_ASSERT( , array[i]->producer==commonEnd, PI_BUNDLE_WRITEEND )
            break;
        }

        /* verify that there are no duplicate processes on rim */
        for ( j = 1; j < i; j++ ) {
            if ( usage == PI_BROADCAST ) {
                PI_ASSERT( , array[i]->consumer!=array[j]->consumer, PI_BUNDLE_DUPLICATE )
            } else {
                PI_ASSERT( , array[i]->producer!=array[j]->producer, PI_BUNDLE_DUPLICATE )
            }
        }

        /* propagate common tag for Selector bundle */
        if ( usage == PI_SELECT )
            array[i]->chan_tag = commonTag;
        /* make each member of collective channel point to this bundle */
        else
            array[i]->bundle = b;

        b->channels[i] = array[i];	// store the channel member in bundle
    }

    b->narrow_end = usage==PI_BROADCAST ? FROM : TO;

    if ( usage == PI_SELECT ) {
        b->comm = MPI_COMM_WORLD;
    } else {
        /* create the communicator */
        MPI_Group world, group;

        /* get handle on WORLD comm. group */
        PI_CALLMPI( MPI_Comm_group( MPI_COMM_WORLD, &world ) )

        int *ranks = malloc( sizeof( int ) * ( b->size + 1 ) );
        PI_ASSERT( , ranks, PI_MALLOC_ERROR )

        /* fill in ranks array for new group; bundle base goes in rank 0 */
        if ( usage == PI_BROADCAST ) {
            ranks[0] = b->channels[0]->producer;
            for ( i = 1; i < ( b->size + 1 ); i++ )
                ranks[i] = b->channels[i-1]->consumer;
        } else {	/* GATHER */
            ranks[0] = b->channels[0]->consumer;
            for ( i = 1; i < ( b->size + 1 ); i++ )
                ranks[i] = b->channels[i-1]->producer;
        }

        PI_CALLMPI( MPI_Group_incl( world, b->size + 1, ranks, &group ) )
        free( ranks );

        PI_CALLMPI( MPI_Comm_create( MPI_COMM_WORLD, group, &( b->comm ) ) )
    }

    b->magic = PI_BUND;
    thisproc.bundles[thisproc.allocated_bundles] = b;

    /* bundle ID is just 1+no. allocated so far */
    b->bund_id = ++thisproc.allocated_bundles;

    snprintf( b->name, PI_MAX_NAMELEN,
		"B%d@P%d", b->bund_id, commonEnd ); // default name "Bn@Pc"

    return b;
}

PI_CHANNEL **PI_CopyChannels_( enum PI_COPYDIR direction, PI_CHANNEL *const array[], int size )
{
    PI_ON_ERROR_RETURN(NULL)
    PI_ASSERT( , thisproc.phase==CONFIG, PI_WRONG_PHASE )
    PI_ASSERT( , array, PI_NULL_CHANNEL )
    PI_ASSERT( , size>0, PI_ZERO_MEMBERS )

    PI_CHANNEL **newArray = malloc( sizeof( PI_CHANNEL * ) * size );
    PI_ASSERT( , newArray, PI_MALLOC_ERROR )

    /* create channels with same or reversed endpoints */
    int i;
    PI_PROCESS *from, *to;
    for ( i = 0; i < size; i++ ) {
        PI_ASSERT( LEVEL(1), ISVALID(PI_CHAN,array[i]), PI_SYSTEM_ERROR )

	from = &thisproc.processes[array[i]->producer];
	to = &thisproc.processes[array[i]->consumer];

	newArray[i] = direction==PI_SAME ?
	    PI_CreateChannel_( from, to ) :
	    PI_CreateChannel_( to, from );
    }
    return newArray;
}

void PI_SetName_(void *object, const char *name)
{
    PI_ON_ERROR_RETURN()
    PI_ASSERT( , thisproc.phase==CONFIG, PI_WRONG_PHASE )
    PI_ASSERT( , object, PI_INVALID_OBJ )

    char *nameField;	 	// points to the object's name array

    /* probe magic number to identify object type */

    if ( ISVALID(PI_PROC,(PI_PROCESS*)object) ) {
	nameField = ((PI_PROCESS*)object)->name;
    }
    else if ( ISVALID(PI_CHAN,(PI_CHANNEL*)object) ) {
	nameField = ((PI_CHANNEL*)object)->name;
    }
    else if ( ISVALID(PI_BUND,(PI_BUNDLE*)object) ) {
	nameField = ((PI_BUNDLE*)object)->name;
    }
    else {
	PI_ASSERT( , 0, PI_INVALID_OBJ )	// can't identify it
    }

    if ( name != NULL )
        strncpy( nameField, name, PI_MAX_NAMELEN );	// make a copy
    else
	strcpy( nameField, "" );	// use empty string if NULL
}

int PI_StartAll_( void )
{
    PI_ON_ERROR_RETURN( 0 )
    PI_ASSERT( , thisproc.phase==CONFIG, PI_WRONG_PHASE )

    thisproc.phase = RUNNING;

    if ( thisproc.rank == 0 ) {

        LOUD printf( "*** Allocated Pilot processes: %d; channels: %d; bundles: %d\n",
                     thisproc.allocated_processes, thisproc.allocated_channels,
                     thisproc.allocated_bundles );
        int spare = thisproc.worldsize - thisproc.allocated_processes;
        if ( spare > 0 )
            LOUD printf( "*** Note that --%d-- MPI processes will be idle!\n", spare );
        LOUD printf( "\n\n" );

        /* synchronize on barrier below, now we're done printing */
        MPI_Barrier( MPI_COMM_WORLD );  //// matches barrier below ////

        /* If an online thread needs to be started, create it now */
        if ( OnlineProcess == OLP_THREAD ) {
            PI_ASSERT( ,
                0==pthread_create( &OnlineThreadID, NULL, OnlineThreadFunc, NULL ),
                PI_START_THREAD )
        }

        /* If there is a log file, we have to explicitly send the filename
           length and string.  The online process/thread will be waiting to
           receive this.  (An online thread could get it out of this node's
           address space, but we still send it for consistency.)
        */
        if ( OnlineProcess != OLP_NONE ) {
            int flen = LogFilename ? strlen(LogFilename)+1 : 0;
            PI_CALLMPI( MPI_Send( &flen, 1, MPI_INT,
                              thisproc.svc_flag[OLP_RANK], 0, MPI_COMM_WORLD ) )
            if ( flen ) {
                PI_CALLMPI( MPI_Send( LogFilename, flen, MPI_CHAR,
                              thisproc.svc_flag[OLP_RANK], 0, MPI_COMM_WORLD ) )
            }
        }

        return 0;
        /* continues executing main(), the master process */
    }

    MPI_Barrier( MPI_COMM_WORLD );  //// matches barrier above ////

    PI_PROCESS *p = &thisproc.processes[thisproc.rank];
    int status = 0;

    if ( p->run ) {
        /* execute function associated with allocated process */
        status = p->run( p->argument, p->argument2 );
    }
    /* if not allocated a process (run==NULL), just fall through */

    /* since we're not the main process, this will normally call exit */
    PI_StopMain_( status );

    /* if we got here, we're a non-main process and Pilot is in "bench
       mode", so return our MPI rank and let the user decide what to do next

       NOTE: now that we have the status arg, should return it instead of rank?
    */
    return thisproc.rank;
}


/* Functions to call after StartAll */

const char *PI_GetName_(const void *object)
{
    PI_ON_ERROR_RETURN( NULL )
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )

    /* special case NULL object -> name of this process */

    if ( object == NULL )
	return thisproc.processes[thisproc.rank].name;

    /* probe magic number to identify object type */

    if ( ISVALID(PI_PROC,(PI_PROCESS*)object) )
	return ((PI_PROCESS*)object)->name;

    if ( ISVALID(PI_CHAN,(PI_CHANNEL*)object) )
	return ((PI_CHANNEL*)object)->name;

    if ( ISVALID(PI_BUND,(PI_BUNDLE*)object) )
	return ((PI_BUNDLE*)object)->name;

    PI_ASSERT( , 0, PI_INVALID_OBJ )	// can't identify it -> abort
    return NULL;	// to make compiler not warn
}

int PI_GetMyRank( void )
{
    return thisproc.rank;
}

void PI_Write_( PI_CHANNEL *c, const char *format, ... )
{
    PI_ON_ERROR_RETURN()
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , c, PI_NULL_CHANNEL )
    PI_ASSERT( , format, PI_NULL_FORMAT )
    PI_ASSERT( LEVEL(1), ISVALID(PI_CHAN,c), PI_SYSTEM_ERROR )
    PI_ASSERT( , c->producer==thisproc.rank, PI_ENDPOINT_WRITER )

    int i;
    va_list argptr;
    int mpiArgCount;
    PI_MPI_RTTI mpiArgs[ PI_MAX_FORMATLEN ];

    PI_BUNDLE *b = c->bundle;	// collective bundle associated with channel
    if ( b ) {			// NULL if point-to-point

	/* make sure we're on the rim of the bundled channel */
        PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )
	PI_ASSERT( , b->narrow_end==TO, PI_BUNDLED_CHANNEL )
    }

    va_start( argptr, format );
    mpiArgCount = ParseFormatString( IO_DIRECTION_WRITE, mpiArgs, format, argptr );
    va_end( argptr );

    for ( i = 0; i < mpiArgCount; i++ ) {
        PI_MPI_RTTI* arg = &mpiArgs[ i ];

        /* Log first item only */
        if ( i==0 ) LOGCALL( "Wri", c->chan_id, format )

        if ( b==NULL ) {

            PI_CALLMPI( MPI_Send( arg->buf, arg->count, arg->type, c->consumer,
                                  c->chan_tag, MPI_COMM_WORLD ) )
        }
        else {

            /* MPI_Gatherv here sends data to consumer process within comm
               communicator (dedicated to this bundle).  In PI_Gather, the
               same MPI_Gatherv receives the data. */
            PI_CALLMPI( MPI_Gatherv(
                            arg->buf, arg->count, arg->type, // what we're sending
                            NULL, NULL, NULL, 0,	// ignored on sender call
                            0, b->comm ) )		// "root" is rank 0 in bundle
        }

        c->write_count = c->write_count + 1;
    }
}

void PI_Read_( PI_CHANNEL *c, const char *format, ... )
{
    PI_ON_ERROR_RETURN()
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , c, PI_NULL_CHANNEL )
    PI_ASSERT( , format, PI_NULL_FORMAT )
    PI_ASSERT( LEVEL(1), ISVALID(PI_CHAN,c), PI_SYSTEM_ERROR )
    PI_ASSERT( , c->consumer==thisproc.rank, PI_ENDPOINT_READER )

    int i;
    va_list argptr;
    int mpiArgCount;
    PI_MPI_RTTI mpiArgs[ PI_MAX_FORMATLEN ];
    MPI_Status status;

    PI_BUNDLE *b = c->bundle;	// collective bundle associated with channel
    if ( b ) {			// NULL if point-to-point

	/* make sure we're on the rim of the bundled channel */
        PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )
	PI_ASSERT( , b->narrow_end==FROM, PI_BUNDLED_CHANNEL )
    }

    va_start( argptr, format );
    mpiArgCount = ParseFormatString( IO_DIRECTION_READ, mpiArgs, format, argptr );
    va_end( argptr );

    c->write_count++;

    for ( i = 0; i < mpiArgCount; i++ ) {
        PI_MPI_RTTI* arg = &mpiArgs[ i ];
        /* Log first item only */
        if ( i==0 ) LOGCALL( "Rea", c->chan_id, format )

        if ( b==NULL ) {

            PI_CALLMPI( MPI_Recv( arg->buf, arg->count, arg->type, c->producer,
                                  c->chan_tag, MPI_COMM_WORLD, &status ) )
        }
        else {

            /* MPI_Bcast here receives data from producer process within comm
               communicator (dedicated to this bundle).  In PI_Broadcast, the
               same MPI_Bcast sends the data. */

            PI_CALLMPI( MPI_Bcast(
                            arg->buf, arg->count, arg->type,	// what we're sending
                            0, b->comm ) )		// "root" is rank 0 in bundle
        }
    }
}

int PI_Select_( PI_BUNDLE *b )
{
    PI_ON_ERROR_RETURN( 0 )
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , b, PI_NULL_BUNDLE )
    PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )
    PI_ASSERT( , b->usage==PI_SELECT, PI_BUNDLE_USAGE )
    PI_ASSERT( , b->narrow_end==TO, PI_ENDPOINT_READER )

    MPI_Status status;
    int i;

    LOGCALL( "Sel", b->bund_id, "" )

    PI_CALLMPI( MPI_Probe( MPI_ANY_SOURCE, b->channels[0]->chan_tag,
                           MPI_COMM_WORLD, &status ) )

    /* note: this is a sequential search! suppose bundle is large?
       May want to build (on the fly) lookup table (hash?) for rank=>index.
       Another alt (since each rank will likely participate in few bundles) is
       to make small lookup table in rank's PI_PROCENVT: [rank].table:tag=>index.
       The table could be PI_MAX_BUNDLES long, and CreateBundle fills in the next entry
       for each producer process.
    */

    for ( i = 0; i < b->size; i++ ) {
        if ( b->channels[i]->producer == status.MPI_SOURCE )
            return i;
    }

    /* If the message source does not match the producer of any of the bundle's
       channels, that's a problem.  PI_ASSERT(, 0, ...) will always abort. */
    PI_ASSERT( , 0, PI_SYSTEM_ERROR )
    return 0;	// never reached, but satisfies compiler
}

int PI_ChannelHasData_( PI_CHANNEL *c )
{
    PI_ON_ERROR_RETURN( 0 )
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , c, PI_NULL_CHANNEL )
    PI_ASSERT( LEVEL(1), ISVALID(PI_CHAN,c), PI_SYSTEM_ERROR )

    int flag;
    MPI_Status s;

    LOGCALL( "Has", c->chan_id, "" )

    PI_CALLMPI( MPI_Iprobe( c->producer, c->chan_tag, MPI_COMM_WORLD, &flag, &s ) )

    return flag;
}


int PI_TrySelect_( PI_BUNDLE *b )
{
    PI_ON_ERROR_RETURN( -1 )
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , b, PI_NULL_BUNDLE )
    PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )
    PI_ASSERT( , b->usage==PI_SELECT, PI_BUNDLE_USAGE )
    PI_ASSERT( , b->narrow_end==TO, PI_ENDPOINT_READER )

    int flag, i;
    MPI_Status status;

    LOGCALL( "Try", b->bund_id, "" )

    PI_CALLMPI( MPI_Iprobe( MPI_ANY_SOURCE, b->channels[0]->chan_tag,
			    MPI_COMM_WORLD, &flag, &status ) )
    if ( flag == 0 ) return -1;		// no channel has data

    /* lookup message source's corresponding channel index in bundle
       (see comment in PI_Select_ re sequential search) */
    for ( i = 0; i < b->size; i++ ) {
        if ( b->channels[i]->producer == status.MPI_SOURCE )
            return i;
    }

    /* If the message source does not match the producer of any of the bundle's
       channels, that's a problem.  PI_ASSERT(, 0, ...) will always abort. */
    PI_ASSERT( , 0, PI_SYSTEM_ERROR )
    return 0;	// never reached, but satisfies compiler
}

PI_CHANNEL *PI_GetBundleChannel_( const PI_BUNDLE *b, int index )
{
    PI_ON_ERROR_RETURN( NULL )
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , b, PI_NULL_BUNDLE )
    PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )
    PI_ASSERT( , index >= 0 && index < b->size, PI_BUNDLE_INDEX )

    return b->channels[index];
}

int PI_GetBundleSize_( const PI_BUNDLE *b )
{
    PI_ON_ERROR_RETURN( 0 )
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , b, PI_NULL_BUNDLE )
    PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )

    return b->size;
}


void PI_Broadcast_( PI_BUNDLE *b, const char *format, ... )
{
    PI_ON_ERROR_RETURN()
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , b, PI_NULL_BUNDLE )
    PI_ASSERT( , format, PI_NULL_FORMAT )
    PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )
    PI_ASSERT( , b->usage==PI_BROADCAST, PI_BUNDLE_USAGE )
    PI_ASSERT( , thisproc.rank==b->channels[0]->producer, PI_ENDPOINT_WRITER )

    int i;
    va_list argptr;
    int mpiArgCount;
    PI_MPI_RTTI mpiArgs[ PI_MAX_FORMATLEN ];

    va_start( argptr, format );
    mpiArgCount = ParseFormatString( IO_DIRECTION_WRITE, mpiArgs, format, argptr );
    va_end( argptr );

    for ( i = 0; i < mpiArgCount; i++ ) {
        PI_MPI_RTTI* arg = &mpiArgs[ i ];

        /* Log first item only */
        if ( i==0 ) LOGCALL( "Bro", b->bund_id, format )

        PI_CALLMPI( MPI_Bcast(
                        arg->buf, arg->count, arg->type,	// what we're sending
                        0, b->comm ) )		// "root" is rank 0 in bundle
    }
}


void PI_Gather_( PI_BUNDLE *b, const char *format, ... )
{
    PI_ON_ERROR_RETURN()
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )
    PI_ASSERT( , b, PI_NULL_BUNDLE )
    PI_ASSERT( , format, PI_NULL_FORMAT )
    PI_ASSERT( LEVEL(1), ISVALID(PI_BUND,b), PI_SYSTEM_ERROR )
    PI_ASSERT( , b->usage==PI_GATHER, PI_BUNDLE_USAGE )
    PI_ASSERT( , thisproc.rank==b->channels[0]->consumer, PI_ENDPOINT_READER )

    int i;
    va_list argptr;
    int mpiArgCount;
    PI_MPI_RTTI mpiArgs[ PI_MAX_FORMATLEN ];

    va_start( argptr, format );
    mpiArgCount = ParseFormatString( IO_DIRECTION_READ, mpiArgs, format, argptr );
    va_end( argptr );

    for ( i = 0; i < mpiArgCount; i++ ) {
        PI_MPI_RTTI* arg = &mpiArgs[ i ];

        /* set up args for MPI_Gather (receiving side) */
        char sendbuf[1];	// root sends 0-length data, so make dummy buffer
        int recvcounts[b->size+1];	// count that each process sends
        int displs[b->size+1];		// displacements in userbuf for recv

        /* Log first item only */
        if ( i==0 ) LOGCALL( "Gat", b->bund_id, format )

        /* prepare recvcounts and displs arrays so that root sends nothing,
           and all the rest send 'count' items */
        recvcounts[0] = displs[0] = 0;
        for ( i=1; i<=b->size; i++ ) {
            recvcounts[i] = arg->count;
            displs[i] = (i-1) * arg->count;	// fits buffer of size*count items
        }

        PI_CALLMPI( MPI_Gatherv(
                        sendbuf, 0, arg->type,	// send 0 data from "root"
                        arg->buf, recvcounts, displs, arg->type,	// receives all data
                        0, b->comm ) )		// "root" is P0 in bundle communicator
    }
}


void PI_StartTime( void )
{
    thisproc.start_time = MPI_Wtime( );
}

double PI_EndTime( void )
{
    double now = MPI_Wtime( );

    if ( thisproc.start_time > 0.0 )
        return (now - thisproc.start_time);

    fprintf( stderr,
             "\nPI_EndTime: Timing not initialized.  Call PI_StartTime() first." );
    return 0.0;
}


int PI_IsLogging()
{
    return thisproc.svc_flag[OLP_LOGFILE];
}


void PI_Log_( const char *text )
{
    PI_ON_ERROR_RETURN()
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )

    /* if logging to file enabled, forward to LogEvent as USER type event */
    if ( thisproc.svc_flag[OLP_LOGFILE] ) LogEvent( USER, text );
}


/* This function might be called before Pilot's tables are all set up, so be
   sure we don't deref. any NULL pointers.  It can be called in any phase of
   the program by any process.  If MPI is not even running yet, it will call
   abort() and get out that way.  If Pilot is in the configuration phase (where
   the same code is running on all nodes), only rank 0 will print, to avoid
   cluttering the display with many duplicate messages.
*/
void PI_Abort( const int errcode, const char *text, const char *file, const int line )
{
    if ( thisproc.phase != CONFIG || thisproc.rank == 0 ) {
	char *procname = "";
	int procarg = 0;
	if ( thisproc.processes ) {
            procname = thisproc.processes[thisproc.rank].name;
            procarg = thisproc.processes[thisproc.rank].argument;
	}
    	fprintf( stderr,
             "\n*** PI_Abort *** (MPI process #%d) Pilot process '%s'(%d), %s:%d:\n%s%s\n",
             thisproc.rank, procname, procarg, file, line,
             ( errcode >= PI_MIN_ERROR && errcode <= PI_MAX_ERROR ) ?
             ErrorText[errcode] : "",
             text ? text : "" );
    }

    /* MPI should shut down the application and may call exit/abort(errcode) */
    int MPI_up;
    MPI_Initialized( &MPI_up );
    if ( MPI_up )
        MPI_Abort( MPI_COMM_WORLD, errcode );
    else
        abort();
    /******** does not return *********/
}

void PI_StopMain_( int status )
{
    PI_ON_ERROR_RETURN()
    PI_ASSERT( , thisproc.phase==RUNNING, PI_WRONG_PHASE )

    int i /***********, j**********/ ;

    /* First job is to shutdown logging, if it's active. */
    if ( thisproc.svc_flag[LOGGING] ) {

	/* Notify log that this process is finished (unless this *is* the log
	   process on rank 1) */
	if ( thisproc.rank != 1 || thisproc.svc_flag[OLP_RANK] != 1 ) {
	    char buff[PI_MAX_LOGLEN];
	    sprintf( buff, "FIN" PI_LOGSEP "%d", status );
            LogEvent( PILOT, buff );
	}

	/* If online thread was running (on rank 0), join with it */
        if ( thisproc.rank == 0 && thisproc.svc_flag[OLP_RANK] == 0 )
            pthread_join( OnlineThreadID, NULL );
    }

    MPI_Barrier( MPI_COMM_WORLD );	/* synchronize all processes */

    /* If user pre-initialized MPI, then leave it initialized.  This is to
       allow Pilot to be re-configured and used again in this program, which is
       needed for running benchmarks, but not in ordinary use.
    */
    if ( MPIPreInit )
        thisproc.phase = PREINIT;
    else {
        thisproc.phase = POSTRUN;
        MPI_Finalize( );
    }

    /* deallocate memory */

/********** old data structure
    for ( i = 0; i < thisproc.worldsize; i++ ) {
        for ( j = 0; j < thisproc.worldsize; j++ ) {
            if ( thisproc.channels[i][j] != NULL )
                free( thisproc.channels[i][j] );
        }
        if ( thisproc.channels[i] != NULL )
            free( thisproc.channels[i] );
    }
******************************/

    for ( i = 0; i < thisproc.allocated_channels; i++ )
	free( thisproc.channels[i] );

    if ( thisproc.channels != NULL )
        free( thisproc.channels );

    if ( thisproc.processes != NULL )
        free( thisproc.processes );

    if ( thisproc.bundles != NULL )
        free( thisproc.bundles );

    /* The main process always returns, but other processes normally exit
       here because otherwise they would return from PI_StartAll and (re)execute
       main's code.  However, if Pilot is in "bench mode", we do return so
       that another Pilot application can be configured and executed.
    */
    if ( thisproc.rank!=0 && !MPIPreInit ) exit( 0 );
}


/*** Internal Functions ***/


/********* want a function that investigates what "reasonable" values are for
 pointers on this platform.  It should check static, stack, & malloc'd addrs,
 and come up with min/max ranges for valid pointers (as opposed to data).  This
 is for use in PI_Read where user must remember to code "&".  Since the "figuring
 out" may be inaccurate on a given platform, the check should occur at Level 2,
 only if the user wants.  So the function calculates the range, then a macro
 ISPTR(p) checks it.  Even if this is dreaming, we can at least check for NULL.
**********/

/*!
********************************************************************************
Called by MPI to handle error in library function.
Handle the error reported from MPI by printing message and aborting.

\param comm MPI communicator where error occurred (will be MPI_COMM_WORLD).
\param code MPI error code
\param ... additional implementation-specific args (ignore)
*******************************************************************************/
static void HandleMPIErrors( MPI_Comm *comm, int *code, ... )
{
    char buff[MPI_MAX_ERROR_STRING], text[MPI_MAX_ERROR_STRING+80];
    int len;

    MPI_Error_string( *code, buff, &len );	// obtain text for error code

    snprintf( text, sizeof(text), " at " __FILE__ ":%d; MPI error code %d:\n%s",
              MPICallLine, *code, buff );

    PI_Abort( PI_SYSTEM_ERROR, text, PI_CallerFile, PI_CallerLine );
    /***** does not return *****/
}

/*!
********************************************************************************
Parse command-line arguments to Pilot. Fills in #Option.
*******************************************************************************/
static char *ParseArgs( int *argc, char ***argv )
{
    int i, j, unrec, count=0, moveto=0;
    char *badargs = NULL;		// assume all -pi... args will be OK

    memset( Option, 0, OPT_END );	// clear all option flags
    LogFilename = NULL;			// assume no log needed
    OnlineProcess = OLP_NONE;		// assume no online process needed

    /* scan args, shuffling non-Pilot args up in *argv array */
    for ( i=0; i<*argc; i++ ) {
        unrec = 0;
        if ( 0==strncmp( (*argv)[i], "-pi", 3 ) ) {	// -pi... arg found

            /* '-pisvc=...' runtime services */
            if ( 0==strncmp( (*argv)[i]+3, "svc=", 4 ) ) {
                for ( j=7; (*argv)[i][j]; j++ ) {
                    switch ( toupper( (*argv)[i][j] ) ) {
                    case 'C':
                        Option[OPT_CALLS] = 1;
                        break;
                    case 'S':
                        Option[OPT_STATS] = 1;
                        break;
                    case 'M':
                        Option[OPT_TOPO] = 1;
                        break;
                    case 'T':
                        Option[OPT_TRACE] = 1;
                        break;
                    case 'D':
                        Option[OPT_DEADLOCK] = 1;
                        break;
                    default:
                        unrec = 1;
                    }
                }
            }

            /* '-pilog=t|p =filename =t|p:filename' */
            else if ( 0==strncmp( (*argv)[i]+3, "log=", 4 ) ) {
                char tp = toupper( (*argv)[i][7] ); // possible t or p flag
                int len = strlen( (*argv)[i] );
                if ( len < 8 ) unrec = 1;
                else if ( len == 8 ) {
                    if ( 'T'==tp || 'P'==tp )
                        OnlineProcess = 'T'==tp ? OLP_THREAD : OLP_PILOT;
                    else LogFilename = (*argv)[i]+7; // found 1-char filename
                }
                else if ( ':'==(*argv)[i][8] ) {
                    if ( 'T'==tp || 'P'==tp )
                        OnlineProcess = 'T'==tp ? OLP_THREAD : OLP_PILOT;
                    else unrec = 1;
                    if ( len > 9 )
                        LogFilename = (*argv)[i]+9;	// found :filename
                }
                else LogFilename = (*argv)[i]+7;	// found filename
            }

            /* '-picheck=n' */
            else if ( 0==strncmp( (*argv)[i]+3, "check=", 6 ) ) {
                if ( 10==strlen( (*argv)[i] ) ) {
                    j = (*argv)[i][9];	// extract level number
                    if ( isdigit(j) ) PI_CheckLevel = j - '0';
                    else unrec = 1;
                }
                else unrec = 1;
            }

            else unrec = 1;

            /* save up all unrecognized arguments to return to caller */
            if ( unrec ) {
                j = badargs ? strlen(badargs) : 0;	// current string len
                badargs = realloc( badargs, j + strlen((*argv)[i]) + 1 );
                strcpy( badargs+j, " " );
                strcpy( badargs+j+1, (*argv)[i] );
            }
        }
        else {
            if ( count < i ) (*argv)[moveto] = (*argv)[i];
            count++;
            moveto++;
        }
    }
    *argc = count;	// adjust remaining arg count

    return badargs;
}


/*!
********************************************************************************
This thread is used to invoke OnlineProcessFunc when the latter has to run
as a thread on node 0.
*******************************************************************************/
static void *OnlineThreadFunc( void *arg )
{
    OnlineProcessFunc( 0, NULL );
    return NULL;    // thread will be joined by PI_StopMain
}


/*!
********************************************************************************
This process handles collecting/printing log entries, and running any other
online services such as deadlock detection.  The log filename, if any,
is received from PI_MAIN.

Currently, the log timestamp is being added here. MPI semantics guarantee
that the events from any given process will be logged in order, but there
is no guarantee of a total ordering of all events from all processes. It
would be more accurate to have PI_Log timestamp the event on its own node,
then have this function apply a correction, like how MPE tries to synchronize
all the clocks.

\note If the program crashes, log entries -- maybe the entire file -- may be
lost in OS buffers. We should probably do some selective periodic flushing,
but not every line to avoid burdening the program with disk activity.
*******************************************************************************/
static int OnlineProcessFunc( int dum1, void *dum2 )
{
    MPI_Status stat;
    int flen;
    char *fname;
    FILE *logfile = NULL;
    char buff[PI_MAX_LOGLEN];

    double start = MPI_Wtime();     // capture time at start of run

    /* get filename length; 0 = no log file */
    PI_CALLMPI( MPI_Recv( &flen, 1, MPI_INT, PI_MAIN, 0, MPI_COMM_WORLD, &stat ) )

    /* open the log file if needed */
    if ( flen ) {
        PI_OLP_ASSERT( fname = malloc( flen ), PI_MALLOC_ERROR )
        PI_CALLMPI( MPI_Recv( fname, flen, MPI_CHAR,
                              PI_MAIN, 0, MPI_COMM_WORLD, &stat ) )
        logfile = fopen( fname, "w" );
        if ( NULL==logfile )
            PI_Abort( PI_LOG_OPEN, fname, __FILE__, __LINE__  );
        /***** does not return *****/

        free( fname );
    }

    if ( thisproc.svc_flag[LOG_TABLES] );   // dump tables to log file
    // might be easier to do in PI_MAIN with calls to LogEvent

    /* startup other OLPs */
    if ( thisproc.svc_flag[OLP_DEADLOCK] ) PI_DetectDL_start_( &thisproc );


    /******** main loop till "FIN" messages ********/

    /* no. of FINs that have to check in (1 less if we are Pilot process) */
    int FINs = thisproc.worldsize - thisproc.svc_flag[OLP_RANK];
    while ( FINs > 0 ) {
        /* wait for next message from LogEvent */
        PI_CALLMPI( MPI_Recv( buff, PI_MAX_LOGLEN, MPI_CHAR,
                              MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &stat ) )

        char *event = buff;
        int contchar = PI_MAX_LOGLEN - 1;   // index of continuation char
        int source = stat.MPI_SOURCE;

        /* receive rest of message while continuation char found */
        while ( event[contchar] == '+' ) {
            if ( event == buff )            // first extension
                event = strdup( buff );     // get msg in dynamic storage
            int msglen = strlen( event );
            PI_OLP_ASSERT( event = realloc( event, msglen+PI_MAX_LOGLEN ),
                            PI_MALLOC_ERROR )   // extend buffer

            /* continue receiving just from source of partial message */
            PI_CALLMPI( MPI_Recv( event+msglen, PI_MAX_LOGLEN, MPI_CHAR,
                              source, 0, MPI_COMM_WORLD, &stat ) )
            contchar = msglen + PI_MAX_LOGLEN - 1;
        }

        /* forward to OLP if event type is one it wants */
        if ( thisproc.svc_flag[OLP_DEADLOCK] )
            if ( event[0] == PILOT || event[0] == CALLS )
                PI_DetectDL_event_( event );

        /* get timestamp (nsec from start) and write event to log file */
        if ( logfile )
            fprintf( logfile, "%.6ld" PI_LOGSEP "%s\n",
                        (long int)((MPI_Wtime()-start)*1000000), event );

        /* check for "P_n_FIN" pattern, where P is the PILOT message type char,
            _ is the separator, and n is the process number.  We need to get
           one from all processes, so decrement counter.
        */
        if ( event[0] == PILOT ) {
            char *sep = strpbrk( event+2, PI_LOGSEP );   // skip over P_n
            if ( sep && 0==strncmp( sep+1, "FIN", 3 ) ) FINs--;
        }

        if ( event != buff ) free( event ); // it was malloc'd
    }

    /* terminate OLPs */
    if ( thisproc.svc_flag[OLP_DEADLOCK] ) PI_DetectDL_end_();

    if ( thisproc.svc_flag[LOG_STATS] );    // TODO dump stats to log file
    // who collects the stats?

    if ( logfile ) fclose( logfile );

    return 0;
}


/*!
********************************************************************************
Add process number and send to online process.  Format of fixed len message:

 		Pn_t_event...c

  - Pn is the process no. (rank)	<- can't sort this w/o leading 0's
  - _ is the field separator PI_LOGSEP
  - t is the numeric log event type
  - event is as much of the text as will fit, leaving room for \\0
  - c is the continuation character '+' or space for last line

Overflow will only be an issue with user events (via PI_Log).
Rank of OLP is in svc_flag[OLP_RANK]. We use tag=0 since it is not assigned
for channels.
\note This can probably be improved with non-blocking sends (for the last
line), but then the function won't be reentrant (which probably doesn't
matter).
*******************************************************************************/
static void LogEvent( LOGEVENT ev, const char *event )
{
    char buff[PI_MAX_LOGLEN];
    int len = snprintf( buff, PI_MAX_LOGLEN-1,
                        "%c" PI_LOGSEP "%d" PI_LOGSEP "%s",
                        ev, thisproc.rank, event );

    /* len >= LOGLEN-1 means that overflow bytes remain in event, so send another
       line starting from that point */
    const char *next =
            event + PI_MAX_LOGLEN-2 - (strpbrk( buff+2, PI_LOGSEP )-buff+1);

    while ( len >= PI_MAX_LOGLEN-1 ) {
        buff[PI_MAX_LOGLEN-1] = '+';	// insert continuation character

        PI_CALLMPI( MPI_Send( buff, PI_MAX_LOGLEN, MPI_CHAR,
                              thisproc.svc_flag[OLP_RANK], 0, MPI_COMM_WORLD ) )

        len = snprintf( buff, PI_MAX_LOGLEN-1, "%s", next );
        next = next + PI_MAX_LOGLEN-2;
    }
    buff[PI_MAX_LOGLEN-1] = ' ';    // indicate last line

    PI_CALLMPI( MPI_Send( buff, PI_MAX_LOGLEN, MPI_CHAR,
                          thisproc.svc_flag[OLP_RANK], 0, MPI_COMM_WORLD ) )
}

/* -------- Format String Parsing -------- */

/*! Use this enum to help mapping between C datatypes and MPI datatypes.
MPI datatypes are not guaranteed to be integer constants. */
typedef enum {
    CTYPE_INVALID = -1,
    CTYPE_CHAR,
    CTYPE_SHORT,
    CTYPE_INT,
    CTYPE_LONG,
    CTYPE_UNSIGNED_CHAR,
    CTYPE_UNSIGNED_SHORT,
    CTYPE_UNSIGNED_LONG,
    CTYPE_UNSIGNED,
    CTYPE_FLOAT,
    CTYPE_DOUBLE,
    CTYPE_LONG_DOUBLE,
    CTYPE_BYTE,
    CTYPE_LONG_LONG,
    CTYPE_UNSIGNED_LONG_LONG,
    CTYPE_USER_DEFINED
} CTYPE;

/*!
********************************************************************************
Stores the first three alphabetic chars of \p bytes into an integer. This
provides O(1) lookup time for the conversion specifiers.

\post Bits 0 - 23 of the return value contain the 3 chars.
*******************************************************************************/
static int32_t BytesToInt( const char *bytes )
{
    int32_t result = 0;
    int i = 0;
    while ( i < 3 && isalpha( bytes[ i ] ) ) {
        result |= ( bytes[ i ] & 0xff ) << ( 16 - i * 8 );
        i += 1;
    }
    return result;
}

/*!
********************************************************************************
Fills in \c rtti->type and \c rtti->cType given a conversion specification.
\return The number of characters that have been successfully processed.
\retval 0 indicates failure since conversion specifiers are at least one
char long.
*******************************************************************************/
static int LookupConversionSpec( const char *key, PI_MPI_RTTI *rtti )
{
    CTYPE cType = CTYPE_INVALID;
    MPI_Datatype mpiType = MPI_DATATYPE_NULL;
    int skip = 0;

// Easy-ish way to define a conversion specifier.
#define CONVERSION_SPEC( c1, c2, c3 ) \
    ( ( (c1) & 0xff ) << 16 | ( (c2) & 0xff ) << 8 | ( (c3) & 0xff ) )

    switch ( BytesToInt( key ) ) {
    case CONVERSION_SPEC( 'b', 0, 0 ):
        cType = CTYPE_BYTE;
        mpiType = MPI_BYTE;
        skip = 1;
        break;

    case CONVERSION_SPEC( 'c', 0, 0 ):
        cType = CTYPE_CHAR;
        mpiType = MPI_CHAR;
        skip = 1;
        break;

    case CONVERSION_SPEC( 'h', 'd', 0 ):
    case CONVERSION_SPEC( 'h', 'i', 0 ):
        cType = CTYPE_SHORT;
        mpiType = MPI_SHORT;
        skip = 2;
        break;

    case CONVERSION_SPEC( 'd', 0, 0 ):
    case CONVERSION_SPEC( 'i', 0, 0 ):
        cType = CTYPE_INT;
        mpiType = MPI_INT;
        skip = 1;
        break;

    case CONVERSION_SPEC( 'l', 'd', 0 ):
    case CONVERSION_SPEC( 'l', 'i', 0 ):
        cType = CTYPE_LONG;
        mpiType = MPI_LONG;
        skip = 2;
        break;

    case CONVERSION_SPEC( 'l', 'l', 'd' ):
    case CONVERSION_SPEC( 'l', 'l', 'i' ):
        cType = CTYPE_LONG_LONG;
        mpiType = MPI_LONG_LONG;
        skip = 3;
        break;

    case CONVERSION_SPEC( 'h', 'h', 'u' ):
        cType = CTYPE_UNSIGNED_CHAR;
        mpiType = MPI_UNSIGNED_CHAR;
        skip = 3;
        break;

    case CONVERSION_SPEC( 'h', 'u', 0 ):
        cType = CTYPE_UNSIGNED_SHORT;
        mpiType = MPI_UNSIGNED_SHORT;
        skip = 2;
        break;

    case CONVERSION_SPEC( 'u', 0, 0 ):
        cType = CTYPE_UNSIGNED;
        mpiType = MPI_UNSIGNED;
        skip = 1;
        break;

    case CONVERSION_SPEC( 'l', 'u', 0 ):
        cType = CTYPE_UNSIGNED_LONG;
        mpiType = MPI_UNSIGNED_LONG;
        skip = 2;
        break;

    case CONVERSION_SPEC( 'l', 'l', 'u' ):
        cType = CTYPE_UNSIGNED_LONG_LONG;
        mpiType = MPI_UNSIGNED_LONG_LONG;
        skip = 3;
        break;

    case CONVERSION_SPEC( 'f', 0, 0 ):
        cType = CTYPE_FLOAT;
        mpiType = MPI_FLOAT;
        skip = 1;
        break;

    case CONVERSION_SPEC( 'l', 'f', 0 ):
        cType = CTYPE_DOUBLE;
        mpiType = MPI_DOUBLE;
        skip = 2;
        break;

    case CONVERSION_SPEC( 'L', 'f', 0 ):
        cType = CTYPE_LONG_DOUBLE;
        mpiType = MPI_LONG_DOUBLE;
        skip = 3;
        break;

    case CONVERSION_SPEC( 'm', 0, 0 ):
        cType = CTYPE_USER_DEFINED;
        /* Let the caller fill in the MPI_Datatype */
        skip = 1;
        break;

    default:
        return 0;
    }
#undef CONVERSION_SPEC

    if ( rtti ) {
        rtti->cType = cType;
        rtti->type = mpiType;
    }

    return skip;
}

/*!
********************************************************************************
Parse a printf like format string into data which describes MPI data.

\param readOrWrite  Whether the va_list should be interpreted as a list of
pointers or scalars.
\param meta  An array of size PI_MAX_FORMATLEN to hold the parsed arguments.
\param fmt  Printf like format to be parsed.
\param ap  The va_list to read the arguments from. This function uses the
va_arg macro, so the value of `ap` is undefined after the call. This
function does not call va_end. See stdarg(3).

\return The number of arguments parsed or -1 when an invalid format string
is encountered.
*******************************************************************************/
static int ParseFormatString( IO_DIRECTION readOrWrite, PI_MPI_RTTI meta[],
                              const char *fmt, va_list ap )
{
    const char *s = fmt;
    int metaIndex;

    PI_ON_ERROR_RETURN( -1 );
    PI_ASSERT( , fmt != NULL, PI_NULL_FORMAT );

    for ( metaIndex = 0; metaIndex < PI_MAX_FORMATLEN; metaIndex++ ) {
        PI_MPI_RTTI *rtti = &meta[ metaIndex ];
        int count = -1; /* -1 for not user specified */

        /* Skip whitespace in fmt */
        while ( isspace(*s) ) s++;

        if ( *s == '\0' )
            break;

        /* The next char must mark the start of a conversion specification. */
        PI_ASSERT( , *s == '%', PI_FORMAT_ARGS );

        s++;
        PI_ASSERT( , *s != '\0', PI_FORMAT_ARGS );

        /* Parse optional array size */
        if ( isdigit(*s) ) {
            count = 0;
            do {
                count = count * 10 + ( *s++ ) - '0';
            } while ( isdigit(*s) );
            PI_ASSERT( , *s != '\0', PI_FORMAT_ARGS );

            /* Disallow "%1" since 1 (one) looks too similar to l (ell).
               Also disallow "%0" as it has no meaning. */
            PI_ASSERT( , count > 1, PI_FORMAT_ARGS );
        }

        /* '*' specifies the field width */
        if ( *s == '*' ) {
            /* Make sure the count has not already been specified */
            PI_ASSERT( , count == -1, PI_FORMAT_ARGS );
            count = va_arg( ap, int );
            PI_ASSERT( , count > 0, PI_FORMAT_ARGS );
            s++;
            PI_ASSERT( , *s != '\0', PI_FORMAT_ARGS );
        }

        /* Figure out which MPI type to use */
        int skip = LookupConversionSpec( s, rtti );
        PI_ASSERT( , skip > 0, PI_FORMAT_ARGS );

        /* Set `rtti->buf` to point to the appropriate data. */
        if ( readOrWrite == IO_DIRECTION_READ || count >= 1 ) {
            if ( count <= 0 )
                // This is a Read into a Scalar.
                rtti->count = 1;
            else
                rtti->count = count;

            /* For user defined types, collect the datatype from the user */
            if ( rtti->cType == CTYPE_USER_DEFINED )
                rtti->type = va_arg( ap, MPI_Datatype );

            rtti->data.address = va_arg( ap, void* );
            rtti->buf = rtti->data.address;
        }
        else if ( readOrWrite == IO_DIRECTION_WRITE ) {
            /* Write a single scalar. */
            rtti->count = 1;
            switch ( rtti->cType ) {
                case CTYPE_CHAR:
                    rtti->data.c = ( char )va_arg( ap, int );
                    rtti->buf = &rtti->data.c;
                    break;
                case CTYPE_SHORT:
                    rtti->data.h = ( short )va_arg( ap, int );
                    rtti->buf = &rtti->data.h;
                    break;
                case CTYPE_INT:
                    rtti->data.d = va_arg( ap, int );
                    rtti->buf = &rtti->data.d;
                    break;
                case CTYPE_LONG:
                    rtti->data.ld = va_arg( ap, long int );
                    rtti->buf = &rtti->data.ld;
                    break;
                case CTYPE_UNSIGNED_CHAR:
                    rtti->data.hhu = ( unsigned char )va_arg( ap, unsigned int );
                    rtti->buf = &rtti->data.hhu;
                    break;
                case CTYPE_UNSIGNED_SHORT:
                    rtti->data.hu = ( unsigned short )va_arg( ap, unsigned int );
                    rtti->buf = &rtti->data.hu;
                    break;
                case CTYPE_UNSIGNED_LONG:
                    rtti->data.lu = va_arg( ap, unsigned long );
                    rtti->buf = &rtti->data.lu;
                    break;
                case CTYPE_UNSIGNED:
                    rtti->data.ld = va_arg( ap, unsigned int );
                    rtti->buf = &rtti->data.ld;
                    break;
                case CTYPE_FLOAT:
                    rtti->data.f = ( float )va_arg( ap, double );
                    rtti->buf = &rtti->data.f;
                    break;
                case CTYPE_DOUBLE:
                    rtti->data.lf = va_arg( ap, double );
                    rtti->buf = &rtti->data.lf;
                    break;
                case CTYPE_LONG_DOUBLE:
                    rtti->data.llf = va_arg( ap, long double );
                    rtti->buf = &rtti->data.llf;
                    break;
                case CTYPE_BYTE:
                    rtti->data.b = ( unsigned char )va_arg( ap, unsigned int );
                    rtti->buf = &rtti->data.b;
                    break;
                case CTYPE_LONG_LONG:
                    rtti->data.lld = va_arg( ap, long long int );
                    rtti->buf = &rtti->data.lld;
                    break;
                case CTYPE_UNSIGNED_LONG_LONG:
                    rtti->data.llu = va_arg( ap, unsigned long long int );
                    rtti->buf = &rtti->data.llu;
                    break;
                case CTYPE_USER_DEFINED:
                    rtti->type = va_arg( ap, MPI_Datatype );
                    rtti->data.address = va_arg( ap, void* );
                    rtti->buf = rtti->data.address;
                    break;
                default:
                    PI_ASSERT( , 0, PI_SYSTEM_ERROR );
            }
        }
        else {
            PI_ASSERT( , 0, PI_SYSTEM_ERROR );
        }

        /* Move onto the next conversion specifier. */
        s += skip;
    }

    /* The format string is nothing but whitespace. */
    PI_ASSERT( , metaIndex != 0, PI_FORMAT_ARGS );
    /* The format string contains too many arguments. */
    PI_ASSERT( , metaIndex != PI_MAX_FORMATLEN, PI_FORMAT_ARGS );

    /* End of format string -- there should now be two PI_END args */
    PI_ASSERT( LEVEL(1), PI_END1==va_arg(ap,int) && PI_END2==va_arg(ap,int),
               PI_FORMAT_ARGS )

    return metaIndex;
}
