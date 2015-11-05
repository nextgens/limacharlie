#include <rpal/rpal.h>
#include <librpcm/librpcm.h>
#include <processLib/processLib.h>
#include <rpHostCommonPlatformLib/rTags.h>
#include <Basic.h>


#ifdef RPAL_PLATFORM_WINDOWS
static RBOOL 
    SetPrivilege
    (
        HANDLE hToken, 
        LPCTSTR lpszPrivilege, 
        BOOL bEnablePrivilege
    )
{
    LUID luid;
    RBOOL bRet = FALSE;

    if( LookupPrivilegeValue( NULL, lpszPrivilege, &luid ) )
    {
        TOKEN_PRIVILEGES tp;

        tp.PrivilegeCount=1;
        tp.Privileges[0].Luid=luid;
        tp.Privileges[0].Attributes=(bEnablePrivilege) ? SE_PRIVILEGE_ENABLED: 0;
        //
        //  Enable the privilege or disable all privileges.
        //
        if( AdjustTokenPrivileges( hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL ) )
        {
            //
            //  Check to see if you have proper access.
            //  You may get "ERROR_NOT_ALL_ASSIGNED".
            //
            bRet = ( GetLastError() == ERROR_SUCCESS );
        }
    }
    return bRet;
}


static RBOOL
    Get_Privilege
    (
        RPCHAR privName
    )
{
    RBOOL isSuccess = FALSE;

    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;

    hProcess = GetCurrentProcess();

    if( NULL != hProcess )
    {
        if( OpenProcessToken( hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken ) )
        {
            if( SetPrivilege( hToken, privName, TRUE ) )
            {
                isSuccess = TRUE;
            }
            
            CloseHandle( hToken );
        }
    }

    return isSuccess;
}
#endif

void 
    test_memoryLeaks
    (
        void
    )
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

void 
    test_procEntries
    (
        void
    )
{
    processLibProcEntry* entries = NULL;
    RU32 entryIndex = 0;
    RU32 nEntries = 0;

    entries = processLib_getProcessEntries( TRUE );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( entries, NULL );

    while( 0 != entries[ entryIndex ].pid )
    {
        nEntries++;

        entryIndex++;
    }

    CU_ASSERT_TRUE( 5 < nEntries );

    rpal_memory_free( entries );
	
	entries = processLib_getProcessEntries( FALSE );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( entries, NULL );

    while( 0 != entries[ entryIndex ].pid )
    {
        nEntries++;

        entryIndex++;
    }

    CU_ASSERT_TRUE( 5 < nEntries );

    rpal_memory_free( entries );
}

void 
    test_processInfo
    (
        void
    )
{
    processLibProcEntry* entries = NULL;
    RU32 entryIndex = 0;
    rSequence proc = NULL;
    RPWCHAR path = NULL;

    entries = processLib_getProcessEntries( TRUE );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( entries, NULL );

    CU_ASSERT_NOT_EQUAL_FATAL( entries[ entryIndex ].pid, 0 );

    proc = processLib_getProcessInfo( entries[ entryIndex ].pid );

    CU_ASSERT_PTR_NOT_EQUAL( proc, NULL );

    CU_ASSERT_TRUE( rSequence_getSTRINGW( proc, RP_TAGS_FILE_PATH, &path ) );

    CU_ASSERT_PTR_NOT_EQUAL( path, NULL );
    CU_ASSERT_NOT_EQUAL( rpal_string_strlenw( path ), 0 );

    rSequence_free( proc );

    rpal_memory_free( entries );
}

void 
    test_servicesList
    (
        void
    )
{
    rList svcs = NULL;
    rSequence svc = NULL;
    RU32 type = PROCESSLIB_SVCS;
#if defined( RPAL_PLATFORM_WINDOWS ) || defined( RPAL_PLATFORM_LINUX )
    RPWCHAR svcName = NULL;
#elif defined( RPAL_PLATFORM_MACOSX )
    RPCHAR svcName = NULL;
#endif

    svcs = processLib_getServicesList( type );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( svcs, NULL );

    CU_ASSERT_TRUE( rList_getSEQUENCE( svcs, RP_TAGS_SVC, &svc ) );

#if defined( RPAL_PLATFORM_WINDOWS ) || defined( RPAL_PLATFORM_LINUX )
    CU_ASSERT_TRUE( rSequence_getSTRINGW( svc, RP_TAGS_SVC_NAME, &svcName ) );

    CU_ASSERT_PTR_NOT_EQUAL( svcName, NULL );

    CU_ASSERT_NOT_EQUAL( rpal_string_strlenw( svcName ), 0 );
#elif defined( RPAL_PLATFORM_MACOSX )
    CU_ASSERT_TRUE( rSequence_getSTRINGA( svc, RP_TAGS_SVC_NAME, &svcName ) );

    CU_ASSERT_PTR_NOT_EQUAL( svcName, NULL );

    CU_ASSERT_NOT_EQUAL( rpal_string_strlen( svcName ), 0 );
#endif

    rSequence_free( svcs );
}

