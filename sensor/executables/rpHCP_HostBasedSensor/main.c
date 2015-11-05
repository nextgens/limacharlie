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

#include <rpal/rpal.h>
#include <rpHostCommonPlatformIFaceLib/rpHostCommonPlatformIFaceLib.h>

#include <processLib/processLib.h>
#include <rpHostCommonPlatformLib/rTags.h>
#include <obfuscationLib/obfuscationLib.h>
#include <notificationsLib/notificationsLib.h>
#include <cryptoLib/cryptoLib.h>
#include <librpcm/librpcm.h>
#include "collectors.h"
#include "keys.h"

//=============================================================================
//  RP HCP Module Requirements
//=============================================================================
#define RPAL_FILE_ID 82
RpHcp_ModuleId g_current_Module_id = 2;

//=============================================================================
//  Global Behavior Variables
//=============================================================================
#define HBS_DEFAULT_BEACON_TIMEOUT              (1*60)
#define HBS_DEFAULT_BEACON_TIMEOUT_FUZZ         (1*60)
#define HBS_EXFIL_QUEUE_MAX_NUM                 500
#define HBS_EXFIL_QUEUE_MAX_SIZE                (1024*10)

// Large blank buffer to be used to patch configurations post-build
#define _HCP_DEFAULT_STATIC_STORE_SIZE                          (1024 * 10)
#define _HCP_DEFAULT_STATIC_STORE_MAGIC                         { 0xFA, 0x57, 0xF0, 0x0D }
static RU8 g_patchedConfig[ _HCP_DEFAULT_STATIC_STORE_SIZE ] = _HCP_DEFAULT_STATIC_STORE_MAGIC;
#define _HCP_DEFAULT_STATIC_STORE_KEY                           { 0xFA, 0x75, 0x01 }

//=============================================================================
//  Global Context
//=============================================================================
HbsState g_hbs_state = { 0 };
RU8* hbs_cloud_pub_key = hbs_cloud_default_pub_key;

//=============================================================================
//  Collector Definition
//=============================================================================
typedef RBOOL ( *hbs_collector_init_func )( HbsState* hbsState,
                                            rSequence config );
typedef RBOOL( *hbs_collector_cleanup_func )( HbsState* hbsState,
                                              rSequence config );
typedef struct
{
    RBOOL isEnabled;
    hbs_collector_init_func init;
    hbs_collector_cleanup_func cleanup;
    rSequence conf;
} HbsCollectorInfo;

//=============================================================================
//  Collectors
//=============================================================================
HbsCollectorInfo g_collectors[] = { ENABLED_COLLECTOR( 0 ),
                                    ENABLED_COLLECTOR( 1 ),
                                    ENABLED_WINDOWS_COLLECTOR( 2 ),
                                    ENABLED_COLLECTOR( 3 ),
                                    ENABLED_WINDOWS_COLLECTOR( 4 ),
                                    ENABLED_COLLECTOR( 5 ),
                                    ENABLED_WINDOWS_COLLECTOR( 6 ),
                                    ENABLED_WINDOWS_COLLECTOR( 7 ),
                                    ENABLED_COLLECTOR( 8 ),
                                    ENABLED_COLLECTOR( 9 ),
                                    ENABLED_COLLECTOR( 10 ),
                                    ENABLED_COLLECTOR( 11 ),
                                    ENABLED_COLLECTOR( 12 ) };

//=============================================================================
//  Utilities
//=============================================================================
static
rSequence
    getStaticConfig
    (

    )
{
    RU8 magic[] = _HCP_DEFAULT_STATIC_STORE_MAGIC;
    rSequence config = NULL;
    RU32 unused = 0;
    RU8 key[] = _HCP_DEFAULT_STATIC_STORE_KEY;

    if( 0 != rpal_memory_memcmp( g_patchedConfig, magic, sizeof( magic ) ) )
    {
        obfuscationLib_toggle( g_patchedConfig, sizeof( g_patchedConfig ), key, sizeof( key ) );

        if( rSequence_deserialise( &config, g_patchedConfig, sizeof( g_patchedConfig ), &unused ) )
        {
            rpal_debug_info( "static store patched, using it as config" );
        }

        obfuscationLib_toggle( g_patchedConfig, sizeof( g_patchedConfig ), key, sizeof( key ) );
    }
    else
    {
        rpal_debug_info( "static store not patched, using defaults" );
    }

    return config;
}

