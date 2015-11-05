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
#include <librpcm/librpcm.h>
#include "collectors.h"
#include <notificationsLib/notificationsLib.h>
#include <rpHostCommonPlatformLib/rTags.h>
#include <processLib/processLib.h>
#include <libOs/libOs.h>

#ifdef RPAL_PLATFORM_WINDOWS
    #include <windows_undocumented.h>
    #include <TlHelp32.h>
#elif defined( RPAL_PLATFORM_MACOSX )
    #include <sys/types.h>
    #include <sys/sysctl.h>
#endif

#define RPAL_FILE_ID       63

#define MAX_SNAPSHOT_SIZE 512

typedef struct
{
    RU32 pid;
    RU32 ppid;
} processEntry;

static processEntry g_snapshot_1[ MAX_SNAPSHOT_SIZE ] = { 0 };
static processEntry g_snapshot_2[ MAX_SNAPSHOT_SIZE ] = { 0 };


static RBOOL
    getSnapshot
    (
        processEntry* toSnapshot
    )
{
    RBOOL isSuccess = FALSE;
    RU32 i = 0;

    if( NULL != toSnapshot )
    {
        rpal_memory_zero( toSnapshot, sizeof( g_snapshot_1 ) );
    }

    if( NULL != toSnapshot )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        HANDLE hSnapshot = NULL;
        PROCESSENTRY32W procEntry = { 0 };
        procEntry.dwSize = sizeof( procEntry );

        if( INVALID_HANDLE_VALUE != ( hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ) ) )
        {
            if( Process32FirstW( hSnapshot, &procEntry ) )
            {
                isSuccess = TRUE;

                do
                {
                    if( 0 == procEntry.th32ProcessID )
                    {
                        continue;
                    }

                    toSnapshot[ i ].pid = procEntry.th32ProcessID;
                    toSnapshot[ i ].ppid = procEntry.th32ParentProcessID;
                    i++;
                } while( Process32NextW( hSnapshot, &procEntry ) &&
                         MAX_SNAPSHOT_SIZE > i );
            }

            CloseHandle( hSnapshot );
        }
#elif defined( RPAL_PLATFORM_LINUX )
        RWCHAR procDir[] = _WCH( "/proc/" );
        rDir hProcDir = NULL;
        rFileInfo finfo = {0};

        if( rDir_open( (RPWCHAR)&procDir, &hProcDir ) )
        {
            isSuccess = TRUE;

            while( rDir_next( hProcDir, &finfo ) &&
                   MAX_SNAPSHOT_SIZE > i )
            {
                if( rpal_string_wtoi( (RPWCHAR)finfo.fileName, &( toSnapshot[ i ].pid ) )
                    && 0 != toSnapshot[ i ].pid )
                {
                    i++;
                }
            }

            rDir_close( hProcDir );
        }
#elif defined( RPAL_PLATFORM_MACOSX )
        int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
        struct kinfo_proc* infos = NULL;
        size_t size = 0;
        int ret = 0;
        int j = 0;

        if( 0 == ( ret = sysctl( mib, ARRAY_N_ELEM( mib ), infos, &size, NULL, 0 ) ) )
        {
            if( NULL != ( infos = rpal_memory_alloc( size ) ) )
            {
                while( 0 != ( ret = sysctl( mib, ARRAY_N_ELEM( mib ), infos, &size, NULL, 0 ) ) && ENOMEM == errno )
                {
                    if( NULL == ( infos = rpal_memory_realloc( infos, size ) ) )
                    {
                        break;
                    }
                }
            }
        }

        if( 0 == ret && NULL != infos )
        {
            isSuccess = TRUE;
            size = size / sizeof( struct kinfo_proc );
            for( j = 0; j < size; j++ )
            {
                toSnapshot[ i ].pid = infos[ j ].kp_proc.p_pid;
                toSnapshot[ i ].ppid = infos[ j ].kp_eproc.e_ppid;
                i++;
            }

            if( NULL != infos )
            {
                rpal_memory_free( infos );
                infos = NULL;
            }
        }
#endif
    }

    return isSuccess;
}

