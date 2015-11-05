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

#include "aad.h"
#include <cryptoLib/cryptoLib.h>
#include <librpcm/librpcm.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID 100


typedef struct
{
    RU8 parentKey[ CRYPTOLIB_HASH_SIZE ];
    RTIME firstSeen;
    RTIME lastSeen;
    RU32 reserved1;
    RU32 reserved2;
    rBloom childrenSeen;

} aad_rel_entry;


static
RS32
    compHash
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

static
RS32
    compParentKey
    (
        aad_rel_entry** pKey1,
        aad_rel_entry** pKey2
    )
{
    RS32 ret = 0;

    if( NULL != pKey1 &&
        NULL != *pKey1 &&
        NULL != pKey2 &&
        NULL != *pKey2 )
    {
        ret = rpal_memory_memcmp( *pKey1, *pKey2, CRYPTOLIB_HASH_SIZE );
    }

    return ret;
}


static
RVOID
    freeParentEntry
    (
        aad_rel_entry** entry
    )
{
    if( NULL != entry &&
        NULL != *entry )
    {
        if( NULL != (*entry)->childrenSeen )
        {
            rpal_bloom_destroy( (*entry)->childrenSeen );
        }

        rpal_memory_free( *entry );
    }
}

static RBOOL
    isEntryInTraining
    (
        aad_rel_entry* entry
    )
{
    RBOOL isInTraining = TRUE;

    if( NULL != entry )
    {
        if( AAD_TRAINING_MAX_TIME <= ( entry->lastSeen - entry->firstSeen ) ||
            AAD_TRAINING_MIN_RELATIONS <= rpal_bloom_getNumEntries( entry->childrenSeen ) )
        {
            isInTraining = FALSE;
        }
    }

    return isInTraining;
}