#ifdef RPAL_PLATFORM_WINDOWS
static RBOOL
    WindowsSetPrivilege
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

        tp.PrivilegeCount = 1;
        tp.Privileges[ 0 ].Luid = luid;
        tp.Privileges[ 0 ].Attributes = ( bEnablePrivilege ) ? SE_PRIVILEGE_ENABLED : 0;
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
    WindowsGetPrivilege
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
            if( WindowsSetPrivilege( hToken, privName, TRUE ) )
            {
                isSuccess = TRUE;
            }

            CloseHandle( hToken );
        }
    }

    return isSuccess;
}
#endif

static 
RBOOL
    getPrivileges
    (

    )
{
    RBOOL isSuccess = FALSE;

#ifdef RPAL_PLATFORM_WINDOWS
    RCHAR strSeDebug[] = "SeDebugPrivilege";
    RCHAR strSeBackup[] = "SeBackupPrivilege";
    RCHAR strSeRestore[] = "SeRestorePrivilege";
    if( !WindowsGetPrivilege( strSeDebug ) )
    {
        rpal_debug_warning( "error getting SeDebugPrivilege" );
    }
    if( !WindowsGetPrivilege( strSeBackup ) )
    {
        rpal_debug_warning( "error getting SeBackupPrivilege" );
    }
    if( !WindowsGetPrivilege( strSeRestore ) )
    {
        rpal_debug_warning( "error getting SeRestorePrivilege" );
    }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    /*cap_t capability_context = 0;
    if( NULL != ( capability_context = cap_get_proc() ) )
    {
        cap_free( capability_context );
    }
    */
#endif

    return isSuccess;
}

static RVOID
    freeExfilEvent
    (
        rSequence seq,
        RU32 unused
    )
{
    UNREFERENCED_PARAMETER( unused );
    rSequence_free( seq );
}

static rList
    beaconHome
    (
        
    )
{
    rList response = NULL;
    rList exfil = NULL;
    rSequence msg = NULL;
    RU32 nExfilMsg = 0;
    rSequence tmpMessage = NULL;

    if( NULL != ( exfil = rList_new( RP_TAGS_MESSAGE, RPCM_SEQUENCE ) ) )
    {
        if( NULL != ( msg = rSequence_new() ) )
        {
            if( rSequence_addBUFFER( msg, RP_TAGS_HASH, g_hbs_state.currentConfigHash, CRYPTOLIB_HASH_SIZE ) )
            {
                if( !rList_addSEQUENCE( exfil, msg ) )
                {
                    rSequence_free( msg );
                    msg = NULL;
                }

                tmpMessage = msg;
            }
            else
            {
                rSequence_free( msg );
                msg = NULL;
            }
        }

        while( rQueue_remove( g_hbs_state.outQueue, &msg, NULL, 0 ) )
        {
            nExfilMsg++;

            if( !rList_addSEQUENCE( exfil, msg ) )
            {
                rSequence_free( msg );
            }
        }

        rpal_debug_info( "%d messages ready for exfil.", nExfilMsg );


        if( rpHcpI_beaconHome( exfil, &response ) )
        {
            rpal_debug_info( "%d messages received from cloud.", rList_getNumElements( response ) );
        }
        else if( g_hbs_state.maxQueueSize > rList_getEstimateSize( exfil ) )
        {
            rpal_debug_info( "beacon failed, re-adding %d messages.", rList_getNumElements( exfil ) );

            // We will attempt to re-add the existing messages back in the queue since this failed
            rList_resetIterator( exfil );
            while( rList_getSEQUENCE( exfil, RP_TAGS_MESSAGE, &msg ) )
            {
                // Ignore message we generate temporarily for the beacon
                if( tmpMessage == msg )
                {
                    rSequence_free( msg );
                    continue;
                }

                if( !rQueue_add( g_hbs_state.outQueue, msg, 0 ) )
                {
                    rSequence_free( msg );
                }
            }
            rList_shallowFree( exfil );
            exfil = NULL;
        }
        else
        {
            rpal_debug_warning( "beacon failed but discarded exfil because of its size." );
        }

        if( NULL != exfil )
        {
            rList_free( exfil );
        }
    }

    return response;
}

