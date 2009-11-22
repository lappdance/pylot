/*!
********************************************************************************
\file three_proc_cycle_read.c
\brief Circular wait with three processes reading.

Scenario:
	M -main_q-> Q -q_r-> R
	  <-----r_main------
M reads
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
PI_CHANNEL* r_main;


int proc_q(int idx, void* ctx)
{
    int recv;
    PI_Read(main_q, "%d", &recv);
    return 0;
}


int proc_r(int idx, void* ctx)
{
    int recv;
    PI_Read(q_r, "%d", &recv);
    return 0;
}


int main(int argc, char* argv[])
{
    PI_PROCESS* q;
    PI_PROCESS* r;
    int recv;

    PI_Configure(&argc, &argv);

    q = PI_CreateProcess(proc_q, 0, NULL);
    r = PI_CreateProcess(proc_r, 0, NULL);
    main_q = PI_CreateChannel(PI_MAIN, q);
    q_r = PI_CreateChannel(q, r);
    r_main = PI_CreateChannel(r, PI_MAIN);

    PI_StartAll();

    PI_Read(r_main, "%d", &recv);

    PI_StopMain(0);
    return 0;
}
