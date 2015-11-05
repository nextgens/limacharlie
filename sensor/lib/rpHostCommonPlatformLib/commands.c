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

#include "commands.h"
#include <rpHostCommonPlatformLib/rTags.h>
#include "configurations.h"
#include "globalContext.h"
#include "crypto.h"
#include <MemoryModule/MemoryModule.h>
#include "obfuscated.h"
#include <obfuscationLib/obfuscationLib.h>
#include "beacon.h"
#include "crashHandling.h"

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <dlfcn.h>
#include <sys/stat.h>
#endif

#define RPAL_FILE_ID    51

static
RU32
    RPAL_THREAD_FUNC thread_quitAndCleanup
    (
        RPVOID context
    )
{
    UNREFERENCED_PARAMETER( context );

    if( 0 == rInterlocked_decrement32( &g_hcpContext.isRunning ) )
    {
        rpal_thread_sleep( MSEC_FROM_SEC( 1 ) );

        stopAllModules();

        stopBeacons();

        if( NULL != g_hcpContext.enrollmentToken &&
            0 != g_hcpContext.enrollmentTokenSize )
        {
            rpal_memory_free( g_hcpContext.enrollmentToken );
        }

        // If the default crashContext is still present, remove it since
        // we are shutting down properly. If it's non-default leave it since
        // somehow we may have had a higher order crash we want to keep
        // track of but we are still leaving through our normal code path.
        if( 1 == getCrashContextSize() )
        {
            cleanCrashContext();
        }

        rpal_Context_cleanup();

        rpal_Context_deinitialize();
    }
    else
    {
        rInterlocked_increment32( &g_hcpContext.isRunning );
    }

    return 0;
}

static
RBOOL 
    loadModule
    (
        rSequence seq
    )
{
    RBOOL isSuccess = FALSE;

    RU32 moduleIndex = (RU32)(-1);

    RPU8 tmpBuff = NULL;
    RU32 tmpSize = 0;

    RPU8 tmpSig = NULL;
    RU32 tmpSigSize = 0;

    rpal_thread_func pEntry = NULL;

    rpHCPModuleContext* modContext = NULL;

    OBFUSCATIONLIB_DECLARE( entryName, RP_HCP_CONFIG_MODULE_ENTRY );

    if( NULL != seq )
    {
        for( moduleIndex = 0; moduleIndex < RP_HCP_CONTEXT_MAX_MODULES; moduleIndex++ )
        {
            if( 0 == g_hcpContext.modules[ moduleIndex ].hThread )
            {
                // Found an empty spot
                break;
            }
        }
    }

    if( RP_HCP_CONTEXT_MAX_MODULES != moduleIndex &&
        (RU32)(-1) != moduleIndex )
    {
        // We got an empty spot for our module
        if( rSequence_getRU8( seq, 
                                RP_TAGS_HCP_MODULE_ID, 
                                &(g_hcpContext.modules[ moduleIndex ].id) ) &&
            rSequence_getBUFFER( seq,
                                    RP_TAGS_BINARY, 
                                    &tmpBuff, 
                                    &tmpSize ) &&
            rSequence_getBUFFER( seq,
                                    RP_TAGS_SIGNATURE,
                                    &tmpSig,
                                    &tmpSigSize ) )
        {
            // We got the data, now verify the buffer signature
            if( CryptoLib_verify( tmpBuff, tmpSize, getRootPublicKey(), tmpSig ) )
            {
                // Ready to load the module
                rpal_debug_info( "loading module in memory" );
                g_hcpContext.modules[ moduleIndex ].hModule = MemoryLoadLibrary( tmpBuff, tmpSize );

                if( NULL != g_hcpContext.modules[ moduleIndex ].hModule )
                {
                    OBFUSCATIONLIB_TOGGLE( entryName );

                    pEntry = (rpal_thread_func)MemoryGetProcAddress( g_hcpContext.modules[ moduleIndex ].hModule, 
                                                            (RPCHAR)entryName );

                    OBFUSCATIONLIB_TOGGLE( entryName );

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
                                CryptoLib_hash( tmpBuff, tmpSize, (RPU8)&(g_hcpContext.modules[ moduleIndex ].hash) );
                                g_hcpContext.modules[ moduleIndex ].isOsLoaded = FALSE;
                                isSuccess = TRUE;
                            }
                            else
                            {
                                rpal_debug_warning( "Error creating handler thread for new module." );
                            }
                        }
                    }
                    else
                    {
                        rpal_debug_warning( "Could not find new module's entry point." );
                    }
                }
                else
                {
                    rpal_debug_warning( "Error loading module in memory." );
                }
            }
            else
            {
                rpal_debug_warning( "New module signature invalid." );
            }
        }
        else
        {
            rpal_debug_warning( "Could not find core module components to load." );
        }

        // Main cleanup
        if( !isSuccess )
        {
            if( NULL != modContext )
            {
                IF_VALID_DO( modContext->isTimeToStop, rEvent_free );
            }

            if( NULL != g_hcpContext.modules[ moduleIndex ].hModule )
            {
                MemoryFreeLibrary( g_hcpContext.modules[ moduleIndex ].hModule );
            }

            rpal_memory_zero( &(g_hcpContext.modules[ moduleIndex ]), 
                              sizeof( g_hcpContext.modules[ moduleIndex ] ) );
        }
    }
    else
    {
        rpal_debug_error( "Could not find a spot for new module, or invalid module id!" );
    }

    return isSuccess;
}


