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
#include <notificationsLib/notificationsLib.h>
#include <cryptoLib/cryptoLib.h>
#include "aad.h"
//=============================================================================
//  RP HCP Module Requirements
//=============================================================================
RpHcp_ModuleId g_current_Module_id = 2;


//=============================================================================
//  Global Context
//=============================================================================
rEvent g_timeToStopEvent = NULL;
rBTree g_stashes_phase_1[ AAD_REL_MAX ] = { 0 };
rBTree g_stashes_phase_2[ AAD_REL_MAX ] = { 0 };
#define FILE_HASH_CHECK_PERCENT     33  // Only check hashes 33% of the time.
#define FILE_HASH_RESET_TIME        (60*60*24)  // Reset the cache every day.
#define AAD_STORAGE_ROOT            _WCH( "%PROGRAMDATA%" )
#define AAD_STORAGE_DIRECTORY       _WCH( "%PROGRAMDATA%\\rpHcp" )

//=============================================================================
//  Utilities
//=============================================================================
static RS32
    compareHash
    (
        RPU8 hash1,
        RPU8 hash2
    )
{
    RS32 ret = 0;

    if( NULL != hash1 &&
        NULL != hash2 )
    {
        ret = rpal_memory_memcmp( hash1, hash2, CRYPTOLIB_HASH_SIZE );
    }

    return ret;
}
//=============================================================================
//  Collection Threads
//=============================================================================
static RU32 RPAL_THREAD_FUNC
    collection_process_modules
    (
        RPVOID context
    )
{
    processLibProcEntry* processEntries = NULL;
    processLibProcEntry* curProcessEntry = NULL;
    rSequence processInfo = NULL;
    rList modules = NULL;
    rSequence module = NULL;
    RPWCHAR processPath = NULL;
    RPWCHAR processName = NULL;
    RPWCHAR modulePath = NULL;
    RPWCHAR moduleName = NULL;
    rSequence report = NULL;
    RU8 fileHash[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    rBTree checkedHashes = NULL;
    RU8 fileNameHash[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    RTIME lastCacheReset = 0;
    
    UNREFERENCED_PARAMETER( context );

    while( !rEvent_wait( g_timeToStopEvent, 0 ) )
    {
        rpal_debug_info( "Initiating new full process scan." );

        if( NULL == checkedHashes )
        {
            checkedHashes = rpal_btree_create( CRYPTOLIB_HASH_SIZE, compareHash, NULL );
            lastCacheReset = rpal_time_getLocal();
        }

        if( NULL != ( processEntries = processLib_getProcessEntries( TRUE ) ) )
        {
            curProcessEntry = processEntries;

            while( 0 != curProcessEntry->pid &&
                   !rEvent_wait( g_timeToStopEvent, 0 ) )
            {
                rpal_thread_sleep( 100 );

                processInfo = NULL;

                if( NULL != ( processInfo = processLib_getProcessInfo( curProcessEntry->pid ) ) &&
                    rSequence_getSTRINGW( processInfo, RP_TAGS_FILE_PATH, &processPath ) )
                {
                    rpal_string_tolowerw( processPath );

                    // We only care about the exe name, not the full path.
                    processName = rpal_file_filePathToFileName( processPath );

                    if( aad_checkRelation_WCH_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_PATH ], processName, processPath ) &&
                        NULL != ( report = aad_newReport_WCH_WCH( AAD_REL_PROCESS_PATH, processName, processPath ) ) )
                    {
                        notifications_publish( RP_TAGS_NOTIFICATION_NEW_RELATION, report );
                        rSequence_free( report );
                    }
                }

                if( NULL != ( modules = processLib_getProcessModules( curProcessEntry->pid ) ) )
                {
                    while( !rEvent_wait( g_timeToStopEvent, 0 ) &&
                           rList_getSEQUENCE( modules, RP_TAGS_DLL, &module ) )
                    {
                        rpal_thread_sleep( 100 );

                        if( rSequence_getSTRINGW( module, RP_TAGS_FILE_PATH, &modulePath ) )
                        {
                            rpal_string_tolowerw( modulePath );

                            if( NULL != processName &&
                                aad_checkRelation_WCH_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_MODULE ], processName, modulePath ) &&
                                NULL != ( report = aad_newReport_WCH_WCH( AAD_REL_PROCESS_MODULE, processName, modulePath ) ) )
                            {
                                notifications_publish( RP_TAGS_NOTIFICATION_NEW_RELATION, report );
                                rSequence_free( report );
                            }

                            if( rSequence_getSTRINGW( module, RP_TAGS_MODULE_NAME, &moduleName ) )
                            {
                                rpal_string_tolowerw( moduleName );

                                if( aad_checkRelation_WCH_WCH( g_stashes_phase_2[ AAD_REL_MODULE_PATH ], moduleName, modulePath ) &&
                                    NULL != ( report = aad_newReport_WCH_WCH( AAD_REL_MODULE_PATH, moduleName, modulePath ) ) )
                                {
                                    notifications_publish( RP_TAGS_NOTIFICATION_NEW_RELATION, report );
                                    rSequence_free( report );
                                }

                                // We can throttle the likeliness of us checking a hash.
                                // But first we check if we've hashed that path in the current run.
                                if( CryptoLib_hash( modulePath, rpal_string_strlenw( modulePath ), fileNameHash ) &&
                                    rpal_btree_add( checkedHashes, fileNameHash, FALSE ) &&
                                    ( rpal_rand() % 100 ) < FILE_HASH_CHECK_PERCENT )
                                {
                                    rpal_debug_info( "Initiating a hash lookup of %ls.", modulePath );

                                    if( CryptoLib_hashFileW( modulePath, fileHash, FALSE ) )
                                    {
                                        if( aad_checkRelation_WCH_HASH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], moduleName, fileHash ) &&
                                            NULL != ( report = aad_newReport_WCH_HASH( AAD_REL_MODULE_HASH, moduleName, fileHash ) ) )
                                        {
                                            notifications_publish( RP_TAGS_NOTIFICATION_NEW_RELATION, report );
                                            rSequence_free( report );
                                        }

                                        // These have been pre-populated by the phase 1, see below
                                        if( aad_checkRelation_HASH_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], fileHash, moduleName ) &&
                                            NULL != ( report = aad_newReport_HASH_WCH( AAD_REL_HASH_MODULE, fileHash, moduleName ) ) )
                                        {
                                            notifications_publish( RP_TAGS_NOTIFICATION_NEW_RELATION, report );
                                            rSequence_free( report );
                                        }

                                        // Checking phase 1 to feed new hashes into phase 2
                                        if( aad_isParentEnabled_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], moduleName ) )
                                        {
                                            rpal_debug_info( "New hash found for HASH_MODULE relation found." );
                                            aad_enableParent_HASH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], fileHash, 20, 0.01 );
                                        }
                                    }
                                    else
                                    {
                                        rpal_debug_warning( "Failed to hash file: %ls.", modulePath );
                                    }
                                }
                            }
                        }
                    }

                    rList_free( modules );
                }

                if( NULL != processInfo )
                {
                    rSequence_free( processInfo );
                }

                curProcessEntry++;
            }

            rpal_memory_free( processEntries );
        }

        if( FILE_HASH_RESET_TIME < ( rpal_time_getLocal() - lastCacheReset ) )
        {
            rpal_debug_info( "Reseting the file hash cache." );
            rpal_btree_destroy( checkedHashes, FALSE );
            lastCacheReset = 0;
            checkedHashes = NULL;
        }
    }


    return 0;
}