static RBOOL
    updateCollectorConfigs
    (
        rList newConfigs
    )
{
    RBOOL isSuccess = FALSE;
    RU8 unused = 0;
    RU32 i = 0;
    rSequence tmpConf = NULL;
    RU32 confId = 0;

    if( rpal_memory_isValid( newConfigs ) )
    {
        rpal_debug_info( "updating collector configurations." );
        
        for( i = 0; i < ARRAY_N_ELEM( g_collectors ); i++ )
        {
            if( NULL != g_collectors[ i ].conf )
            {
                rpal_debug_info( "freeing collector %d config.", i );
                rSequence_free( g_collectors[ i ].conf );
                g_collectors[ i ].conf = NULL;
            }
        }

        while( rList_getSEQUENCE( newConfigs, RP_TAGS_HBS_CONFIGURATION, &tmpConf ) )
        {
            if( rSequence_getRU32( tmpConf, RP_TAGS_HBS_CONFIGURATION_ID, &confId ) &&
                confId < ARRAY_N_ELEM( g_collectors ) )
            {
                if( rSequence_getRU8( tmpConf, RP_TAGS_IS_DISABLED, &unused ) )
                {
                    g_collectors[ confId ].isEnabled = FALSE;
                }
                else
                {
                    g_collectors[ confId ].isEnabled = TRUE;
                    g_collectors[ confId ].conf = rSequence_duplicate( tmpConf );
                    rpal_debug_info( "set new collector %d config.", confId );
                }
            }
        }
                
        isSuccess = TRUE;
    }

    return isSuccess;
}

static RVOID
    shutdownCollectors
    (

    )
{
    RU32 i = 0;

    if( !rEvent_wait( g_hbs_state.isTimeToStop, 0 ) )
    {
        rpal_debug_info( "signaling to collectors to stop." );
        rEvent_set( g_hbs_state.isTimeToStop );

        if( NULL != g_hbs_state.hThreadPool )
        {
            rpal_debug_info( "destroying collector thread pool." );
            rThreadPool_destroy( g_hbs_state.hThreadPool, TRUE );
            g_hbs_state.hThreadPool = NULL;

            for( i = 0; i < ARRAY_N_ELEM( g_collectors ); i++ )
            {
                if( g_collectors[ i ].isEnabled )
                {
                    rpal_debug_info( "cleaning up collector %d.", i );
                    g_collectors[ i ].cleanup( &g_hbs_state, g_collectors[ i ].conf );
                    rSequence_free( g_collectors[ i ].conf );
                    g_collectors[ i ].conf = NULL;
                }
            }
        }
    }
}

static RBOOL
    startCollectors
    (

    )
{
    RBOOL isSuccess = FALSE;
    RU32 i = 0;

    rEvent_unset( g_hbs_state.isTimeToStop );
    if( NULL != ( g_hbs_state.hThreadPool = rThreadPool_create( 1, 
                                                                15,
                                                                MSEC_FROM_SEC( 10 ) ) ) )
    {
        isSuccess = TRUE;

        for( i = 0; i < ARRAY_N_ELEM( g_collectors ); i++ )
        {
            if( g_collectors[ i ].isEnabled )
            {
                if( !g_collectors[ i ].init( &g_hbs_state, g_collectors[ i ].conf ) )
                {
                    isSuccess = FALSE;
                    rpal_debug_warning( "collector %d failed to init.", i );
                }
                else
                {
                    rpal_debug_info( "collector %d started.", i );
                }
            }
            else
            {
                rpal_debug_info( "collector %d disabled.", i );
            }
        }
    }

    return isSuccess;
}

