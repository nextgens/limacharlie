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

#include <rpHostCommonPlatformLib/rpHostCommonPlatformLib.h>
#include "configurations.h"
#include "globalContext.h"
#include "obfuscated.h"
#include <obfuscationLib/obfuscationLib.h>
#include "beacon.h"
#include <rpHostCommonPlatformLib/rTags.h>
#include <librpcm/librpcm.h>
#include "commands.h"
#include "crashHandling.h"
#include "crypto.h"

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <dlfcn.h>
#endif

#define RPAL_FILE_ID    53

rpHCPContext g_hcpContext = {0};
rpHCPId g_idTemplate = {0};

// Large blank buffer to be used to patch configurations post-build
#define _HCP_DEFAULT_STATIC_STORE_SIZE                          (1024 * 10)
#define _HCP_DEFAULT_STATIC_STORE_MAGIC                         { 0xFA, 0x57, 0xF0, 0x0D }
static RU8 g_patchedConfig[ _HCP_DEFAULT_STATIC_STORE_SIZE ] =  _HCP_DEFAULT_STATIC_STORE_MAGIC;
#define _HCP_DEFAULT_STATIC_STORE_KEY                           { 0xFA, 0x75, 0x01 }

//=============================================================================
//  Helpers
//=============================================================================

#ifdef RPAL_PLATFORM_WINDOWS
static
BOOL
    ctrlHandler
    (
        DWORD type
    )
{
    OBFUSCATIONLIB_DECLARE( store, RP_HCP_CONFIG_CRASH_STORE );
    UNREFERENCED_PARAMETER( type );

    if( CTRL_SHUTDOWN_EVENT == type )
    {
        // This is an emergency shutdown.
        // Trying to do this cleanly is pointless since Windows
        // will kill us very shortly, so let's just clean up
        // the CC so we don't report a pointless "crash".
        OBFUSCATIONLIB_TOGGLE( store );
        rpal_file_deletew( (RPWCHAR)store, FALSE );
        OBFUSCATIONLIB_TOGGLE( store );
    }

    // Pass the signal along
    return FALSE;
}
#endif


rSequence
    hcpIdToSeq
    (
        rpHCPId id
    )
{
    rSequence seq = NULL;

    if( NULL != ( seq = rSequence_new() ) )
    {
        if( !rSequence_addRU8( seq, RP_TAGS_HCP_ID_ORG, id.id.orgId ) ||
            !rSequence_addRU8( seq, RP_TAGS_HCP_ID_SUBNET, id.id.subnetId ) ||
            !rSequence_addRU32( seq, RP_TAGS_HCP_ID_UNIQUE, id.id.uniqueId ) ||
            !rSequence_addRU8( seq, RP_TAGS_HCP_ID_PLATFORM, id.id.platformId ) ||
            !rSequence_addRU8( seq, RP_TAGS_HCP_ID_CONFIG, id.id.configId ) )
        {
            DESTROY_AND_NULL( seq, rSequence_free );
        }
    }

    return seq;
}

rpHCPId
    seqToHcpId
    (
        rSequence seq
    )
{
    rpHCPId id = g_idTemplate;

    if( NULL != seq )
    {
        if( !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_ORG, 
                               &(id.id.orgId) ) ||
            !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_SUBNET, 
                               &(id.id.subnetId) ) ||
            !rSequence_getRU32( seq, 
                                RP_TAGS_HCP_ID_UNIQUE, 
                                &(id.id.uniqueId) ) ||
            !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_PLATFORM, 
                               &(id.id.platformId) ) ||
            !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_CONFIG, 
                               &(id.id.configId) ) )
        {
            id.raw = 0;
        }
    }

    return id;
}

