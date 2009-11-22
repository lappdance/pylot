/*!
********************************************************************************
\file three_proc_cycle_select.c
\brief Circular wait with three processes reading.

Scenario:
	M -main_q-> Q -q_r-> R
	  <-selector-
	  <-----selector------

M selects on Q and R
Q reads
R reads

Result:
1) M selects before both Q and R read:
   "Operation creates circular wait with above processes"
2) Q and R read first: "Select cannot be fulfilled"
*******************************************************************************/

#include <pilot.h>
#include <stddef.h>
#include <stdio.h>


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

PI_CHANNEL* q_r;
PI_CHANNEL* s_q;
PI_CHANNEL* main_s;


int proc_q(int idx, void* ctx)
{
    int recv;
    PI_Read(s_q, "%d", &recv);
    return 0;
}


int proc_r(int idx, void* ctx)
{
    int recv;
    PI_Read(q_r, "%d", &recv);
    return 0;
}


int proc_s(int idx, void* ctx)
{
    int recv;
    PI_Read(main_s, "%d", &recv);
    return 0;
}


int main(int argc, char* argv[])
{
    PI_PROCESS* q;
    PI_PROCESS* r;
    PI_PROCESS* s;
    PI_CHANNEL* channels[2];

    PI_BUNDLE* selector;
    int i;

    PI_Configure(&argc, &argv);

    q = PI_CreateProcess(proc_q, 0, NULL);
    r = PI_CreateProcess(proc_r, 0, NULL);
    s = PI_CreateProcess(proc_s, 0, NULL);

    /* A cycle is created between main, s and q. */
    q_r = PI_CreateChannel(q, r);
    s_q = PI_CreateChannel(s, q);
    main_s = PI_CreateChannel(PI_MAIN, s);
    channels[0] = PI_CreateChannel(q, PI_MAIN);
    channels[1] = PI_CreateChannel(r, PI_MAIN);

    selector = PI_CreateBundle(PI_SELECT, channels, ARRAY_SIZE(channels));

    PI_StartAll();

    for (i = 0; i < ARRAY_SIZE(channels); i++) {
        int recv;
        int chan_idx = PI_Select(selector);
        PI_Read(channels[chan_idx], "%d", &recv);
    }
    PI_Write(main_s, "%d", 1);

    PI_StopMain(0);
    return 0;
}