//=============================================================================
//  Persistence
//=============================================================================
static RBOOL
    loadStashesFromDisk
    (

    )
{
    RBOOL isStashesFound = FALSE;

    RWCHAR storageDir[] = { AAD_STORAGE_DIRECTORY };

    rDir hDir = NULL;
    rFileInfo fInfo = { 0 };
    RU32 relTypeId = 0;

    RPU8 fileBuffer = NULL;
    RU32 fileSize = 0;

    if( rDir_open( storageDir, &hDir ) )
    {
        while( rDir_next( hDir, &fInfo ) )
        {
            if( !IS_FLAG_ENABLED( RPAL_FILE_ATTRIBUTE_DIRECTORY, fInfo.attributes ) )
            {
                if( rpal_string_wtoi( fInfo.fileName, &relTypeId ) &&
                    AAD_REL_MAX > relTypeId )
                {
                    if( rpal_file_readw( fInfo.filePath, &fileBuffer, &fileSize, FALSE ) )
                    {
                        if( aad_loadStashFromBuffer( g_stashes_phase_2[ relTypeId ], fileBuffer, fileSize ) )
                        {
                            isStashesFound = TRUE;
                            rpal_debug_info( "Finishes loading stash from: %ls.", fInfo.filePath );
                        }

                        rpal_memory_free( fileBuffer );
                    }
                    else
                    {
                        rpal_debug_warning( "Error reading stash from: %ls.", fInfo.filePath );
                    }
                }
            }
        }

        rDir_close( hDir );
    }

    return isStashesFound;
}