static RVOID
    sendStartupEvent
    (

    )
{
    rSequence wrapper = NULL;
    rSequence startupEvent = NULL;

    if( NULL != ( wrapper = rSequence_new() ) )
    {
        if( NULL != ( startupEvent = rSequence_new() ) )
        {
            if( rSequence_addSEQUENCE( wrapper, RP_TAGS_NOTIFICATION_STARTING_UP, startupEvent ) )
            {
                if( !rSequence_addTIMESTAMP( startupEvent, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() ) ||
                    !rQueue_add( g_hbs_state.outQueue, wrapper, 0 ) )
                {
                    rSequence_free( wrapper );
                }
            }
            else
            {
                rSequence_free( wrapper );
                rSequence_free( startupEvent );
            }
        }
        else
        {
            rSequence_free( wrapper );
        }
    }
}

static RVOID
    sendShutdownEvent
    (

    )
{
    rList cloudMessages = NULL;
    rSequence wrapper = NULL;
    rSequence shutdownEvent = NULL;

    if( NULL != ( wrapper = rSequence_new() ) )
    {
        if( NULL != ( shutdownEvent = rSequence_new() ) )
        {
            if( rSequence_addSEQUENCE( wrapper, RP_TAGS_NOTIFICATION_SHUTTING_DOWN, shutdownEvent ) )
            {
                if( rSequence_addTIMESTAMP( shutdownEvent, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() ) &&
                    rQueue_add( g_hbs_state.outQueue, wrapper, 0 ) )
                {
                    if( NULL != ( cloudMessages = beaconHome() ) )
                    {
                        rList_free( cloudMessages );
                    }
                }
                else
                {
                    rSequence_free( wrapper );
                }
            }
            else
            {
                rSequence_free( wrapper );
                rSequence_free( shutdownEvent );
            }
        }
        else
        {
            rSequence_free( wrapper );
        }
    }
}

static RVOID
    publishCloudNotifications
    (
        rList notifications
    )
{
    rSequence notif = NULL;
    RPU8 buff = NULL;
    RU32 buffSize = 0;
    RPU8 sig = NULL;
    RU32 sigSize = 0;
    rpHCPId curId = { 0 };
    rSequence cloudEvent = NULL;
    rSequence targetId = { 0 };
    RU32 eventId = 0;
    rSequence localEvent = NULL;
    RU64 expiry = 0;
    rpHCPId tmpId = { 0 };
    rSequence receipt = NULL;

    while( rList_getSEQUENCE( notifications, RP_TAGS_HBS_CLOUD_NOTIFICATION, &notif ) )
    {
        cloudEvent = NULL;

        if( rSequence_getBUFFER( notif, RP_TAGS_BINARY, &buff, &buffSize ) &&
            rSequence_getBUFFER( notif, RP_TAGS_SIGNATURE, &sig, &sigSize ) )
        {
            if( CryptoLib_verify( buff, buffSize, hbs_cloud_pub_key, sig ) )
            {
                if( !rpHcpI_getId( &curId ) )
                {
                    rpal_debug_error( "error getting current id for cloud notifications." );
                }
                else
                {
                    if( !rSequence_deserialise( &cloudEvent, buff, buffSize, NULL ) )
                    {
                        cloudEvent = NULL;
                        rpal_debug_warning( "error deserializing cloud event." );
                    }
                }
            }
            else
            {
                rpal_debug_warning( "cloud event signature invalid." );
            }
        }

        if( rpal_memory_isValid( cloudEvent ) )
        {
            if( rSequence_getSEQUENCE( cloudEvent, RP_TAGS_HCP_ID, &targetId ) &&
                rSequence_getRU32( cloudEvent, RP_TAGS_HBS_NOTIFICATION_ID, &eventId ) &&
                rSequence_getSEQUENCE( cloudEvent, RP_TAGS_HBS_NOTIFICATION, &localEvent ) )
            {
                rSequence_getTIMESTAMP( cloudEvent, RP_TAGS_EXPIRY, &expiry );

                tmpId = rpHcpI_seqToHcpId( targetId );

                curId.id.configId = 0;
                tmpId.id.configId = 0;
                
                if( NULL != ( receipt = rSequence_new() ) )
                {
                    if( rSequence_addSEQUENCE( receipt, 
                                               RP_TAGS_HBS_CLOUD_NOTIFICATION, 
                                               rSequence_duplicate( cloudEvent ) ) )
                    {
                        if( !rQueue_add( g_hbs_state.outQueue, receipt, 0 ) )
                        {
                            rSequence_free( receipt );
                            receipt = NULL;
                        }
                    }
                    else
                    {
                        rSequence_free( receipt );
                        receipt = NULL;
                    }
                }

                if( curId.raw == tmpId.raw &&
                    rpal_time_getGlobal() <= expiry )
                {
                    if( !notifications_publish( eventId, localEvent ) )
                    {
                        rpal_debug_error( "error publishing event from cloud." );
                    }
                }
                else
                {
                    rpal_debug_warning( "event expired or for wrong id." );
                }
            }

            if( rpal_memory_isValid( cloudEvent ) )
            {
                rSequence_free( cloudEvent );
                cloudEvent = NULL;
            }
        }
    }
}

