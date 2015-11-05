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

#include <librpcm/librpcm.h>
#include "privateHeaders.h"

#define RPAL_FILE_ID    37


rList
    rList_new_from
    (
        rpcm_tag elemTag,
        rpcm_type elemType,
        RU32 from
    )
{
    _rList* list = NULL;

    list = rpal_memory_alloc_from( sizeof( _rList ), from );

    if( rpal_memory_isValid( list ) )
    {
        if( !initSet( &(list->set), from ) )
        {
            rpal_memory_free( list );
            list = NULL;
        }
        else
        {
            list->elemTags = elemTag;
            list->elemTypes = elemType;
        }

        list->lastElem = NULL;
    }

    return (rList)list;
}

RVOID
    rList_free
    (
        rList list
    )
{
    _rList* pList = (_rList*)list;

    if( rpal_memory_isValid( list ) )
    {
        freeSet( &(pList->set) );

        rpal_memory_free( list );
    }
}

RVOID
    rList_shallowFree
    (
        rList list
    )
{
    _rList* pList = (_rList*)list;

    if( rpal_memory_isValid( list ) )
    {
        freeShallowSet( &(pList->set) );

        rpal_memory_free( list );
    }
}

RBOOL
    rList_addElement
    (
        rList list,
        rpcm_type type,
        RPVOID pElem,
        RU32 elemSize
    )
{
    RBOOL isSuccess = FALSE;

    _rList* pList = NULL;

    if( rpal_memory_isValid( list ) &&
        NULL != pElem &&
        0 != elemSize )
    {
        pList = (_rList*)list;

        if( pList->elemTypes == type )
        {
            isSuccess = set_addElement( &pList->set, pList->elemTags, type, pElem, elemSize );
        }
    }

    return isSuccess;
}


RBOOL
    rList_addRU8
    (
        rList list,
        RU8 val
    )
{
    return rList_addElement( list, RPCM_RU8, &val, sizeof( val ) );
}

RBOOL
    rList_addRU16
    (
        rList list,
        RU16 val
    )
{
    return rList_addElement( list, RPCM_RU16, &val, sizeof( val ) );
}

RBOOL
    rList_addRU32
    (
        rList list,
        RU32 val
    )
{
    return rList_addElement( list, RPCM_RU32, &val, sizeof( val ) );
}

RBOOL
    rList_addRU64
    (
        rList list,
        RU64 val
    )
{
    return rList_addElement( list, RPCM_RU64, &val, sizeof( val ) );
}

RBOOL
    rList_addSTRINGA
    (
        rList list,
        RPCHAR string
    )
{
    return rList_addElement( list, RPCM_STRINGA, string, sizeof( string ) );
}

RBOOL
    rList_addSTRINGW
    (
        rList list,
        RPWCHAR string
    )
{
    return rList_addElement( list, RPCM_STRINGW, string, sizeof( string ) );
}

RBOOL
    rList_addBUFFER
    (
        rList list,
        RPU8 buffer,
        RU32 bufferSize
    )
{
    return rList_addElement( list, RPCM_BUFFER, buffer, bufferSize );
}

RBOOL
    rList_addTIMESTAMP
    (
        rList list,
        RU64 val
    )
{
    return rList_addElement( list, RPCM_TIMESTAMP, &val, sizeof( val ) );
}

RBOOL
    rList_addIPV4
    (
        rList list,
        RU32 val
    )
{
    return rList_addElement( list, RPCM_IPV4, &val, sizeof( val ) );
}

RBOOL
    rList_addIPV6
    (
        rList list,
        RU8 val[ 16 ]
    )
{
    return rList_addElement( list, RPCM_IPV6, &val, sizeof( RU8 ) * 16 );
}

RBOOL
    rList_addPOINTER32
    (
        rList list,
        RU32 val
    )
{
    return rList_addElement( list, RPCM_POINTER_32, &val, sizeof( val ) );
}

RBOOL
    rList_addPOINTER64
    (
        rList list,
        RU64 val
    )
{
    return rList_addElement( list, RPCM_POINTER_64, &val, sizeof( val ) );
}

