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

#define RPAL_FILE_ID                            65

static
RVOID
    _getStringsList
    (
        rList stringsAList,
        rList stringsWList,
        RPU8 pBuff,
        RU64 size,
        RU32 minLength,
        RU32 maxLength
    )
{
    RPU8 pCurr;
    RPU8 pEnd;
    RPCHAR pStartStr = NULL;
    RBOOL isChar;
    RPU16 pwCurr;
    RPU16 pwEnd;
    RBOOL isWChar;
    RPWCHAR pwStartStr = NULL;

    pCurr = pBuff;
    pEnd = pBuff + size;

    // currently we only deal with NULL terminated strings
    // start with ascii strings...
    while( pCurr < pEnd )
    {
        isChar = rpal_string_isprint( *pCurr );

        if( NULL == pStartStr && isChar )  // found the begining of a string
        {
            pStartStr = (RPCHAR)pCurr;
        }
        else if( NULL != pStartStr && ( !isChar || 0 == *pCurr ) ) // found the end of a string
        {
            // is string NULL terminated
            if( 0 == *pCurr )
            {
                // strlen is really pCurr - pStartStr
                if( (RU32)( (RPCHAR)pCurr - pStartStr ) >= minLength &&
                    (RU32)( (RPCHAR)pCurr - pStartStr ) <= maxLength ) // is string long enough
                {
                    rList_addSTRINGA( stringsAList, pStartStr );
                }
            }
            pStartStr = NULL;
        }
        pCurr++;
    }
    // Now look for Unicode strings
    pwCurr = (RPU16)pBuff;
    pwEnd = pwCurr + ( size / 2 );
    while( pwCurr < pwEnd )
    {
        isWChar = rpal_string_iswprint( *pwCurr );

        if( NULL == pwStartStr && isWChar )  // found the begining of a string
        {
            pwStartStr = (RPWCHAR)pwCurr;
        }
        else if( NULL != pwStartStr && ( !isWChar || 0 == *pwCurr ) ) // found the end of a string
        {
            // is string NULL terminated
            if( 0 == *pwCurr )
            {
                // wcslen is really pCurr - pStartStr
                if( (RU32)( (RPWCHAR)pwCurr - pwStartStr ) >= minLength &&
                    (RU32)( (RPWCHAR)pwCurr - pwStartStr ) <= maxLength ) // is string long enough
                {
                    rList_addSTRINGW( stringsWList, pwStartStr );
                }
            }
            pwStartStr = NULL;
        }
        pwCurr++;
    }
}

static
RBOOL
    _isStringInList
    (
        rList searchStrings,
        RPWCHAR str
    )
{
    RBOOL found = FALSE;
    RPWCHAR strVal;

    if( NULL != searchStrings && NULL != str )
    {
        rList_resetIterator( searchStrings );
        while( rList_getSTRINGW( searchStrings, RP_TAGS_STRING, &strVal ) )
        {
            if( rpal_string_matchw( strVal, str ) )
            {
                found = TRUE;
                break;
            }
        }
    }

    return found;
}

