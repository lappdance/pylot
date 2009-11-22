/*
Entry point of the test suite. If you wish to add a new suite, simply add the
suite's registration function to the array of SuiteRegisterFuncs below.

See "unittests.h" for the def'n of SuiteRegisterFunc.
*/

#include "unittests.h"
#include <stdio.h>

const int default_argc = 1;
static char* _argv[] = { "unittests" };
char** const default_argv = _argv;

// Add new suite registration functions to this list.
static SuiteRegisterFunc suites[] = {
    AddInitSuite,
    AddSingleRWSuite,
    AddArrayRWSuite,
    AddMixedValueSuite,
    AddSelectorSuite,
    AddBroadcasterSuite,
    AddGathererSuite,
    AddExtraReadWriteSuite,
    AddFormatSuite,

    NULL,
};

int main(int argc, char* argv[])
{
    int rank, numProcs;
    int i;
    CU_ErrorCode err = CUE_SUCCESS;

    // Calling MPI_Init will put Pilot into "Bench Mode".
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    if (numProcs < NUM_REQUIRED_PROCS) {
        if (rank == 0) {
            fprintf(stderr,
                "Error:\n"
                "  There are not enough MPI processes available to run the unit tests.\n"
                "  Please specifiy a number >= %d.\n",
                NUM_REQUIRED_PROCS);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();

    // Add all the suites
    for (i = 0; suites[i] != NULL; i++) {
        err = (suites[i])();
        if (err != CUE_SUCCESS) goto CLEANUP;
    }

    // Start the tests
    CU_BasicRunMode runMode =
            rank == 0 ? CU_BRM_VERBOSE : CU_BRM_SILENT;

    CU_basic_set_mode(runMode);
    CU_basic_run_tests();

CLEANUP:
    CU_cleanup_registry();
    MPI_Finalize();
    return err;
}

PI_PROCESS* CreateAliasedProcess(PI_WORK_FUNC f, const char *alias, int index,
                                 void *opt_pointer)
{
    PI_PROCESS* proc = PI_CreateProcess(f, index, opt_pointer);
    PI_SetName(proc, alias);
    return proc;
}

static void dummyTest(void)
{}

void AddTest(CU_pSuite suite, const char* name, CU_TestFunc func)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // We only want to run the tests on Master. All the other procs will be
    // participating in the tests, but should not be explicitly starting them.
    //
    // If this isn't rank 0, a dummy test function is added to make sure that
    // the suite's init and cleanup functions still get called.
    if (rank == 0)
        CU_add_test(suite, name, func);
    else
        CU_add_test(suite, name, dummyTest);
}
