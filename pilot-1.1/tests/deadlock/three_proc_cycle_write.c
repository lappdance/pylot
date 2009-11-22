/*!
********************************************************************************
\file three_proc_cycle_write.c
\brief Circular wait with three processes writing.

Scenario:
	M -main_q-> Q -q_r-> R
	  <-----r_main------
M writes
Q writes
R writes

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
    PI_Write(q_r, "%d", 1);
    return 0;
}


int proc_r(int idx, void* ctx)
{
    PI_Write(r_main, "%d", 2);
    return 0;
}


int main(int argc, char* argv[])
{
    PI_PROCESS* q;
    PI_PROCESS* r;

    PI_Configure(&argc, &argv);

    q = PI_CreateProcess(proc_q, 0, NULL);
    r = PI_CreateProcess(proc_r, 0, NULL);
    main_q = PI_CreateChannel(PI_MAIN, q);
    q_r = PI_CreateChannel(q, r);
    r_main = PI_CreateChannel(r, PI_MAIN);

    PI_StartAll();

    PI_Write(main_q, "%d", 0);

    PI_StopMain(0);
    return 0;
}
