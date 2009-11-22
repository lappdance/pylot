/*
Tests for PI_Read and PI_Write where these functions are expected to abort with
a specified error code from pilot_error.h.
*/
#include "unittests.h"

PI_PROCESS *test8_r, *test8_w;
PI_CHANNEL *test8_r_chans[1], *test8_w_chans[1];
PI_BUNDLE *test8_r_bundle, *test8_w_bundle;

int test8_reader(int index, void* arg2)
{
    int recv;
    PI_Read(test8_r_chans[index], "%d", &recv);
    return 0;
}

int test8_writer(int index, void* arg2)
{
    int send = 21;
    PI_Write(test8_w_chans[index], "%d", send);
    return 0;
}

void pi_write_fail_on_bundled_channel(void)
{
    PI_Errno = 0;
    PI_Write(test8_r_chans[0], "%d", 1);

    CU_ASSERT_EQUAL(PI_Errno, PI_BUNDLED_CHANNEL);

    // unblock the reading process
    if (PI_Errno != 0)
        PI_Broadcast(test8_r_bundle, "%d", 1);
}

void pi_read_fail_on_bundled_channel(void)
{
    int recv;
    PI_Errno = 0;
    PI_Read(test8_w_chans[0], "%d", &recv);

    CU_ASSERT_EQUAL(PI_Errno, PI_BUNDLED_CHANNEL);
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    test8_r = CreateAliasedProcess(test8_reader, "test8_reader", 0, NULL);
    test8_w = CreateAliasedProcess(test8_writer, "test8_writer", 0, NULL);

    test8_r_chans[0] = PI_CreateChannel(PI_MAIN, test8_r);
    test8_r_bundle = PI_CreateBundle(PI_BROADCAST, test8_r_chans, 1);

    test8_w_chans[0] = PI_CreateChannel(test8_w, PI_MAIN);
    test8_w_bundle = PI_CreateBundle(PI_GATHER, test8_w_chans, 1);

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddExtraReadWriteSuite(void)
{
    CU_pSuite suite = CU_add_suite("Extra read/write tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(
        suite,
        "PI_Write should fail when channel is part of non-selector bundle",
        pi_write_fail_on_bundled_channel
    );

    AddTest(
        suite,
        "PI_Read should fail when channel is part of non-selector bundle",
        pi_read_fail_on_bundled_channel
    );

    return CUE_SUCCESS;
}
