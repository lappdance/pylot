/*
Unit tests for PI_Select. Tests that PI_Select works with at least 3 processes.
*/
#include "unittests.h"

PI_PROCESS *test5_1, *test5_2, *test5_3;
PI_CHANNEL *from_test5[3];
PI_BUNDLE *test5_selector;

int select_write(int q, void *p) {

    int a;

    a = 78;

    PI_Write(from_test5[q],"%d",a);
    return 0;
}

void test5(void) {

    int i;
    int r;
    PI_CHANNEL *sel;

    for (i = 0; i < 3; i++) {
        int s = PI_Select(test5_selector);
        sel = PI_GetBundleChannel(test5_selector,s);
        PI_Read(sel,"%d",&r);
        CU_ASSERT(s < 3);
        CU_ASSERT_EQUAL(r,78);
    }
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    test5_1 = CreateAliasedProcess(select_write,"test5",0,NULL);
    test5_2 = CreateAliasedProcess(select_write,"test5",1,NULL);
    test5_3 = CreateAliasedProcess(select_write,"test5",2,NULL);

    from_test5[0] = PI_CreateChannel(test5_1,PI_MAIN);
    from_test5[1] = PI_CreateChannel(test5_2,PI_MAIN);
    from_test5[2] = PI_CreateChannel(test5_3,PI_MAIN);

    test5_selector = PI_CreateBundle(PI_SELECT, from_test5, 3);
    PI_SetName(test5_selector, "test5 selector");

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddSelectorSuite(void)
{
    CU_pSuite suite = CU_add_suite("Selector Tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(suite, "selector tests", test5);

    return CUE_SUCCESS;
}