static RBOOL
    storeStashesToDisk
    (

    )
{
    RBOOL isStored = FALSE;

    RWCHAR filePath[ ARRAY_N_ELEM( AAD_STORAGE_DIRECTORY ) + 4 ] = {0};
    RWCHAR storageDir[] = { AAD_STORAGE_DIRECTORY };
    rFileInfo fInfo = { 0 };

    RU32 relTypeId = 0;
    RPU8 fileBuffer = NULL;
    RU32 fileSize = 0;

    // This is ugly but I want to assemble the path on the stack.
    rpal_memory_memcpy( filePath, storageDir, sizeof( storageDir ) );
    rpal_string_strcatw( filePath, _WCH( "/" ) );

    if( !rpal_file_getInfow( storageDir, &fInfo ) )
    {
        rpal_debug_info( "Storage directory does not exist, creating it: %ls.", storageDir );

        if( !rDir_create( storageDir ) )
        {
            rpal_debug_warning( "Could not create storage directory." );
        }
    }

    for( relTypeId = 0; relTypeId < AAD_REL_MAX; relTypeId++ )
    {
        isStored = FALSE;

        if( NULL != ( rpal_string_itow( relTypeId, ( filePath + rpal_string_strlenw( storageDir ) + 1 ), 10 ) ) )
        {
            // We only persist phase 2 stashes since phase 1 is stateless
            if( aad_dumpStashToBuffer( g_stashes_phase_2[ relTypeId ], &fileBuffer, &fileSize ) )
            {
                if( rpal_file_writew( filePath, fileBuffer, fileSize, TRUE ) )
                {
                    isStored = TRUE;
                }
                else
                {
                    rpal_debug_warning( "Error writing stash to file: %ls.", filePath );
                }

                rpal_memory_free( fileBuffer );
            }
        }
    }

    return isStored;
}


static RBOOL
    loadDefaultParents
    (

    )
{
    RBOOL isLoaded = FALSE;

    RWCHAR process_1[] = _WCH( "services.exe" );
    RWCHAR process_2[] = _WCH( "smss.exe" );
    RWCHAR process_3[] = _WCH( "lsass.exe" );
    RWCHAR process_4[] = _WCH( "iexplore.exe" );
    RWCHAR process_5[] = _WCH( "firefox.exe" );
    RWCHAR process_6[] = _WCH( "chrome.exe" );
    RWCHAR module_1[] = _WCH( "ntdll.dll" );
    RWCHAR module_2[] = _WCH( "wininet.dll" );

    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_MODULE ], process_1, 100, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_MODULE ], process_2, 100, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_MODULE ], process_3, 100, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_MODULE ], process_4, 100, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_MODULE ], process_5, 100, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_MODULE ], process_6, 100, 0.01 );

    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_PATH ], process_1, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_PATH ], process_2, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_PATH ], process_3, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_PATH ], process_4, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_PATH ], process_5, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_PROCESS_PATH ], process_6, 10, 0.01 );

    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_PATH ], module_1, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_PATH ], module_2, 10, 0.01 );

    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], module_1, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], module_2, 10, 0.01 );

    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], module_1, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], module_2, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], process_1, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], process_2, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], process_3, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], process_4, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], process_5, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_MODULE_HASH ], process_6, 10, 0.01 );

    // Certain relations like AAD_REL_HASH_MODULE require a pre-population phase 1
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], module_1 );
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], module_2 );
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], process_1 );
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], process_2 );
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], process_3 );
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], process_4 );
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], process_5 );
    aad_enablePhase1Parent_WCH( g_stashes_phase_1[ AAD_REL_HASH_MODULE ], process_6 );

    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], module_1, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], module_2, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], process_1, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], process_2, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], process_3, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], process_4, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], process_5, 10, 0.01 );
    aad_enableParent_WCH( g_stashes_phase_2[ AAD_REL_HASH_MODULE ], process_6, 10, 0.01 );


    isLoaded = TRUE;

    return isLoaded;
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
    RU32 i = 0;

    rThread hThread_process_modules = NULL;

    FORCE_LINK_THAT( HCP_IFACE );

    g_timeToStopEvent = isTimeToStop;

    rpal_debug_info( "Initializing stashes." );
    for( i = 0; i < ARRAY_N_ELEM( g_stashes_phase_2 ); i++ )
    {
        g_stashes_phase_1[ i ] = aad_initStash_phase_1();
        g_stashes_phase_2[ i ] = aad_initStash_phase_2();
    }

    if( loadStashesFromDisk() )
    {
        rpal_debug_info( "Stashes loaded from disk." );
    }
    
    if( !loadDefaultParents() )
    {
        rpal_debug_error( "Failed to load defaults." );
    }

    rpal_debug_info( "Starting collection threads." );
    hThread_process_modules = rpal_thread_new( collection_process_modules, NULL );




    while( !rEvent_wait( isTimeToStop, (10*1000) ) )
    {
        
    }




    rpal_debug_info( "Shutting down collection threads." );
    rpal_thread_wait( hThread_process_modules, 1000 * 10 );
    rpal_thread_free( hThread_process_modules );

    rpal_debug_info( "Storing stashes to disk." );
    storeStashesToDisk();

    rpal_debug_info( "Freeing stashes." );
    for( i = 0; i < ARRAY_N_ELEM( g_stashes_phase_2 ); i++ )
    {
        rpal_btree_destroy( g_stashes_phase_1[ i ], FALSE );
        rpal_btree_destroy( g_stashes_phase_2[ i ], FALSE );
    }

    return ret;
}

