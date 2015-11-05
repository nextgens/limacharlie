/*
Copyright 2015 refractionPOINT

Licensed under the Apache License, Version 2.0 ( the "License" );
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <libOs/libOs.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID   35

#ifdef RPAL_PLATFORM_WINDOWS
#pragma warning( disable: 4127 )
#pragma warning( disable: 4306 )
#include <windows_undocumented.h>
#define FILETIME2ULARGE( uli, ft )  (uli).u.LowPart = (ft).dwLowDateTime, (uli).u.HighPart = (ft).dwHighDateTime
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <errno.h>
#include <sys/time.h>
#if defined( RPAL_PLATFORM_MACOSX )
#include <ifaddrs.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/thread_act.h>
#include <mach/mach_port.h>
#endif
#endif


static RBOOL g_isNetworkingInitialize = FALSE;

#ifdef RPAL_PLATFORM_WINDOWS
static WSADATA g_wsaData = {0};

static HMODULE wintrustLib = NULL;
static WinVerifyTrust_f WinVerifyTrust = NULL;
static WTHelperGetProvSignerFromChain_f WTHelperGetProvSignerFromChain = NULL;
static WTHelperProvDataFromStateData_f WTHelperProvDataFromStateData = NULL;
static CryptCATAdminAcquireContext_f CryptCATAdminAcquireContext = NULL;
static CryptCATAdminReleaseContext_f CryptCATAdminReleaseContext = NULL;
static CryptCATAdminReleaseCatalogContext_f CryptCATAdminReleaseCatalogContext = NULL;
static CryptCATAdminEnumCatalogFromHash_f CryptCATAdminEnumCatalogFromHash = NULL;
static CryptCATAdminCalcHashFromFileHandle_f CryptCATAdminCalcHashFromFileHandle = NULL;
static CryptCATCatalogInfoFromContext_f CryptCATCatalogInfoFromContext = NULL;

static HMODULE crypt32 = NULL;
static CertNameToStr_f RCertNameToStr = NULL;
static CertFreeCertificateChainEngine_f RCertFreeCertificateChainEngine = NULL;
static CertFreeCertificateChain_f RCertFreeCertificateChain = NULL;
static CertVerifyCertificateChainPolicy_f RCertVerifyCertificateChainPolicy = NULL;
static CertGetCertificateChain_f RCertGetCertificateChain = NULL;
static CertCreateCertificateChainEngine_f RCertCreateCertificateChainEngine = NULL;

static
RBOOL
    libOs_getFileSignature
    (
        RPWCHAR   pwfilePath,
        rSequence signature,
        RU32      operation,
        RBOOL*    pIsSigned,
        RBOOL*    pIsVerified_local,
        RBOOL*    pIsVerified_global
    );

// TODO : Code a generic lib loader
static
RBOOL
    loadCrypt32
    (

    )
{
    RBOOL isLoaded = FALSE;

    RCHAR importCrypt32[] = "crypt32.dll";
    RCHAR import1[] = "CertNameToStrA";
    RCHAR import2[] = "CertFreeCertificateChainEngine";
    RCHAR import3[] = "CertFreeCertificateChain";
    RCHAR import4[] = "CertVerifyCertificateChainPolicy";
    RCHAR import5[] = "CertGetCertificateChain";
    RCHAR import6[] = "CertCreateCertificateChainEngine";

    if( NULL == crypt32 )
    {
        // Some potential weird race condition, but extremely unlikely
        if( NULL != ( crypt32 = GetModuleHandle( (RPCHAR)&importCrypt32  ) ) ||
            NULL != ( crypt32 = LoadLibraryA( (RPCHAR)&importCrypt32 ) ) )
        {
            RCertNameToStr = (CertNameToStr_f)GetProcAddress( crypt32, (RPCHAR)&import1 );
            RCertFreeCertificateChainEngine = (CertFreeCertificateChainEngine_f)GetProcAddress( crypt32, (RPCHAR)&import2 );
            RCertFreeCertificateChain = (CertFreeCertificateChain_f)GetProcAddress( crypt32, (RPCHAR)&import3 );
            RCertVerifyCertificateChainPolicy = (CertVerifyCertificateChainPolicy_f)GetProcAddress( crypt32, (RPCHAR)&import4 );
            RCertGetCertificateChain = (CertGetCertificateChain_f)GetProcAddress( crypt32, (RPCHAR)&import5 );
            RCertCreateCertificateChainEngine = (CertCreateCertificateChainEngine_f)GetProcAddress( crypt32, (RPCHAR)&import6 );
        }
    }
    
    if( NULL != RCertNameToStr &&
        NULL != RCertFreeCertificateChainEngine &&
        NULL != RCertFreeCertificateChain &&
        NULL != RCertVerifyCertificateChainPolicy &&
        NULL != RCertGetCertificateChain &&
        NULL != RCertCreateCertificateChainEngine )
    {
        isLoaded = TRUE;
    }

    return isLoaded;
}


static
RBOOL
    loadWinTrustApi
    (

    )
{
    RBOOL isLoaded = FALSE;

    RCHAR importLibWintrust[] = "wintrust.dll";
    RCHAR import1[] = "WinVerifyTrust";
    RCHAR import2[] = "WTHelperGetProvSignerFromChain";
    RCHAR import3[] = "WTHelperProvDataFromStateData";
    RCHAR import4[] = "CryptCATAdminAcquireContext";
    RCHAR import5[] = "CryptCATAdminReleaseContext";
    RCHAR import6[] = "CryptCATAdminReleaseCatalogContext";
    RCHAR import7[] = "CryptCATAdminEnumCatalogFromHash";
    RCHAR import8[] = "CryptCATAdminCalcHashFromFileHandle";
    RCHAR import9[] = "CryptCATCatalogInfoFromContext";

    if( NULL == wintrustLib )
    {
        // Some potential weird race condition, but extremely unlikely
        if( NULL != ( wintrustLib = LoadLibraryA( (RPCHAR)&importLibWintrust ) ) )
        {
            WinVerifyTrust = (WinVerifyTrust_f)GetProcAddress( wintrustLib, (RPCHAR)&import1 );
            WTHelperGetProvSignerFromChain = (WTHelperGetProvSignerFromChain_f)GetProcAddress( wintrustLib, (RPCHAR)&import2 );
            WTHelperProvDataFromStateData = (WTHelperProvDataFromStateData_f)GetProcAddress( wintrustLib, (RPCHAR)&import3 );
            CryptCATAdminAcquireContext = (CryptCATAdminAcquireContext_f)GetProcAddress( wintrustLib, (RPCHAR)&import4 );
            CryptCATAdminReleaseContext = (CryptCATAdminReleaseContext_f)GetProcAddress( wintrustLib, (RPCHAR)&import5 );
            CryptCATAdminReleaseCatalogContext = (CryptCATAdminReleaseCatalogContext_f)GetProcAddress( wintrustLib, (RPCHAR)&import6 );
            CryptCATAdminEnumCatalogFromHash = (CryptCATAdminEnumCatalogFromHash_f)GetProcAddress( wintrustLib, (RPCHAR)&import7 );
            CryptCATAdminCalcHashFromFileHandle = (CryptCATAdminCalcHashFromFileHandle_f)GetProcAddress( wintrustLib, (RPCHAR)&import8 );
            CryptCATCatalogInfoFromContext = (CryptCATCatalogInfoFromContext_f)GetProcAddress( wintrustLib, (RPCHAR)&import9 );
        }
    }

    if( NULL != WinVerifyTrust &&
        NULL != WTHelperGetProvSignerFromChain &&
        NULL != WTHelperProvDataFromStateData &&
        NULL != CryptCATAdminAcquireContext &&
        NULL != CryptCATAdminReleaseContext &&
        NULL != CryptCATAdminReleaseCatalogContext &&
        NULL != CryptCATAdminEnumCatalogFromHash &&
        NULL != CryptCATAdminCalcHashFromFileHandle &&
        NULL != CryptCATCatalogInfoFromContext )
    {
        isLoaded = TRUE;
    }

    return isLoaded;
}
#endif

RBOOL
    _initializeNetworking
    (

    )
{
    RBOOL isSuccess = FALSE;

    if( g_isNetworkingInitialize )
    {
        isSuccess = TRUE;
    }
    else
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( 0 == WSAStartup( MAKEWORD(2, 2), &g_wsaData ) )
        {
            isSuccess = TRUE;
        }
#else
    isSuccess = TRUE;
#endif

        if( isSuccess )
        {
            g_isNetworkingInitialize = TRUE;
        }
    }

    return isSuccess;
}




RPCHAR
    libOs_getHostName
    (
        
    )
{
    RPCHAR outName = NULL;
    RCHAR name[ 256 ] = {0};

    if( _initializeNetworking() )
    {
        if( 0 == gethostname( (RPCHAR)&name, 256 ) )
        {
            outName = rpal_string_strdupa( name );
        }
    }

    return outName;
}

RU32
    libOs_getMainIp
    (

    )
{
    RU32 ip = 0;
#ifdef RPAL_PLATFORM_WINDOWS
    RCHAR name[ 256 ] = {0};
    struct hostent* hostinfo = NULL;
    struct in_addr addr = {0};

    if( _initializeNetworking() )
    {
        if( 0 == gethostname( (RPCHAR)&name, 256 ) )
        {
            if( NULL != ( hostinfo = gethostbyname( (RPCHAR)&name ) ) )
            {
#ifdef RPAL_PLATFORM_64_BIT
                char** tmp = NULL;
                tmp = hostinfo->h_addr_list;
                if( ( (RU64)tmp & 0x00000000FFFFFFFF ) == 0x00000000BAADF00D )
                {
                    tmp = (char**)(((RU64)tmp & 0xFFFFFFFF00000000) >> 32);
                }

                addr.s_addr = *(u_long*)(tmp[ 0 ]);
#else
                if( 0 != hostinfo->h_addr_list[ 0 ] )
                {
                    addr.s_addr = *(u_long *) hostinfo->h_addr_list[ 0 ];
                }
#endif
                ip = addr.S_un.S_addr;
            }
        }
    }
#elif defined( RPAL_PLATFORM_LINUX )
    static struct ifreq ifreqs[ 32 ];
    struct ifconf ifconf;
    int sd = 0;
    int r = 0;
    int i = 0;
    int ifc_num = 0;
    
    memset( &ifconf, 0, sizeof( ifconf ) );
    ifconf.ifc_req = ifreqs;
    ifconf.ifc_len = sizeof( ifreqs );

    if( 0 <= ( sd = socket( PF_INET, SOCK_STREAM, 0 ) ) )
    {
		if( 0 == ( r = ioctl( sd, SIOCGIFCONF, (char *)&ifconf ) ) )
		{
			ifc_num = ifconf.ifc_len / sizeof( struct ifreq );
			for( i = 0; i < ifc_num; ++i )
			{
				if( AF_INET != ifreqs[ i ].ifr_addr.sa_family ||
					0x0100007F == (RU32)( (struct sockaddr_in *)&ifreqs[ i ].ifr_addr )->sin_addr.s_addr )
				{
					continue;
				}
		
				ip = (RU32)( (struct sockaddr_in *)&ifreqs[ i ].ifr_addr )->sin_addr.s_addr;
				break;
			}
		}
    }

    close( sd );
#elif defined( RPAL_PLATFORM_MACOSX )
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	
	// retrieve the current interfaces - returns 0 on success
	if( 0 == ( success = getifaddrs( &interfaces ) ) )
	{
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while( NULL != temp_addr )
		{
			if( AF_INET == temp_addr->ifa_addr->sa_family &&
			    0x0100007F != (RU32)( ( (struct sockaddr_in *)temp_addr->ifa_addr )->sin_addr.s_addr ) )
			{
				ip = (RU32)( ( (struct sockaddr_in *)temp_addr->ifa_addr )->sin_addr.s_addr );
				break;
			}
			
			temp_addr = temp_addr->ifa_next;
		}
	}
	
	// Free memory
	freeifaddrs( interfaces );
#else
    rpal_debug_not_implemented();
#endif
    return ip;
}


RU8
    libOs_getCpuUsage
    (

    )
{
    RU8 percent = (RU8)(-1);

#ifdef RPAL_PLATFORM_WINDOWS
    static RBOOL isLoadable = TRUE;
    static GetSystemTimes_f getSysTimes = NULL;
    RCHAR libName[] = "kernel32.dll";
    RCHAR funcName[] = "GetSystemTimes";
    ULARGE_INTEGER tIdle;
    ULARGE_INTEGER tKernel;
    ULARGE_INTEGER tUser;

    static ULARGE_INTEGER hIdle;
    static ULARGE_INTEGER hKernel;
    static ULARGE_INTEGER hUser;

    ULONGLONG curIdle;
    ULONGLONG curUsed;
    FLOAT prt = 0;

    if( NULL == getSysTimes &&
        isLoadable )
    {
        // Try to load the function, only available XP SP1++
        getSysTimes = (GetSystemTimes_f)GetProcAddress( LoadLibraryA( libName ), funcName );

        isLoadable = FALSE;
    }

    if( NULL != getSysTimes )
    {
        if( getSysTimes( (LPFILETIME)&tIdle, (LPFILETIME)&tKernel, (LPFILETIME)&tUser ) )
        {
            curIdle = tIdle.QuadPart - hIdle.QuadPart;
            curUsed = tKernel.QuadPart - hKernel.QuadPart;
            curUsed += tUser.QuadPart - hUser.QuadPart;

            if( 0 != curUsed )
            {
                prt = (FLOAT)( (curUsed - curIdle) * 100 / curUsed );
            }
            else
            {
                prt = 0;
            }

            percent = (BYTE)( prt );

            hUser.QuadPart = tUser.QuadPart;
            hKernel.QuadPart = tKernel.QuadPart;
            hIdle.QuadPart = tIdle.QuadPart;
        }
    }
#elif defined( RPAL_PLATFORM_LINUX )
    static long long int hIdle;
    static long long int hKernel;
    static long long int hUser;
    static long long int hUserLow;
    long long int tUser = 0;
    long long int tKernel = 0;
    long long int tIdle = 0;
    long long int tUserLow = 0;
    long long int total = 0;
    long long int used = 0;
    FILE* hStat = NULL;
    RCHAR statFile[] = "/proc/stat";
    int unused;
    
    if( NULL != ( hStat = fopen( (RPCHAR)statFile, "r" ) ) )
    {
        unused = fscanf( hStat, "cpu %llu %llu %llu %llu",
  			 &tUser,
			 &tUserLow,
			 &tKernel,
			 &tIdle );
    
        fclose( hStat );
    
        if( hUser > tUser ||
            hKernel > tKernel ||
            hIdle > tIdle ||
            hUserLow > tUserLow )
        {
            // Overflow
            percent = 0;
        }
        else
        {
            total = ( tUser - hUser ) + ( tUserLow - hUserLow ) + ( tKernel - hKernel );
            used = total;
            total += ( tIdle - hIdle );
            if( 0 == used || 0 == total )
            {
                percent = 0;
            }
            else
            {
                percent = ( (float)used / (float)total ) * 100;
            }
        }
    
        hIdle = tIdle;
        hUser = tUser;
        hUserLow = tUserLow;
        hKernel = tKernel;
    }
#elif defined( RPAL_PLATFORM_MACOSX )
    double load = 0;
    if( 1 == getloadavg( &load, 1 ) )
    {
        if( 1 > load )
        {
            percent = load * 100;
        }
        else
        {
            percent = 100;
        }
    }
#else
    rpal_debug_not_implemented();
#endif

    return percent;
}


RU32
    libOs_getUsageProportionalTimeout
    (
        RU32 maxTimeout
    )
{
    RU32 timeout = 0;
    RU8 usage = 0;

    usage = libOs_getCpuUsage();
    if( 0xFF == usage )
    {
        // Looks like we don't have access to the usage
        // so let's assume 50%... :-/
        usage = 50;
    }

    timeout = (RU32)( ( (RFLOAT)usage / 100 ) * maxTimeout );
    
    return timeout;
}


RU32
    libOs_getOsVersion
    (

    )
{
    RU32 version = 0;
#ifdef RPAL_PLATFORM_WINDOWS
    OSVERSIONINFO versionEx = {0};

    versionEx.dwOSVersionInfoSize = sizeof( versionEx );

    if( GetVersionEx( &versionEx ))
    {
        if( 5 > versionEx.dwMajorVersion ||
            ( 5 == versionEx.dwMajorVersion && 1 > versionEx.dwMinorVersion ) )
        {
            version = OSLIB_VERSION_WINDOWS_OLD;
        }
        else if( 5 == versionEx.dwMajorVersion && 1 == versionEx.dwMinorVersion )
        {
            version = OSLIB_VERSION_WINDOWS_XP;
        }
        else if( 5 == versionEx.dwMajorVersion && 2 == versionEx.dwMinorVersion )
        {
            version = OSLIB_VERSION_WINDOWS_2K3;
        }
        else if( 6 == versionEx.dwMajorVersion && 0 == versionEx.dwMinorVersion )
        {
            version = OSLIB_VERSION_WINDOWS_VISTA_2K8;
        }
        else if( 6 == versionEx.dwMajorVersion && 1 == versionEx.dwMinorVersion )
        {
            version = OSLIB_VERSION_WINDOWS_7_2K8R2;
        }
        else if( 6 == versionEx.dwMajorVersion && 2 == versionEx.dwMinorVersion )
        {
            version = OSLIB_VERSION_WINDOWS_8;
        }
        else if( 6 < versionEx.dwMajorVersion ||
                 ( 6 == versionEx.dwMajorVersion && 2 < versionEx.dwMinorVersion ) )
        {
            version = OSLIB_VERSION_WINDOWS_FUTURE;
        }
    }
#else
    rpal_debug_not_implemented();
#endif

    return version;
}


RBOOL
    libOs_getInstalledPackages
    (
        LibOsPackageInfo** pPackages,
        RU32* nPackages
    )
{
    RBOOL isSuccess = FALSE;
    RU32 i = 0;
#ifdef RPAL_PLATFORM_WINDOWS
    HKEY root = HKEY_LOCAL_MACHINE;
    RWCHAR packagesDir[] = _WCH( "software\\microsoft\\windows\\currentversion\\uninstall\\" );
    RWCHAR dName[] = _WCH( "displayname" );
    RWCHAR dVersion[] = _WCH( "displayversion" );
    RWCHAR packId[ 256 ] = {0};
    RWCHAR tmp[ 512 ] = {0};
    HKEY hPackages = NULL;
    HKEY hPackage = NULL;
    RU32 type = 0;
    RU32 size = 0;
#endif

    if( NULL != pPackages &&
        NULL != nPackages )
    {
        *pPackages = NULL;
        *nPackages = 0;
#ifdef RPAL_PLATFORM_WINDOWS
        if( ERROR_SUCCESS == RegOpenKeyW( root, packagesDir, &hPackages ) )
        {
            while( ERROR_SUCCESS == RegEnumKeyW( hPackages, i, packId, sizeof( packId ) / sizeof( RWCHAR ) ) )
            {
                i++;
                isSuccess = TRUE;
                *nPackages = i;

                if( ERROR_SUCCESS == RegOpenKeyW( hPackages, packId, &hPackage ) )
                {
                    *pPackages = rpal_memory_realloc( *pPackages, sizeof( LibOsPackageInfo ) * i );

                    if( NULL != *pPackages )
                    {
                        rpal_memory_zero( &(*pPackages)[ i - 1 ], sizeof( (*pPackages)[ i - 1 ] ) );
                        size = sizeof( tmp ) - sizeof( RWCHAR );

                        if( ERROR_SUCCESS == RegQueryValueExW( hPackage, dName, NULL, (LPDWORD)&type, (RPU8)tmp, (LPDWORD)&size ) )
                        {
                            if( REG_SZ == type ||
                                REG_EXPAND_SZ == type )
                            {
                                size = rpal_string_strlenw( tmp ) * sizeof( RWCHAR );
                                rpal_memory_memcpy( &(*pPackages)[ i - 1 ].name, tmp, size >= sizeof( (*pPackages)[ i - 1 ].name ) ? sizeof( (*pPackages)[ i - 1 ].name ) - sizeof( RWCHAR ) : size );
                            }
                        }

                        if( ERROR_SUCCESS == RegQueryValueExW( hPackage, dVersion, NULL, (LPDWORD)&type, (RPU8)tmp, (LPDWORD)&size ) )
                        {
                            if( REG_SZ == type ||
                                REG_EXPAND_SZ == type )
                            {
                                size = rpal_string_strlenw( tmp );
                                rpal_memory_memcpy( &(*pPackages)[ i - 1 ].version, tmp, size >= sizeof( (*pPackages)[ i - 1 ].version ) ? sizeof( (*pPackages)[ i - 1 ].version ) - sizeof( RWCHAR ) : size );
                            }
                        }
                    }
                    else
                    {
                        isSuccess = FALSE;
                        break;
                    }

                    RegCloseKey( hPackage );
                }

                rpal_memory_zero( tmp, sizeof( tmp ) );
            }

            RegCloseKey( hPackages );
        }

        if( !isSuccess )
        {
            rpal_memory_free( *pPackages );
            *pPackages = NULL;
            *nPackages = 0;
        }
#else
        rpal_debug_not_implemented();
#endif
    }

    return isSuccess;
}


#ifdef RPAL_PLATFORM_WINDOWS
static
RBOOL
    libOS_validateCertChain
    (
        rSequence  signature,
        RU32       operation,
        RBOOL*     pIsVerified_local,
        RBOOL*     pIsVerified_global,
        CRYPT_PROVIDER_SGNR* cryptProviderSigner
    )
{
    RBOOL isSucceed = FALSE;
    RU32 tag = RP_TAGS_CERT_CHAIN_ERROR;
    PCCERT_CHAIN_CONTEXT pchainContext = NULL;
    HCERTCHAINENGINE     hchainEngine  = NULL;

    CERT_CHAIN_PARA           chainPara    = {0};
    CERT_CHAIN_ENGINE_CONFIG  engineConfig = {0};
    RU32 chainBuildingFlags = CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT |
                              CERT_CHAIN_CACHE_END_CERT;

    if ( loadCrypt32() &&
         NULL != pIsVerified_local   &&
         NULL != pIsVerified_global  &&
         NULL != cryptProviderSigner &&
         NULL != cryptProviderSigner->pasCertChain &&
         NULL != cryptProviderSigner->pasCertChain->pCert )
    {
        *pIsVerified_local = FALSE;
        *pIsVerified_global = FALSE;

        rpal_memory_zero( &engineConfig, sizeof( engineConfig ) );
        engineConfig.cbSize = sizeof( engineConfig );
        engineConfig.dwUrlRetrievalTimeout = 0;

        if( RCertCreateCertificateChainEngine( &engineConfig,
                                               &hchainEngine ) )
        {   
            rpal_memory_zero( &chainPara, sizeof( chainPara ) );
            chainPara.cbSize = sizeof( chainPara );
            chainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
            chainPara.RequestedUsage.Usage.cUsageIdentifier = 0;
            chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;

            if (  IS_FLAG_ENABLED( OSLIB_SIGNCHECK_NO_NETWORK, operation ) )
            {
                chainBuildingFlags = chainBuildingFlags | 
                                     CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY |
                                     CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL;
            }
            
            // Chain building must be done with the certificate timestamp
            // See this article for info : https://www.eldos.com/security/articles/5731.php?page=all
            if( RCertGetCertificateChain( hchainEngine,
                                          cryptProviderSigner->pasCertChain->pCert,
                                          &cryptProviderSigner->sftVerifyAsOf,
                                          NULL,
                                          &chainPara,
                                          chainBuildingFlags,
                                          NULL,
                                          &pchainContext ) )
            {       
                switch ( pchainContext->TrustStatus.dwErrorStatus )
                {
                    case CERT_TRUST_NO_ERROR :
                        *pIsVerified_local = TRUE;
                        tag = RP_TAGS_CERT_CHAIN_VERIFIED;
                        break;

                    case CERT_TRUST_IS_PARTIAL_CHAIN |
                         CERT_TRUST_IS_UNTRUSTED_ROOT |
                         CERT_TRUST_IS_NOT_SIGNATURE_VALID:
                        tag = RP_TAGS_CERT_CHAIN_UNTRUSTED;
                        break;

                    case CERT_TRUST_IS_NOT_TIME_VALID:
                        tag = RP_TAGS_CERT_TRUST_IS_NOT_TIME_VALID;
                        break;
        
                    case CERT_TRUST_IS_REVOKED:
                        tag = RP_TAGS_CERT_TRUST_IS_REVOKED;
                        break;

                    default:
                        tag = RP_TAGS_CERT_CHAIN_UNTRUSTED;
                }

                // Check for self signed certificats
                if( pchainContext->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED )
                {
                    tag = RP_TAGS_CERT_SELF_SIGNED;
                }
                isSucceed = TRUE;
            }
        }
    }
    rSequence_addRU32( signature, RP_TAGS_CERT_CHAIN_STATUS, tag );

    if ( !IS_FLAG_ENABLED( OSLIB_SIGNCHECK_NO_NETWORK, operation ) )
    {
        *pIsVerified_global = *pIsVerified_local;
    }

    if( NULL != pchainContext ) RCertFreeCertificateChain( pchainContext );
    if( NULL != hchainEngine )  RCertFreeCertificateChainEngine( hchainEngine );
    return isSucceed;
}
#endif


#ifdef RPAL_PLATFORM_WINDOWS
static
RU32
    libOS_decodeCertName
    (
        RU32 certEncodingType,
        PCERT_NAME_BLOB pName,
        RU32 strType,
        RPCHAR *str
    )
{
    RU32 strSize  = 0;

    if ( ( NULL != pName ) && ( NULL == (*str) ) )
    {
        strSize = RCertNameToStr( certEncodingType,
                                  pName,
                                  strType,
                                  NULL,
                                  0 );

        if( (  0 != strSize ) &&
            ( NULL != ( ( *str ) = rpal_memory_alloc( ( strSize * sizeof( RCHAR ) ) + 1 ) ) ) )
        {
            if( 0 == RCertNameToStr( certEncodingType,
                                     pName,
                                     strType,
                                     *str,
                                     strSize ) )
            {
                rpal_memory_free( *str );
                strSize = 0;
                str = NULL;
            }
            else
            {
                // I don't trust the Windows API and since this may be coming from user data, we play it safe.
                (*str)[ strSize * sizeof( RCHAR ) ] = 0;
            }
        }
    }
    return strSize;
}
#endif


#ifdef RPAL_PLATFORM_WINDOWS
static
RBOOL
    libOs_retreiveSignatureInfo
    (
        WINTRUST_DATA*  winTrust_data,
        rSequence       signature,
        RU32            operation,
        RBOOL*          pIsVerified_local,
        RBOOL*          pIsVerified_global
    )
{
    RBOOL      isSucceed  = FALSE;
    RPCHAR     issuerStr  = NULL;
    RPCHAR     subjectStr = NULL;
    //FILETIME   fileTime   = { 0 };
    //SYSTEMTIME sysTime    = { 0 };
    CRYPT_PROVIDER_DATA* cryptProviderData   = NULL;
    CRYPT_PROVIDER_SGNR* cryptProviderSigner = NULL;

    if( loadWinTrustApi()  &&
        loadCrypt32()      &&
        NULL != pIsVerified_local &&
        NULL != pIsVerified_global )
    {
        if( NULL != winTrust_data ||
            NULL != winTrust_data->hWVTStateData )
        {

            cryptProviderData = WTHelperProvDataFromStateData( winTrust_data->hWVTStateData );
            cryptProviderSigner = WTHelperGetProvSignerFromChain( cryptProviderData, 0, FALSE, 0 );
            if( NULL != cryptProviderSigner ||
                NULL != cryptProviderSigner->pasCertChain ||
                NULL != cryptProviderSigner->pasCertChain->pCert ||
                NULL != cryptProviderSigner->pasCertChain->pCert->pCertInfo )
            {
                //FileTimeToLocalFileTime(&cryptProviderSigner->pasCertChain->pCert->pCertInfo->NotBefore, &fileTime);
                //FileTimeToSystemTime(&fileTime, &sysTime);

                //_tprintf(_T("Signature Date = %.2d/%.2d/%.4d at %.2d:%2.d:%.2d\n"), sysTime.wDay, sysTime.wMonth,sysTime.wYear, sysTime.wHour,sysTime.wMinute,sysTime.wSecond);
                
                if( 0 != libOS_decodeCertName( X509_ASN_ENCODING,
                                               &cryptProviderSigner->pasCertChain->pCert->pCertInfo->Issuer,
                                               CERT_X500_NAME_STR,
                                               &issuerStr ) )
                {
                    rSequence_addSTRINGA( signature, RP_TAGS_CERT_ISSUER, issuerStr );
                }

                if( 0 != libOS_decodeCertName( X509_ASN_ENCODING,
                                               &cryptProviderSigner->pasCertChain->pCert->pCertInfo->Subject,
                                               CERT_X500_NAME_STR,
                                               &subjectStr ) )
                {
                    rSequence_addSTRINGA( signature, RP_TAGS_CERT_SUBJECT, subjectStr );        
                }

                if( ( IS_FLAG_ENABLED( OSLIB_SIGNCHECK_INCLUDE_RAW_CERT, operation ) ) &&
                    ( 0 != cryptProviderSigner->pasCertChain->pCert->cbCertEncoded )   &&
                    ( NULL != (RPU8)cryptProviderSigner->pasCertChain->pCert->pbCertEncoded ) )
                {
                    rSequence_addBUFFER( signature, RP_TAGS_CERT_BUFFER,
                                         (RPU8)cryptProviderSigner->pasCertChain->pCert->pbCertEncoded,
                                         (RU32)cryptProviderSigner->pasCertChain->pCert->cbCertEncoded );
                }

                if( IS_FLAG_ENABLED(OSLIB_SIGNCHECK_CHAIN_VERIFICATION, operation) )
                {
                    isSucceed = libOS_validateCertChain( signature,
                                                         operation,
                                                         pIsVerified_local,
                                                         pIsVerified_global,
                                                         cryptProviderSigner );
                }
                else
                {
                    rSequence_addRU32( signature, RP_TAGS_CERT_CHAIN_STATUS, RP_TAGS_CERT_CHAIN_NOT_VERIFIED );
                    isSucceed = TRUE;
                    *pIsVerified_local = FALSE;
                    *pIsVerified_global = FALSE;
                }

                rpal_memory_free( issuerStr );
                rpal_memory_free( subjectStr );
            }
        }
    }
    return isSucceed;
}
#endif


#ifdef RPAL_PLATFORM_WINDOWS
static
RBOOL
    libOs_getCATSignature
    (
        RPWCHAR   pwFilePath,
        rSequence signature,
        RU32      operation,
        RBOOL*    pIsSigned,
        RBOOL*    pIsVerified_local,
        RBOOL*    pIsVerified_global
    )
{
    RBOOL    isSucceed = FALSE;
    HCATINFO catalogHandle = NULL;
    CATALOG_INFO catalogInfo = {0};
    HCATADMIN hAdmin = NULL;
    HANDLE fileHandle = NULL;
    RU8*  hash = NULL;
    DWORD hashSize = 0;

    if ( loadWinTrustApi() &&
         NULL != pIsSigned &&
         NULL != pIsVerified_local &&
         NULL != pIsVerified_global )
    {
        fileHandle = CreateFileW( pwFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
        if( fileHandle != INVALID_HANDLE_VALUE )
        {

            CryptCATAdminCalcHashFromFileHandle( fileHandle, &hashSize, NULL, 0 );
            if( 0 < hashSize ) 
            {
                if( NULL != ( hash = rpal_memory_alloc( hashSize * sizeof(RU8) ) ) )
                {
                    if( CryptCATAdminCalcHashFromFileHandle( fileHandle, &hashSize, hash, 0 ) &&
                        CryptCATAdminAcquireContext( &hAdmin, NULL, 0 ) &&
                        NULL != hAdmin )
                    {
                        // Enum catalogs to find the ones that contains the hash
                        catalogHandle = CryptCATAdminEnumCatalogFromHash(hAdmin, hash, hashSize, 0, NULL);
                        if ( NULL != catalogHandle)
                        {
                            rpal_memory_zero( &catalogInfo, sizeof(catalogInfo) );
                            if( CryptCATCatalogInfoFromContext( catalogHandle, &catalogInfo, 0 ) )
                            {
                                isSucceed = libOs_getFileSignature( catalogInfo.wszCatalogFile, signature, operation, pIsSigned, pIsVerified_local, pIsVerified_global );
                            }
                            CryptCATAdminReleaseCatalogContext( hAdmin, catalogHandle, 0 );
                        }
                        else
                        {
                            // No CAT found for this file
                            isSucceed = TRUE;
                            *pIsSigned = FALSE;
                            *pIsVerified_local = FALSE;
                            *pIsVerified_global = FALSE;
                        }
                    }
                    rpal_memory_free( hash );
                    CryptCATAdminReleaseContext( hAdmin, 0 );
                }
            }
            CloseHandle( fileHandle );
        }
    }
    return isSucceed;
}
#endif

static
RBOOL
    libOs_getFileSignature
    (
        RPWCHAR   pwfilePath,
        rSequence signature,
        RU32      operation,
        RBOOL*    pIsSigned,
        RBOOL*    pIsVerified_local,
        RBOOL*    pIsVerified_global
    )
{
    RBOOL isSucceed = FALSE;
#ifdef RPAL_PLATFORM_WINDOWS
    RU32  lStatus = 0;
    DWORD dwLastError = 0;

    if( loadWinTrustApi() && 
        NULL != pIsSigned &&
        NULL != pIsVerified_local &&
        NULL != pIsVerified_global)
    {
        WINTRUST_FILE_INFO fileInfo = {0};
        WINTRUST_DATA winTrustData  = {0};
        GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
        *pIsSigned = FALSE;

        rpal_memory_zero( &fileInfo, sizeof( fileInfo ) );
        fileInfo.cbStruct = sizeof( fileInfo );
        fileInfo.pcwszFilePath = pwfilePath;
        fileInfo.hFile = NULL;
        fileInfo.pgKnownSubject = NULL;

        rpal_memory_zero( &winTrustData, sizeof( winTrustData ) );
        winTrustData.cbStruct = sizeof( winTrustData );
        winTrustData.pPolicyCallbackData = NULL;
        winTrustData.pSIPClientData = NULL;
        winTrustData.dwUIChoice = WTD_UI_NONE;
        winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
        winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
        winTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
        winTrustData.hWVTStateData = NULL;
        winTrustData.pwszURLReference = NULL;
        winTrustData.dwUIContext = 0;
        winTrustData.pFile = &fileInfo;

        lStatus = WinVerifyTrust( NULL, &WVTPolicyGUID, &winTrustData );

        switch( lStatus ) 
        {
            case 0:
                // If the Windows trust provider verifies that the subject is trusted
                // for the specified action, the return value is zero.
                *pIsSigned = TRUE;
                isSucceed = libOs_retreiveSignatureInfo( &winTrustData, signature, operation, pIsVerified_local, pIsVerified_global );
                break;
        
            case TRUST_E_NOSIGNATURE:
                dwLastError = rpal_error_getLast();

                if ( TRUST_E_NOSIGNATURE == dwLastError ||
                     TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
                     TRUST_E_PROVIDER_UNKNOWN == dwLastError ) 
                {
                    isSucceed = libOs_getCATSignature( pwfilePath, signature, operation, pIsSigned, pIsVerified_local, pIsVerified_global );
                } 
                break;

            // Case where the file is signed but the signature is unknown (untrusted) by Windows. Most thirdparty signature will end here.
            case CRYPT_E_SECURITY_SETTINGS:
            case TRUST_E_SUBJECT_NOT_TRUSTED:
            case CERT_E_CHAINING:
                *pIsSigned = TRUE;
                libOs_retreiveSignatureInfo( &winTrustData, signature, operation, pIsVerified_local, pIsVerified_global );
                break;

            case CRYPT_E_FILE_ERROR:
                rpal_debug_warning( "file IO error 0x%x", lStatus );
                break;

            default:
                rpal_debug_warning( "error checking sig 0x%x", lStatus );
                break;
        }

        // Cleanup
        winTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
        lStatus = WinVerifyTrust( NULL, &WVTPolicyGUID, &winTrustData );
    }
#else
    rpal_debug_not_implemented();
#endif
    return isSucceed;
}   


RBOOL
    libOs_getSignature
    (
        RPWCHAR    pwfilePath,
        rSequence* signature,
        RU32       operation,
        RBOOL*     pIsSigned,
        RBOOL*     pIsVerified_local,
        RBOOL*     pIsVerified_global
    )
{
    RBOOL isSucceed = FALSE;
    if( NULL == *signature &&
        NULL != pIsSigned  &&
        NULL != pIsVerified_local  &&
        NULL != pIsVerified_global &&
        NULL != ( *signature = rSequence_new() ) )
    {
        rSequence_addSTRINGW( *signature, RP_TAGS_FILE_PATH, pwfilePath );
        isSucceed = libOs_getFileSignature( pwfilePath, *signature, operation, pIsSigned, pIsVerified_local, pIsVerified_global );

        if( !isSucceed && NULL != *signature )
        {
            rSequence_free( *signature );
            *signature = NULL;
        }
    }
    return isSucceed;
}


RBOOL
    libOs_getSystemCPUTime
    (
        RU64* cpuTime
    )
{
#if defined( RPAL_PLATFORM_WINDOWS )
    static GetSystemTimes_f gst = NULL;
    RBOOL isSuccess = FALSE;
    FILETIME ftUser = { 0 };
    FILETIME ftKernel = { 0 };
    FILETIME ftIdle = { 0 };
    RCHAR getSystemTime[] = "GetSystemTimes";
    ULARGE_INTEGER uUser = { 0 };
    ULARGE_INTEGER uKernel = { 0 };
    ULARGE_INTEGER uIdle = { 0 };

    if( NULL == cpuTime )
    {
        return FALSE;
    }

    if( NULL == gst )
    {
        gst = (GetSystemTimes_f)GetProcAddress( GetModuleHandleW( _WCH( "kernel32.dll" ) ),
                                                getSystemTime );
        if( NULL == gst )
        {
            rpal_debug_error( "Cannot get the address to GetSystemTimes -- error code %u.", GetLastError() );
            isSuccess = FALSE;
        }
    }

    if( NULL != gst && 
        gst( &ftIdle, &ftKernel, &ftUser ) )
    {
        FILETIME2ULARGE( uUser, ftUser );
        FILETIME2ULARGE( uKernel, ftKernel );
        FILETIME2ULARGE( uIdle, ftIdle );
        
        *cpuTime = uUser.QuadPart + uKernel.QuadPart + uIdle.QuadPart;
        
        isSuccess = TRUE;
    }
    else
    {
        rpal_debug_error( "Unable to get system times -- error code %u.", GetLastError() );
        isSuccess = FALSE;
    }

    return isSuccess;
#elif defined( RPAL_PLATFORM_LINUX )
    static RU64 user_hz = 0;
    FILE* proc_stat = NULL;
    RCHAR line[ 256 ] = { 0 };
    RCHAR* saveptr = NULL;
    RCHAR* tok = NULL;
    RPCHAR unused = NULL;

    if( 0 == user_hz )
    {
        long tmp = sysconf( _SC_CLK_TCK );
        if( tmp < 0 )
        {
            rpal_debug_error( "Cannot find the proc clock tick size -- error code %u.", errno );
            return FALSE;
        }
        user_hz = (RU64)tmp;
    }
    if( NULL == cpuTime )
    {
        return TRUE;
    }

    if( NULL != ( proc_stat = fopen( "/proc/stat", "r" ) ) )
    {
        rpal_memory_zero( line, sizeof( line ) );

        while( !feof( proc_stat ) )
        {
            unused = fgets( line, sizeof( line ), proc_stat );
            if( line == strstr( line, "cpu " ) )
            {
                saveptr = NULL;
                tok = strtok_r( line, " ", &saveptr );
                *cpuTime = 0;
                
                while( NULL != tok )
                {
                    if( isdigit( tok[ 0 ] ) )
                    {
                        *cpuTime += (RU64)atol( tok );
                    }
                    tok = strtok_r( NULL, " ", &saveptr );
                }

                /* user_hz = clock ticks per second; we want time in microseconds. */
                *cpuTime = ( *cpuTime * 1000000 ) / user_hz;
                break;
            }
        }
        fclose( proc_stat );
    }
    else
    {
        rpal_debug_error( "Cannot open /proc/stat -- error code %u.", errno );
    }

    return *cpuTime > 0;