static
RBOOL
    getStoreConf
    (

    )
{
    RBOOL isSuccess = FALSE;

    RPU8 storeFile = NULL;
    RU32 storeFileSize = 0;

    rpHCPIdentStore* storeV2 = NULL;

    OBFUSCATIONLIB_DECLARE( store, RP_HCP_CONFIG_IDENT_STORE );

    OBFUSCATIONLIB_TOGGLE( store );

    if( rpal_file_readw( (RPWCHAR)store, (RPVOID)&storeFile, &storeFileSize, FALSE ) )
    {
        if( sizeof( rpHCPIdentStore ) <= storeFileSize )
        {
            storeV2 = (rpHCPIdentStore*)storeFile;
            if( storeV2->enrollmentTokenSize == storeFileSize - sizeof( rpHCPIdentStore ) )
            {
                isSuccess = TRUE;
                rpal_debug_info( "ident store found" );
                if( NULL != ( g_hcpContext.enrollmentToken = rpal_memory_alloc( storeV2->enrollmentTokenSize ) ) )
                {
                    rpal_memory_memcpy( g_hcpContext.enrollmentToken, storeV2->enrollmentToken, storeV2->enrollmentTokenSize );
                    g_hcpContext.enrollmentTokenSize = storeV2->enrollmentTokenSize;
                }
                g_hcpContext.currentId = storeV2->agentId;
            }
            else
            {
                rpal_debug_warning( "inconsistent ident store, reseting" );
                rpal_file_deletew( (RPWCHAR)store, FALSE );
            }
        }

        rpal_memory_free( storeFile );
    }

    OBFUSCATIONLIB_TOGGLE( store );

    // Set some always-correct defaults
    g_hcpContext.currentId.id.platformId = RP_HCP_ID_MAKE_PLATFORM( RP_HCP_PLATFORM_CURRENT_CPU, RP_HCP_PLATFORM_CURRENT_MAJOR, RP_HCP_PLATFORM_CURRENT_MINOR );

    return isSuccess;
}

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

//=============================================================================
//  API
//=============================================================================

RBOOL
    rpHostCommonPlatformLib_launch
    (
        RU8 configHint,
        RPCHAR primaryHomeUrl,
        RPCHAR secondaryHomeUrl
    )
{
    RBOOL isInitSuccessful = FALSE;
    rSequence staticConfig = NULL;
    RPCHAR tmpStr = NULL;
    rSequence tmpSeq = NULL;
    RPU8 tmpBuffer = NULL;
    RU32 tmpSize = 0;

    rpal_debug_info( "launching hcp" );

#ifdef RPAL_PLATFORM_WINDOWS
    if( setGlobalCrashHandler() &&
        SetConsoleCtrlHandler( (PHANDLER_ROUTINE)ctrlHandler, TRUE ) )
    {
        rpal_debug_info( "global crash handler set" );
    }
    else
    {
        rpal_debug_warning( "error setting global crash handler" );
    }
#endif

    if( 1 == rInterlocked_increment32( &g_hcpContext.isRunning ) )
    {
        if( rpal_initialize( NULL, RPAL_COMPONENT_HCP ) )
        {
            CryptoLib_init();

            g_hcpContext.currentId.raw = g_idTemplate.raw;

            // We attempt to load some initial config from the serialized
            // rSequence that can be patched in this binary.
            if( NULL != ( staticConfig = getStaticConfig() ) )
            {
                if( rSequence_getSTRINGA( staticConfig, RP_TAGS_HCP_PRIMARY_URL, &tmpStr ) )
                {
                    g_hcpContext.primaryUrl = rpal_string_strdupa( tmpStr );
                    rpal_debug_info( "loading primary url from static config" );
                }

                if( rSequence_getSTRINGA( staticConfig, RP_TAGS_HCP_SECONDARY_URL, &tmpStr ) )
                {
                    g_hcpContext.secondaryUrl = rpal_string_strdupa( tmpStr );
                    rpal_debug_info( "loading secondary url from static config" );
                }

                if( rSequence_getSEQUENCE( staticConfig, RP_TAGS_HCP_ID, &tmpSeq ) )
                {
                    g_hcpContext.currentId = seqToHcpId( tmpSeq );
                    rpal_debug_info( "loading default id from static config" );
                }

                if( rSequence_getBUFFER( staticConfig, RP_TAGS_HCP_C2_PUBLIC_KEY, &tmpBuffer, &tmpSize ) )
                {
                    setC2PublicKey( rpal_memory_duplicate( tmpBuffer, tmpSize ) );
                    rpal_debug_info( "loading c2 public key from static config" );
                }

                if( rSequence_getBUFFER( staticConfig, RP_TAGS_HCP_ROOT_PUBLIC_KEY, &tmpBuffer, &tmpSize ) )
                {
                    setRootPublicKey( rpal_memory_duplicate( tmpBuffer, tmpSize ) );
                    rpal_debug_info( "loading root public key from static config" );
                }

                rSequence_free( staticConfig );
            }

            // Now we will override the defaults (if present) with command
            // line parameters.
            if( NULL != primaryHomeUrl &&
                0 != rpal_string_strlen( primaryHomeUrl ) )
            {
                if( NULL != g_hcpContext.primaryUrl )
                {
                    rpal_memory_free( g_hcpContext.primaryUrl );
                    g_hcpContext.primaryUrl = NULL;
                }
                g_hcpContext.primaryUrl = rpal_string_strdupa( primaryHomeUrl );
            }

            if( NULL != secondaryHomeUrl &&
                0 != rpal_string_strlen( secondaryHomeUrl  ) )
            {
                if( NULL != g_hcpContext.secondaryUrl )
                {
                    rpal_memory_free( g_hcpContext.secondaryUrl );
                    g_hcpContext.secondaryUrl = NULL;
                }
                g_hcpContext.secondaryUrl = rpal_string_strdupa( secondaryHomeUrl );
            }

            g_hcpContext.enrollmentToken = NULL;
            g_hcpContext.enrollmentTokenSize = 0;

            getStoreConf();  /* Sets the agent ID platform. */
            
            // Set the current configId
            g_hcpContext.currentId.id.configId = configHint;

            if( startBeacons() )
            {
                isInitSuccessful = TRUE;
            }
            else
            {
                rpal_debug_warning( "error starting beacons" );
            }

            CryptoLib_deinit();
        }
        else
        {
            rpal_debug_warning( "hcp platform could not init rpal" );
        }
    }
    else
    {
        rInterlocked_decrement32( &g_hcpContext.isRunning );
        rpal_debug_info( "hcp already launched" );
    }

    return isInitSuccessful;
}



