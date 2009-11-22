#include <stdio.h>
#include <pilot.h>

PI_CHANNEL *chans[2];
PI_PROCESS *procs[2];

int WorkFunc( int q, void *p ) {
    int x, y, sum;

    PI_Read( chans[q], "%d%d", &x, &y );
    sum = x + y;
    printf( "The sum of %d and %d is %d\n", x, y, sum );

    return 0;
}

int main( int argc, char *argv[] ) {
    int i;

    PI_Configure( &argc, &argv );

    for ( i = 0; i < 2; i++ ) {
        procs[i] = PI_CreateProcess( WorkFunc, i, NULL );
        chans[i] = PI_CreateChannel( PI_MAIN, procs[i] );
    }

    PI_StartAll();

    PI_Write( chans[0], "%d%d", 50, 25 );
    PI_Write( chans[1], "%d%d", -100, 25 );

    PI_StopMain( 0 );
    return 0;
}
