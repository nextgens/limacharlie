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

#include "beacon.h"
#include "configurations.h"
#include "globalContext.h"
#include "obfuscated.h"
#include <obfuscationLib/obfuscationLib.h>
#include <libb64/libb64.h>
#include <zlib/zlib.h>
#include <cryptoLib/cryptoLib.h>
#include "crypto.h"
#include <rpHostCommonPlatformLib/rTags.h>
#include <libOs/libOs.h>
#include "commands.h"
#include <liburl/liburl.h>
#include "crashHandling.h"

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <curl/curl.h>
#endif

#define RPAL_FILE_ID     50

//=============================================================================
//  Private defines and datastructures
//=============================================================================

//=============================================================================
//  Defined as platform specific later
//=============================================================================
static
RBOOL
    postHttp
    (
        RPCHAR location,
        RPCHAR params,
        RBOOL isEnforceCert,
        RPVOID* receivedData,
        RU32* receivedSize
    );

//=============================================================================
//  Helpers
//=============================================================================
static
RPU8
    findAndCapEncodedDataInBeacon
    (
        RPU8 beaconData,
        RU32 maxSize
    )
{
    RPU8 pData = NULL;
    RPU8 end = NULL;
    OBFUSCATIONLIB_DECLARE( header, RP_HCP_CONFIG_DATA_HEADER );
    OBFUSCATIONLIB_DECLARE( footer, RP_HCP_CONFIG_DATA_FOOTER );

    if( NULL != beaconData )
    {
        OBFUSCATIONLIB_TOGGLE( header );
        OBFUSCATIONLIB_TOGGLE( footer );
        
        if( ( sizeof( header ) + sizeof( footer ) + 1 ) < maxSize )
        {
            pData = rpal_memory_memmem( beaconData, maxSize, (RPCHAR)header, sizeof(header) - sizeof(RCHAR) );
    
            if( NULL != pData )
            {
                pData += sizeof( header ) - sizeof( RCHAR );
    
                end = rpal_memory_memmem( pData, maxSize - (RU32)( pData - beaconData - sizeof( header ) - sizeof( RCHAR ) ), footer, sizeof(footer) - sizeof(RCHAR) );
    
                if( NULL != end )
                {
                    if( maxSize > (RU32)( end - beaconData ) )
                    {
                        *end = 0;
                    }
                    else
                    {
                        pData = NULL;
                    }
                }
                else
                {
                    pData = NULL;
                }
            }
        }

        OBFUSCATIONLIB_TOGGLE( header );
        OBFUSCATIONLIB_TOGGLE( footer );
    }

    return pData;
}