static
RVOID
    _searchForStrings
    (
        rList stringsFound,
        rList searchStrings,
        RPU8 pBuff,
        RU64 size,
        RU64 baseAddr,
        RU32 minLength,
        RU32 maxLength
    )
{
    RPU8 pCurr;
    RPU8 pEnd;
    RPCHAR pStartStr = NULL;
    RBOOL isChar;
    RPU16 pwCurr;
    RPU16 pwEnd;
    RBOOL isWChar;
    RPWCHAR pwStartStr = NULL;
    RPWCHAR thisStrW = NULL;
    rSequence newFoundStr;

    pCurr = pBuff;
    pEnd = pBuff + size;

    // currently we only deal with NULL terminated strings
    // start with ascii strings...
    while( pCurr < pEnd )
    {
        isChar = rpal_string_isprint( *pCurr );

        if( NULL == pStartStr && isChar )  // found the begining of a string
        {
            pStartStr = (RPCHAR)pCurr;
        }
        else if( NULL != pStartStr && ( !isChar || 0 == *pCurr ) ) // found the end of a string
        {
            // is string NULL or Non-Ascii terminated
            if( 0 == *pCurr || !rpal_string_charIsAscii( *pCurr ) )
            {
                // Null terminate it so we can use it like a normal string
                *pCurr = 0;

                // strlen is really pCurr - pStartStr
                if( (RU32)( (RPCHAR)pCurr - pStartStr ) >= minLength &&
                    (RU32)( (RPCHAR)pCurr - pStartStr ) <= maxLength ) // is string long enough
                {
                    // convert string to wide char for comparision
                    if( NULL != ( thisStrW = rpal_string_atow( pStartStr ) ) )
                    {
                        if( _isStringInList( searchStrings, thisStrW ) && NULL != ( newFoundStr = rSequence_new() ) )
                        {
                            rSequence_addSTRINGW( newFoundStr, RP_TAGS_STRING, thisStrW );
                            rSequence_addRU64( newFoundStr, RP_TAGS_MEMORY_ADDRESS, baseAddr + ( (RPU8)pStartStr - pBuff ) );

                            if( !rList_addSEQUENCE( stringsFound, newFoundStr ) )
                            {
                                rSequence_free( newFoundStr );
                            }
                        }
                        rpal_memory_free( thisStrW );
                    }
                }
            }
            pStartStr = NULL;
        }
        pCurr++;
    }
    // Now look for Unicode strings
    pwCurr = (RPU16)pBuff;
    pwEnd = pwCurr + ( size / 2 );
    while( pwCurr < pwEnd )
    {
        isWChar = rpal_string_iswprint( *pwCurr );

        if( NULL == pwStartStr && isWChar )  // found the begining of a string
        {
            pwStartStr = (RPWCHAR)pwCurr;
        }
        else if( NULL != pwStartStr && ( !isWChar || 0 == *pwCurr ) ) // found the end of a string
        {
            // is string NULL terminated
            if( 0 == *pwCurr )
            {
                // wcslen is really pCurr - pStartStr
                if( (RU32)( (RPWCHAR)pwCurr - pwStartStr ) >= minLength &&
                    (RU32)( (RPWCHAR)pwCurr - pwStartStr ) <= maxLength ) // is string long enough
                {

                    if( _isStringInList( searchStrings, pwStartStr ) && NULL != ( newFoundStr = rSequence_new() ) )
                    {
                        rSequence_addSTRINGW( newFoundStr, RP_TAGS_STRING, pwStartStr );
                        rSequence_addRU64( newFoundStr, RP_TAGS_MEMORY_ADDRESS, baseAddr + ( (RPU8)pwStartStr - pBuff ) );

                        if( !rList_addSEQUENCE( stringsFound, newFoundStr ) )
                        {
                            rSequence_free( newFoundStr );
                        }
                    }
                }
            }
            pwStartStr = NULL;
        }
        pwCurr++;
    }
}


static
rSequence
    _findStringsInProcess
    (
        RU32 pid,
        rList searchStrings,
        RU32 minLength,
        RU32 maxLength
    )
{
    rSequence info = NULL;

    rList memMapList = NULL;
    rSequence region = NULL;
    RU64 memBase = 0;
    RU64 memSize = 0;
    RPU8 pRegion = NULL;
    rList stringsFound = NULL;

    if( NULL != searchStrings )
    {
        if( NULL != ( info = rSequence_new() ) )
        {
            rSequence_addRU32( info, RP_TAGS_PROCESS_ID, pid );

            if( NULL != ( memMapList = processLib_getProcessMemoryMap( pid ) ) &&
                ( NULL != ( stringsFound = rList_new( RP_TAGS_STRINGSW, RPCM_SEQUENCE ) ) ) )
            {
                while( rList_getSEQUENCE( memMapList, RP_TAGS_MEMORY_REGION, &region ) )
                {
                    if( rSequence_getPOINTER64( region, RP_TAGS_BASE_ADDRESS, &memBase ) &&
                        rSequence_getRU64( region, RP_TAGS_MEMORY_SIZE, &memSize ) )
                    {
                        if( processLib_getProcessMemory( pid, (RPVOID)rpal_ULongToPtr( memBase ), memSize, (RPVOID*)&pRegion ) )
                        {
                            // now search for strings inside this region
                            _searchForStrings( stringsFound, searchStrings, pRegion, memSize, memBase, minLength, maxLength);

                            rpal_memory_free( pRegion );
                        }
                    }
                }

                if( !rSequence_addLIST( info, RP_TAGS_STRINGS_FOUND, stringsFound ) )
                {
                    rList_free( stringsFound );
                }
            }
            else
            {
                rSequence_addRU32( info, RP_TAGS_ERROR, rpal_error_getLast() );
            }

            if( NULL != memMapList )
            {
                rList_free( memMapList );
            }
        }
    }

    return info;
}


