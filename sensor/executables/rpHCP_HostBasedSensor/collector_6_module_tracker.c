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

#ifdef RPAL_PLATFORM_WINDOWS
#include <windows_undocumented.h>
#include <TlHelp32.h>
#endif

#define RPAL_FILE_ID         66


typedef struct
{
    RU64 fuzz; // This is a predictable hash-ish value used to balance the btree naturally
    RU32 procId;
    RU64 baseAddr;
    RU32 size;
} _moduleHistEntry;

static
RS32
    _isModule
    (
        _moduleHistEntry* m1,
        _moduleHistEntry* m2
    )
{
    RS32 ret = 0;

    if( NULL != m1 &&
        NULL != m2 )
    {
        ret = (RS32)rpal_memory_memcmp( m1, m2, sizeof( *m1 ) );
    }

    return ret;
}



#ifdef RPAL_PLATFORM_WINDOWS
static
RVOID
    _notifyOfNewModule
    (
        MODULEENTRY32W* entry
    )
{
    rSequence notif = NULL;

    if( NULL != entry &&
        NULL != ( notif = rSequence_new() ) )
    {
        if( rSequence_addRU32( notif, RP_TAGS_PROCESS_ID, entry->th32ProcessID ) &&
            rSequence_addSTRINGW( notif, RP_TAGS_MODULE_NAME, entry->szModule ) &&
            rSequence_addSTRINGW( notif, RP_TAGS_FILE_PATH, entry->szExePath ) &&
            rSequence_addPOINTER64( notif, RP_TAGS_BASE_ADDRESS, (RU64)entry->modBaseAddr ) &&
            rSequence_addRU64( notif, RP_TAGS_MEMORY_SIZE, entry->modBaseSize ) &&
            rSequence_addTIMESTAMP( notif, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() ) )
        {
            notifications_publish( RP_TAGS_NOTIFICATION_MODULE_LOAD, notif );
        }

        rSequence_free( notif );
    }
}
#endif

static
RPVOID
    diffProcessModules
    (
        rEvent isTimeToStop,
        RPVOID ctx
    )
{
    RBOOL isFirstRun = TRUE;
    rBTree previousSnapshot = NULL;
    rBTree newSnapshot = NULL;
    _moduleHistEntry curModule = { 0 };

#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE hSnapshotProc = NULL;
    HANDLE hSnapshotMod = NULL;
    MODULEENTRY32W modEntry = { 0 };
    PROCESSENTRY32W procEntry = { 0 };
    modEntry.dwSize = sizeof( modEntry );
    procEntry.dwSize = sizeof( procEntry );
#endif

    UNREFERENCED_PARAMETER( ctx );

    while( rpal_memory_isValid( isTimeToStop ) &&
          !rEvent_wait( isTimeToStop, 0 ) )
    {
        newSnapshot = rpal_btree_create( sizeof( curModule ), (rpal_btree_comp_f)_isModule, NULL );

#ifdef RPAL_PLATFORM_WINDOWS
        if( INVALID_HANDLE_VALUE != ( hSnapshotProc = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ) ) )
        {
            if( Process32FirstW( hSnapshotProc, &procEntry ) )
            {
                do
                {
                    if( 0 == procEntry.th32ProcessID )
                    {
                        continue;
                    }

                    curModule.procId = procEntry.th32ProcessID;

                    if( INVALID_HANDLE_VALUE != ( hSnapshotMod = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE,
                        procEntry.th32ProcessID ) ) )
                    {
                        if( Module32FirstW( hSnapshotMod, &modEntry ) )
                        {
                            do
                            {
                                curModule.baseAddr = (RU64)modEntry.modBaseAddr;
                                curModule.size = modEntry.modBaseSize;
                                curModule.fuzz = curModule.baseAddr | ( (RU64)( curModule.procId ^ curModule.size ) << 32 );

                                if( !isFirstRun )
                                {
                                    if( !rpal_btree_search( previousSnapshot, &curModule, NULL, TRUE ) )
                                    {
                                        _notifyOfNewModule( &modEntry );
                                    }
                                }

                                rpal_btree_add( newSnapshot, &curModule, TRUE );
                            } while( rpal_memory_isValid( isTimeToStop ) &&
                                     !rEvent_wait( isTimeToStop, 20 ) &&
                                     Module32NextW( hSnapshotMod, &modEntry ) );
                        }

                        CloseHandle( hSnapshotMod );
                    }
                } while( Process32NextW( hSnapshotProc, &procEntry ) );
            }

            CloseHandle( hSnapshotProc );
        }
#endif

        if( NULL != previousSnapshot )
        {
            rpal_btree_destroy( previousSnapshot, TRUE );
        }

        previousSnapshot = newSnapshot;
        newSnapshot = NULL;

        if( isFirstRun )
        {
            isFirstRun = FALSE;
        }
    }

    rpal_btree_destroy( previousSnapshot, TRUE );

    return NULL;
}

RBOOL
    collector_6_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( rThreadPool_task( hbsState->hThreadPool, diffProcessModules, NULL ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    collector_6_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        isSuccess = TRUE;
    }

    return isSuccess;
}
