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
#include <processLib/processLib.h>
#include <rpHostCommonPlatformLib/rTags.h>

#ifdef RPAL_PLATFORM_DEBUG
#define _DEFAULT_TIME_DELTA     (60 * 60 * 1)
#else
#define _DEFAULT_TIME_DELTA     (60 * 5)
#endif

static
RBOOL
    isMemInModule
    (
        RU64 memBase,
        RU64 memSize,
        rList modules
    )
{
    RBOOL isInMod = FALSE;
    rSequence mod = NULL;

    RU64 modBase = 0;
    RU64 modSize = 0;

    if( NULL != modules )
    {
        rList_resetIterator( modules );

        while( rList_getSEQUENCE( modules, RP_TAGS_DLL, &mod ) )
        {
            if( rSequence_getPOINTER64( mod, RP_TAGS_BASE_ADDRESS, &modBase ) &&
                rSequence_getRU64( mod, RP_TAGS_MEMORY_SIZE, &modSize ) )
            {
                if( IS_WITHIN_BOUNDS( NUMBER_TO_PTR( memBase ), memSize, NUMBER_TO_PTR( modBase ), modSize ) )
                {
                    isInMod = TRUE;
                    break;
                }
            }
        }

        rList_resetIterator( modules );
    }

    return isInMod;
}