static
RVOID
    mem_map
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    RU32 pid;
    rList memMapList = NULL;
    rList modulesList = NULL;
    rSequence modEntry = NULL;
    rSequence memEntry = NULL;

    RPWCHAR tmpModName = NULL;
    RPWCHAR tmpModPath = NULL;
    RU64 memStart = 0;
    RU64 memSize = 0;
    RU64 modStart = 0;
    RU64 modSize = 0;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getRU32( event, RP_TAGS_PROCESS_ID, &pid ) )
        {
            if( NULL != ( memMapList = processLib_getProcessMemoryMap( pid ) ) )
            {
                // Try to enhance the raw map
                if( NULL != ( modulesList = processLib_getProcessModules( pid ) ) )
                {
                    // Looking for memory pages within the known module
                    rList_resetIterator( memMapList );
                    while( rList_getSEQUENCE( memMapList, RP_TAGS_MEMORY_REGION, &memEntry ) )
                    {
                        if( rSequence_getPOINTER64( memEntry, RP_TAGS_BASE_ADDRESS, &memStart ) &&
                            rSequence_getRU64( memEntry, RP_TAGS_MEMORY_SIZE, &memSize ) )
                        {
                            tmpModName = NULL;
                            tmpModPath = NULL;

                            rList_resetIterator( modulesList );
                            while( rList_getSEQUENCE( modulesList, RP_TAGS_DLL, &modEntry ) )
                            {
                                if( rSequence_getPOINTER64( modEntry, RP_TAGS_BASE_ADDRESS, &modStart ) &&
                                    rSequence_getRU64( modEntry, RP_TAGS_MEMORY_SIZE, &modSize ) )
                                {
                                    if( memStart >= modStart && memStart <= ( modStart + modSize ) )
                                    {
                                        // Match, we get just the basic info
                                        rSequence_getSTRINGW( modEntry, RP_TAGS_MODULE_NAME, &tmpModName );
                                        rSequence_getSTRINGW( modEntry, RP_TAGS_FILE_PATH, &tmpModPath );
                                        break;
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }

                            // I can assert that the strings read from the memEntry WILL NOT be used
                            // hereon since doing so would be dangerous as I am about to modify
                            // the memEntry sequence after the read and therefore those pointers
                            // may not be good anymore after this point.
                            rSequence_unTaintRead( memEntry );

                            if( NULL != tmpModName )
                            {
                                rSequence_addSTRINGW( memEntry, RP_TAGS_MODULE_NAME, tmpModName );
                            }
                            if( NULL != tmpModPath )
                            {
                                rSequence_addSTRINGW( memEntry, RP_TAGS_FILE_PATH, tmpModPath );
                            }
                        }
                    }

                    rList_resetIterator( memMapList );

                    rList_free( modulesList );
                }

                if( !rSequence_addLIST( event, RP_TAGS_MEMORY_MAP, memMapList ) )
                {
                    rList_free( memMapList );
                }
            }
            else
            {
                rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
            }
        }
    }

    rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
    notifications_publish( RP_TAGS_NOTIFICATION_MEM_MAP_REP, event );
}