static
RBOOL 
    unloadModule
    (
        rSequence seq
    )
{
    RBOOL isSuccess = FALSE;

    RpHcp_ModuleId moduleId = (RU8)(-1);
    RU32 moduleIndex = 0;

    if( NULL != seq )
    {
        if( rSequence_getRU8( seq, 
                              RP_TAGS_HCP_MODULE_ID, 
                              &moduleId ) )
        {
            for( moduleIndex = 0; moduleIndex < RP_HCP_CONTEXT_MAX_MODULES; moduleIndex++ )
            {
                if( moduleId == g_hcpContext.modules[ moduleIndex ].id )
                {
                    break;
                }
            }
        }
    }

    if( (RU32)(-1) != moduleIndex &&
        RP_HCP_CONTEXT_MAX_MODULES != moduleIndex )
    {
#ifdef RP_HCP_LOCAL_LOAD
        if( g_hcpContext.modules[ moduleIndex ].isOsLoaded )
        {
            // We do not unload modules loaded by the OS in debug since
            // they are used to debug modules during development.
            return FALSE;
        }
#endif
        if( rEvent_set( g_hcpContext.modules[ moduleIndex ].isTimeToStop ) &&
            rpal_thread_wait( g_hcpContext.modules[ moduleIndex ].hThread, (30*1000) ) )
        {
            isSuccess = TRUE;

            rEvent_free( g_hcpContext.modules[ moduleIndex ].context.isTimeToStop );
            rpal_thread_free( g_hcpContext.modules[ moduleIndex ].hThread );

            if( g_hcpContext.modules[ moduleIndex ].isOsLoaded )
            {
#ifdef RPAL_PLATFORM_WINDOWS
                FreeLibrary( (HMODULE)g_hcpContext.modules[ moduleIndex ].hModule );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                dlclose( g_hcpContext.modules[ moduleIndex ].hModule );
#endif
            }
            else
            {
                MemoryFreeLibrary( g_hcpContext.modules[ moduleIndex ].hModule );
            }

            rpal_memory_zero( &(g_hcpContext.modules[ moduleIndex ]),
                              sizeof( g_hcpContext.modules[ moduleIndex ] ) );
        }
    }

    return isSuccess;
}





