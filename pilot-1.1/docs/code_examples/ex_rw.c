#include <stdio.h>
#include <pilot.h>

PI_CHANNEL *chan_B, *chan_C;
PI_PROCESS *B, *C;

int b( int q, void *p ) {
    int x, y, sum;

    PI_Read( chan_B, "%d%d", &x, &y );
    sum = x + y;
    printf( "The sum of %d and %d is %d\n", x, y, sum );

    return 0;
}

int c( int q, void *p ) {
    float x, y, sum;

    PI_Read( chan_C, "%f%f", &x, &y );
    sum = x + y;
    printf( "The sum of %f and %f is %f\n", x, y, sum );

    return 0;
}

int main( int argc, char *argv[] ) {
    PI_Configure( &argc, &argv );

    B = PI_CreateProcess( b, 2, NULL );
    C = PI_CreateProcess( c, 3, NULL );
    chan_B = PI_CreateChannel( PI_MAIN, B );
    chan_C = PI_CreateChannel( PI_MAIN, C );

    PI_StartAll();

    PI_Write( chan_B, "%d%d", 50, 25 );
    PI_Write( chan_C, "%f%f", 2.501f, 10.00f );

    PI_StopMain( 0 );

    return 0;
}
