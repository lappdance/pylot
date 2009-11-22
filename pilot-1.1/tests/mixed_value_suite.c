/*
Tests that PI_Read and PI_Write can safely send and receive a sequence of mixed
types. Tests scalar and array types.
*/
#include "unittests.h"

PI_PROCESS *test4a_proc;
PI_CHANNEL *to_test4a,*from_test4a;

PI_PROCESS *test4b_proc;
PI_CHANNEL *to_test4b,*from_test4b;

PI_PROCESS *test4c_proc;
PI_CHANNEL *to_test4c,*from_test4c;

int mixed_single(int q, void *p) {

    int a;
    char b;
    float c;
    double d;

    PI_Read(to_test4a,"%d%c%f%lf",&a,&b,&c,&d);

    PI_Write(from_test4a,"%d%c%f%lf",a,b,c,d);
    return 0;
}

void test4a(void) {

    int a,a_;
    char b,b_;
    float c,c_;
    double d,d_;

    a = 992;
    b = '!';
    c = 5.00232;
    d = 88.222232201;

    PI_Write(to_test4a,"%d%c%f%lf",a,b,c,d);
    PI_Read(from_test4a,"%d%c%f%lf",&a_,&b_,&c_,&d_);

    CU_ASSERT_EQUAL(a,a_);
    CU_ASSERT_EQUAL(b,b_);
    CU_ASSERT_DOUBLE_EQUAL(c,c_,0.00001);
    CU_ASSERT_DOUBLE_EQUAL(d,d_,0.0000000001);

}

int mixed_array(int q, void *p) {

    int a[5];
    char b[4];
    float c[3];
    double d[2];

    PI_Read(to_test4b,"%5d%4c%3f%2lf",a,b,c,d);

    PI_Write(from_test4b,"%5d%4c%3f%2lf",a,b,c,d);
    return 0;
}

void test4b(void) {
    int a[5],a_[5];
    char b[4],b_[4];
    float c[3],c_[3];
    double d[2],d_[2];

    a[0] = 4;
    a[1] = 2;
    a[2] = 9;
    a[3] = 12;
    a[4] = 19;
    b[0] = 'a';
    b[1] = '*';
    b[2] = 'b';
    b[3] = '&';
    c[0] = 1.001;
    c[1] = 2.001;
    c[2] = 3.003;
    d[0] = 1.11111111;
    d[1] = 2.22222222;

    PI_Write(to_test4b,"%5d%4c%3f%2lf",a,b,c,d);
    PI_Read(from_test4b,"%5d%4c%3f%2lf",a_,b_,c_,d_);

    CU_ASSERT_EQUAL(a[0],a_[0]);
    CU_ASSERT_EQUAL(a[1],a_[1]);
    CU_ASSERT_EQUAL(a[2],a_[2]);
    CU_ASSERT_EQUAL(a[3],a_[3]);
    CU_ASSERT_EQUAL(a[4],a_[4]);

    CU_ASSERT_EQUAL(b_[0],'a');
    CU_ASSERT_EQUAL(b_[1],'*');
    CU_ASSERT_EQUAL(b_[2],'b');
    CU_ASSERT_EQUAL(b_[3],'&');

    CU_ASSERT_DOUBLE_EQUAL(c[0],c_[0],0.00001);
    CU_ASSERT_DOUBLE_EQUAL(c[1],c_[1],0.00001);
    CU_ASSERT_DOUBLE_EQUAL(c[2],c_[2],0.00001);

    CU_ASSERT_DOUBLE_EQUAL(d[0],d_[0],0.0000000001);
    CU_ASSERT_DOUBLE_EQUAL(d[1],d_[1],0.0000000001);

}

int mixed_singleandarray(int q, void *p) {

    int a;
    char b;
    float c;
    double d;

    int array_a[2];
    char array_b[2];
    float array_c[3];
    double array_d[4];

    PI_Read(to_test4c,"%d%2d%c%2c%f%2f%lf%2lf",&a,array_a,&b,array_b,&c,array_c,&d,array_d);
    PI_Write(from_test4c,"%d%2d%c%2c%f%2f%lf%2lf",a,array_a,b,array_b,c,array_c,d,array_d);
    return 0;
}

void test4c(void) {

    int a,array_a[2];
    char b,array_b[2];
    float c,array_c[2];
    double d,array_d[2];

    int ra,rarray_a[2];
    char rb,rarray_b[2];
    float rc,rarray_c[2];
    double rd,rarray_d[2];

    a = 124;
    array_a[0] = 125;
    array_a[1] = -521;

    b = '9';
    array_b[0] = 'p';
    array_b[1] = ']';

    c = 88.1231443;
    array_c[0] = 238.20001;
    array_c[1] = -999.22320;

    d = 99992.0000000001;
    array_d[0] = -10.0000000007;
    array_d[1] = 8.00000200021;

    PI_Write(to_test4c,"%d%2d%c%2c%f%2f%lf%2lf",a,array_a,b,array_b,c,array_c,d,array_d);
    PI_Read(from_test4c,"%d%2d%c%2c%f%2f%lf%2lf",&ra,rarray_a,&rb,rarray_b,&rc,rarray_c,&rd,rarray_d);

    // RELATED TO BUG 20 */
    CU_ASSERT_EQUAL(a,ra);
    CU_ASSERT_EQUAL(array_a[0],rarray_a[0]);
    CU_ASSERT_EQUAL(array_a[1],rarray_a[1]);

    CU_ASSERT_EQUAL('9',rb);
    CU_ASSERT_EQUAL('p',rarray_b[0]);
    CU_ASSERT_EQUAL(']',rarray_b[1]);

    CU_ASSERT_DOUBLE_EQUAL(c,rc,0.00001);
    CU_ASSERT_DOUBLE_EQUAL(array_c[0],rarray_c[0],0.00001);
    CU_ASSERT_DOUBLE_EQUAL(array_c[1],rarray_c[1],0.00001);

    CU_ASSERT_DOUBLE_EQUAL(d,rd,0.0000000001);
    CU_ASSERT_DOUBLE_EQUAL(array_d[0],rarray_d[0],0.0000000001);
    CU_ASSERT_DOUBLE_EQUAL(array_d[1],rarray_d[1],0.0000000001);
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    test4a_proc = CreateAliasedProcess(mixed_single,"test4a",0,NULL);
    to_test4a = PI_CreateChannel(PI_MAIN,test4a_proc);
    from_test4a = PI_CreateChannel(test4a_proc,PI_MAIN);

    test4b_proc = CreateAliasedProcess(mixed_array,"test4b",0,NULL);
    to_test4b = PI_CreateChannel(PI_MAIN,test4b_proc);
    from_test4b = PI_CreateChannel(test4b_proc,PI_MAIN);

    test4c_proc = CreateAliasedProcess(mixed_singleandarray,"test4c",0,NULL);
    to_test4c = PI_CreateChannel(PI_MAIN,test4c_proc);
    from_test4c = PI_CreateChannel(test4c_proc,PI_MAIN);

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddMixedValueSuite(void)
{
    CU_pSuite suite = CU_add_suite("Pilot Mixed Values Read/Write Tests", init, cleanup);
    if (suite == NULL)
        return CU_get_error();

    AddTest(suite, "mixed types -- single",test4a);
    AddTest(suite, "mixed types -- array",test4b);
    AddTest(suite, "mixed types -- single & array",test4c);

    return CUE_SUCCESS;
}
