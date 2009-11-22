/*
Initialization tests for Pilot. Namely ensure that the correct number of
processes are returned from PI_Configure().

NOTE: this suite will fail if you run it with the deadlock detector on as Pilot
will return the number of process available to the user. When the DL detector is
on, one MPI process is reserved by Pilot and hence PI_Configure will return the
number of MPI processes - 1.
*/
#include "unittests.h"

static int numPilotProcs;

void ShouldGiveCorrectNumProcs(void)
{
    int mpiProcs;
    MPI_Comm_size(MPI_COMM_WORLD, &mpiProcs);
    CU_ASSERT_EQUAL(numPilotProcs, mpiProcs);
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    numPilotProcs = PI_Configure(&argc, &argv);

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddInitSuite(void)
{
    CU_pSuite suite = CU_add_suite("Pilot Initialization Tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(suite, "Should return correct number of procs", ShouldGiveCorrectNumProcs);

    return CUE_SUCCESS;
}
