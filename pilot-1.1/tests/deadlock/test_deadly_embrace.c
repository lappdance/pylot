/*!
********************************************************************************
\file test_deadly_embrace.c
\brief Two processes read from each other.

Scenario:
	A -chan-> B
	  <-chan-
A reads
B reads

Result:
1) either order: "Conflicting channels create deadly embrace"
*******************************************************************************/

#include <pilot.h>
#include <stddef.h>

PI_CHANNEL* a_to_b;
PI_CHANNEL* b_to_a;

int process_a(int p, void* q)
{
    int recv;
    PI_Read(b_to_a, "%d", &recv);
    return 0;
}

int process_b(int p, void* q)
{
    int recv;
    PI_Read(a_to_b, "%d", &recv);
    return 0;
}

int main(int argc, char* argv[])
{
    PI_PROCESS* a;
    PI_PROCESS* b;

    PI_CheckLevel = 1;
    PI_Configure(&argc, &argv);

    a = PI_CreateProcess(process_a, 0, NULL);
    b = PI_CreateProcess(process_b, 0, NULL);

    a_to_b = PI_CreateChannel(a, b);
    b_to_a = PI_CreateChannel(b, a);

    PI_StartAll();

    PI_StopMain(0);
    return 0;
}