RBOOL
    rpHostCommonPlatformLib_stop
    (

    )
{
    if( 0 == rInterlocked_decrement32( &g_hcpContext.isRunning ) )
    {
        stopBeacons();
        stopAllModules();

        rpal_memory_free( g_hcpContext.primaryUrl );
        rpal_memory_free( g_hcpContext.secondaryUrl );

        if( NULL != g_hcpContext.enrollmentToken &&
            0 != g_hcpContext.enrollmentTokenSize )
        {
            rpal_memory_free( g_hcpContext.enrollmentToken );
        }

        freeKeys();

#ifdef RPAL_PLATFORM_WINDOWS
        SetConsoleCtrlHandler( (PHANDLER_ROUTINE)ctrlHandler, FALSE );
#endif

        rpal_Context_cleanup();

        rpal_Context_deinitialize();

        // If the default crashContext is still present, remove it since
        // we are shutting down properly. If it's non-default leave it since
        // somehow we may have had a higher order crash we want to keep
        // track of but we are still leaving through our normal code path.
        if( 1 == getCrashContextSize() )
        {
            rpal_debug_info( "clearing default crash context" );
            cleanCrashContext();
        }
    }
    else
    {
        rInterlocked_increment32( &g_hcpContext.isRunning );
    }

    rpal_debug_info( "finished stopping hcp" );

    return TRUE;
}

