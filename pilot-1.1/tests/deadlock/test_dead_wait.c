/*!
********************************************************************************
\file test_dead_wait.c
\brief Waiting on channel where other end is dead.

Scenario:
	A -chan-> B
A exits
B reads

Result:
1) A exits first: "Process at other end of channel has exited"
2) B reads first: "Process exiting leaves earlier operation hung"
*******************************************************************************/

#include <pilot.h>
#include <stdlib.h>

PI_CHANNEL *chan;

int a_worker(int idx, void *p)
{
    return 0;
}

int b_worker(int idx, void *p)
{
    int recv;
    PI_Read(chan, "%d", &recv);
    return 0;
}

int main(int argc, char *argv[])
{
    PI_PROCESS *a;
    PI_PROCESS *b;

    PI_Configure(&argc, &argv);

    a = PI_CreateProcess(a_worker, 0, NULL);
    b = PI_CreateProcess(b_worker, 0, NULL);
    chan = PI_CreateChannel(a, b);

    PI_StartAll();

    /* We should never get here */
    PI_StopMain(0);
    return 0;
}