//=============================================================================
//  Entry Point
//=============================================================================
RU32
RPAL_THREAD_FUNC
    RpHcpI_mainThread
    (
        rEvent isTimeToStop
    )
{
    RU32 ret = 0;

    RU64 nextBeaconTime = 0;

    rList cloudMessages = NULL;
    rSequence msg = NULL;
    rList newConfigurations = NULL;
    rList newNotifications = NULL;
    RPU8 newConfigurationHash = NULL;
    RU32 newHashSize = 0;
    rSequence staticConfig = NULL;
    RU8* tmpBuffer = NULL;
    RU32 tmpSize = 0;

    FORCE_LINK_THAT( HCP_IFACE );

    CryptoLib_init();

    getPrivileges();

    // This is the event for the collectors, it is different than the
    // hbs proper event so that we can restart the collectors without
    // signaling hbs as a whole.
    if( NULL == ( g_hbs_state.isTimeToStop = rEvent_create( TRUE ) ) )
    {
        return (RU32)-1;
    }

    // By default, no collectors are running
    rEvent_set( g_hbs_state.isTimeToStop );

    // We attempt to load some initial config from the serialized
    // rSequence that can be patched in this binary.
    if( NULL != ( staticConfig = getStaticConfig() ) )
    {
        if( rSequence_getBUFFER( staticConfig, RP_TAGS_HBS_ROOT_PUBLIC_KEY, &tmpBuffer, &tmpSize ) )
        {
            hbs_cloud_pub_key = rpal_memory_duplicate( tmpBuffer, tmpSize );
            if( NULL == hbs_cloud_pub_key )
            {
                hbs_cloud_pub_key = hbs_cloud_default_pub_key;
            }
            rpal_debug_info( "loading hbs root public key from static config" );
        }

        if( rSequence_getRU32( staticConfig, RP_TAGS_MAX_QUEUE_SIZE, &g_hbs_state.maxQueueNum ) )
        {
            rpal_debug_info( "loading max queue size from static config" );
        }
        else
        {
            g_hbs_state.maxQueueNum = HBS_EXFIL_QUEUE_MAX_NUM;
        }

        if( rSequence_getRU32( staticConfig, RP_TAGS_MAX_SIZE, &g_hbs_state.maxQueueSize ) )
        {
            rpal_debug_info( "loading max queue num from static config" );
        }
        else
        {
            g_hbs_state.maxQueueSize = HBS_EXFIL_QUEUE_MAX_SIZE;
        }

        rSequence_free( staticConfig );
    }
    else
    {
        hbs_cloud_pub_key = hbs_cloud_default_pub_key;
        g_hbs_state.maxQueueNum = HBS_EXFIL_QUEUE_MAX_NUM;
        g_hbs_state.maxQueueSize = HBS_EXFIL_QUEUE_MAX_SIZE;
    }

    if( !rQueue_create( &g_hbs_state.outQueue, freeExfilEvent, g_hbs_state.maxQueueNum ) )
    {
        rEvent_free( g_hbs_state.isTimeToStop );
        return (RU32)-1;
    }

    // We simply enqueue a message to let the cloud know we're starting
    sendStartupEvent();

    while( !rEvent_wait( isTimeToStop, (RU32)nextBeaconTime ) )
    {
        nextBeaconTime = MSEC_FROM_SEC( HBS_DEFAULT_BEACON_TIMEOUT + 
                         ( rpal_rand() % HBS_DEFAULT_BEACON_TIMEOUT_FUZZ ) );

        if( NULL != ( cloudMessages = beaconHome() ) )
        {
            while( rList_getSEQUENCE( cloudMessages, RP_TAGS_MESSAGE, &msg ) )
            {
                // Cloud message indicating next requested beacon time, as a Seconds delta
                if( rSequence_getTIMEDELTA( msg, RP_TAGS_TIMEDELTA, &nextBeaconTime ) )
                {
                    nextBeaconTime = MSEC_FROM_SEC( nextBeaconTime );
                    rpal_debug_info( "received set_next_beacon" );
                }

                if( NULL == newConfigurations &&
                    rSequence_getLIST( msg, RP_TAGS_HBS_CONFIGURATIONS, &newConfigurations ) )
                {
                    rpal_debug_info( "received a new profile" );

                    if( rSequence_getBUFFER( msg, RP_TAGS_HASH, &newConfigurationHash, &newHashSize ) &&
                        newHashSize == CRYPTOLIB_HASH_SIZE )
                    {
                        newConfigurations = rList_duplicate( newConfigurations );
                        rpal_memory_memcpy( &( g_hbs_state.currentConfigHash ), 
                                            newConfigurationHash, 
                                            CRYPTOLIB_HASH_SIZE );
                        g_hbs_state.isProfilePresent = TRUE;
                    }
                    else
                    {
                        newConfigurations = NULL;
                        rpal_debug_error( "profile hash received is invalid" );
                    }
                }

                if( NULL == newNotifications &&
                    rSequence_getLIST( msg, RP_TAGS_HBS_CLOUD_NOTIFICATIONS, &newNotifications ) )
                {
                    rpal_debug_info( "received cloud events" );
                    newNotifications = rList_duplicate( newNotifications );
                }
            }

            rList_free( cloudMessages );
        }

        // If this is the initial boot and we have no profile yet, we'll load a dummy
        // blank profile and use our defaults.
        if( NULL == newConfigurations &&
            !g_hbs_state.isProfilePresent &&
            !rEvent_wait( isTimeToStop, 0 ) )
        {
            newConfigurations = rList_new( RP_TAGS_HCP_MODULES, RPCM_SEQUENCE );
            rpal_debug_info( "setting empty profile" );
        }

        if( NULL != newConfigurations )
        {
            // We try to be as responsive as possible when asked to quit
            // so if we happen to have received the signal during a beacon
            // we will action the quit instead of the config change.
            if( !rEvent_wait( isTimeToStop, 0 ) )
            {
                rpal_debug_info( "begining sensor update on new profile" );
                shutdownCollectors();

                updateCollectorConfigs( newConfigurations );
                rList_free( newConfigurations );
                newConfigurations = NULL;

                startCollectors();
            }
            else
            {
                rList_free( newConfigurations );
            }

            newConfigurations = NULL;
        }

        if( NULL != newNotifications )
        {
            if( !rEvent_wait( isTimeToStop, 0 ) )
            {
                publishCloudNotifications( newNotifications );
            }

            rList_free( newNotifications );

            newNotifications = NULL;
        }
    }

    // We issue one last beacon indicating we are stopping
    sendShutdownEvent();

    // Shutdown everything
    shutdownCollectors();

    // Cleanup the last few resources
    rEvent_free( g_hbs_state.isTimeToStop );
    rQueue_free( g_hbs_state.outQueue );

    CryptoLib_deinit();

    if( hbs_cloud_default_pub_key != hbs_cloud_pub_key &&
        NULL != hbs_cloud_pub_key )
    {
        rpal_memory_free( hbs_cloud_pub_key );
        hbs_cloud_pub_key = NULL;
    }

    return ret;
}

