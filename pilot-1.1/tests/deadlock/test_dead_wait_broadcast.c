/*!
********************************************************************************
\file test_dead_wait_broadcast.c
\brief Waiting on broadcast channel where broadcaster is dead.

Scenario:
	M --> P
	  --> Q
M exits
P & Q read

Result:
1) M exits first: "Process at other end of channel has exited"
2) P and/or Q read first: "Process exiting leaves earlier operation hung"
*******************************************************************************/

#include <pilot.h>
#include <stddef.h>
#include <stdio.h>

int worker(int idx, void* ctx)
{
    PI_CHANNEL** chans = (PI_CHANNEL**)ctx;
    int recv;
    PI_Read(chans[idx], "%d", &recv);
    printf("proc %d read 0x%02x from main\n", idx, recv);
    return 0;
}

int main(int argc, char* argv[])
{
    PI_CHANNEL* channels[2];
    PI_PROCESS* p;
    PI_PROCESS* q;
    PI_BUNDLE* broadcaster;

    PI_Configure(&argc, &argv);

    p = PI_CreateProcess(worker, 0, channels);
    q = PI_CreateProcess(worker, 1, channels);

    channels[0] = PI_CreateChannel(PI_MAIN, p);
    channels[1] = PI_CreateChannel(PI_MAIN, q);
    broadcaster = PI_CreateBundle(PI_BROADCAST, channels, 2);

    PI_StartAll();

    /* Without this broadcast, the program should deadlock. */
    /* PI_Broadcast(broadcaster, "%d", 0x10); */

    PI_StopMain(0);
    return 0;
}