RBOOL
    aad_checkRelation_WCH_WCH
    (
        rBTree stash,
        RPWCHAR parentKey,
        RPWCHAR childKey
    )
{
    RBOOL isNewAndRelevant = FALSE;

    aad_rel_entry* entry = NULL;
    RU8 key[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    RU8* pKey = key;

    if( rpal_memory_isValid( stash ) &&
        NULL != parentKey &&
        NULL != childKey )
    {
        rpal_btree_manual_lock( stash );

        if( CryptoLib_hash( parentKey, rpal_string_strlenw( parentKey ), (RPU8)&key ) &&
            rpal_btree_search( stash, &pKey, &entry, TRUE ) )
        {
            if( rpal_bloom_addIfNew( entry->childrenSeen, childKey, rpal_string_strlenw( childKey ) ) )
            {
                // We've never seen this relation
                entry->lastSeen = rpal_time_getGlobal();
                if( 0 == entry->firstSeen )
                {
                    entry->firstSeen = rpal_time_getGlobal();
                }

                if( !isEntryInTraining( entry ) )
                {
                    isNewAndRelevant = TRUE;
                    rpal_debug_print( "New relation detected ( %ls -> %ls ).\n", parentKey, childKey );
                }
                else
                {
                    rpal_debug_print( "New relation detected ( %ls -> %ls ), ignored because it's still in training.\n", parentKey, childKey );
                }
            }
        }

        rpal_btree_manual_unlock( stash );
    }

    return isNewAndRelevant;
}

RBOOL
    aad_checkRelation_WCH_HASH
    (
        rBTree stash,
        RPWCHAR parentKey,
        RPU8 childKey
    )
{
    RBOOL isNewAndRelevant = FALSE;

    aad_rel_entry* entry = NULL;
    RU8 key[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    RU8* pKey = key;

    if( rpal_memory_isValid( stash ) &&
        NULL != parentKey &&
        NULL != childKey )
    {
        rpal_btree_manual_lock( stash );

        if( CryptoLib_hash( parentKey, rpal_string_strlenw( parentKey ), (RPU8)&key ) &&
            rpal_btree_search( stash, &pKey, &entry, TRUE ) )
        {
            if( rpal_bloom_addIfNew( entry->childrenSeen, childKey, CRYPTOLIB_HASH_SIZE ) )
            {
                // We've never seen this relation
                entry->lastSeen = rpal_time_getGlobal();
                if( 0 == entry->firstSeen )
                {
                    entry->firstSeen = rpal_time_getGlobal();
                }

                if( !isEntryInTraining( entry ) )
                {
                    isNewAndRelevant = TRUE;
                }
            }
        }

        rpal_btree_manual_unlock( stash );
    }

    return isNewAndRelevant;
}

RBOOL
    aad_checkRelation_HASH_WCH
    (
        rBTree stash,
        RPU8 parentKey,
        RPWCHAR childKey
    )
{
    RBOOL isNewAndRelevant = FALSE;

    aad_rel_entry* entry = (aad_rel_entry*)parentKey;
    RU8 key[ CRYPTOLIB_HASH_SIZE ] = { 0 };

    if( rpal_memory_isValid( stash ) &&
        NULL != parentKey &&
        NULL != childKey )
    {
        rpal_btree_manual_lock( stash );

        if( CryptoLib_hash( childKey, rpal_string_strlenw( childKey ), (RPU8)&key ) &&
            rpal_btree_search( stash, &entry, &entry, TRUE ) )
        {
            if( rpal_bloom_addIfNew( entry->childrenSeen, key, CRYPTOLIB_HASH_SIZE ) )
            {
                // We've never seen this relation
                entry->lastSeen = rpal_time_getGlobal();
                if( 0 == entry->firstSeen )
                {
                    entry->firstSeen = rpal_time_getGlobal();
                }

                if( !isEntryInTraining( entry ) )
                {
                    isNewAndRelevant = TRUE;
                }
            }
        }

        rpal_btree_manual_unlock( stash );
    }

    return isNewAndRelevant;
}

RBOOL
    aad_enableParent_WCH
    (
        rBTree stash,
        RPWCHAR parentKey,
        RU32 nExpected,
        RDOUBLE nFPRatio
    )
{
    RBOOL isEnabled = FALSE;

    aad_rel_entry* entry = NULL;

    if( rpal_memory_isValid( stash ) &&
        NULL != parentKey )
    {
        if( rpal_btree_manual_lock( stash ) )
        {
            
            if( NULL != ( entry = rpal_memory_alloc( sizeof( aad_rel_entry ) ) ) )
            {
                entry->childrenSeen = 0;
                entry->firstSeen = 0;
                entry->lastSeen = 0;

                if( NULL != ( entry->childrenSeen = rpal_bloom_create( nExpected, nFPRatio ) ) )
                {
                    if( CryptoLib_hash( parentKey, rpal_string_strlenw( parentKey ), (RPU8)&( entry->parentKey ) ) )
                    {
                        if( !rpal_btree_search( stash, &entry, NULL, TRUE ) )
                        {
                            if( rpal_btree_add( stash, &entry, TRUE ) )
                            {
                                isEnabled = TRUE;
                            }
                        }
                        else
                        {
                            freeParentEntry( &entry );
                            isEnabled = TRUE;
                        }
                    }
                }

                if( !isEnabled )
                {
                    freeParentEntry( &entry );
                }
            }

            rpal_btree_manual_unlock( stash );
        }
    }

    return isEnabled;
}

RBOOL
    aad_enablePhase1Parent_WCH
    (
        rBTree stash,
        RPWCHAR parentKey
    )
{
    RBOOL isEnabled = FALSE;

    RU8 hash[ CRYPTOLIB_HASH_SIZE ] = { 0 };

    if( rpal_memory_isValid( stash ) &&
        NULL != parentKey )
    {
        if( rpal_btree_manual_lock( stash ) )
        {

            if( CryptoLib_hash( parentKey, rpal_string_strlenw( parentKey ), hash ) )
            {
                if( !rpal_btree_search( stash, hash, NULL, TRUE ) )
                {
                    if( rpal_btree_add( stash, hash, TRUE ) )
                    {
                        isEnabled = TRUE;
                    }
                }
                else
                {
                    isEnabled = TRUE;
                }
            }

            rpal_btree_manual_unlock( stash );
        }
    }

    return isEnabled;
}

RBOOL
    aad_enableParent_HASH
    (
        rBTree stash,
        RPU8 parentKey,
        RU32 nExpected,
        RDOUBLE nFPRatio
    )
{
    RBOOL isEnabled = FALSE;

    aad_rel_entry* entry = NULL;

    if( rpal_memory_isValid( stash ) &&
        NULL != parentKey )
    {
        if( NULL != ( entry = rpal_memory_alloc( sizeof( aad_rel_entry ) ) ) )
        {
            entry->childrenSeen = 0;
            entry->firstSeen = 0;
            entry->lastSeen = 0;

            if( NULL != ( entry->childrenSeen = rpal_bloom_create( nExpected, nFPRatio ) ) )
            {
                rpal_memory_memcpy( &( entry->parentKey ), parentKey, CRYPTOLIB_HASH_SIZE );
                if( rpal_btree_add( stash, &entry, FALSE ) )
                {
                    isEnabled = TRUE;
                }
            }

            if( !isEnabled )
            {
                freeParentEntry( &entry );
            }
        }
    }

    return isEnabled;
}


RBOOL
    aad_loadStashFromBuffer
    (
        rBTree stash,
        RPU8 pBuffer,
        RU32 bufferSize
    )
{
    RBOOL isLoaded = FALSE;

    aad_rel_entry* entry = NULL;
    aad_rel_entry* newEntry = NULL;
    RU32 totalSize = 0;
    RU32 nEntries = 0;

    if( rpal_memory_isValid( stash ) &&
        NULL != pBuffer &&
        sizeof( RU32 ) < bufferSize )
    {
        entry = (aad_rel_entry*)( pBuffer + sizeof( RU32 ) );
        totalSize = *(RU32*)pBuffer;

        isLoaded = TRUE;

        while( IS_WITHIN_BOUNDS( entry, totalSize, pBuffer, bufferSize ) )
        {
            if( NULL != ( newEntry = rpal_memory_alloc( sizeof( aad_rel_entry ) ) ) )
            {
                rpal_memory_memcpy( newEntry, entry, sizeof( *newEntry ) );
                newEntry->childrenSeen = rpal_bloom_deserialize( (RPU8)entry + sizeof( *entry ), totalSize - sizeof( *entry ) );

                if( !rpal_btree_add( stash, &newEntry, FALSE ) )
                {
                    rpal_memory_free( newEntry );
                    isLoaded = FALSE;
                    break;
                }
            }
            else
            {
                isLoaded = FALSE;
                break;
            }

            entry = (aad_rel_entry*)( (RPU8)entry + ( totalSize + sizeof( RU32 ) ) );
            totalSize = *(RU32*)( (RPU8)entry - sizeof( RU32 ) );
            nEntries++;
        }
    }

    if( isLoaded )
    {
        rpal_debug_info( "%d entries loaded into stash.", nEntries );
    }
    else
    {
        rpal_debug_warning( "failed to load entry into stash after %d entries.", nEntries );
    }

    return isLoaded;
}


RBOOL
    aad_dumpStashToBuffer
    (
        rBTree stash,
        RPU8* pOutBuffer,
        RU32* pOutBufferSize
    )
{
    RBOOL isDumped = FALSE;

    aad_rel_entry* entry = NULL;
    aad_rel_entry* nextEntry = NULL;
    rBlob blob = NULL;
    RPU8 bloom = NULL;
    RU32 bloomSize = 0;
    RU32 totalSize = 0;
    RU32 nEntries = 0;

    if( rpal_memory_isValid( stash ) &&
        NULL != pOutBuffer &&
        NULL != pOutBufferSize )
    {
        if( NULL != ( blob = rpal_blob_create( 0, 0 ) ) )
        {
            if( rpal_btree_manual_lock( stash ) )
            {
                if( rpal_btree_minimum( stash, &nextEntry, TRUE ) )
                {
                    isDumped = TRUE;

                    do
                    {
                        entry = nextEntry;

                        if( rpal_bloom_serialize( entry->childrenSeen, &bloom, &bloomSize ) )
                        {
                            totalSize = bloomSize + sizeof( *entry );

                            rpal_blob_add( blob, &totalSize, sizeof( RU32 ) );
                            rpal_blob_add( blob, entry, sizeof( *entry ) );
                            rpal_blob_add( blob, bloom, bloomSize );

                            rpal_memory_free( bloom );
                            nEntries++;
                        }
                        
                    } while( rpal_btree_next( stash, &entry, &nextEntry, TRUE ) );
                }

                rpal_btree_manual_unlock( stash );
            }

            if( isDumped )
            {
                *pOutBuffer = rpal_blob_getBuffer( blob );
                *pOutBufferSize = rpal_blob_getSize( blob );
                rpal_blob_freeWrapperOnly( blob );
            }
            else
            {
                rpal_blob_free( blob );
            }
        }
    }

    if( isDumped )
    {
        rpal_debug_info( "dumped %d entries from stash.", nEntries );
    }
    else
    {
        rpal_debug_warning( "failed to dump entry from stash after %d entries.", nEntries );
    }

    return isDumped;
}


RBOOL
    aad_isParentEnabled_WCH
    (
        rBTree stash,
        RPWCHAR parentKey
    )
{
    RBOOL isEnabled = FALSE;

    aad_rel_entry* entry = NULL;
    RU8 key[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    RU8* pKey = key;

    if( rpal_memory_isValid( stash ) &&
        NULL != parentKey )
    {
        if( CryptoLib_hash( parentKey, rpal_string_strlenw( parentKey ), (RPU8)key ) &&
            rpal_btree_search( stash, &pKey, &entry, FALSE ) )
        {
            isEnabled = TRUE;
        }
    }

    return isEnabled;
}


rBTree
    aad_initStash_phase_2
    (

    )
{
    rBTree stash = NULL;

    stash = rpal_btree_create( sizeof( aad_rel_entry* ), compParentKey, freeParentEntry );

    return stash;
}

rBTree
    aad_initStash_phase_1
    (

    )
{
    rBTree stash = NULL;

    stash = rpal_btree_create( CRYPTOLIB_HASH_SIZE, compHash, NULL );

    return stash;
}

rSequence
    aad_newReport_WCH_WCH
    (
        RU32 relTypeId,
        RPWCHAR parentKey,
        RPWCHAR childKey
    )
{
    rSequence report = NULL;

    if( NULL != parentKey &&
        NULL != childKey )
    {
        if( NULL != ( report = rSequence_new() ) )
        {
            if( !rSequence_addRU32( report, RP_TAGS_AAD_RELATION_TYPE, relTypeId ) ||
                !rSequence_addSTRINGW( report, RP_TAGS_AAD_PARENT_KEY, parentKey ) ||
                !rSequence_addSTRINGW( report, RP_TAGS_AAD_CHILD_KEY, childKey ) ||
                !rSequence_addRU8( report, RP_TAGS_REPORT_NOW, 1 ) )
            {
                rSequence_free( report );
                report = NULL;
            }
        }
    }

    return report;
}

rSequence
    aad_newReport_WCH_HASH
    (
        RU32 relTypeId,
        RPWCHAR parentKey,
        RPU8 childKey
    )
{
    rSequence report = NULL;

    if( NULL != parentKey &&
        NULL != childKey )
    {
        if( NULL != ( report = rSequence_new() ) )
        {
            if( !rSequence_addRU32( report, RP_TAGS_AAD_RELATION_TYPE, relTypeId ) ||
                !rSequence_addSTRINGW( report, RP_TAGS_AAD_PARENT_KEY, parentKey ) ||
                !rSequence_addBUFFER( report, RP_TAGS_AAD_CHILD_KEY, childKey, CRYPTOLIB_HASH_SIZE ) ||
                !rSequence_addRU8( report, RP_TAGS_REPORT_NOW, 1 ) )
            {
                rSequence_free( report );
                report = NULL;
            }
        }
    }

    return report;
}

rSequence
    aad_newReport_HASH_WCH
    (
        RU32 relTypeId,
        RPU8 parentKey,
        RPWCHAR childKey
    )
{
    rSequence report = NULL;

    if( NULL != parentKey &&
        NULL != childKey )
    {
        if( NULL != ( report = rSequence_new() ) )
        {
            if( !rSequence_addRU32( report, RP_TAGS_AAD_RELATION_TYPE, relTypeId ) ||
                !rSequence_addSTRINGW( report, RP_TAGS_AAD_CHILD_KEY, childKey ) ||
                !rSequence_addBUFFER( report, RP_TAGS_AAD_PARENT_KEY, parentKey, CRYPTOLIB_HASH_SIZE ) ||
                !rSequence_addRU8( report, RP_TAGS_REPORT_NOW, 1 ) )
            {
                rSequence_free( report );
                report = NULL;
            }
        }
    }

    return report;
}