RBOOL
    rList_addSEQUENCE
    (
        rList list,
        rSequence newSeq
    )
{
    return rList_addElement( list, RPCM_SEQUENCE, newSeq, sizeof( newSeq ) );
}

RBOOL
    rList_addSEQUENCEdup
    (
        rList list,
        rSequence newSeq
    )
{
    RBOOL isSuccess = FALSE;
    rSequence tmpSeq = NULL;

    if( NULL != ( tmpSeq = rSequence_duplicate( newSeq ) ) )
    {
        if( rList_addElement( list, RPCM_SEQUENCE, tmpSeq, sizeof( tmpSeq ) ) )
        {
            isSuccess = TRUE;
        }
        else
        {
            rSequence_free( tmpSeq );
        }
    }

    return isSuccess;
}

RBOOL
    rList_addLIST
    (
        rList list,
        rList newList
    )
{
    return rList_addElement( list, RPCM_LIST, newList, sizeof( newList ) );
}

RBOOL
    rList_addLISTdup
    (
        rList list,
        rList newList
    )
{
    RBOOL isSuccess = FALSE;
    rList tmpList = NULL;

    if( NULL != ( tmpList = rList_duplicate( newList ) ) )
    {
        if( rList_addElement( list, RPCM_LIST, tmpList, sizeof( tmpList ) ) )
        {
            isSuccess = TRUE;
        }
        else
        {
            rList_free( tmpList );
        }
    }

    return isSuccess;
}

RBOOL
    rList_getElement
    (
        rList list,
        rpcm_tag* tag,
        rpcm_type* type,
        RPVOID pElem,
        RU32* pElemSize
    )
{
    RBOOL isSuccess = FALSE;

    _rList* tmpList = NULL;

    if( rpal_memory_isValid( list ) )
    {
        tmpList = (_rList*)list;
// REMOVE ME WHEN ALIGNMENT CHANGES TO privateHeader.h are effective
#pragma warning( disable: 4366 )
        isSuccess = set_getElement( &tmpList->set, tag, type, pElem, pElemSize, &tmpList->lastElem, FALSE );

        if( !isSuccess )
        {
            ((_rList*)list)->lastElem = NULL;
        }
    }

    return isSuccess;
}