static RBOOL
    notifyOfProcess
    (
        RU32 pid,
        RU32 ppid,
        RBOOL isStarting
    )
{
    RBOOL isSuccess = FALSE;
    rSequence info = NULL;
    rSequence parentInfo = NULL;

    if( !isStarting ||
        NULL == ( info = processLib_getProcessInfo( pid ) ) )
    {
        info = rSequence_new();
    }

    if( rpal_memory_isValid( info ) )
    {
        rSequence_addRU32( info, RP_TAGS_PROCESS_ID, pid );
        rSequence_addRU32( info, RP_TAGS_PARENT_PROCESS_ID, ppid );
        rSequence_addTIMESTAMP( info, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );

        if( isStarting )
        {
            if( NULL != ( parentInfo = processLib_getProcessInfo( ppid ) ) &&
                !rSequence_addSEQUENCE( info, RP_TAGS_PARENT, parentInfo ) )
            {
                rSequence_free( parentInfo );
            }
        }

        if( isStarting )
        {
            if( notifications_publish( RP_TAGS_NOTIFICATION_NEW_PROCESS, info ) )
            {
                isSuccess = TRUE;
                rpal_debug_info( "new process starting: %d", pid );
            }
        }
        else
        {
            if( notifications_publish( RP_TAGS_NOTIFICATION_TERMINATE_PROCESS, info ) )
            {
                isSuccess = TRUE;
                rpal_debug_info( "new process terminating: %d", pid );
            }
        }

        rSequence_free( info );
    }

    return isSuccess;
}

static RPVOID
    processDiffThread
    (
        rEvent isTimeToStop,
        RPVOID ctx
    )
{
    processEntry* currentSnapshot = g_snapshot_1;
    processEntry* previousSnapshot = g_snapshot_2;
    processEntry* tmpSnapshot = NULL;
    RBOOL isFirstSnapshots = TRUE;
    RU32 i = 0;
    RU32 j = 0;
    RBOOL isFound = FALSE;
    RU32 nThLoop = 0;
    RU32 currentTimeout = 0;
    UNREFERENCED_PARAMETER( ctx );

    while( !rEvent_wait( isTimeToStop, currentTimeout ) )
    {
        tmpSnapshot = currentSnapshot;
        currentSnapshot = previousSnapshot;
        previousSnapshot = tmpSnapshot;

        if( getSnapshot( currentSnapshot ) )
        {
            if( isFirstSnapshots )
            {
                isFirstSnapshots = FALSE;
                continue;
            }

            // Diff to find new processes
            for( i = 0; i < MAX_SNAPSHOT_SIZE; i++ )
            {
                isFound = FALSE;

                if( 0 == currentSnapshot[ i ].pid )
                {
                    break;
                }

                for( j = 0; j < MAX_SNAPSHOT_SIZE; j++ )
                {
                    if( 0 == previousSnapshot[ j ].pid )
                    {
                        break;
                    }

                    if( previousSnapshot[ j ].pid == currentSnapshot[ i ].pid )
                    {
                        isFound = TRUE;
                        break;
                    }
                }

                if( !isFound )
                {
                    if( !notifyOfProcess( currentSnapshot[ i ].pid, 
                                          currentSnapshot[ i ].ppid,
                                          TRUE ) )
                    {
                        rpal_debug_warning( "error reporting new process: %d", 
                                            currentSnapshot[ i ].pid );
                    }
                }
            }

            // Diff to find terminated processes
            for( i = 0; i < MAX_SNAPSHOT_SIZE; i++ )
            {
                isFound = FALSE;

                if( 0 == previousSnapshot[ i ].pid )
                {
                    break;
                }

                for( j = 0; j < MAX_SNAPSHOT_SIZE; j++ )
                {
                    if( 0 == currentSnapshot[ j ].pid )
                    {
                        break;
                    }

                    if( previousSnapshot[ i ].pid == currentSnapshot[ j ].pid )
                    {
                        isFound = TRUE;
                        break;
                    }
                }

                if( !isFound )
                {
                    if( !notifyOfProcess( previousSnapshot[ i ].pid,
                                          previousSnapshot[ i ].ppid,
                                          FALSE ) )
                    {
                        rpal_debug_warning( "error reporting terminated process: %d",
                                            previousSnapshot[ i ].pid );
                    }
                }
            }
        }

        nThLoop++;
        if( 0 == nThLoop % 20 )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            // The Windows API is much more efficient than on Nix so we can affort
            // going faster between our diffs.
            currentTimeout = libOs_getUsageProportionalTimeout( 500 ) + 100;
#else
            currentTimeout = libOs_getUsageProportionalTimeout( 800 ) + 200;
#endif
        }
    }

    return NULL;
}


RBOOL
    collector_1_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;
    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( rThreadPool_task( hbsState->hThreadPool, processDiffThread, NULL ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    collector_1_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != hbsState &&
        rpal_memory_isValid( config ) )
    {
        isSuccess = TRUE;
    }

    return isSuccess;
}