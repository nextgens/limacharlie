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
#include <cryptoLib/cryptoLib.h>

#define RPAL_FILE_ID 72

typedef struct
{
    RU8 nameHash[ CRYPTOLIB_HASH_SIZE ];
    RU8 fileHash[ CRYPTOLIB_HASH_SIZE ];
    RU64 codeSize;

} CodeIdent;

static rBloom knownCode = NULL;


static
RVOID
    processCodeIdentW
    (
        RPWCHAR name,
        RPU8 pFileHash,
        RU64 codeSize
    )
{
    CodeIdent ident = { 0 };
    rSequence notif = NULL;
    
    ident.codeSize = codeSize;

    if( NULL != name )
    {
        CryptoLib_hash( name, rpal_string_strlenw( name ) * sizeof( RWCHAR ), ident.nameHash );
    }

    if( NULL != pFileHash )
    {
        rpal_memory_memcpy( ident.fileHash, pFileHash, CRYPTOLIB_HASH_SIZE );
    }

    if( rpal_bloom_addIfNew( knownCode, &ident, sizeof( ident ) ) )
    {
        if( NULL != ( notif = rSequence_new() ) )
        {
            if( rSequence_addSTRINGW( notif, RP_TAGS_FILE_PATH, name ) &&
                rSequence_addBUFFER( notif, RP_TAGS_HASH, pFileHash, CRYPTOLIB_HASH_SIZE ) &&
                rSequence_addRU32( notif, RP_TAGS_MEMORY_SIZE, (RU32)codeSize ) &&
                rSequence_addTIMESTAMP( notif, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() ) )
            {
                notifications_publish( RP_TAGS_NOTIFICATION_CODE_IDENTITY, notif );
            }
            rSequence_free( notif );
        }
    }
}

static
RVOID
    processCodeIdentA
    (
        RPCHAR name,
        RPU8 pFileHash,
        RU64 codeSize
    )
{
    CodeIdent ident = { 0 };
    rSequence notif = NULL;

    ident.codeSize = codeSize;

    if( NULL != name )
    {
        CryptoLib_hash( name, rpal_string_strlen( name ) * sizeof( RCHAR ), ident.nameHash );
    }

    if( NULL != pFileHash )
    {
        rpal_memory_memcpy( ident.fileHash, pFileHash, CRYPTOLIB_HASH_SIZE );
    }

    if( rpal_bloom_addIfNew( knownCode, &ident, sizeof( ident ) ) )
    {
        if( NULL != ( notif = rSequence_new() ) )
        {
            if( rSequence_addSTRINGA( notif, RP_TAGS_FILE_NAME, name ) &&
                rSequence_addBUFFER( notif, RP_TAGS_HASH, pFileHash, CRYPTOLIB_HASH_SIZE ) &&
                rSequence_addRU32( notif, RP_TAGS_MEMORY_SIZE, (RU32)codeSize ) &&
                rSequence_addTIMESTAMP( notif, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() ) )
            {
                notifications_publish( RP_TAGS_NOTIFICATION_CODE_IDENTITY, notif );
            }
            rSequence_free( notif );
        }
    }
}

static
RVOID
    processNewProcesses
    (
        rpcm_tag notifType,
        rSequence event
    )
{
    RPWCHAR nameW = NULL;
    RPCHAR nameA = NULL;
    RU8 fileHash[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    RU64 size = 0;

    UNREFERENCED_PARAMETER( notifType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getSTRINGA( event, RP_TAGS_FILE_PATH, &nameA ) ||
            rSequence_getSTRINGW( event, RP_TAGS_FILE_PATH, &nameW ) )
        {
            if( NULL != nameA &&
                !CryptoLib_hashFileA( nameA, fileHash, TRUE ) )
            {
                rpal_debug_info( "unable to fetch file hash for ident" );
            }
            
            if( NULL != nameW &&
                !CryptoLib_hashFileW( nameW, fileHash, TRUE ) )
            {
                rpal_debug_info( "unable to fetch file hash for ident" );
            }

            rSequence_getRU64( event, RP_TAGS_MEMORY_SIZE, &size );

            if( NULL != nameA )
            {
                processCodeIdentA( nameA, fileHash, size );
            }
            else if( NULL != nameW )
            {
                processCodeIdentW( nameW, fileHash, size );
            }
        }
    }
}


static
RVOID
    processNewModule
    (
        rpcm_tag notifType,
        rSequence event
    )
{
    RPWCHAR nameW = NULL;
    RPCHAR nameA = NULL;
    RU8 fileHash[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    RU64 size = 0;

    UNREFERENCED_PARAMETER( notifType );

    if( rpal_memory_isValid( event ) )
    {
        if( rSequence_getSTRINGA( event, RP_TAGS_FILE_PATH, &nameA ) ||
            rSequence_getSTRINGW( event, RP_TAGS_FILE_PATH, &nameW ) )
        {
            if( NULL != nameA &&
                !CryptoLib_hashFileA( nameA, fileHash, TRUE ) )
            {
                rpal_debug_info( "unable to fetch file hash for ident" );
            }

            if( NULL != nameW &&
                !CryptoLib_hashFileW( nameW, fileHash, TRUE ) )
            {
                rpal_debug_info( "unable to fetch file hash for ident" );
            }

            rSequence_getRU64( event, RP_TAGS_MEMORY_SIZE, &size );

            if( NULL != nameA )
            {
                processCodeIdentA( nameA, fileHash, size );
            }
            else if( NULL != nameW )
            {
                processCodeIdentW( nameW, fileHash, size );
            }
        }
    }
}


RBOOL
    collector_3_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;
    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( NULL != ( knownCode = rpal_bloom_create( 50000, 0.00001 ) ) )
        {
            isSuccess = FALSE;

            if( notifications_subscribe( RP_TAGS_NOTIFICATION_NEW_PROCESS, NULL, 0, NULL, processNewProcesses ) &&
                notifications_subscribe( RP_TAGS_NOTIFICATION_MODULE_LOAD, NULL, 0, NULL, processNewModule ) )
            {
                isSuccess = TRUE;
            }
            else
            {
                notifications_unsubscribe( RP_TAGS_NOTIFICATION_MODULE_LOAD, NULL, processNewModule );
                rpal_bloom_destroy( knownCode );
            }
        }
    }

    return isSuccess;
}

RBOOL
    collector_3_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( notifications_unsubscribe( RP_TAGS_NOTIFICATION_NEW_PROCESS, NULL, processNewProcesses ) &&
            notifications_unsubscribe( RP_TAGS_NOTIFICATION_MODULE_LOAD, NULL, processNewModule ) )
        {
            isSuccess = TRUE;
        }

        rpal_bloom_destroy( knownCode );
        knownCode = NULL;
    }

    return isSuccess;
}