static
RVOID
    mem_read
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    RU32 pid;
    RU64 baseAddr;
    RU32 memSize;
    RPVOID mem;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getRU32( event, RP_TAGS_PROCESS_ID, &pid ) &&
            rSequence_getRU64( event, RP_TAGS_BASE_ADDRESS, &baseAddr ) &&
            rSequence_getRU32( event, RP_TAGS_MEMORY_SIZE, &memSize ) )
        {
            if( processLib_getProcessMemory( pid, (RPVOID)rpal_ULongToPtr( baseAddr ), memSize, &mem ) )
            {
                rSequence_addBUFFER( event, RP_TAGS_MEMORY_DUMP, (RPU8)mem, memSize );
                rpal_memory_free( mem );
            }
            else
            {
                rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
                rpal_debug_error( "failed to get memory (base address = 0x%llx, size = 0x%x ) for pid 0x%x.", baseAddr, memSize, pid );
            }
        }

        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
        notifications_publish( RP_TAGS_NOTIFICATION_MEM_READ_REP, event );
    }
}


static
RVOID
    mem_handles
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    RU32 pid;
    rList handleList;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getRU32( event, RP_TAGS_PROCESS_ID, &pid ) )
        {
            if( NULL != ( handleList = processLib_getHandles( pid, TRUE, NULL ) ) )
            {
                if( !rSequence_addLIST( event, RP_TAGS_HANDLES, handleList ) )
                {
                    rList_free( handleList );
                }
            }
            else
            {
                rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
            }
        }

        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
        notifications_publish( RP_TAGS_NOTIFICATION_MEM_HANDLES_REP, event );
    }
}

static
RVOID
    mem_find_handle
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    RPWCHAR needle = NULL;
    rList handleList;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getSTRINGW( event, RP_TAGS_HANDLE_NAME, &needle ) )
        {
            rSequence_unTaintRead( event );

            if( NULL != ( handleList = processLib_getHandles( 0, TRUE, needle ) ) )
            {
                if( !rSequence_addLIST( event, RP_TAGS_HANDLES, handleList ) )
                {
                    rList_free( handleList );
                }
            }
            else
            {
                rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
                rpal_debug_error( "failed to get handles for pid 0x%x.", 0 );
            }
        }

        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
        notifications_publish( RP_TAGS_NOTIFICATION_MEM_FIND_HANDLE_REP, event );
    }
}

static
RVOID
    mem_strings
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    RU32 pid = 0;
    rList memMapList = NULL;
    rSequence region = NULL;
    RU64 memBase = 0;
    RU64 memSize = 0;
    RPU8 pRegion = NULL;
    rList stringsAList = NULL;
    rList stringsWList = NULL;
    RU32 minLength = 5;
    RU32 maxLength = 128;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getRU32( event, RP_TAGS_PROCESS_ID, &pid ) )
        {
            if( NULL != ( memMapList = processLib_getProcessMemoryMap( pid ) ) &&
                ( NULL != ( stringsAList = rList_new( RP_TAGS_STRINGSA, RPCM_STRINGA ) ) ) &&
                ( NULL != ( stringsWList = rList_new( RP_TAGS_STRINGSW, RPCM_STRINGW ) ) ) )
            {
                while( rList_getSEQUENCE( memMapList, RP_TAGS_MEMORY_REGION, &region ) )
                {
                    if( rSequence_getPOINTER64( region, RP_TAGS_BASE_ADDRESS, &memBase ) &&
                        rSequence_getRU64( region, RP_TAGS_MEMORY_SIZE, &memSize ) )
                    {
                        if( processLib_getProcessMemory( pid, (RPVOID)rpal_ULongToPtr( memBase ), memSize, (RPVOID*)&pRegion ) )
                        {
                            // now search for strings inside this region
                            _getStringsList( stringsAList, stringsWList, pRegion, memSize, minLength, maxLength );

                            rpal_memory_free( pRegion );
                        }
                    }
                }

                if( !rSequence_addLIST( event, RP_TAGS_STRINGSA, stringsAList ) )
                {
                    rList_free( stringsAList );
                }
                if( !rSequence_addLIST( event, RP_TAGS_STRINGSW, stringsWList ) )
                {
                    rList_free( stringsWList );
                }
            }
            else
            {
                rSequence_addRU32( event, RP_TAGS_ERROR, rpal_error_getLast() );
            }

            if( NULL != memMapList )
            {
                rList_free( memMapList );
            }
        }

        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
        notifications_publish( RP_TAGS_NOTIFICATION_MEM_STRINGS_REP, event );
    }
}


