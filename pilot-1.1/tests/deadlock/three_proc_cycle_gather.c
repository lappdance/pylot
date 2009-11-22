/*!
********************************************************************************
\file three_proc_cycle_gather.c
\brief Circular wait with two processes reading and one gather.

Scenario:
	M -main_q-> Q -q_r-> R
	  <-----gatherer------
M gathers
Q reads
R reads

Result:
1) any order: "Operation creates circular wait with above processes"
*******************************************************************************/

#include <pilot.h>
#include <stddef.h>
#include <stdio.h>

PI_CHANNEL* main_q;
PI_CHANNEL* q_r;

int Q(int idx, void* ctx)
{
    int recv;
    PI_Read(main_q, "%d", &recv);
    return 0;
}

int R(int idx, void* ctx)
{
    int recv;
    PI_Read(q_r, "%d", &recv);
    return 0;
}

int main(int argc, char* argv[])
{
    PI_PROCESS* q;
    PI_PROCESS* r;
    PI_CHANNEL* chans[1];
    PI_BUNDLE* gatherer;

    PI_Configure(&argc, &argv);

    q = PI_CreateProcess(Q, 0, NULL);
    r = PI_CreateProcess(R, 0, NULL);
    main_q = PI_CreateChannel(PI_MAIN, q);
    q_r = PI_CreateChannel(q, r);
    chans[0] = PI_CreateChannel(r, PI_MAIN);
    gatherer = PI_CreateBundle(PI_GATHER, chans, 1);

    PI_StartAll();

    {
        int results[1]; // One int from one channel.
        PI_Gather(gatherer, "%*d", 1, results);
    }

    PI_StopMain(0);
    return 0;
}
