#include <stdio.h>
#include <pilot.h>

PI_CHANNEL *chan_B, *chan_C;
PI_PROCESS *B, *C;

int b( int q, void *p ) {
    printf( "I AM Process B -- my subscript is %d\n", q );
    return 0;
}

int c( int q, void *p ) {
    printf( "I AM Process C -- my subscript is %d\n", q );
    return 0;
}

int main( int argc,char *argv[] ) {
    PI_Configure( &argc, &argv );

    B = PI_CreateProcess( b, 2, NULL );
    C = PI_CreateProcess( c, 3, NULL );
    chan_B = PI_CreateChannel( PI_MAIN, B );
    chan_C = PI_CreateChannel( PI_MAIN, C );

    PI_StartAll();

    printf( "I AM MAIN!\n" );

    PI_StopMain( 0 );
    return 0;
}