void 
    test_modules
    (
        void
    )
{
    processLibProcEntry* entries = NULL;
    RU32 entryIndex = 0;
    rList mods = NULL;
    rSequence mod = NULL;
    RPWCHAR path = NULL;

    entries = processLib_getProcessEntries( TRUE );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( entries, NULL );

    CU_ASSERT_NOT_EQUAL_FATAL( entries[ entryIndex ].pid, 0 );

    mods = processLib_getProcessModules( entries[ entryIndex ].pid );

    CU_ASSERT_PTR_NOT_EQUAL( mods, NULL );

    CU_ASSERT_TRUE( rList_getSEQUENCE( mods, RP_TAGS_DLL, &mod ) );

    CU_ASSERT_TRUE( rSequence_getSTRINGW( mod, RP_TAGS_FILE_PATH, &path ) );

    CU_ASSERT_PTR_NOT_EQUAL( path, NULL );
    CU_ASSERT_NOT_EQUAL( rpal_string_strlenw( path ), 0 );

    rSequence_free( mods );

    rpal_memory_free( entries );
}

void 
    test_memmap
    (
        void
    )
{
    processLibProcEntry* entries = NULL;
    RU32 entryIndex = 0;
    rList regions = NULL;
    rSequence region = NULL;
    RU32 nRegions = 0;
    
    RU8 type = 0;
    RU8 protect = 0;
    RU64 ptr = 0;
    RU64 size = 0;

    entries = processLib_getProcessEntries( TRUE );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( entries, NULL );

    CU_ASSERT_NOT_EQUAL_FATAL( entries[ entryIndex ].pid, 0 );

    regions = processLib_getProcessMemoryMap( entries[ entryIndex ].pid );

    CU_ASSERT_PTR_NOT_EQUAL( regions, NULL );

    while( rList_getSEQUENCE( regions, RP_TAGS_MEMORY_REGION, &region ) )
    {
        CU_ASSERT_TRUE( rSequence_getRU8( region, RP_TAGS_MEMORY_TYPE, &type ) );
        CU_ASSERT_TRUE( rSequence_getRU8( region, RP_TAGS_MEMORY_ACCESS, &protect ) );
        CU_ASSERT_TRUE( rSequence_getPOINTER64( region, RP_TAGS_BASE_ADDRESS, &ptr ) );
        CU_ASSERT_TRUE( rSequence_getRU64( region, RP_TAGS_MEMORY_SIZE, &size ) );
        nRegions++;
    }

    CU_ASSERT_TRUE( 2 < nRegions ); 

    rSequence_free( regions );

    rpal_memory_free( entries );
}

void 
    test_handles
    (
        void
    )
{
    rList handles = NULL;
    rSequence handle = NULL;
    RU32 nHandles = 0;
    RU32 nNamedHandles = 0;
    RPWCHAR handleName = NULL;

    handles = processLib_getHandles( 0, FALSE, NULL );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( handles, NULL );

    while( rList_getSEQUENCE( handles, RP_TAGS_HANDLE_INFO, &handle ) )
    {
        nHandles++;
    }

    CU_ASSERT_TRUE( 100 < nHandles );

    rList_free( handles );

    handles = processLib_getHandles( 0, TRUE, NULL );

    CU_ASSERT_PTR_NOT_EQUAL_FATAL( handles, NULL );

    while( rList_getSEQUENCE( handles, RP_TAGS_HANDLE_INFO, &handle ) )
    {
        nNamedHandles++;

        CU_ASSERT_TRUE( rSequence_getSTRINGW( handle, RP_TAGS_HANDLE_NAME, &handleName ) );
        CU_ASSERT_TRUE( 0 != rpal_string_strlenw( handleName ) );
    }

    CU_ASSERT_TRUE( nNamedHandles < nHandles );

    rList_free( handles );
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
#ifdef RPAL_PLATFORM_WINDOWS
    RCHAR strSeDebug[] = "SeDebugPrivilege";
    Get_Privilege( strSeDebug );
#endif
    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( argv );

    rpal_initialize( NULL, 1 );

    CU_initialize_registry();

    if( NULL != ( suite = CU_add_suite( "raptd", NULL, NULL ) ) )
    {
        if( NULL == CU_add_test( suite, "procEntries", test_procEntries ) ||
            NULL == CU_add_test( suite, "processInfo", test_processInfo ) ||
            NULL == CU_add_test( suite, "modules", test_modules ) ||
            NULL == CU_add_test( suite, "memmap", test_memmap ) ||
            NULL == CU_add_test( suite, "handles", test_handles ) ||
            NULL == CU_add_test( suite, "services", test_servicesList ) ||
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