#ifdef RP_HCP_LOCAL_LOAD
RBOOL
    rpHostCommonPlatformLib_load
    (
        RPWCHAR modulePath
    )
{
    RBOOL isSuccess = FALSE;

    RU32 moduleIndex = 0;
    rpal_thread_func pEntry = NULL;
    rpHCPModuleContext* modContext = NULL;
    RPCHAR errorStr = NULL;

    OBFUSCATIONLIB_DECLARE( entrypoint, RP_HCP_CONFIG_MODULE_ENTRY );

    if( NULL != modulePath )
    {
        for( moduleIndex = 0; moduleIndex < RP_HCP_CONTEXT_MAX_MODULES; moduleIndex++ )
        {
            if( 0 == g_hcpContext.modules[ moduleIndex ].hThread )
            {
                // Found an empty spot
#ifdef RPAL_PLATFORM_WINDOWS
                g_hcpContext.modules[ moduleIndex ].hModule = LoadLibraryW( modulePath );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                RPCHAR tmpPath = NULL;
                if( NULL != ( tmpPath = rpal_string_wtoa( modulePath ) ) )
                {
                    g_hcpContext.modules[ moduleIndex ].hModule = dlopen( tmpPath, RTLD_NOW | RTLD_LOCAL );
                    rpal_memory_free( tmpPath );
                }
#endif
                if( NULL != g_hcpContext.modules[ moduleIndex ].hModule )
                {
                    OBFUSCATIONLIB_TOGGLE( entrypoint );
#ifdef RPAL_PLATFORM_WINDOWS
                    pEntry = (rpal_thread_func)GetProcAddress( (HMODULE)g_hcpContext.modules[ moduleIndex ].hModule, 
                                                               (RPCHAR)entrypoint );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                    pEntry = (rpal_thread_func)dlsym( g_hcpContext.modules[ moduleIndex ].hModule, (RPCHAR)entrypoint );
#endif
                    OBFUSCATIONLIB_TOGGLE( entrypoint );

                    if( NULL != pEntry )
                    {
                        modContext = &(g_hcpContext.modules[ moduleIndex ].context);

                        modContext->pCurrentId = &(g_hcpContext.currentId);
                        modContext->func_beaconHome = doBeacon;
                        modContext->isTimeToStop = rEvent_create( TRUE );
                        modContext->rpalContext = rpal_Context_get();

                        if( NULL != modContext->isTimeToStop )
                        {
                            g_hcpContext.modules[ moduleIndex ].isTimeToStop  = modContext->isTimeToStop;

                            g_hcpContext.modules[ moduleIndex ].hThread = rpal_thread_new( pEntry, modContext );

                            if( 0 != g_hcpContext.modules[ moduleIndex ].hThread )
                            {
                                g_hcpContext.modules[ moduleIndex ].isOsLoaded = TRUE;
                                isSuccess = TRUE;
                                rpal_debug_info( "module %S successfully loaded manually.", modulePath );
                            }
                        }
                    }
                    else
                    {
#ifdef RPAL_PLATFORM_WINDOWS
                        FreeLibrary( (HMODULE)g_hcpContext.modules[ moduleIndex ].hModule );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                        dlclose( g_hcpContext.modules[ moduleIndex ].hModule );
#endif
                        g_hcpContext.modules[ moduleIndex ].hModule = NULL;
                        rpal_debug_error( "Could not manually finding the entry point to a module!" );
                    }
                }
                else
                {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                    errorStr = dlerror();
#endif
                    rpal_debug_error( "Could not manually load module %S: %s", modulePath, errorStr );
                }

                break;
            }
        }
    }
    //forceCrash();
    return isSuccess;
}

RBOOL
    rpHostCommonPlatformLib_unload
    (
        RU8 moduleId
    )
{
    RBOOL isSuccess = FALSE;

    RU32 moduleIndex = 0;

    for( moduleIndex = 0; moduleIndex < RP_HCP_CONTEXT_MAX_MODULES; moduleIndex++ )
    {
        if( moduleId == g_hcpContext.modules[ moduleIndex ].id )
        {
            if( rEvent_set( g_hcpContext.modules[ moduleIndex ].isTimeToStop ) &&
                rpal_thread_wait( g_hcpContext.modules[ moduleIndex ].hThread, (30*1000) ) )
            {
                isSuccess = TRUE;
#ifdef RPAL_PLATFORM_WINDOWS
                FreeLibrary( (HMODULE)g_hcpContext.modules[ moduleIndex ].hModule );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                dlclose( g_hcpContext.modules[ moduleIndex ].hModule );
#endif
                rEvent_free( g_hcpContext.modules[ moduleIndex ].context.isTimeToStop );
                rpal_thread_free( g_hcpContext.modules[ moduleIndex ].hThread );
            }

            break;
        }
    }

    return isSuccess;
}
#endif

