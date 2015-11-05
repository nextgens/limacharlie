#include <rpal/rpal.h>
#include <librpcm/librpcm.h>
#include <Basic.h>

#define RPAL_FILE_ID     89


void test_memoryLeaks(void)
{
    RU32 memUsed = 0;

    rpal_Context_cleanup();

    memUsed = rpal_memory_totalUsed();

    CU_ASSERT_EQUAL( memUsed, 0 );

    if( 0 != memUsed )
    {
        rpal_debug_critical( "Memory leak: %d bytes.\n", memUsed );
        printf( "\nMemory leak: %d bytes.\n", memUsed );

        rpal_memory_findMemory();
    }
}


void test_CreateAndDestroy(void)
{
    rSequence seq = NULL;
    rList list = NULL;
    rIterator ite = NULL;

    seq = rSequence_new();

    CU_ASSERT_TRUE_FATAL( NULL != seq );

    rSequence_free( seq );

    list = rList_new( 1, 2 );

    CU_ASSERT_TRUE_FATAL( NULL != list );

    ite = rIterator_new( list );

    CU_ASSERT_TRUE_FATAL( NULL != ite );

    rIterator_free( ite );

    rList_free( list );
}

void test_AddAndRemove(void)
{
    rSequence seq = NULL;
    rList list = NULL;
    rList tmpList = NULL;
    rIterator ite = NULL;
    rpcm_tag tag = RPCM_INVALID_TAG;
    rpcm_type type = RPCM_INVALID_TYPE;
    RPVOID ptr = NULL;
    RU32 size = 0;
    RU32 count = 0;

    RU8 tmp8 = 0;
    RU32 tmp32 = 0;
    RPCHAR tmpStringA = NULL;
    RPWCHAR tmpStringW = NULL;

    seq = rSequence_new();

    CU_ASSERT_TRUE_FATAL( NULL != seq );

    CU_ASSERT_TRUE( rSequence_addRU8( seq, 42, 10 ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGA( seq, 24, "hello" ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGW( seq, 666, _WCH( "you" ) ) );
    
    CU_ASSERT_TRUE( rSequence_getRU8( seq, 42, &tmp8 ) );
    CU_ASSERT_EQUAL( tmp8, 10 );
    
    CU_ASSERT_FALSE( rSequence_getRU8( seq, 24, &tmp8 ) );

    CU_ASSERT_TRUE( rSequence_getSTRINGA( seq, 24, &tmpStringA ) );
    CU_ASSERT_STRING_EQUAL( tmpStringA, "hello" );

    CU_ASSERT_TRUE( rSequence_getSTRINGW( seq, 666, &tmpStringW ) );
    CU_ASSERT_TRUE( 0 == rpal_memory_memcmp( tmpStringW, _WCH( "you" ), sizeof( _WCH( "you" ) ) ) );
    
    CU_ASSERT_FALSE( rSequence_addRU32( seq, 42, 0xDEADBEEF ) );


    list = rList_new( 10, RPCM_RU32 );

    CU_ASSERT_TRUE_FATAL( NULL != list );


    CU_ASSERT_FALSE( rList_addRU16( list, 666 ) );
    
    CU_ASSERT_TRUE( rList_addRU32( list, 1 ) );
    CU_ASSERT_TRUE( rList_addRU32( list, 2 ) );
    CU_ASSERT_TRUE( rList_addRU32( list, 3 ) );
    CU_ASSERT_TRUE( rList_addRU32( list, 4 ) );

    ite = rIterator_new( list );

    CU_ASSERT_TRUE_FATAL( NULL != ite );

    while( rIterator_next( ite, &tag, &type, &ptr, &size ) )
    {
        CU_ASSERT_EQUAL( tag, 10 );
        CU_ASSERT_EQUAL( type, RPCM_RU32 );
        CU_ASSERT_EQUAL( size, sizeof( RU32 ) );
        tmp32 = *(RU32*)ptr;
        CU_ASSERT_EQUAL( tmp32, ++count );
    }

    CU_ASSERT_EQUAL( count, 4 );

    rIterator_free( ite );

    CU_ASSERT_TRUE( rSequence_addLIST( seq, 66, list ) );

    CU_ASSERT_TRUE( rSequence_getLIST( seq, 66, &tmpList ) );

    rSequence_free( seq );
}

void test_SerialiseAndDeserialise(void)
{
    rSequence seq = NULL;
    rSequence tmpSeq = NULL;
    rList list = NULL;
    rBlob blob = NULL;
    rIterator ite = NULL;
    rpcm_tag tag = RPCM_INVALID_TAG;
    rpcm_type type = RPCM_INVALID_TYPE;
    RPVOID ptr = NULL;
    RU32 size = 0;
    RU32 count = 0;

    RU8 tmp8 = 0;
    RU64 tmp64 = 0;
    RPCHAR tmpStringA = NULL;
    RPWCHAR tmpStringW = NULL;
    RU32 consumed = 0;

    seq = rSequence_new();
    list = rList_new( 1, RPCM_RU64 );
    blob = rpal_blob_create( 0, 0 );

    CU_ASSERT_FATAL( NULL != seq );
    CU_ASSERT_FATAL( NULL != list );
    CU_ASSERT_FATAL( NULL != blob );

    CU_ASSERT_TRUE( rList_addRU64( list, 1 ) );
    CU_ASSERT_TRUE( rList_addRU64( list, 2 ) );
    CU_ASSERT_TRUE( rList_addRU64( list, 3 ) );
    CU_ASSERT_TRUE( rList_addRU64( list, 4 ) );

    CU_ASSERT_TRUE( rSequence_addRU8( seq, 42, 10 ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGA( seq, 24, "hello" ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGW( seq, 666, _WCH( "you" ) ) );
    CU_ASSERT_TRUE( rSequence_addLIST( seq, 66, list ) );
    
    tmpSeq = rSequence_duplicate( seq );
    CU_ASSERT_NOT_EQUAL( tmpSeq, NULL );
    CU_ASSERT_TRUE( rSequence_addSEQUENCE( tmpSeq, 77, rSequence_duplicate( tmpSeq ) ) );
    CU_ASSERT_TRUE( rSequence_addSEQUENCE( seq, 88, rSequence_duplicate( tmpSeq ) ) );

    rSequence_free( tmpSeq );

    tmpSeq = NULL;

    CU_ASSERT_TRUE_FATAL( rSequence_serialise( seq, blob ) );

    rpal_file_write( "rpcm_test_seq", rpal_blob_getBuffer( blob ), rpal_blob_getSize( blob ), TRUE );

    rSequence_free( seq );
    seq = NULL;
    list = NULL;

    CU_ASSERT_TRUE_FATAL( rSequence_deserialise( &seq, (RPU8)rpal_blob_getBuffer( blob ), rpal_blob_getSize( blob ), &consumed ) );
    CU_ASSERT_EQUAL( consumed, rpal_blob_getSize( blob ) );

    rpal_blob_free( blob );

    CU_ASSERT_TRUE( rSequence_getRU8( seq, 42, &tmp8 ) );
    CU_ASSERT_EQUAL( tmp8, 10 );
    
    CU_ASSERT_FALSE( rSequence_getRU8( seq, 24, &tmp8 ) );

    CU_ASSERT_TRUE( rSequence_getSTRINGA( seq, 24, &tmpStringA ) );
    CU_ASSERT_STRING_EQUAL( tmpStringA, "hello" );

    CU_ASSERT_TRUE( rSequence_getSTRINGW( seq, 666, &tmpStringW ) );
    CU_ASSERT_TRUE( 0 == rpal_memory_memcmp( tmpStringW, _WCH( "you" ), sizeof( _WCH( "you" ) ) ) );
    
    CU_ASSERT_TRUE( rSequence_getLIST( seq, 66, &list ) );

    ite = rIterator_new( list );

    CU_ASSERT_TRUE_FATAL( NULL != ite );

    while( rIterator_next( ite, &tag, &type, &ptr, &size ) )
    {
        CU_ASSERT_EQUAL( tag, 1 );
        CU_ASSERT_EQUAL( type, RPCM_RU64 );
        CU_ASSERT_EQUAL( size, sizeof( RU64 ) );
        tmp64 = *(RU64*)ptr;
        CU_ASSERT_EQUAL( tmp64, ++count );
    }

    CU_ASSERT_EQUAL( count, 4 );

    rIterator_free( ite );

    CU_ASSERT_TRUE( rSequence_getSEQUENCE( seq, 88, &tmpSeq ) );
    CU_ASSERT_NOT_EQUAL( tmpSeq, NULL );

    rSequence_free( seq );
}





void test_duplicate(void)
{
    rSequence seq = NULL;
    rSequence newSeq = NULL;
    rList list = NULL;
    rList newList = NULL;
    rIterator ite = NULL;
    rpcm_tag tag = RPCM_INVALID_TAG;
    rpcm_type type = RPCM_INVALID_TYPE;
    RPVOID ptr = NULL;
    RU32 size = 0;
    RU32 count = 0;

    RU8 tmp8 = 0;
    RU64 tmp64 = 0;
    RPCHAR tmpStringA = NULL;
    RPWCHAR tmpStringW = NULL;

    seq = rSequence_new();
    list = rList_new( 1, RPCM_RU64 );

    CU_ASSERT_FATAL( NULL != seq );
    CU_ASSERT_FATAL( NULL != list );

    CU_ASSERT_TRUE( rList_addRU64( list, 1 ) );
    CU_ASSERT_TRUE( rList_addRU64( list, 2 ) );
    CU_ASSERT_TRUE( rList_addRU64( list, 3 ) );
    CU_ASSERT_TRUE( rList_addRU64( list, 4 ) );

    CU_ASSERT_TRUE( rSequence_addRU8( seq, 42, 10 ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGA( seq, 24, "hello" ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGW( seq, 666, _WCH( "you" ) ) );
    CU_ASSERT_TRUE( rSequence_addLIST( seq, 66, list ) );

    

    newSeq = rSequence_duplicate( seq );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( newSeq, NULL );

    rSequence_free( seq );

    CU_ASSERT_TRUE( rSequence_getRU8( newSeq, 42, &tmp8 ) );
    CU_ASSERT_EQUAL( tmp8, 10 );
    
    CU_ASSERT_FALSE( rSequence_getRU8( newSeq, 24, &tmp8 ) );

    CU_ASSERT_TRUE( rSequence_getSTRINGA( newSeq, 24, &tmpStringA ) );
    CU_ASSERT_STRING_EQUAL( tmpStringA, "hello" );

    CU_ASSERT_TRUE( rSequence_getSTRINGW( newSeq, 666, &tmpStringW ) );
    CU_ASSERT_TRUE( 0 == rpal_memory_memcmp( tmpStringW, _WCH( "you" ), sizeof( _WCH( "you" ) ) ) );
    
    CU_ASSERT_TRUE( rSequence_getLIST( newSeq, 66, &newList ) );

    ite = rIterator_new( newList );

    CU_ASSERT_TRUE_FATAL( NULL != ite );

    while( rIterator_next( ite, &tag, &type, &ptr, &size ) )
    {
        CU_ASSERT_EQUAL( tag, 1 );
        CU_ASSERT_EQUAL( type, RPCM_RU64 );
        CU_ASSERT_EQUAL( size, sizeof( RU64 ) );
        tmp64 = *(RU64*)ptr;
        CU_ASSERT_EQUAL( tmp64, ++count );
    }

    CU_ASSERT_EQUAL( count, 4 );

    rIterator_free( ite );

    rSequence_free( newSeq );
}


void test_isEqual(void)
{
    rList root1 = NULL;
    rList root2 = NULL;

    rSequence seq1 = NULL;
    rSequence seq12 = NULL;
    rSequence seq2 = NULL;
    rSequence seq22 = NULL;

    root1 = rList_new( 1, RPCM_SEQUENCE );
    root2 = rList_new( 1, RPCM_SEQUENCE );
    seq1 = rSequence_new();
    seq2 = rSequence_new();
    seq12 = rSequence_new();
    seq22 = rSequence_new();

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( root1, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( root2, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq1, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq12, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq2, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq22, NULL );

    CU_ASSERT_TRUE( rSequence_addRU32( seq1, 2, 1 ) );
    CU_ASSERT_TRUE( rSequence_addRU32( seq12, 3, 2 ) );
    CU_ASSERT_TRUE( rSequence_addRU32( seq2, 2, 1 ) );
    CU_ASSERT_TRUE( rSequence_addRU32( seq22, 3, 2 ) );

    CU_ASSERT_TRUE( rList_addSEQUENCE( root1, seq1 ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( root1, seq12 ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( root2, seq2 ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( root2, seq22 ) );

    CU_ASSERT_TRUE( rList_isEqual( root1, root2 ) );

    rList_free( root1 );
    rList_free( root2 );



    root1 = rList_new( 1, RPCM_SEQUENCE );
    root2 = rList_new( 1, RPCM_SEQUENCE );
    seq1 = rSequence_new();
    seq2 = rSequence_new();
    seq12 = rSequence_new();
    seq22 = rSequence_new();

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( root1, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( root2, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq1, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq12, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq2, NULL );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( seq22, NULL );

    CU_ASSERT_TRUE( rList_addSEQUENCE( root1, seq1 ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( root1, seq12 ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( root2, seq2 ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( root2, seq22 ) );

    CU_ASSERT_TRUE( rSequence_addRU32( seq1, 2, 1 ) );
    CU_ASSERT_TRUE( rSequence_addRU32( seq12, 2, 2 ) );
    CU_ASSERT_TRUE( rSequence_addRU32( seq2, 2, 1 ) );

    CU_ASSERT_FALSE( rList_isEqual( root1, root2 ) );

    rList_free( root1 );
    rList_free( root2 );
}

void test_complex(void)
{
    rSequence container = NULL;
    rList root = NULL;
    rSequence level1 = NULL;
    rList level2 = NULL;
    rSequence level3 = NULL;
    RPWCHAR elem = _WCH( "hello" );

    container = rSequence_new();
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( container, NULL );

    root = rList_new( 1, RPCM_SEQUENCE );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( root, NULL );

    level1 = rSequence_new();
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( level1, NULL );

    level2 = rList_new( 3, RPCM_SEQUENCE );
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( level2, NULL );

    level3 = rSequence_new();
    CU_ASSERT_PTR_NOT_EQUAL_FATAL( level3, NULL );

    CU_ASSERT_TRUE( rSequence_addSTRINGW( level3, 4, elem ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGW( level3, 41, elem ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( level2, level3 ) );
    CU_ASSERT_TRUE( rSequence_addLIST( level1, 2, level2 ) );
    CU_ASSERT_TRUE( rList_addSEQUENCE( root, level1 ) );
    CU_ASSERT_TRUE( rSequence_addLIST( container, 1, root ) );

    rSequence_free( container );
}

void test_EstimateSize( void )
{
    rSequence seq = NULL;
    rList list = NULL;
    
    seq = rSequence_new();

    CU_ASSERT_TRUE_FATAL( NULL != seq );

    CU_ASSERT_TRUE( rSequence_addRU8( seq, 42, 10 ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGA( seq, 24, "hello" ) );
    CU_ASSERT_TRUE( rSequence_addSTRINGW( seq, 666, _WCH( "you" ) ) );
    CU_ASSERT_FALSE( rSequence_addRU32( seq, 42, 0xDEADBEEF ) );

    CU_ASSERT_EQUAL( rSequence_getEstimateSize( seq ), 30 );

    list = rList_new( 10, RPCM_RU32 );

    CU_ASSERT_TRUE_FATAL( NULL != list );

    CU_ASSERT_FALSE( rList_addRU16( list, 666 ) );

    CU_ASSERT_TRUE( rList_addRU32( list, 1 ) );
    CU_ASSERT_TRUE( rList_addRU32( list, 2 ) );
    CU_ASSERT_TRUE( rList_addRU32( list, 3 ) );
    CU_ASSERT_TRUE( rList_addRU32( list, 4 ) );
    CU_ASSERT_TRUE( rSequence_addLIST( seq, 66, list ) );

    CU_ASSERT_EQUAL( rSequence_getEstimateSize( seq ), 100 );

    rSequence_free( seq );
}

int
    main
    (
        int argc,
        char* argv[]
    )
{
    int ret = 1;

    CU_pSuite suite = NULL;

    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( argv );

    rpal_initialize( NULL, 1 );

    CU_initialize_registry();

    if( NULL != ( suite = CU_add_suite( "rpcm", NULL, NULL ) ) )
    {
        if( NULL == CU_add_test( suite, "createAndDestroy", test_CreateAndDestroy ) || 
            NULL == CU_add_test( suite, "addAndRemove", test_AddAndRemove ) ||
            NULL == CU_add_test( suite, "serializeAndDeserialize", test_SerialiseAndDeserialise ) ||
            NULL == CU_add_test( suite, "duplicate", test_duplicate ) ||
            NULL == CU_add_test( suite, "isEqual", test_isEqual ) ||
            NULL == CU_add_test( suite, "complex", test_complex ) ||
            NULL == CU_add_test( suite, "estimateSize", test_EstimateSize ) ||
            NULL == CU_add_test( suite, "memoryLeaks", test_memoryLeaks ) )
        {
            ret = 0;
        }
    }

    CU_basic_run_tests();

    CU_cleanup_registry();

    rpal_Context_deinitialize();

    return ret;
}