static
RVOID
    mem_find_string
    (
        rpcm_tag eventType,
        rSequence event
    )
{
    RU32 pid = 0;
    RU32 currentPid = 0;
    rList searchStrings = NULL;
    rList processes = NULL;
    rSequence process = NULL;
    processLibProcEntry* pids = NULL;
    RU32 nCurrent = 0;
    RU32 minLength = 5;
    RU32 maxLength = 128;

    UNREFERENCED_PARAMETER( eventType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getRU32( event, RP_TAGS_PROCESS_ID, &pid ) && 
            rSequence_getLIST( event, RP_TAGS_STRINGSW, &searchStrings ) )
        {
            currentPid = processLib_getCurrentPid();

            if( NULL != ( processes = rList_new( RP_TAGS_PROCESS, RPCM_SEQUENCE ) ) )
            {
                if( 0 != pid )
                {
                    if( NULL != ( process = _findStringsInProcess( pid, 
                                                                   searchStrings, 
                                                                   minLength, 
                                                                   maxLength ) ) )
                    {
                        if( !rList_addSEQUENCE( processes, process ) )
                        {
                            rSequence_free( process );
                        }
                    }
                }
                else
                {
                    if( NULL != ( pids = processLib_getProcessEntries( TRUE ) ) )
                    {
                        while( 0 != pids[ nCurrent ].pid )
                        {
                            if( currentPid != pids[ nCurrent ].pid )
                            {
                                if( NULL != ( process = _findStringsInProcess( pids[ nCurrent ].pid, 
                                                                               searchStrings, 
                                                                               minLength, 
                                                                               maxLength ) ) )
                                {
                                    if( !rList_addSEQUENCE( processes, process ) )
                                    {
                                        rSequence_free( process );
                                    }
                                }
                            }

                            nCurrent++;
                        }

                        rpal_memory_free( pids );
                    }
                }

                if( !rSequence_addLIST( event, RP_TAGS_PROCESSES, processes ) )
                {
                    rList_free( processes );
                }
            }
        }

        rSequence_addTIMESTAMP( event, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );
        notifications_publish( RP_TAGS_NOTIFICATION_MEM_FIND_STRING_REP, event );
    }
}

RBOOL
    collector_10_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( notifications_subscribe( RP_TAGS_NOTIFICATION_MEM_MAP_REQ, NULL, 0, NULL, mem_map ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_MEM_READ_REQ, NULL, 0, NULL, mem_read ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_MEM_HANDLES_REQ, NULL, 0, NULL, mem_handles ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_MEM_FIND_HANDLE_REQ, NULL, 0, NULL, mem_find_handle ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_MEM_STRINGS_REQ, NULL, 0, NULL, mem_strings ) &&
            notifications_subscribe( RP_TAGS_NOTIFICATION_MEM_FIND_STRING_REQ, NULL, 0, NULL, mem_find_string ) )
        {
            isSuccess = TRUE;
        }
        else
        {
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_MAP_REQ, NULL, mem_map );
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_READ_REQ, NULL, mem_read );
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_HANDLES_REQ, NULL, mem_handles );
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_FIND_HANDLE_REQ, NULL, mem_find_handle );
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_STRINGS_REQ, NULL, mem_strings );
        }
    }

    return isSuccess;
}

RBOOL
    collector_10_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_MAP_REQ, NULL, mem_map );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_READ_REQ, NULL, mem_read );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_HANDLES_REQ, NULL, mem_handles );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_FIND_HANDLE_REQ, NULL, mem_find_handle );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_STRINGS_REQ, NULL, mem_strings );
        notifications_unsubscribe( RP_TAGS_NOTIFICATION_MEM_FIND_STRING_REQ, NULL, mem_find_string );

        isSuccess = TRUE;
    }

    return isSuccess;
}
