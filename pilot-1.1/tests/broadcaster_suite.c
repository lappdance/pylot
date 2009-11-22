/*
Tests for PI_Broadcast. Ensures that PI_Broadcast can broadcast to at least 4
worker processes.
*/
#include "unittests.h"

PI_PROCESS *test6_1, *test6_2, *test6_3, *test6_4;
PI_CHANNEL *from_test6[4];
PI_CHANNEL *to_test6[4];
PI_BUNDLE *test6_bundle;

int broadcast_read(int q, void *p) {

    float r[1];

    PI_Read(from_test6[q],"%*f", 1, r);
    PI_Write(to_test6[q],"%*f", 1, r);
    return 0;
}

void test6(void) {

    float ff[1];
    ff[0] = 99.1012;
    float read[1];
    int i;

    PI_Broadcast(test6_bundle,"%*f", 1, ff);

    for (i = 0; i < 4; i++) {
        PI_Read(to_test6[i],"%*f", 1, read);
        CU_ASSERT_DOUBLE_EQUAL(ff[0],read[0],0.00001);
    }

}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    test6_1 = CreateAliasedProcess(broadcast_read,"test6",0,NULL);
    test6_2 = CreateAliasedProcess(broadcast_read,"test6",1,NULL);
    test6_3 = CreateAliasedProcess(broadcast_read,"test6",2,NULL);
    test6_4 = CreateAliasedProcess(broadcast_read,"test6",3,NULL);

    from_test6[0] = PI_CreateChannel(PI_MAIN,test6_1);
    to_test6[0] = PI_CreateChannel(test6_1,PI_MAIN);

    from_test6[1] = PI_CreateChannel(PI_MAIN,test6_2);
    to_test6[1] = PI_CreateChannel(test6_2,PI_MAIN);

    from_test6[2] = PI_CreateChannel(PI_MAIN,test6_3);
    to_test6[2] = PI_CreateChannel(test6_3,PI_MAIN);

    from_test6[3] = PI_CreateChannel(PI_MAIN,test6_4);
    to_test6[3] = PI_CreateChannel(test6_4,PI_MAIN);

    test6_bundle = PI_CreateBundle(PI_BROADCAST, from_test6,4);
    PI_SetName(test6_bundle, "test6 broadcaster");

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddBroadcasterSuite(void)
{
    CU_pSuite suite = CU_add_suite("Broadcaster Tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(suite, "broadcaster tests", test6);

    return CUE_SUCCESS;
}
