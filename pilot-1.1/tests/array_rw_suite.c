/*
Tests for PI_Read and PI_Write involving arrays. This suite tests the "%num" and
"%*" syntax and validates that the entire array is sent successfully via MPI.
*/
#include "unittests.h"

PI_PROCESS *test3a_proc;
PI_CHANNEL *to_test3a,*from_test3a;

PI_PROCESS *test3b_proc;
PI_CHANNEL *to_test3b,*from_test3b;

PI_PROCESS *test3c_proc;
PI_CHANNEL *to_test3c,*from_test3c;

PI_PROCESS *test3d_proc;
PI_CHANNEL *to_test3d,*from_test3d;

PI_PROCESS *test3e_proc;
PI_CHANNEL *to_test3e, *from_test3e;

int array_int(int q,void *p) {

    int temp[20];

    PI_Read(to_test3a,"%20d",temp);
    PI_Write(from_test3a,"%20d",temp);
    return 0;
}

void test3a(void) {

    int echo[20];
    int back[20];
    int i;

    for (i = 0; i < 20; i++) {
        echo[i] = (i + 5) % 2;
    }

    PI_Write(to_test3a,"%20d",echo);
    PI_Read(from_test3a,"%20d",back);

    for (i = 0; i < 20; i++) {
        CU_ASSERT(echo[i] == back[i]);
    }
}

int array_char(int q,void *p) {

    char temp[20];

    PI_Read(to_test3b,"%20c",temp);
    PI_Write(from_test3b,"%20c",temp);
    return 0;
}

void test3b(void) {

    char echo[20];
    char back[20] = "";

    strcpy(echo,"AAAABBBBCCCCDDDDEEE");

    PI_Write(to_test3b,"%20c",echo);
    PI_Read(from_test3b,"%20c",back);

    CU_ASSERT_NSTRING_EQUAL(echo,back,20);
}

int array_float(int q,void *p) {

    float temp[20];

    PI_Read(to_test3c,"%20f",temp);
    PI_Write(from_test3c,"%20f",temp);
    return 0;
}

void test3c(void) {

    float echo[20];
    float back[20];
    int i;

    for (i = 0; i < 20; i++) {
        echo[i] = i * i * 0.025;
    }

    PI_Write(to_test3c,"%20f",echo);
    PI_Read(from_test3c,"%20f",back);

    for (i = 0; i < 20; i++) {
        CU_ASSERT_DOUBLE_EQUAL(back[i],echo[i],0.00001)
    }
}

int array_double(int q,void *p) {

    double temp[20];

    PI_Read(to_test3d,"%20lf",temp);
    PI_Write(from_test3d,"%20lf",temp);
    return 0;
}

void test3d(void) {

    double echo[20];
    double back[20];
    int i;

    for (i = 0; i < 20; i++) {
        echo[i] = i * i * 3.0000001;
    }

    PI_Write(to_test3d,"%20lf",echo);
    PI_Read(from_test3d,"%20lf",back);

    for (i = 0; i < 20; i++) {
        CU_ASSERT_DOUBLE_EQUAL(back[i],echo[i],0.0000000001)
    }

}

int array_mpi_type(int arg1, void *arg2) {
    long temp[20];

    PI_Read(to_test3e, "%20m", MPI_LONG, temp);
    PI_Write(from_test3e, "%*m", 20, MPI_LONG, temp);
    return 0;
}

void test3e(void) {
    long echo[20];
    long back[20];
    int i;

    for (i = 0; i < 20; i++)
        echo[i] = i * (i << 1);

    PI_Write(to_test3e, "%20m", MPI_LONG, echo);
    PI_Read(from_test3e, "%*m", 20, MPI_LONG, back);

    for (i = 0; i < 20; i++)
        CU_ASSERT_EQUAL(back[i], echo[i]);
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    test3a_proc = CreateAliasedProcess(array_int,"test3a",0,NULL);
    to_test3a = PI_CreateChannel(PI_MAIN,test3a_proc);
    from_test3a = PI_CreateChannel(test3a_proc,PI_MAIN);

    test3b_proc = CreateAliasedProcess(array_char,"test3b",0,NULL);
    to_test3b = PI_CreateChannel(PI_MAIN,test3b_proc);
    from_test3b = PI_CreateChannel(test3b_proc,PI_MAIN);

    test3c_proc = CreateAliasedProcess(array_float,"test3c",0,NULL);
    to_test3c = PI_CreateChannel(PI_MAIN,test3c_proc);
    from_test3c = PI_CreateChannel(test3c_proc,PI_MAIN);

    test3d_proc = CreateAliasedProcess(array_double,"test3d",0,NULL);
    to_test3d = PI_CreateChannel(PI_MAIN,test3d_proc);
    from_test3d = PI_CreateChannel(test3d_proc,PI_MAIN);

    test3e_proc = CreateAliasedProcess(array_mpi_type,"test3e",0,NULL);
    to_test3e = PI_CreateChannel(PI_MAIN,test3e_proc);
    from_test3e = PI_CreateChannel(test3e_proc,PI_MAIN);

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddArrayRWSuite(void)
{
    CU_pSuite suite = CU_add_suite("Pilot Array Read/Write Tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(suite, "array int send/echo", test3a);
    AddTest(suite, "array char send/echo", test3b);
    AddTest(suite, "array float send/echo", test3c);
    AddTest(suite, "array double send/echo", test3d);
    AddTest(suite, "array mpi datatype send/echo", test3e);

    return CUE_SUCCESS;
}
