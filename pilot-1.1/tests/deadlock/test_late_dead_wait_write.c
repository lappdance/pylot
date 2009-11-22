/*!
********************************************************************************
\file test_late_dead_wait_write.c
\brief Writing on channel where other end is dead.

Scenario:
	B -chan-> A
A waits 3 secs, then exits
B writes

The wait attempts to force result #1, but cannot be certain.

Result:
1) B writes first: "Process exiting leaves earlier operation hung"
2) A exits first: "Process at other end of channel has exited"
*******************************************************************************/

#include <pilot.h>
#include <stdlib.h>
#include <unistd.h>

PI_CHANNEL *chan;

int a_worker(int idx, void *p)
{
    /* Hang around for a few seconds before exiting. */
    sleep(3);
    return 0;
}

int b_worker(int idx, void *p)
{
    PI_Write(chan, "%d", 0x10);
    return 0;
}

int main(int argc, char *argv[])
{
    PI_PROCESS *a;
    PI_PROCESS *b;

    PI_Configure(&argc, &argv);

    a = PI_CreateProcess(a_worker, 0, NULL);
    b = PI_CreateProcess(b_worker, 0, NULL);
    chan = PI_CreateChannel(b, a);

    PI_StartAll();

    PI_StopMain(0);
    return 0;
}