static
RPVOID
    lookForHiddenModules
    (
        rEvent isTimeToStop,
        RPVOID ctx
    )
{
    processLibProcEntry* procs = NULL;
    processLibProcEntry* proc = NULL;
    rList mods = NULL;
    rList map = NULL;
    rSequence region = NULL;
    RU8 memType = 0;
    RU8 memProtect = 0;
    RU64 memBase = 0;
    RU64 memSize = 0;

    RPU8 pMem = NULL;

    RBOOL isPrefetched = FALSE;
    RBOOL isCurrentExec = FALSE;
    RBOOL isHidden = FALSE;

    rSequence procInfo = NULL;

#ifdef RPAL_PLATFORM_WINDOWS
    PIMAGE_DOS_HEADER pDos = NULL;
    PIMAGE_NT_HEADERS pNt = NULL;
#endif

    UNREFERENCED_PARAMETER( ctx );

    if( NULL != ( procs = processLib_getProcessEntries( TRUE ) ) )
    {
        proc = procs;

        while( 0 != proc->pid &&
            rpal_memory_isValid( isTimeToStop ) &&
            !rEvent_wait( isTimeToStop, 0 ) )
        {
            rpal_thread_sleep( MSEC_FROM_SEC( 2 ) );
            if( NULL != ( mods = processLib_getProcessModules( proc->pid ) ) )
            {
                if( NULL != ( map = processLib_getProcessMemoryMap( proc->pid ) ) )
                {
                    // Now we got all the info needed for a single process, compare
                    while( rpal_memory_isValid( isTimeToStop ) &&
                        !rEvent_wait( isTimeToStop, 0 ) &&
                        ( isPrefetched || rList_getSEQUENCE( map, RP_TAGS_MEMORY_REGION, &region ) ) )
                    {
                        if( isPrefetched )
                        {
                            isPrefetched = FALSE;
                        }

                        if( rSequence_getRU8( region, RP_TAGS_MEMORY_TYPE, &memType ) &&
                            rSequence_getRU8( region, RP_TAGS_MEMORY_ACCESS, &memProtect ) &&
                            rSequence_getPOINTER64( region, RP_TAGS_BASE_ADDRESS, &memBase ) &&
                            rSequence_getRU64( region, RP_TAGS_MEMORY_SIZE, &memSize ) )
                        {
                            if( PROCESSLIB_MEM_TYPE_PRIVATE == memType ||
                                PROCESSLIB_MEM_TYPE_MAPPED == memType )
                            {
                                if( PROCESSLIB_MEM_ACCESS_EXECUTE == memProtect ||
                                    PROCESSLIB_MEM_ACCESS_EXECUTE_READ == memProtect ||
                                    PROCESSLIB_MEM_ACCESS_EXECUTE_READ_WRITE == memProtect ||
                                    PROCESSLIB_MEM_ACCESS_EXECUTE_WRITE_COPY == memProtect )
                                {
                                    isCurrentExec = TRUE;
                                }
                                else
                                {
                                    isCurrentExec = FALSE;
                                }

                                if( !isMemInModule( memBase, memSize, mods ) )
                                {
                                    // Exec memory found outside of a region marked to belong to
                                    // a module, keep looking in for module.
                                    if( ( 1024 * 1024 * 10 ) >= memSize &&
                                        processLib_getProcessMemory( proc->pid, NUMBER_TO_PTR( memBase ), memSize, (RPVOID*)&pMem ) )
                                    {
                                        isHidden = FALSE;
#ifdef RPAL_PLATFORM_WINDOWS
                                        // Let's just check for MZ and PE for now, we can get fancy later.
                                        pDos = (PIMAGE_DOS_HEADER)pMem;
                                        if( IS_WITHIN_BOUNDS( (RPU8)pMem, sizeof( IMAGE_DOS_HEADER ), pMem, memSize ) &&
                                            IMAGE_DOS_SIGNATURE == pDos->e_magic )
                                        {
                                            pNt = (PIMAGE_NT_HEADERS)( (RPU8)pDos + pDos->e_lfanew );

                                            if( IS_WITHIN_BOUNDS( pNt, sizeof( *pNt ), pMem, memSize ) &&
                                                IMAGE_NT_SIGNATURE == pNt->Signature )
                                            {
                                                if( isCurrentExec )
                                                {
                                                    // If the current region is exec, we've got a hidden module.
                                                    isHidden = TRUE;
                                                }
                                                else
                                                {
                                                    // We need to check if the next section in memory is
                                                    // executable and outside of known modules since the PE
                                                    // headers may have been marked read-only before the .text.
                                                    if( rList_getSEQUENCE( map, RP_TAGS_MEMORY_REGION, &region ) )
                                                    {
                                                        isPrefetched = TRUE;

                                                        if( ( PROCESSLIB_MEM_TYPE_PRIVATE == memType ||
                                                              PROCESSLIB_MEM_TYPE_MAPPED == memType ) &&
                                                            ( PROCESSLIB_MEM_ACCESS_EXECUTE == memProtect ||
                                                              PROCESSLIB_MEM_ACCESS_EXECUTE_READ == memProtect ||
                                                              PROCESSLIB_MEM_ACCESS_EXECUTE_READ_WRITE == memProtect ||
                                                              PROCESSLIB_MEM_ACCESS_EXECUTE_WRITE_COPY == memProtect ) )
                                                        {
                                                            isHidden = TRUE;
                                                        }
                                                    }
                                                }
                                            }
                                        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                                        if( isCurrentExec &&
                                            0x7F == ( pMem )[ 0 ] &&
                                            'E' == ( pMem )[ 1 ] &&
                                            'L' == ( pMem )[ 2 ] &&
                                            'F' == ( pMem )[ 3 ] )
                                        {
                                            isHidden = TRUE;
                                        }
#endif

                                        rpal_memory_free( pMem );

                                        if( isHidden &&
                                            !rEvent_wait( isTimeToStop, 0 ) )
                                        {
                                            rpal_debug_info( "found a hidden module in %d.", proc->pid );

                                            if( NULL != ( procInfo = processLib_getProcessInfo( proc->pid ) ) )
                                            {
                                                if( !rSequence_addSEQUENCE( region, RP_TAGS_PROCESS, procInfo ) )
                                                {
                                                    rSequence_free( procInfo );
                                                }
                                            }

                                            rSequence_addTIMESTAMP( region, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
                                            notifications_publish( RP_TAGS_NOTIFICATION_HIDDEN_MODULE_DETECTED, region );
                                            break;
                                        }

                                        rpal_thread_sleep( 10 );
                                    }
                                }
                            }
                        }
                    }

                    rList_free( map );
                }

                rList_free( mods );
            }

            proc++;
        }

        rpal_memory_free( procs );
    }

    return NULL;
}

RBOOL
    collector_5_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;
    RU64 timeDelta = 0;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( rpal_memory_isValid( config ) ||
            !rSequence_getTIMEDELTA( config, RP_TAGS_TIMEDELTA, &timeDelta ) )
        {
            timeDelta = _DEFAULT_TIME_DELTA;
        }

        if( rThreadPool_scheduleRecurring( hbsState->hThreadPool, timeDelta, lookForHiddenModules, NULL, TRUE ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    collector_5_cleanup
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