RBOOL
    rList_getRU8
    (
        rList list,
        rpcm_tag tag,
        RU8* pVal
    )
{
    rpcm_type type = RPCM_RU8;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getRU16
    (
        rList list,
        rpcm_tag tag,
        RU16* pVal
    )
{
    rpcm_type type = RPCM_RU16;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getRU32
    (
        rList list,
        rpcm_tag tag,
        RU32* pVal
    )
{
    rpcm_type type = RPCM_RU32;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getRU64
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_RU64;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getSTRINGA
    (
        rList list,
        rpcm_tag tag,
        RPCHAR* pVal
    )
{
    rpcm_type type = RPCM_STRINGA;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getSTRINGW
    (
        rList list,
        rpcm_tag tag,
        RPWCHAR* pVal
    )
{
    rpcm_type type = RPCM_STRINGW;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getBUFFER
    (
        rList list,
        rpcm_tag tag,
        RU8* pVal,
        RU32* pValSize
    )
{
    rpcm_type type = RPCM_BUFFER;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, pValSize );
}

RBOOL
    rList_getTIMESTAMP
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_TIMESTAMP;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getIPV4
    (
        rList list,
        rpcm_tag tag,
        RU32* pVal
    )
{
    rpcm_type type = RPCM_IPV4;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getIPV6
    (
        rList list,
        rpcm_tag tag,
        RU8* pVal
    )
{
    rpcm_type type = RPCM_IPV6;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getPOINTER32
    (
        rList list,
        rpcm_tag tag,
        RU32* pVal
    )
{
    rpcm_type type = RPCM_POINTER_32;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getPOINTER64
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_POINTER_64;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getTIMEDELTA
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_TIMEDELTA;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getSEQUENCE
    (
        rList list,
        rpcm_tag tag,
        rSequence* pVal
    )
{
    rpcm_type type = RPCM_SEQUENCE;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}

RBOOL
    rList_getLIST
    (
        rList list,
        rpcm_tag tag,
        rList* pVal
    )
{
    rpcm_type type = RPCM_LIST;
    rpcm_tag ltag = tag;

    return rList_getElement( list, &ltag, &type, pVal, NULL );
}


RBOOL
    rList_serialise
    (
        rList list,
        rBlob blob
    )
{
    RBOOL isSuccess = FALSE;

    _rList* tmpList = NULL;
    rpcm_tag elemTags = 0;
    rpcm_type elemTypes = 0;

    if( rpal_memory_isValid( list ) &&
        NULL != blob )
    {
        tmpList = (_rList*)list;

        elemTags = rpal_hton32( tmpList->elemTags );
        elemTypes = tmpList->elemTypes;

        if( rpal_blob_add( blob, &elemTags, sizeof( elemTags ) ) &&
            rpal_blob_add( blob, &elemTypes, sizeof( elemTypes ) ) )
        {
            isSuccess = set_serialise( &tmpList->set, blob );
        }
    }

    return isSuccess;
}

RBOOL
    rList_deserialise_from
    (
        rList* pList,
        RPU8 buffer,
        RU32 bufferSize,
        RU32* pBytesConsumed,
        RU32 from
    )
{
    RBOOL isSuccess = FALSE;

    _rList* tmpList = NULL;

    if( NULL != pList &&
        NULL != buffer &&
        sizeof( rpcm_tag ) + sizeof( rpcm_type ) <= bufferSize )
    {
        *pList = rList_new_from( 0, 0, from );

        if( rpal_memory_isValid( *pList ) )
        {
            tmpList = (_rList*)*pList;
            tmpList->elemTags = rpal_ntoh32( *(rpcm_tag*)buffer );
            tmpList->elemTypes = *(rpcm_type*)(buffer + sizeof( rpcm_tag ) );

            buffer += sizeof( rpcm_tag ) + sizeof( rpcm_type );
            bufferSize -= ( sizeof( rpcm_tag ) + sizeof( rpcm_type ) );
            isSuccess = set_deserialise( &tmpList->set, buffer, bufferSize, pBytesConsumed );

            if( isSuccess &&
                NULL != pBytesConsumed )
            {
                *pBytesConsumed += sizeof( rpcm_tag ) + sizeof( rpcm_type );
            }
        }
    }

    return isSuccess;
}

RBOOL
    rList_setInfo
    (
        rList list,
        rpcm_tag elemTags,
        rpcm_type elemTypes
    )
{
    RBOOL isSuccess = FALSE;

    _rList* pList = NULL;

    if( rpal_memory_isValid( list ) )
    {
        pList = (_rList*)list;

        pList->elemTags = elemTags;
        pList->elemTypes = elemTypes;

        isSuccess = TRUE;
    }

    return isSuccess;
}

RU32
    rList_getNumElements
    (
        rList list
    )
{
    RU32 size = 0;

    _rList* pList = NULL;

    if( rpal_memory_isValid( list ) )
    {
        pList = (_rList*)list;

        size = pList->set.nElements;
    }

    return size;
}

RBOOL
    rList_resetIterator
    (
        rList list
    )
{
    RBOOL isSuccess = FALSE;
    _rList* pList = (_rList*)list;

    if( NULL != pList )
    {
        pList->lastElem = NULL;
    }

    return isSuccess;
}


rList
    rList_duplicate_from
    (
        rList original,
        RU32 from
    )
{
    _rList* newList = NULL;
    _rList* pList = (_rList*)original;

    if( NULL != original )
    {
        newList = rpal_memory_alloc_from( sizeof( _rList ), from );

        if( NULL != newList )
        {
            newList->elemTags = pList->elemTags;
            newList->elemTypes = pList->elemTypes;
            newList->lastElem = NULL;

            if( !set_duplicate( &(pList->set), &(newList->set) ) )
            {
                rpal_memory_free( newList );
                newList = NULL;
            }
        }
    }

    return newList;
}
