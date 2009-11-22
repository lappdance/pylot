/*!
********************************************************************************
\file three_unsatisfiable_select.c
\brief Selecting on three processes that cannot write.

Scenario:
	M -main_p-> P
	  -main_q-> Q
	  -main_r-> R
	  <-selector-

M writes to P, Q, R
P, Q, R all read

M selects on P, Q, R
P reads
Q reads
R reads

The first wave of write/read is for synchronization, but it doesn't affect
that differing execution orders will produce two results.

Result:
1) M selects before some read: "Earlier select cannot be fulfilled"
2) P, Q, R all read before select: "Select cannot be fulfilled"
*******************************************************************************/

#include <pilot.h>
#include <stddef.h>


int worker(int idx, void* ctx)
{
    PI_CHANNEL** chan = (PI_CHANNEL**)ctx;
    int recv;
    if (chan && *chan) {
        /* Synchronizing read first. */
        PI_Read(*chan, "%d", &recv);
        /* This read should cause the desired dl. */
        PI_Read(*chan, "%d", &recv);
    }
    return 0;
}


int main(int argc, char* argv[])
{
    PI_PROCESS *P, *Q, *R;
    PI_CHANNEL* to_main[3];
    PI_CHANNEL* main_p;
    PI_CHANNEL* main_q;
    PI_CHANNEL* main_r;
    PI_BUNDLE* sel;

    PI_Configure(&argc, &argv);

    P = PI_CreateProcess(worker, 0, &main_p); PI_SetName(P, "P");
    Q = PI_CreateProcess(worker, 0, &main_q); PI_SetName(Q, "Q");
    R = PI_CreateProcess(worker, 0, &main_r); PI_SetName(R, "R");

    main_p = PI_CreateChannel(PI_MAIN, P);
    main_q = PI_CreateChannel(PI_MAIN, Q);
    main_r = PI_CreateChannel(PI_MAIN, R);

    to_main[0] = PI_CreateChannel(P, PI_MAIN);
    to_main[1] = PI_CreateChannel(Q, PI_MAIN);
    to_main[2] = PI_CreateChannel(R, PI_MAIN);
    sel = PI_CreateBundle(PI_SELECT, to_main, 3);

    PI_StartAll();

    /* P, Q and R are waiting. Write to them to tell them to advance. */
    PI_Write(main_p, "%d", 1);
    PI_Write(main_q, "%d", 2);
    PI_Write(main_r, "%d", 3);

    PI_Select(sel);

    PI_StopMain(0);
    return 0;
}