static
RBOOL
    stripBeaconEncoding
    (
        RPCHAR beaconString,
        RPU8* pRawData,
        RU32* pRawSize
    )
{
    RBOOL isSuccess = FALSE;

    RPU8 tmpBuff1 = NULL;
    RU32 tmpSize1 = 0;

    if( NULL != beaconString &&
        NULL != pRawData &&
        NULL != pRawSize )
    {
        // Remove the base64
        if( base64_decode( beaconString, (RPVOID*)&tmpBuff1, &tmpSize1, FALSE ) )
        {
            *pRawData = tmpBuff1;
            *pRawSize = tmpSize1;
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

static
rString
    assembleOutboundStream
    (
        RpHcp_ModuleId moduleId,
        rList payload,
        RU8 sessionKey[ CRYPTOLIB_SYM_KEY_SIZE ],
        RU8 sessionIv[ CRYPTOLIB_SYM_IV_SIZE ]
    )
{
    rString str = NULL;

    rSequence hdrSeq = NULL;
    rSequence hcpid = NULL;

    rBlob blob = NULL;

    RPCHAR encoded = NULL;

    RPU8 encBuffer = NULL;
    RU64 encSize = 0;

    RPU8 finalBuffer = NULL;
    RU32 finalSize = 0;

    RPCHAR hostName = NULL;

    RBOOL isSuccess = FALSE;

    OBFUSCATIONLIB_DECLARE( payHdr, RP_HCP_CONFIG_PAYLOAD_HDR );

    str = rpal_stringbuffer_new( 0, 0, FALSE );

    if( NULL != str )
    {
        if( NULL != ( hdrSeq = rSequence_new() ) )
        {
            if( NULL != ( hcpid = hcpIdToSeq( g_hcpContext.currentId ) ) )
            {
                // System basic information
                // Host Name
                if( NULL != ( hostName = libOs_getHostName() ) )
                {
                    rSequence_addSTRINGA( hdrSeq, RP_TAGS_HOST_NAME, hostName );
                    rpal_memory_free( hostName );
                }
                else
                {
                    rpal_debug_warning( "could not determine hostname" );
                }

                // Internal IP
                rSequence_addIPV4( hdrSeq, RP_TAGS_IP_ADDRESS, libOs_getMainIp() );

                if( rSequence_addSEQUENCE( hdrSeq, RP_TAGS_HCP_ID, hcpid ) &&
                    rSequence_addRU8( hdrSeq, RP_TAGS_HCP_MODULE_ID, moduleId ) &&
                    rSequence_addBUFFER( hdrSeq, RP_TAGS_SYM_KEY, sessionKey, CRYPTOLIB_SYM_KEY_SIZE ) &&
                    rSequence_addBUFFER( hdrSeq, RP_TAGS_SYM_IV, sessionIv, CRYPTOLIB_SYM_IV_SIZE ) )
                {
                    if( NULL != g_hcpContext.enrollmentToken &&
                        0 != g_hcpContext.enrollmentTokenSize )
                    {
                        rSequence_addBUFFER( hdrSeq, RP_TAGS_HCP_ENROLLMENT_TOKEN, g_hcpContext.enrollmentToken, g_hcpContext.enrollmentTokenSize );
                    }

                    if( NULL != payload )
                    {
                        blob = rpal_blob_create( 0, 0 );
                    }

                    if( NULL == payload ||
                        NULL != blob )
                    {
                        if( rSequence_serialise( hdrSeq, blob ) &&
                            ( NULL == payload ||
                              rList_serialise( payload, blob ) ) )
                        {
                            encSize = compressBound( rpal_blob_getSize( blob ) );
                            encBuffer = rpal_memory_alloc( (RU32)encSize );

                            if( NULL == payload ||
                                NULL != encBuffer )
                            {
                                if( NULL == payload ||
                                    Z_OK == compress( encBuffer, (uLongf*)&encSize, (RPU8)rpal_blob_getBuffer( blob ), rpal_blob_getSize( blob ) ) )
                                {
                                    FREE_N_NULL( blob, rpal_blob_free );

                                    if( NULL == payload ||
                                        CryptoLib_fastAsymEncrypt( encBuffer, (RU32)encSize, getC2PublicKey(), &finalBuffer, &finalSize ) )
                                    {
                                        FREE_N_NULL( encBuffer, rpal_memory_free );

                                        if( NULL == payload ||
                                            base64_encode( finalBuffer, finalSize, &encoded, TRUE ) )
                                        {
                                            isSuccess = TRUE;

                                            if( NULL != payload )
                                            {
                                                FREE_N_NULL( finalBuffer, rpal_memory_free );

                                                DO_IFF( rpal_stringbuffer_add( str, "&" ), isSuccess );

                                                OBFUSCATIONLIB_TOGGLE( payHdr );

                                                DO_IFF( rpal_stringbuffer_add( str, (RPCHAR)payHdr ), isSuccess );
                                                DO_IFF( rpal_stringbuffer_add( str, encoded ), isSuccess );
                                        
                                                OBFUSCATIONLIB_TOGGLE( payHdr );
                                            }

                                            IF_VALID_DO( encoded, rpal_memory_free );
                                        }

                                        IF_VALID_DO( finalBuffer, rpal_memory_free );
                                    }
                                }

                                IF_VALID_DO( encBuffer, rpal_memory_free );
                            }
                        }

                        IF_VALID_DO( blob, rpal_blob_free );
                    }
                }
            }

            rSequence_free( hdrSeq );
        }

        if( !isSuccess )
        {
            rpal_stringbuffer_free( str );
            str = NULL;
        }
    }

    return str;
}

static
rList
    assembleRequest
    (
        RPU8 optCrashCtx,
        RU32 optCrashCtxSize
    )
{
    rSequence req = NULL;
    RU32 moduleIndex = 0;
    rList msgList = NULL;
    rList modList = NULL;
    rSequence modEntry = NULL;

    if( NULL != ( req = rSequence_new() ) )
    {
        // Add some basic info
        rSequence_addRU32( req, RP_TAGS_MEMORY_USAGE, rpal_memory_totalUsed() );
        rSequence_addTIMESTAMP( req, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );

        // If we have a crash context to report
        if( NULL != optCrashCtx )
        {
            if( !rSequence_addBUFFER( req, RP_TAGS_HCP_CRASH_CONTEXT, optCrashCtx, optCrashCtxSize ) )
            {
                rpal_debug_error( "error adding crash context of size %d to hcp beacon", optCrashCtxSize );
            }
            else
            {
                rpal_debug_info( "crash context is being bundled in hcp beacon" );
            }
        }

        // List of loaded modules
        if( NULL != ( modList = rList_new( RP_TAGS_HCP_MODULE, RPCM_SEQUENCE ) ) )
        {
            for( moduleIndex = 0; moduleIndex < RP_HCP_CONTEXT_MAX_MODULES; moduleIndex++ )
            {
                if( NULL != g_hcpContext.modules[ moduleIndex ].hModule )
                {
                    if( NULL != ( modEntry = rSequence_new() ) )
                    {
                        if( !rSequence_addBUFFER( modEntry, 
                                                    RP_TAGS_HASH, 
                                                    g_hcpContext.modules[ moduleIndex ].hash, 
                                                    sizeof( g_hcpContext.modules[ moduleIndex ].hash ) ) ||
                            !rSequence_addRU8( modEntry, 
                                                RP_TAGS_HCP_MODULE_ID, 
                                                g_hcpContext.modules[ moduleIndex ].id ) ||
                            !rList_addSEQUENCE( modList, modEntry ) )
                        {
                            break;
                        }

                        // We take the opportunity to cleanup the list of modules...
                        if( rpal_thread_wait( g_hcpContext.modules[ moduleIndex ].hThread, 0 ) )
                        {
                            // This thread has exited, which is our signal that the module
                            // has stopped executing...
                            rEvent_free( g_hcpContext.modules[ moduleIndex ].isTimeToStop );
                            rpal_thread_free( g_hcpContext.modules[ moduleIndex ].hThread );
                            rpal_memory_zero( &(g_hcpContext.modules[ moduleIndex ]),
                                              sizeof( g_hcpContext.modules[ moduleIndex ] ) );

                            if( !rSequence_addRU8( modEntry, RP_TAGS_HCP_MODULE_TERMINATED, 1 ) )
                            {
                                break;
                            }
                        }
                    }
                }
            }

            if( !rSequence_addLIST( req, RP_TAGS_HCP_MODULES, modList ) )
            {
                rList_free( modList );
            }
        }

        if( NULL != ( msgList = rList_new( RP_TAGS_MESSAGE, RPCM_SEQUENCE ) ) )
        {
            if( !rList_addSEQUENCE( msgList, req ) )
            {
                rList_free( msgList );
                rSequence_free( req );
                msgList = NULL;
            }
        }
        else
        {
            rSequence_free( req );
        }
    }

    return msgList;
}



//=============================================================================
//  Base beacon
//=============================================================================
static
RU32
    RPAL_THREAD_FUNC thread_beacon
    (
        RPVOID context
    )
{
    RU32 status = 0;
    rList requests = NULL;
    rList responses = NULL;
    rIterator ite = NULL;
    rpcm_tag tag = 0;
    rpcm_type type = 0;
    rSequence msg = NULL;

    RPU8 crashContext = NULL;
    RU32 crashContextSize = 0;
    RU8 defaultCrashContext = 1;

    UNREFERENCED_PARAMETER( context );

    // First let's check if we have a crash context already present
    // which would indicate we did not shut down properly
    if( !acquireCrashContextPresent( &crashContext, &crashContextSize ) )
    {
        crashContext = NULL;
        crashContextSize = 0;
    }
    
    // Set a default crashContext to be removed before exiting
    setCrashContext( &defaultCrashContext, sizeof( defaultCrashContext ) );

    while( !rEvent_wait( g_hcpContext.isBeaconTimeToStop, 0 ) )
    {
        if( 0 == g_hcpContext.beaconTimeout )
        {
            // This is the default timeout to re-establish contact home
            g_hcpContext.beaconTimeout = RP_HCP_CONFIG_DEFAULT_BEACON_TIMEOUT_INIT;

            requests = assembleRequest( crashContext, crashContextSize );
            
            // If we just sent a crash context, free it so we don't keep sending it
            if( NULL != crashContext )
            {
                rpal_memory_free( crashContext );
                crashContext = NULL;
                crashContextSize = 0;
            }

            if( NULL != requests &&
                doBeacon( RP_HCP_MODULE_ID_HCP, requests, &responses ) &&
                NULL != responses )
            {
                // This is the default beacon timeout when contact is already established
                g_hcpContext.beaconTimeout = RP_HCP_CONFIG_DEFAULT_BEACON_TIMEOUT;

                // Go through the response list of messages
                if( NULL != ( ite = rIterator_new( responses ) ) )
                {
                    while( rIterator_next( ite, &tag, &type, &msg, NULL ) )
                    {
                        // We ignore non-messages for forward compat
                        if( RP_TAGS_MESSAGE == tag &&
                            RPCM_SEQUENCE == type )
                        {
                            processMessage( msg );
                        }
                    }

                    rIterator_free( ite );
                }
            }

            // Free resources.
            IF_VALID_DO( requests, rList_free );
            requests = NULL;
            IF_VALID_DO( responses, rList_free );
            responses = NULL;
        }

        // Sleep for 1 second, we aproximate beacon times
        rpal_thread_sleep( 1 * 1000 );
        if( 0 != g_hcpContext.beaconTimeout ){ g_hcpContext.beaconTimeout--; }
    }

    return status;
}

//=============================================================================
//  API
//=============================================================================
RBOOL
    startBeacons
    (

    )
{
    RBOOL isSuccess = FALSE;

    g_hcpContext.isBeaconTimeToStop = rEvent_create( TRUE );

    if( NULL != g_hcpContext.isBeaconTimeToStop )
    {
        g_hcpContext.hBeaconThread = rpal_thread_new( thread_beacon, NULL );

        if( 0 != g_hcpContext.hBeaconThread )
        {
            isSuccess = TRUE;
        }
        else
        {
            rEvent_free( g_hcpContext.isBeaconTimeToStop );
            g_hcpContext.isBeaconTimeToStop = NULL;
        }
    }

    return isSuccess;
}



RBOOL
    stopBeacons
    (

    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != g_hcpContext.isBeaconTimeToStop )
    {
        rEvent_set( g_hcpContext.isBeaconTimeToStop );

        if( 0 != g_hcpContext.hBeaconThread )
        {
            rpal_thread_wait( g_hcpContext.hBeaconThread, MSEC_FROM_SEC( 40 ) );
            rpal_thread_free( g_hcpContext.hBeaconThread );

            isSuccess = TRUE;
        }

        rEvent_free( g_hcpContext.isBeaconTimeToStop );
        g_hcpContext.isBeaconTimeToStop = NULL;
    }

    return isSuccess;
}

RBOOL
    doBeacon
    (
        RpHcp_ModuleId sourceModuleId,
        rList toSend,
        rList* pReceived
    )
{
    RBOOL isSuccess = FALSE;

    RPU8 receivedData = NULL;
    RU32 receivedSize = 0;

    RPU8 encodedData = NULL;

    RPU8 rawPayloadBuff = NULL;
    RU32 rawPayloadSize = 0;
    
    RU32 zError = 0;
    
    RPU8 signature = NULL;
    RPU8 payloadBuff = NULL;
    RU32 payloadSize = 0;

    RU8 sessionKey[ CRYPTOLIB_SYM_KEY_SIZE ] = {0};
    RU8 sessionIv[ CRYPTOLIB_SYM_IV_SIZE ] = {0};

    rString payloadToSend = NULL;

    RU32 decryptedSize = 0;

    RPU8 tmpBuff = NULL;
    RU64 tmpSize = 0;

    OBFUSCATIONLIB_DECLARE( url1, RP_HCP_CONFIG_HOME_URL_PRIMARY );
    OBFUSCATIONLIB_DECLARE( url2, RP_HCP_CONFIG_HOME_URL_SECONDARY );

    RPCHAR effectivePrimary = (RPCHAR)url1;
    RPCHAR effectiveSecondary = (RPCHAR)url2;

    rpal_debug_info( "called by module %d.", sourceModuleId );

    // Pre-generate the session key and iv we want for the session
    CryptoLib_genRandomBytes( sessionKey, sizeof( sessionKey ) );
    CryptoLib_genRandomBytes( sessionIv, sizeof( sessionIv ) );

    // Assemble the stream to send
    payloadToSend = assembleOutboundStream( sourceModuleId, toSend, sessionKey, sessionIv );

    if( NULL != payloadToSend )
    {
        // Send the beacon
        if( NULL == g_hcpContext.primaryUrl )
        {
            OBFUSCATIONLIB_TOGGLE( url1 );
            OBFUSCATIONLIB_TOGGLE( url2 );
        }
        else
        {
            effectivePrimary = g_hcpContext.primaryUrl;
            effectiveSecondary = g_hcpContext.secondaryUrl;
        }

        if( postHttp( effectivePrimary, 
                      rpal_stringbuffer_getString( payloadToSend ), 
                      FALSE, 
                      (RPVOID*)&receivedData, 
                      &receivedSize ) )
        {
            isSuccess = TRUE;
        }
        else if( postHttp( effectiveSecondary, 
                            rpal_stringbuffer_getString( payloadToSend ), 
                            FALSE, 
                            (RPVOID*)&receivedData, 
                            &receivedSize ) )
        {
            isSuccess = TRUE;
            rpal_debug_info( "beacon using secondary home." );
        }
        else
        {
            rpal_debug_warning( "could not contact server." );
        }

        if( NULL == g_hcpContext.primaryUrl )
        {
            OBFUSCATIONLIB_TOGGLE( url1 );
            OBFUSCATIONLIB_TOGGLE( url2 );
        }

        if( isSuccess )
        {
            isSuccess = FALSE;

            if( NULL != receivedData &&
                0 != receivedSize )
            {
                if( NULL != pReceived )
                {
                    *pReceived = NULL;

                    // Process the data received
                    encodedData = findAndCapEncodedDataInBeacon( receivedData, receivedSize );

                    if( NULL != encodedData )
                    {
                        if( stripBeaconEncoding( (RPCHAR)encodedData, &rawPayloadBuff, &rawPayloadSize ) )
                        {
                            if( rawPayloadSize >= CRYPTOLIB_SIGNATURE_SIZE )
                            {
                                // Verify signature
                                signature = rawPayloadBuff;
                                payloadBuff = signature + CRYPTOLIB_SIGNATURE_SIZE;
                                payloadSize = rawPayloadSize - CRYPTOLIB_SIGNATURE_SIZE;

                                if( verifyC2Signature( payloadBuff, payloadSize, signature ) )
                                {
                                    // Remove the symetric crypto using the session key
                                    if( CryptoLib_symDecrypt( payloadBuff, payloadSize, sessionKey, sessionIv, &decryptedSize ) )
                                    {
                                        // Remove the compression                        
                                        tmpSize = rpal_ntoh32( *(RU32*)payloadBuff );

                                        if( 0 != tmpSize &&
                                                (-1) != tmpSize )
                                        {
                                            tmpBuff = rpal_memory_alloc( (RSIZET)tmpSize );

                                            if( rpal_memory_isValid( tmpBuff ) )
                                            {
                                                if( Z_OK == ( zError = uncompress( tmpBuff, 
                                                                (uLongf*)&tmpSize, 
                                                                payloadBuff + sizeof( RU32 ), 
                                                                payloadSize - sizeof( RU32 ) ) ) )
                                                {
                                                    // Payload is valid, turn it into a sequence to return
                                                    if( rList_deserialise( pReceived, tmpBuff, (RU32)tmpSize, NULL ) )
                                                    {
                                                        rpal_debug_info( "success." );
                                                        isSuccess = TRUE;
                                                    }
                                                    else
                                                    {
                                                        rpal_debug_error( "could not deserialise list." );
                                                    }
                                                }
                                                else
                                                {
                                                    rpal_debug_error( "could not decompress received data: %d (%d bytes).", zError, payloadSize );
                                                }

                                                rpal_memory_free( tmpBuff );
                                            }
                                            else
                                            {
                                                rpal_debug_error( "could not allocate memory for data received, asking for: %d.", tmpSize );
                                            }
                                        }
                                        else
                                        {
                                            rpal_debug_error( "invalid size field in decrypted data." );
                                        }
                                    }
                                    else
                                    {
                                        rpal_debug_error( "failed to decrypt received data." );
                                    }
                                }
                                else
                                {
                                    rpal_debug_error( "signature verification failed." );
                                }
                            }
                            else
                            {
                                rpal_debug_error( "stripped payload failed sanity check." );
                            }
                            
                            rpal_memory_free( rawPayloadBuff );
                        }
                        else
                        {
                            rpal_debug_error( "could not strip BASE64 encoding." );
                        }
                    }
                    else
                    {
                        rpal_debug_error( "could not find the encoded data." );
                    }
                }
                else
                {
                    rpal_debug_info( "not interested in the returned data." );
                    isSuccess = TRUE;
                }

                FREE_AND_NULL( receivedData );
                receivedSize = 0;
            }
            else
            {
                rpal_debug_warning( "no data received from the server." );
                isSuccess = FALSE;
            }
        }

        rpal_stringbuffer_free( payloadToSend );
    }
    else
    {
        rpal_debug_error( "failed to generate payload for beacon." );
    }

    return isSuccess;
}

//=============================================================================
//  Platform Specific
//=============================================================================
#ifdef RPAL_PLATFORM_WINDOWS
#include <windows_undocumented.h>
static
RBOOL
    postHttp
    (
        RPCHAR location,
        RPCHAR params,
        RBOOL isEnforceCert,
        RPVOID* receivedData,
        RU32* receivedSize
    )
{
    RBOOL isSuccess = FALSE;

    DWORD timeout = ( 1000 * 10 );

    DWORD bytesToRead = 0;
    DWORD bytesReceived = 0;
    DWORD bytesRead = 0;
    RPU8 pReceivedBuffer = NULL;

    URL_COMPONENTSA components = {0};

    RBOOL isSecure = FALSE;
    RCHAR pUser[ 1024 ] = {0};
    RCHAR pPass[ 1024 ] = {0};
    RCHAR pUrl[ 1024 ] = {0};
    RCHAR pPage[ 1024 ] = {0};
    INTERNET_PORT port = 0;

    InternetCrackUrl_f pInternetCrackUrl = NULL;
    InternetOpen_f pInternetOpen = NULL;
    InternetConnect_f pInternetConnect = NULL;
    InternetCloseHandle_f pInternetCloseHandle = NULL;
    HttpOpenRequest_f pHttpOpenRequest = NULL;
    HttpSendRequest_f pHttpSendRequest = NULL;
    InternetQueryDataAvailable_f pInternetQueryDataAvailable = NULL;
    InternetReadFile_f pInternetReadFile = NULL;
    InternetSetOption_f pInternetSetOption = NULL;

    HMODULE hWininet = NULL;
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hHttp = NULL;
    DWORD flags = INTERNET_FLAG_DONT_CACHE | 
                  INTERNET_FLAG_NO_UI | 
                  INTERNET_FLAG_NO_CACHE_WRITE;

    RCHAR contentType[] = "Content-Type: application/x-www-form-urlencoded";
    RCHAR userAgent[] = "rpHCP";
    RCHAR method[] = "POST";
    RCHAR importDll[] = "wininet.dll";
    RCHAR import1[] = "InternetCrackUrlA";
    RCHAR import2[] = "InternetOpenA";
    RCHAR import3[] = "InternetConnectA";
    RCHAR import4[] = "InternetCloseHandle";
    RCHAR import5[] = "HttpOpenRequestA";
    RCHAR import6[] = "HttpSendRequestA";
    RCHAR import7[] = "InternetQueryDataAvailable";
    RCHAR import8[] = "InternetReadFile";
    RCHAR import9[] = "InternetSetOptionA";

    if( NULL != location )
    {
        rpal_debug_info( "posting to %s", location );
        
        if( NULL != ( hWininet = GetModuleHandleA( (RPCHAR)&importDll ) ) ||
            NULL != ( hWininet = LoadLibraryA( (RPCHAR)&importDll ) ) )
        {
            pInternetCrackUrl = (InternetCrackUrl_f)GetProcAddress( hWininet, (RPCHAR)&import1 );
            pInternetOpen = (InternetOpen_f)GetProcAddress( hWininet, (RPCHAR)&import2 );
            pInternetConnect = (InternetConnect_f)GetProcAddress( hWininet, (RPCHAR)&import3 );
            pInternetCloseHandle = (InternetCloseHandle_f)GetProcAddress( hWininet, (RPCHAR)&import4 );
            pHttpOpenRequest = (HttpOpenRequest_f)GetProcAddress( hWininet, (RPCHAR)&import5 );
            pHttpSendRequest = (HttpSendRequest_f)GetProcAddress( hWininet, (RPCHAR)&import6 );
            pInternetQueryDataAvailable = (InternetQueryDataAvailable_f)GetProcAddress( hWininet, (RPCHAR)&import7 );
            pInternetReadFile = (InternetReadFile_f)GetProcAddress( hWininet, (RPCHAR)&import8 );
            pInternetSetOption = (InternetSetOption_f)GetProcAddress( hWininet, (RPCHAR)&import9 );


            if( NULL != pInternetCrackUrl &&
                NULL != pInternetOpen &&
                NULL != pInternetConnect &&
                NULL != pInternetCloseHandle &&
                NULL != pHttpOpenRequest &&
                NULL != pHttpSendRequest &&
                NULL != pInternetQueryDataAvailable &&
                NULL != pInternetReadFile &&
                NULL != pInternetSetOption )
            {
                components.lpszHostName = pUrl;
                components.dwHostNameLength = sizeof( pUrl );
                components.lpszUrlPath = pPage;
                components.dwUrlPathLength = sizeof( pPage );
                components.lpszUserName = pUser;
                components.dwUserNameLength = sizeof( pUser );
                components.lpszPassword = pPass;
                components.dwPasswordLength = sizeof( pPass );
                components.dwStructSize = sizeof( components );

                if( !pInternetCrackUrl( location, 0, 0, &components ) )
                {
                    if( rpal_string_strlen( location ) < ARRAY_N_ELEM( pUrl ) )
                    {
                        rpal_string_strcpya( pUrl, location );
                    }
                    components.nPort = 80;
                    components.nScheme = INTERNET_SCHEME_HTTP;
                }

                port = components.nPort;

                if( INTERNET_SCHEME_HTTPS == components.nScheme )
                {
                    isSecure = TRUE;
                }

                if( !isEnforceCert && isSecure )
                {
                    flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID | 
                                INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
                }

                if( isSecure )
                {
                    flags |= INTERNET_FLAG_SECURE;
                }

                hInternet = pInternetOpen( (RPCHAR)&userAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );

                if( NULL != hInternet )
                {
                    pInternetSetOption( hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof( timeout ) );

                    hConnect = pInternetConnect( hInternet, 
                                                    pUrl, 
                                                    port,
                                                    pUser, pPass,
                                                    INTERNET_SERVICE_HTTP,
                                                    flags, (DWORD_PTR)NULL );

                    if( NULL != hConnect )
                    {
                        hHttp = pHttpOpenRequest( hConnect, 
                                                    (RPCHAR)&method, 
                                                    pPage ? pPage : "", 
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    flags,
                                                    (DWORD_PTR)NULL );

                        if( hHttp )
                        {
                            if( pHttpSendRequest( hHttp, 
                                                    (RPCHAR)&contentType,
                                                    (DWORD)(-1),
                                                    params,
                                                    (DWORD)rpal_string_strlen( params ) ) )
                            {
                                isSuccess = TRUE;

                                if( NULL != receivedData &&
                                    NULL != receivedSize )
                                {
                                    while( pInternetQueryDataAvailable( hHttp, &bytesToRead, 0, 0 ) &&
                                            0 != bytesToRead )
                                    {
                                        pReceivedBuffer = rpal_memory_realloc( pReceivedBuffer, bytesReceived + bytesToRead );

                                        if( rpal_memory_isValid( pReceivedBuffer ) )
                                        {
                                            if( pInternetReadFile( hHttp, pReceivedBuffer + bytesReceived, bytesToRead, &bytesRead ) )
                                            {
                                                bytesReceived += bytesRead;
                                                bytesToRead = 0;
                                                bytesRead = 0;
                                            }
                                            else
                                            {
                                                rpal_memory_free( pReceivedBuffer );
                                                pReceivedBuffer = NULL;
                                                bytesReceived = 0;
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            pReceivedBuffer = NULL;
                                            bytesReceived = 0;
                                            isSuccess = FALSE;
                                            break;
                                        }
                                    }

                                    if( isSuccess &&
                                        rpal_memory_isValid( pReceivedBuffer ) &&
                                        0 != bytesReceived )
                                    {
                                        *receivedData = pReceivedBuffer;
                                        *receivedSize = bytesReceived;
                                    }
                                }
                            }

                                
                            pInternetCloseHandle( hHttp );
                        }
                            
                        pInternetCloseHandle( hConnect );
                    }
                        
                    pInternetCloseHandle( hInternet );
                }
            }
        }
    }

    return isSuccess;
}


#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
static
RSIZET
    _curlToBuffer
    (
        RPU8 ptr,
        RSIZET size,
        RSIZET nmemb, 
        rBlob dataBlob
    )
{
    RSIZET processed = 0;
    RSIZET toProcess = 0;
    
    toProcess = size * nmemb;
    if( NULL != ptr &&
        0 != toProcess &&
        NULL != dataBlob )
    {
        if( rpal_blob_add( dataBlob, ptr, toProcess ) )
        {
            processed = toProcess;
        }
    }
    
    return processed;
}

static
RBOOL
    postHttp
    (
        RPCHAR location,
        RPCHAR params,
        RBOOL isEnforceCert,
        RPVOID* receivedData,
        RU32* receivedSize
    )
{
    RBOOL isSuccess = FALSE;
    CURL* curlCtx = NULL;
    RU32 timeout = ( 1000 * 10 );
    RCHAR userAgent[] = "rpHCP";
    rBlob dataReceived = NULL;
    RBOOL isDataReturned = FALSE;
    CURLcode curlError = CURLE_OK;
    
    if( NULL != location )
    {
        if( NULL != ( dataReceived = rpal_blob_create( 0, 0 ) ) )
        {
            if( NULL != ( curlCtx = curl_easy_init() ) )
            {
                rpal_debug_info( "posting to %s", location );
                
                if( CURLE_OK == ( curlError = curl_easy_setopt( curlCtx, CURLOPT_URL, location ) ) &&
                    CURLE_OK == ( curlError = curl_easy_setopt( curlCtx, CURLOPT_USERAGENT, userAgent ) ) &&
                    CURLE_OK == ( curlError = curl_easy_setopt( curlCtx, CURLOPT_POSTFIELDS, params ) ) &&
                    CURLE_OK == ( curlError = curl_easy_setopt( curlCtx, CURLOPT_WRITEFUNCTION, (RPVOID)_curlToBuffer ) ) &&
                    CURLE_OK == ( curlError = curl_easy_setopt( curlCtx, CURLOPT_WRITEDATA, (RPVOID)dataReceived ) ) &&
                    CURLE_OK == ( curlError = curl_easy_setopt( curlCtx, CURLOPT_NOSIGNAL, 1 ) ) )
                {
                    if( CURLE_OK == ( curlError = curl_easy_perform( curlCtx ) ) )
                    {
                        isSuccess = TRUE;
                        
                        if( NULL != receivedData )
                        {
                            *receivedData = rpal_blob_getBuffer( dataReceived );
                            isDataReturned = TRUE;
                        }
                        if( NULL != receivedSize )
                        {
                            *receivedSize = rpal_blob_getSize( dataReceived );
                        }
                    }
                    else
                    {
                        rpal_debug_warning( "error performing post: %d", curlError );
                    }
                }
                else
                {
                    rpal_debug_error( "error setting curl options: %d", curlError );
                }
                
                curl_easy_cleanup( curlCtx );
            }
            else
            {
                rpal_debug_error( "error creating curl context" );
            }
            
            if( !isDataReturned )
            {
                rpal_blob_free( dataReceived );
            }
            else
            {
                rpal_blob_freeWrapperOnly( dataReceived );
            }
        }
    }
    
    return isSuccess;
}

#endif

