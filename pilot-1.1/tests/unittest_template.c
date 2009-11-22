#include "unittests.h"

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    // Add your initialization code here.

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

#error "Name your suite appropriately"
CU_ErrorCode AddMySuite(void)
{
    // Make sure you give your suite a unique name.
    CU_pSuite suite = CU_add_suite("", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    // Put calls to AddTest() here...

    return CUE_SUCCESS;
}