#elif defined( RPAL_PLATFORM_MACOSX )
    static RU64 numCPU = 0;
    struct timeval now = { 0 };
    RS32 iCPU = 0;
    size_t len = 4;

    if( 0 == numCPU )
    {
        if( 0 == sysctlbyname( "hw.ncpu", &iCPU, &len, NULL, 0 ) )
        {
            numCPU = (RU64)iCPU;
        }
        else
        {
            rpal_debug_error( "Cannot get the number of CPUs -- error code %u.", errno );
            return FALSE;
        }
    }

    if( 0 == gettimeofday( &now, NULL ) )
    {
        if( NULL != cpuTime )
        {
            *cpuTime = numCPU * ( (RU64)now.tv_sec * 1000000 + (RU64)now.tv_usec );
        }
        return TRUE;
    }
    else
    {
        rpal_debug_error( "Cannot get the system time -- error code %u.", errno );
    }

    return FALSE;
#else
    rpal_debug_not_implemented();
#endif
}


RBOOL
    libOs_getProcessInfo
    (
        RU64* pTime,
        RU64* pSizeMemResident,
        RU64* pNumPageFaults
    )
{
    RBOOL isSuccess = FALSE;
#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE hSelf = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE,
                                GetCurrentProcessId() );

    FILETIME ftCreation = { 0 };
    FILETIME ftExit = { 0 };
    FILETIME ftKernel = { 0 };
    FILETIME ftUser = { 0 };

    ULARGE_INTEGER uKernel = { 0 };
    ULARGE_INTEGER uUser = { 0 };
    
    static GetProcessMemoryInfo_f gpmi = NULL;
    PROCESS_MEMORY_COUNTERS pmc = { 0 };

    RCHAR k32GetProcessMemoryInfo[] = "K32GetProcessMemoryInfo";
    RCHAR getProcessMemoryInfo[] = "GetProcessMemoryInfo";

    if( NULL != hSelf )
    {
        isSuccess = TRUE;

        if( NULL != pTime )
        {
            if( GetProcessTimes( hSelf, &ftCreation, &ftExit, &ftKernel, &ftUser ) )
            {
                FILETIME2ULARGE( uKernel, ftKernel );
                FILETIME2ULARGE( uUser, ftUser );
                *pTime = uUser.QuadPart + uKernel.QuadPart;
            }
            else
            {
                rpal_debug_error( "Cannot get process times -- error code %u.", GetLastError() );
                isSuccess = FALSE;
            }
        }

        if( NULL != pSizeMemResident || NULL != pNumPageFaults )
        {
            if( NULL == gpmi )
            {
                gpmi = (GetProcessMemoryInfo_f)GetProcAddress( GetModuleHandleW( _WCH( "kernel32.dll" ) ),
                                                               k32GetProcessMemoryInfo );
                if( NULL == gpmi )
                {
                    gpmi = (GetProcessMemoryInfo_f)GetProcAddress( GetModuleHandleW( _WCH( "kernel32.dll" ) ),
                                                                   getProcessMemoryInfo );
                    if( NULL == gpmi )
                    {
                        rpal_debug_error(
                                "Cannot load the address to GetProcessMemoryInfo nor K32GetProcessMemoryInfo -- error code %u.",
                                GetLastError()
                                );
                        isSuccess = FALSE;
                    }
                }
            }

            if( NULL != gpmi && 
                gpmi( hSelf, &pmc, sizeof( pmc ) ) )
            {
                if( NULL != pNumPageFaults )
                {
                    *pNumPageFaults = pmc.PageFaultCount;
                }
                if( NULL != pSizeMemResident )
                {
                    *pSizeMemResident = pmc.WorkingSetSize;
                }
            }
            else
            {
                rpal_debug_error( "Failure while fetching process memory info -- error code %u.", GetLastError() );
                isSuccess = FALSE;
            }
        }

        CloseHandle( hSelf );
    }

