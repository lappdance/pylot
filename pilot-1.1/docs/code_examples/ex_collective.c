#include <stdio.h>
#include <pilot.h>

#define NUM_PROCS  2
#define NUM_CHANS  2

PI_CHANNEL *chans[ NUM_CHANS ];
PI_CHANNEL *chans_out[ NUM_CHANS ];
PI_PROCESS *procs[ NUM_PROCS ];
PI_BUNDLE *gather_bundle;
PI_BUNDLE *broadcast_bundle;

int WorkFunc( int q, void *p ) {
    int i;
    int array[5];

    PI_Read( chans[q], "%5d", array );

    for ( i = 0; i < 5; i++ ) {
        array[i] = array[i] + q;
    }

    PI_Write( chans_out[q], "%5d", array );
    return 0;
}

int main( int argc, char *argv[] ) {
    int i, sum;
    int send[5];
    int recv[5 * NUM_CHANS];
    float avg;

    for ( i = 0; i < 5; i++ ) {
        send[i] = i + 1;
    }

    PI_Configure( &argc, &argv );

    for ( i = 0; i < NUM_PROCS; i++ ) {
        procs[i] = PI_CreateProcess( WorkFunc, i, NULL );
        chans[i] = PI_CreateChannel( PI_MAIN, procs[i] );
        chans_out[i] = PI_CreateChannel( procs[i], PI_MAIN );
    }
    broadcast_bundle = PI_CreateBundle( PI_BROADCAST, chans, NUM_CHANS );
    gather_bundle = PI_CreateBundle( PI_GATHER, chans_out, NUM_CHANS );

    PI_StartAll();

    PI_Broadcast( broadcast_bundle, "%5d", send );
    PI_Gather( gather_bundle, "%5d", recv );

    sum = 0;
    for ( i = 0; i < 5 * NUM_CHANS; i++ ) {
        sum += recv[i];
    }
    avg = sum / ( 5.0f * NUM_CHANS );

    printf( "\nThe average was: %f\n", avg );

    PI_StopMain( 0 );
    return 0;
}
