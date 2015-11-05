#include <rpal/rpal.h>
#include <libOs/libOs.h>
#include <rpHostCommonPlatformLib/rTags.h>
#include <Basic.h>

#define RPAL_FILE_ID      86


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
	parseSignature
	( 
	    rSequence signature  
	)
{
	RPCHAR strBuffer = NULL;
	RPWCHAR wStrBuffer = NULL;
	RU32   signatureStatus = 0;
	RU32   byteBufferSize = 0;
	RU8*   byteBuffer = NULL;

	CU_ASSERT_TRUE( rSequence_getSTRINGW( signature, RP_TAGS_FILE_PATH, &wStrBuffer ) );
	CU_ASSERT_TRUE( rSequence_getRU32( signature, RP_TAGS_CERT_CHAIN_STATUS, &signatureStatus ) );
	if( rSequence_getSTRINGA( signature, RP_TAGS_CERT_ISSUER, &strBuffer ) )
	{
		CU_ASSERT_TRUE( 0 < rpal_string_strsize( strBuffer ) );
	}
		
	if( rSequence_getSTRINGA( signature, RP_TAGS_CERT_SUBJECT, &strBuffer ) )
	{
		CU_ASSERT_TRUE( 0 < rpal_string_strsize( strBuffer ) );
	}
	
	if( rSequence_getBUFFER( signature, RP_TAGS_CERT_BUFFER, &byteBuffer, &byteBufferSize ) )
	{
		CU_ASSERT_TRUE( 0 < byteBufferSize );
		CU_ASSERT_PTR_NOT_EQUAL_FATAL( byteBuffer, NULL );
	}
}


void
	test_signCheck
	(
	    void
	)
{
    rList filePaths = NULL;
	RPWCHAR filePath = NULL;
	rSequence fileSignature = NULL;
	RU32 operationFlags = OSLIB_SIGNCHECK_NO_NETWORK | OSLIB_SIGNCHECK_CHAIN_VERIFICATION | OSLIB_SIGNCHECK_INCLUDE_RAW_CERT;
	RBOOL isSigned = FALSE;
	RBOOL isVerified_local = FALSE;
	RBOOL isVerified_global = FALSE;

	filePaths = rList_new( RP_TAGS_FILE_PATH, RPCM_STRINGW );
	CU_ASSERT_PTR_NOT_EQUAL_FATAL( filePaths, NULL );

#ifdef RPAL_PLATFORM_WINDOWS
	// Windows signed file
	rList_addSTRINGW( filePaths, _WCH( "C:\\WINDOWS\\system32\\smss.exe" ) );
	rList_addSTRINGW( filePaths, _WCH( "C:\\Program Files\\Internet Explorer\\iexplore.exe" ) );

	// Catalogs files
	rList_addSTRINGW( filePaths, _WCH( "C:\\WINDOWS\\explorer.exe" ) );
	rList_addSTRINGW( filePaths, _WCH( "C:\\WINDOWS\\system32\\calc.exe" ) );
	rList_addSTRINGW( filePaths, _WCH( "C:\\WINDOWS\\system32\\cmd.exe" ) );

	// ThirdParty
	rList_addSTRINGW( filePaths, _WCH( "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe" ) );
#endif

    while( rList_getSTRINGW( filePaths, RP_TAGS_FILE_PATH, &filePath ) )
	{
		if( libOs_getSignature( filePath, &fileSignature, operationFlags, &isSigned, &isVerified_local, &isVerified_global ) )
		{
			CU_ASSERT_PTR_NOT_EQUAL_FATAL( fileSignature, NULL );

			if ( isSigned )
			{
				parseSignature( fileSignature );
			}

			rSequence_free( fileSignature );
			fileSignature = NULL;
		}
	}
    rList_free( filePaths );
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
        if( NULL == CU_add_test( suite, "signCheck", test_signCheck ) )
        {
            ret = 0;
        }
    }

    CU_basic_run_tests();

    CU_cleanup_registry();

    rpal_Context_deinitialize();

    return ret;
}