RBOOL
    processMessage
    (
        rSequence seq
    )
{
    RBOOL isSuccess = FALSE;
    RU8 command = 0;
    rSequence idSeq = NULL;
    rpHCPId tmpId = {0};
    RU64 tmpTime = 0;
    rThread hQuitThread = 0;
    RU64 delta = 0;

    rpHCPIdentStore identStore = {0};
    RPU8 token = NULL;
    RU32 tokenSize = 0;
    rFile hStore = NULL;

    OBFUSCATIONLIB_DECLARE( store, RP_HCP_CONFIG_IDENT_STORE );

    if( NULL != seq )
    {
        if( rSequence_getRU8( seq, RP_TAGS_OPERATION, &command ) )
        {
            rpal_debug_info( "Received command 0x%0X.", command );
            switch( command )
            {
            case RP_HCP_COMMAND_LOAD_MODULE:
                isSuccess = loadModule( seq );
                break;
            case RP_HCP_COMMAND_UNLOAD_MODULE:
                isSuccess = unloadModule( seq );
                break;
            case RP_HCP_COMMAND_SET_NEXT_BEACON:
                if( rSequence_getTIMEDELTA( seq, RP_TAGS_TIMEDELTA, &delta ) )
                {
                    g_hcpContext.beaconTimeout = delta;
                    rpal_debug_info( "setting next beacon delta to %d", delta );
                    isSuccess = TRUE;
                }
                break;
            case RP_HCP_COMMAND_SET_HCP_ID:
                if( rSequence_getSEQUENCE( seq, RP_TAGS_HCP_ID, &idSeq ) )
                {
                    tmpId = seqToHcpId( idSeq );

                    if( 0 != tmpId.raw )
                    {
                        g_hcpContext.currentId = tmpId;
                        
                        OBFUSCATIONLIB_TOGGLE( store );
                        
                        if( rSequence_getBUFFER( seq, RP_TAGS_HCP_ENROLLMENT_TOKEN, &token, &tokenSize ) )
                        {
                            // Enrollment V2
                            identStore.agentId = tmpId;
                            identStore.enrollmentTokenSize = tokenSize;
                            if( rFile_open( (RPWCHAR)store, &hStore, RPAL_FILE_OPEN_ALWAYS | RPAL_FILE_OPEN_WRITE ) )
                            {
                                if( rFile_write( hStore, sizeof( identStore ), &identStore ) &&
                                    rFile_write( hStore, tokenSize, token ) )
                                {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                                    RPCHAR tmpStore = NULL;
                                    if( NULL != ( tmpStore = rpal_string_wtoa( (RPWCHAR)store ) ) )
                                    {
                                        chmod( tmpStore, S_IRUSR | S_IWUSR );
                                        rpal_memory_free( tmpStore );
                                    }
#endif
                                    isSuccess = TRUE;
                                }

                                rFile_close( hStore );
                            }
                            else
                            {
                                rpal_debug_warning( "could not write enrollment token to disk" );
                            }

                            if( NULL != ( g_hcpContext.enrollmentToken = rpal_memory_alloc( tokenSize ) ) )
                            {
                                rpal_memory_memcpy( g_hcpContext.enrollmentToken, token, tokenSize );
                                g_hcpContext.enrollmentTokenSize = tokenSize;

                                isSuccess = TRUE;
                            }
                        }
                        else
                        {
                            // Enrollment V1
                            if( rpal_file_writew( (RPWCHAR)store, 
                                                  &g_hcpContext.currentId, 
                                                  sizeof( g_hcpContext.currentId ), 
                                                  TRUE ) )
                            {
                                rpal_debug_info( "agentid set to %x.%x.%x.%x.%x", 
                                                 g_hcpContext.currentId.id.orgId, 
                                                 g_hcpContext.currentId.id.subnetId, 
                                                 g_hcpContext.currentId.id.uniqueId, 
                                                 g_hcpContext.currentId.id.platformId, 
                                                 g_hcpContext.currentId.id.configId );
                                isSuccess = TRUE;
                            }
                            else
                            {
                                rpal_debug_warning( "new id could not be written to identity file: %S", (RPWCHAR)store );
                            }
                        }

                        OBFUSCATIONLIB_TOGGLE( store );
                    }
                }
                break;
            case RP_HCP_COMMAND_SET_GLOBAL_TIME:
                if( rSequence_getTIMESTAMP( seq, RP_TAGS_TIMESTAMP, &tmpTime ) )
                {
                    rpal_time_setGlobalOffset( tmpTime - rpal_time_getLocal() );
                    isSuccess = TRUE;
                }
                break;
            case RP_HCP_COMMAND_QUIT:
                if( 0 != ( hQuitThread = rpal_thread_new( thread_quitAndCleanup, NULL ) ) )
                {
                    rpal_thread_free( hQuitThread );
                    isSuccess = TRUE;
                }
                break;
            default:
                break;
            }

            if( isSuccess )
            {
                rpal_debug_info( "Command was successful." );
            }
            else
            {
                rpal_debug_warning( "Command was not successful." );
            }
        }
    }

    return isSuccess;
}

RBOOL
    stopAllModules
    (

    )
{
    RBOOL isSuccess = TRUE;

    RU32 moduleIndex = 0;

    for( moduleIndex = 0; moduleIndex < RP_HCP_CONTEXT_MAX_MODULES; moduleIndex++ )
    {
        if( 0 != g_hcpContext.modules[ moduleIndex ].hThread )
        {
            if( rEvent_set( g_hcpContext.modules[ moduleIndex ].isTimeToStop ) &&
                rpal_thread_wait( g_hcpContext.modules[ moduleIndex ].hThread, RP_HCP_CONTEXT_MODULE_TIMEOUT ) )
            {
                rEvent_free( g_hcpContext.modules[ moduleIndex ].context.isTimeToStop );
                rpal_thread_free( g_hcpContext.modules[ moduleIndex ].hThread );

                if( g_hcpContext.modules[ moduleIndex ].isOsLoaded )
                {
#ifdef RPAL_PLATFORM_WINDOWS
                    FreeLibrary( (HMODULE)g_hcpContext.modules[ moduleIndex ].hModule );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                    dlclose( g_hcpContext.modules[ moduleIndex ].hModule );
#endif
                }
                else
                {
                    MemoryFreeLibrary( g_hcpContext.modules[ moduleIndex ].hModule );
                }

                rpal_memory_zero( &(g_hcpContext.modules[ moduleIndex ]),
                                  sizeof( g_hcpContext.modules[ moduleIndex ] ) );
            }
            else
            {
                isSuccess = FALSE;
            }
        }
    }

    rpal_debug_info( "finished stopping all modules" );

    return isSuccess;
}


