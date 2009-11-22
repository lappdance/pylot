/*!
********************************************************************************
\file test_dead_wait_select.c
\brief Selecting on bundle where other end is dead.

Scenario:
	A -selector-> B
A exits
B selects

Result:
1) A exits first: "Select cannot be fulfilled"
2) B reads first: "Process exiting leaves earlier operation hung"
*******************************************************************************/

#include <pilot.h>
#include <stdlib.h>
#include <unistd.h>

PI_BUNDLE *selector;

int a_worker(int idx, void *p)
{
    return 0;
}

int b_worker(int idx, void *p)
{
    PI_Select(selector);
    return 0;
}

int main(int argc, char *argv[])
{
    PI_PROCESS *a;
    PI_PROCESS *b;
    PI_CHANNEL *channels[1];

    PI_Configure(&argc, &argv);

    a = PI_CreateProcess(a_worker, 0, NULL);
    b = PI_CreateProcess(b_worker, 0, NULL);
    channels[0] = PI_CreateChannel(a, b);

    selector = PI_CreateBundle(PI_SELECT, channels, 1);

    PI_StartAll();

    PI_StopMain(0);
    return 0;
}
