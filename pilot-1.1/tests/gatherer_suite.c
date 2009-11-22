/*
Tests for PI_Gather.

Tests that:
 - gathering is possible from 3 processes.
 - gathering a large array (> 10000 ints) does not cause problems.
 - it is possible to gather on a process other than PI_MAIN.
*/
#include "unittests.h"
#include <stdio.h>

PI_PROCESS *test7_1, *test7_2, *test7_3;
PI_CHANNEL *to_test7[3];
PI_BUNDLE *test7_bundle;

PI_PROCESS *test7_4;
PI_CHANNEL *test7_large_array_channels[1];
PI_BUNDLE *test7_large_array_bundle;

int gather_write(int q, void *p) {

    char c[1];

    c[0] = 65 + q;

    PI_Write(to_test7[q],"%*c", 1, c);
    return 0;
}

void test7a(void) {
    char cc[3];

    PI_Gather(test7_bundle,"%*c", 1, cc);

    CU_ASSERT_EQUAL(cc[0],65);
    CU_ASSERT_EQUAL(cc[1],66);
    CU_ASSERT_EQUAL(cc[2],67);
}

int gather_array_write(int idx, void* arg2)
{
    int i;
    int len = 10001;
    int* ints = malloc(sizeof(int) * len);
    if (ints == NULL) {
        perror("gather_array_write()");
        return -1;
    }
    for (i = 0; i < len; i++)
        ints[i] = i * i;
    PI_Write(test7_large_array_channels[idx], "%*d", len, ints);
    free(ints);
    return 0;
}

void test7b(void)
{
    int i;
    int len = 10001;
    int* ints = malloc(sizeof(int) * len);
    if (ints == NULL) {
        CU_FAIL_FATAL("Unable to allocate memory for test7b");
    }
    PI_Gather(test7_large_array_bundle, "%*d", len, ints);
    for (i = 0; i < len; i++)
        CU_ASSERT_EQUAL(ints[i], i * i);
    free(ints);
}

PI_PROCESS *test7c_w[2], *test7c_nonmain;
PI_CHANNEL *to_nonmain_gatherer[2], *from_nonmain_gatherer;
PI_BUNDLE  *nonmain_bundle;

int test7c_writer(int proc_id, void* arg2)
{
    int ints[5];
    int i;
    for (i = 0; i < 5; i++)
        ints[i] = proc_id + 'A';
    PI_Write(to_nonmain_gatherer[proc_id], "%5d", ints);
    return 0;
}

int nonmain_gatherer(int arg1, void* arg2)
{
    int ints[10] = {0};
    int i, success = 1;
    PI_Gather(nonmain_bundle, "%*d", 5, ints);
    for (i = 0; i < 10; i++) {
        if (ints[i] != (i / 5) + 'A') {
            success = 0;
            break;
        }
    }

    PI_Write(from_nonmain_gatherer, "%d", success);
    return success;
}

/* non-PI_MAIN gatherer */
void test7c(void)
{
    int success;
    PI_Read(from_nonmain_gatherer, "%d", &success);
    CU_ASSERT(success);
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    test7_1 = CreateAliasedProcess(gather_write,"test7",0,NULL);
    test7_2 = CreateAliasedProcess(gather_write,"test7",1,NULL);
    test7_3 = CreateAliasedProcess(gather_write,"test7",2,NULL);

    to_test7[0] = PI_CreateChannel(test7_1,PI_MAIN);
    to_test7[1] = PI_CreateChannel(test7_2,PI_MAIN);
    to_test7[2] = PI_CreateChannel(test7_3,PI_MAIN);

    test7_bundle = PI_CreateBundle(PI_GATHER, to_test7,3);
    PI_SetName(test7_bundle, "test7 gatherer");

    test7_4 = CreateAliasedProcess(gather_array_write, "test7_4", 0, NULL);
    test7_large_array_channels[0] = PI_CreateChannel(test7_4, PI_MAIN);
    test7_large_array_bundle = PI_CreateBundle(PI_GATHER, test7_large_array_channels, 1);

    test7c_w[0] = CreateAliasedProcess(test7c_writer, "test7c_writer0", 0, NULL);
    test7c_w[1] = CreateAliasedProcess(test7c_writer, "test7c_writer1", 1, NULL);
    test7c_nonmain = CreateAliasedProcess(nonmain_gatherer, "nonmain_gatherer", 0, NULL);
    to_nonmain_gatherer[0] = PI_CreateChannel(test7c_w[0], test7c_nonmain);
    to_nonmain_gatherer[1] = PI_CreateChannel(test7c_w[1], test7c_nonmain);
    nonmain_bundle = PI_CreateBundle(PI_GATHER, to_nonmain_gatherer, 2);
    from_nonmain_gatherer = PI_CreateChannel(test7c_nonmain, PI_MAIN);

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddGathererSuite(void)
{
    CU_pSuite suite = CU_add_suite("Gatherer Tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(suite, "gatherer tests", test7a);
    AddTest(suite, "gatherer large array", test7b);
    AddTest(suite, "non-main gatherer", test7c);

    return CUE_SUCCESS;
}
