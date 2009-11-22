/*
Unit tests for PI_Write and PI_Read testing a single value.
*/
#include "unittests.h"

PI_PROCESS *test2a_proc;
PI_CHANNEL *to_test2a,*from_test2a;

PI_PROCESS *test2b_proc;
PI_CHANNEL *to_test2b,*from_test2b;

PI_PROCESS *test2c_proc;
PI_CHANNEL *to_test2c,*from_test2c;

PI_PROCESS *test2d_proc;
PI_CHANNEL *to_test2d,*from_test2d;

PI_PROCESS *test2e_proc;
PI_CHANNEL *to_test2e, *from_test2e;

int single_int(int q,void *p) {

    int temp;

    PI_Read(to_test2a,"%d",&temp);
    PI_Write(from_test2a,"%d",temp);
    return 0;
}

void test2a(void) {

    int echo = 775;
    int back;

    PI_Write(to_test2a,"%d",echo);
    PI_Read(from_test2a,"%d",&back);

    CU_ASSERT(back == 775);
}

int single_char(int q,void *p) {

    char temp;

    PI_Read(to_test2b,"%c",&temp);
    PI_Write(from_test2b,"%c",temp);
    return 0;
}

void test2b(void) {

    char echo = '@';
    char back;

    PI_Write(to_test2b,"%c",echo);
    PI_Read(from_test2b,"%c",&back);

    CU_ASSERT(back == '@');
}

int single_float(int q,void *p) {

    float temp;

    PI_Read(to_test2c,"%f",&temp);
    PI_Write(from_test2c,"%f",temp);
    return 0;
}

void test2c(void) {

    float echo = 123.456;
    float back;

    PI_Write(to_test2c,"%f",echo);
    PI_Read(from_test2c,"%f",&back);

    CU_ASSERT_DOUBLE_EQUAL(back,echo,0.00001);
}

int single_double(int q,void *p) {

    double temp;

    PI_Read(to_test2d,"%lf",&temp);
    PI_Write(from_test2d,"%lf",temp);
    return 0;
}

void test2d(void) {

    double echo = 99.121312;
    double back;

    PI_Write(to_test2d,"%lf",echo);
    PI_Read(from_test2d,"%lf",&back);

    CU_ASSERT_DOUBLE_EQUAL(back,echo,0.0000000001);
}

int single_mpi_type(int arg1, void *arg2) {
    long double temp;

    PI_Read(to_test2e, "%m", MPI_LONG_DOUBLE, &temp);
    PI_Write(from_test2e, "%m", MPI_LONG_DOUBLE, &temp);
    return 0;
}

void test2e(void) {
    long double echo = 2142.314159;
    long double back;

    PI_Write(to_test2e, "%m", MPI_LONG_DOUBLE, &echo);
    PI_Read(from_test2e, "%m", MPI_LONG_DOUBLE, &back);

    CU_ASSERT_DOUBLE_EQUAL(back, echo, 0.0000000001);
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    test2a_proc = CreateAliasedProcess(single_int,"test2a",0,NULL);
    to_test2a = PI_CreateChannel(PI_MAIN,test2a_proc);
    from_test2a = PI_CreateChannel(test2a_proc,PI_MAIN);

    test2b_proc = CreateAliasedProcess(single_char,"test2b",0,NULL);
    to_test2b = PI_CreateChannel(PI_MAIN,test2b_proc);
    from_test2b = PI_CreateChannel(test2b_proc,PI_MAIN);

    test2c_proc = CreateAliasedProcess(single_float,"test2c",0,NULL);
    to_test2c = PI_CreateChannel(PI_MAIN,test2c_proc);
    from_test2c = PI_CreateChannel(test2c_proc,PI_MAIN);

    test2d_proc = CreateAliasedProcess(single_double,"test2d",0,NULL);
    to_test2d = PI_CreateChannel(PI_MAIN,test2d_proc);
    from_test2d = PI_CreateChannel(test2d_proc,PI_MAIN);

    test2e_proc = CreateAliasedProcess(single_mpi_type, "test2e", 0, NULL);
    to_test2e = PI_CreateChannel(PI_MAIN, test2e_proc);
    from_test2e = PI_CreateChannel(test2e_proc, PI_MAIN);

    PI_StartAll();

    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddSingleRWSuite(void)
{
    CU_pSuite suite = CU_add_suite("Single read/write tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(suite, "single int send/echo", test2a);
    AddTest(suite, "single char send/echo", test2b);
    AddTest(suite, "single float send/echo", test2c);
    AddTest(suite, "single double send/echo", test2d);
    AddTest(suite, "single mpi type send/echo", test2e);

    return CUE_SUCCESS;
}
