#include <stdio.h>
#include <pilot.h>

PI_CHANNEL *chans[2];
PI_CHANNEL *chans_out[2];
PI_PROCESS *procs[2];

int WorkFunc( int q, void *p ) {
    int i, sum;
    int array[10];

    PI_Read( chans[q], "%10d", array );

    sum = 0;
    for ( i = 0; i < 10; i++ ) {
        sum += array[i];
    }

    PI_Write( chans_out[q], "%d", sum );

    return 0;
}

int main( int argc, char *argv[] ) {
    int i;
    int array_a[10];
    int array_b[10];
    int sum_a, sum_b;

    for ( i = 0; i < 10; i++ ) {
        array_a[i] = i + 2;
        array_b[i] = i + 3;
    }

    PI_Configure( &argc, &argv );

    for ( i = 0; i < 2; i++ ) {
        procs[i] = PI_CreateProcess( WorkFunc, i, NULL );
        chans[i] = PI_CreateChannel( PI_MAIN, procs[i] );
        chans_out[i] = PI_CreateChannel( procs[i], PI_MAIN );
    }

    PI_StartAll();

    PI_Write( chans[0], "%10d", array_a );
    PI_Write( chans[1], "%10d", array_b );

    PI_Read( chans_out[0], "%d", &sum_a );
    PI_Read( chans_out[1], "%d", &sum_b );

    printf( "\nThe sum of A is: %d.\nThe sum of B is: %d.\n", sum_a, sum_b );

    PI_StopMain( 0 );
    return 0;
}