#else
    struct rusage usage = { 0 };

    if( 0 == getrusage( RUSAGE_SELF, &usage ) )
    {
        isSuccess = TRUE;
        if( NULL != pTime )
        {
            *pTime = ( (RU64)usage.ru_utime.tv_sec ) * 1000000 + (RU64)usage.ru_utime.tv_usec;
            *pTime += ( (RU64)usage.ru_stime.tv_sec ) * 1000000 + (RU64)usage.ru_stime.tv_usec;
        }
        if( NULL != pNumPageFaults )
        {
            *pNumPageFaults = usage.ru_majflt;  /* Only care about I/O-causing page faults. */
        }
        if( NULL != pSizeMemResident )
        {
#if defined( RPAL_PLATFORM_LINUX )
            *pSizeMemResident = usage.ru_maxrss * 1024;  /* Linux gives this in KB. */
#elif defined( RPAL_PLATFORM_MACOSX )
            *pSizeMemResident = usage.ru_maxrss;         /* OS X gives this in B. */
#else
        rpal_debug_not_implemented();
#endif
        }
    }
    else
    {
        rpal_debug_error( "Cannot get process resource usage information -- error code %u.", errno );
        isSuccess = FALSE;
    }
#endif
    return isSuccess;
}


RBOOL
    libOs_getThreadTime
    (
        rThreadID threadId,
        RU64* pTime
    )
{
    RBOOL isSuccess = TRUE;

    if( NULL != pTime )
    {
#if defined( RPAL_PLATFORM_WINDOWS )
        HANDLE hThread = NULL;
        FILETIME ftCreation = { 0 };
        FILETIME ftExit = { 0 };
        FILETIME ftSystem = { 0 };
        FILETIME ftUser = { 0 };
        ULARGE_INTEGER uSystem = { 0 };
        ULARGE_INTEGER uUser = { 0 };

        hThread = OpenThread( THREAD_ALL_ACCESS , FALSE, threadId );
        if( NULL != hThread )
        {
            if( GetThreadTimes( hThread, &ftCreation, &ftExit, &ftSystem, &ftUser ) )
            {
                FILETIME2ULARGE( uSystem, ftSystem );
                FILETIME2ULARGE( uUser, ftUser );
                *pTime = uUser.QuadPart + uSystem.QuadPart;
                isSuccess = TRUE;
            }
            else
            {
                rpal_debug_error( "GetThreadTimes failed for thread ID %u, handle %p.", threadId, hThread );
            }
            CloseHandle( hThread );
        }
        else
        {
            rpal_debug_error( "Unable to open a new handle to thread ID %u to get its performance.\n", threadId );
        }
#elif defined( RPAL_PLATFORM_MACOSX )
        kern_return_t kr;
        thread_basic_info_data_t info = { 0 };
        mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;

        if( KERN_SUCCESS == ( kr = thread_info( threadId, THREAD_BASIC_INFO, (thread_info_t)&info, &count ) ) )
        {
            *pTime = ( (RU64)info.user_time.seconds + (RU64)info.system_time.seconds ) * 1000000 +
                    (RU64)info.user_time.microseconds + (RU64)info.system_time.microseconds;
            isSuccess = TRUE;
        }
        else
        {
            rpal_debug_error( "Access to thread info failed -- return code %d.", (int)kr );
            isSuccess = FALSE;
        }
#elif defined( RPAL_PLATFORM_LINUX )
    static RU64 user_hz = 0;
    char path_stat[ 1024 ] = { 0 };
    FILE* file_stat = NULL;
    long tmp = 0;
    unsigned long utime = 0;
    unsigned long stime = 0;

    if( 0 == user_hz )
    {
        tmp = sysconf( _SC_CLK_TCK );
        if( tmp < 0 )
        {
            rpal_debug_error( "Cannot find the proc clock tick size -- error code %u.", errno );
            return FALSE;
        }
        user_hz = (RU64)tmp;
    }

    snprintf( path_stat, sizeof( path_stat ), "/proc/%d/task/%d/stat", getpid(), threadId );
    if( NULL != ( file_stat = fopen( path_stat, "r" ) ) )
    {
        if( 2 == fscanf( file_stat,
                         "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", 
                         &utime,
                         &stime ) )
        {
            /* user_hz = clock ticks per second; we want time in microseconds. */
            *pTime = ( ( (RU64)utime + (RU64)stime ) * 1000000 ) / user_hz;
            isSuccess = TRUE;
        }
        else
        {
            rpal_debug_error( "Unable to read file %s properly.", path_stat );
        }

        fclose( file_stat );
    }
#else
    rpal_debug_not_implemented();
#endif
    }

    return isSuccess;
}


/* EOF */
