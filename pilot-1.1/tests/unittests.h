#ifndef UNITTESTS_H
#define UNITTESTS_H

#include <pilot.h>
#include <pilot_error.h>
#include <CUnit/Basic.h>
#include <stdlib.h>
#include <mpi.h>

/*
The number of required processes for the tests to run successfully. This number
is the maximum number of processes required by all test suites plus one process
for PI_MAIN.

That is, NUM_REQUIRED_PROCS should be:

    max(process_count(s) for all s in suites) + 1

If your suite uses more than the number below, you will have to increase this
number.
*/
#define NUM_REQUIRED_PROCS 8

typedef CU_ErrorCode (*SuiteRegisterFunc)(void);

extern int PI_GetMyRank(void);

extern const int default_argc;
extern char** const default_argv;

/*== Helper functions ==*/
PI_PROCESS* CreateAliasedProcess(PI_WORK_FUNC f, const char* alias, int index, void* opt_pointer);

/* Use AddTest() instead of CU_add_test() (it works on all nodes). */
void AddTest(CU_pSuite suite, const char* name, CU_TestFunc func);

/*== Suite registration functions ==*/
CU_ErrorCode AddSingleRWSuite(void);
CU_ErrorCode AddArrayRWSuite(void);
CU_ErrorCode AddMixedValueSuite(void);
CU_ErrorCode AddSelectorSuite(void);
CU_ErrorCode AddBroadcasterSuite(void);
CU_ErrorCode AddGathererSuite(void);
CU_ErrorCode AddExtraReadWriteSuite(void);
CU_ErrorCode AddFormatSuite(void);
CU_ErrorCode AddInitSuite(void);

#endif /* UNITTESTS_H */
