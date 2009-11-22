/*
Unit tests for the format parser. This suite indirectly tests ParseFormatString
through PI_Read and PI_Write.

NOTE: you should *not* run this suite with the deadlock detector on.
*/
#include "unittests.h"

PI_CHANNEL* dummy_chan;
PI_PROCESS* dummy_proc;

PI_CHANNEL* all_types_chan;
PI_PROCESS* all_types_proc;

int dummy_process_func( int arg1, void* arg2 )
{
    return 0;
}

int all_types_func( int arg1, void* arg2 )
{
    unsigned char b, hhu;
    char c;
    short hd;
    int d;
    long int ld;
    long long int lld;
    unsigned short hu;
    unsigned long lu;
    unsigned long long llu;
    float f;
    double lf;
    long double Lf;
    float mpi_float;

    // Since we're not checking the contents of each variable, some of the above
    // variables appear in the argument list multiple times (where synonyms are
    // being read).
    PI_Read(
        all_types_chan,
        "%b %c %hi %hd %d %i %ld %li %lld %lli %hhu %hu %lu %llu %f %lf %Lf %m",
        &b, &c, &hd, &hd, &d, &d, &ld, &ld, &lld, &lld, &hhu, &hu, &lu, &llu,
        &f, &lf, &Lf, MPI_FLOAT, &mpi_float
    );
    return 0;
}

void ShouldFailOnWhitespaceOnly( void )
{
    PI_Errno = 0;
    PI_Write( dummy_chan, "  \t   ", 1 );
    CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );

    PI_Errno = 0;
    PI_Write( dummy_chan, "", 1 );
    CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );

    PI_Errno = 0;
    PI_Write( dummy_chan, "\n\r \v\t", 1 );
    CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );
}

void ShouldFailOnInvalidSizeArray( void )
{
    int a[1] = {0};
    const char* fmts[] = { "%1d", "%1ld", "%0d", "%-1d", NULL };
    int i;

    for ( i = 0; fmts[i] != NULL; i++ ) {
        PI_Errno = 0;
        PI_Write( dummy_chan, fmts[i], a );
        CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );
    }

    for ( i = 0; i >= -3; i-- ) {
        PI_Errno = 0;
        PI_Write( dummy_chan, "%*d", i, a );
        CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );
    }

    PI_Errno = 0;
    PI_Write( dummy_chan, "%3*d", 1, a );
    CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );
}

void ShouldFailOnLolByob( void )
{
    // An old version of Pilot would parse %lol%%byob as if it were %lf%b
    PI_Errno = 0;
    PI_Write( dummy_chan, "%lol%%byob", 2.0f, 'b' );
    CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );
}

void ShouldNotAcceptPartialConversionSpec( void )
{
    PI_Errno = 0;
    PI_Write( dummy_chan, " %", 1 );
    CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );
}

void ShouldFailOnNullFormatString( void )
{
    PI_Errno = 0;
    PI_Write( dummy_chan, NULL, 1 );
    CU_ASSERT_EQUAL( PI_Errno, PI_NULL_FORMAT );
}

void ShouldAcceptBasicFormats( void )
{
    float f = 11.0f;
    PI_Errno = 0;
    PI_Write(
        all_types_chan,
        "%b %c %hi %hd %d %i %ld %li %lld %lli %hhu %hu %lu %llu %f %lf %Lf %m",
        'b', 'c', (short) 1, (short) 1, 2, 2, 3L, 3L, 4LL, 4LL, 'u',
        (unsigned short) 5, 6UL, 7ULL, 8.0f, 9.0, (long double) 10,
        MPI_FLOAT, &f
    );
    CU_ASSERT_EQUAL( PI_Errno, 0 );
}

void ShouldNotCrashWithLongFormat( void )
{
    PI_Errno = 0;
    PI_Write( dummy_chan, "%d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d "
        "%d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d",
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55
    );
    // If we get here, then the test is mostly a success regardless of the assert
    CU_ASSERT_EQUAL( PI_Errno, PI_FORMAT_ARGS );
}

static int init(void)
{
    int argc = default_argc;
    char** argv = default_argv;
    PI_QuietMode = 1;
    PI_OnErrorReturn = 1;

    PI_Configure(&argc, &argv);

    dummy_proc = PI_CreateProcess( dummy_process_func, 0, NULL );
    dummy_chan = PI_CreateChannel( PI_MAIN, dummy_proc );

    all_types_proc = PI_CreateProcess( all_types_func, 0, NULL );
    all_types_chan = PI_CreateChannel( PI_MAIN, all_types_proc );

    PI_StartAll();
    return 0;
}

static int cleanup(void)
{
    if (PI_GetMyRank() == 0)
        PI_StopMain(0);
    return 0;
}

CU_ErrorCode AddFormatSuite(void)
{
    CU_pSuite suite = CU_add_suite( "Format Parsing", init, cleanup );
    if (suite == NULL)
        return CU_get_error();

    AddTest( suite, "Should fail on whitespace only format", ShouldFailOnWhitespaceOnly );
    AddTest( suite, "Should fail with invalid sized array", ShouldFailOnInvalidSizeArray );
    AddTest( suite, "Should fail to parse \"%lol%%byob\"", ShouldFailOnLolByob );
    AddTest( suite, "Should not accept an incomplete conversion spec", ShouldNotAcceptPartialConversionSpec );
    AddTest( suite, "Should fail on NULL format string", ShouldFailOnNullFormatString );
    AddTest( suite, "Try all format codes", ShouldAcceptBasicFormats );
    AddTest( suite, "Should not crash with too long format string", ShouldNotCrashWithLongFormat );

    return CUE_SUCCESS;
}
