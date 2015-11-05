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

#define RPAL_FILE_ID   38


rSequence
    rSequence_new_from
    (
        RU32 from
    )
{
    _rSequence* seq = NULL;

    seq = rpal_memory_alloc_from( sizeof( _rSequence ), from );

    if( rpal_memory_isValid( seq ) )
    {
        if( !initSet( &(seq->set), from ) )
        {
            rpal_memory_free( seq );
            seq = NULL;
        }
    }

    return (rSequence)seq;
}

RVOID
    rSequence_free
    (
        rSequence seq
    )
{
    if( rpal_memory_isValid( seq ) )
    {
        freeSet( &(((_rSequence*)seq)->set) );

        rpal_memory_free( seq );
    }
}

RVOID
    rSequence_shallowFree
    (
        rSequence seq
    )
{
    if( rpal_memory_isValid( seq ) )
    {
        freeShallowSet( &(((_rSequence*)seq)->set) );

        rpal_memory_free( seq );
    }
}

RBOOL
    rSequence_addElement
    (
        rSequence seq,
        rpcm_tag tag,
        rpcm_type type,
        RPVOID pElem,
        RU32 elemSize
    )
{
    RBOOL isSuccess = FALSE;

    _rSequence* pSeq = NULL;

    if( rpal_memory_isValid( seq ) &&
        NULL != pElem &&
        0 != elemSize )
    {
        pSeq = (_rSequence*)seq;

        if( FALSE == isTagInSet( &pSeq->set, tag )  )
        {
            isSuccess = set_addElement( &pSeq->set, tag, type, pElem, elemSize );
        }
    }

    return isSuccess;
}

RBOOL
    rSequence_addRU8
    (
        rSequence seq,
        rpcm_tag tag,
        RU8 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_RU8, &val, sizeof( val ) );
}

RBOOL
    rSequence_addRU16
    (
        rSequence seq,
        rpcm_tag tag,
        RU16 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_RU16, &val, sizeof( val ) );
}

RBOOL
    rSequence_addRU32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_RU32, &val, sizeof( val ) );
}

RBOOL
    rSequence_addRU64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_RU64, &val, sizeof( val ) );
}

RBOOL
    rSequence_addSTRINGA
    (
        rSequence seq,
        rpcm_tag tag,
        RPCHAR string
    )
{
    return rSequence_addElement( seq, tag, RPCM_STRINGA, string, sizeof( string ) );
}

RBOOL
    rSequence_addSTRINGW
    (
        rSequence seq,
        rpcm_tag tag,
        RPWCHAR string
    )
{
    return rSequence_addElement( seq, tag, RPCM_STRINGW, string, sizeof( string ) );
}

RBOOL
    rSequence_addBUFFER
    (
        rSequence seq,
        rpcm_tag tag,
        RPU8 buffer,
        RU32 bufferSize
    )
{
    return rSequence_addElement( seq, tag, RPCM_BUFFER, buffer, bufferSize );
}

RBOOL
    rSequence_addTIMESTAMP
    (
        rSequence seq,
        rpcm_tag tag,
        RU64 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_TIMESTAMP, &val, sizeof( val ) );
}

RBOOL
    rSequence_addIPV4
    (
        rSequence seq,
        rpcm_tag tag,
        RU32 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_IPV4, &val, sizeof( val ) );
}

RBOOL
    rSequence_addIPV6
    (
        rSequence seq,
        rpcm_tag tag,
        RU8 val[ 16 ]
    )
{
    return rSequence_addElement( seq, tag, RPCM_IPV6, &val, sizeof( RU8 ) * 16 );
}

RBOOL
    rSequence_addPOINTER32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_POINTER_32, &val, sizeof( val ) );
}

RBOOL
    rSequence_addPOINTER64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64 val
    )
{
    return rSequence_addElement( seq, tag, RPCM_POINTER_64, &val, sizeof( val ) );
}

RBOOL
    rSequence_addSEQUENCE
    (
        rSequence seq,
        rpcm_tag tag,
        rSequence newSeq
    )
{
    return rSequence_addElement( seq, tag, RPCM_SEQUENCE, newSeq, sizeof( newSeq ) );
}

RBOOL
    rSequence_addLISTdup
    (
        rSequence seq,
        rpcm_tag tag,
        rList newList
    )
{
    RBOOL isSuccess = FALSE;
    rList tmpList = NULL;

    if( NULL != ( tmpList = rList_duplicate( newList ) ) )
    {
        if( rSequence_addElement( seq, tag, RPCM_LIST, tmpList, sizeof( tmpList ) ) )
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
    rSequence_addSEQUENCEdup
    (
        rSequence seq,
        rpcm_tag tag,
        rSequence newSeq
    )
{
    RBOOL isSuccess = FALSE;
    rSequence tmpSeq = NULL;

    if( NULL != ( tmpSeq = rSequence_duplicate( newSeq ) ) )
    {
        if( rSequence_addElement( seq, tag, RPCM_SEQUENCE, tmpSeq, sizeof( tmpSeq ) ) )
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
    rSequence_addLIST
    (
        rSequence seq,
        rpcm_tag tag,
        rList newList
    )
{
    return rSequence_addElement( seq, tag, RPCM_LIST, newList, sizeof( newList ) );
}


RBOOL
    rSequence_getElement
    (
        rSequence seq,
        rpcm_tag* tag,
        rpcm_type* type,
        RPVOID pElem,
        RU32* pElemSize
    )
{
    RBOOL isSuccess = FALSE;

    _rSequence* tmpSeq = NULL;

    if( rpal_memory_isValid( seq ) )
    {
        tmpSeq = (_rSequence*)seq;
        
        isSuccess = set_getElement( &tmpSeq->set, tag, type, pElem, pElemSize, NULL, FALSE );
    }

    return isSuccess;
}

RBOOL
    rSequence_getRU8
    (
        rSequence seq,
        rpcm_tag tag,
        RU8* pVal
    )
{
    rpcm_type type = RPCM_RU8;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getRU16
    (
        rSequence seq,
        rpcm_tag tag,
        RU16* pVal
    )
{
    rpcm_type type = RPCM_RU16;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getRU32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32* pVal
    )
{
    rpcm_type type = RPCM_RU32;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getRU64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_RU64;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getSTRINGA
    (
        rSequence seq,
        rpcm_tag tag,
        RPCHAR* pVal
    )
{
    rpcm_type type = RPCM_STRINGA;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getSTRINGW
    (
        rSequence seq,
        rpcm_tag tag,
        RPWCHAR* pVal
    )
{
    rpcm_type type = RPCM_STRINGW;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getBUFFER
    (
        rSequence seq,
        rpcm_tag tag,
        RPU8* pVal,
        RU32* pValSize
    )
{
    rpcm_type type = RPCM_BUFFER;

    return rSequence_getElement( seq, &tag, &type, pVal, pValSize );
}

RBOOL
    rSequence_getTIMESTAMP
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_TIMESTAMP;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getIPV4
    (
        rSequence seq,
        rpcm_tag tag,
        RU32* pVal
    )
{
    rpcm_type type = RPCM_IPV4;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getIPV6
    (
        rSequence seq,
        rpcm_tag tag,
        RU8* pVal
    )
{
    rpcm_type type = RPCM_IPV6;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getPOINTER32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32* pVal
    )
{
    rpcm_type type = RPCM_POINTER_32;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getPOINTER64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_POINTER_64;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getTIMEDELTA
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    )
{
    rpcm_type type = RPCM_TIMEDELTA;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getSEQUENCE
    (
        rSequence seq,
        rpcm_tag tag,
        rSequence* pVal
    )
{
    rpcm_type type = RPCM_SEQUENCE;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_getLIST
    (
        rSequence seq,
        rpcm_tag tag,
        rList* pVal
    )
{
    rpcm_type type = RPCM_LIST;

    return rSequence_getElement( seq, &tag, &type, pVal, NULL );
}

RBOOL
    rSequence_removeElement
    (
        rSequence seq,
        rpcm_tag tag,
        rpcm_type type
    )
{
    RBOOL isSuccess = FALSE;
    _rSequence* pSeq = (_rSequence*)seq;

    isSuccess = set_removeElement( &(pSeq->set), tag, type );

    return isSuccess;
}

RBOOL
    rSequence_serialise
    (
        rSequence seq,
        rBlob blob
    )
{
    RBOOL isSuccess = FALSE;

    _rSequence* tmpSeq = NULL;

    if( rpal_memory_isValid( seq ) &&
        NULL != blob )
    {
        tmpSeq = (_rSequence*)seq;
        isSuccess = set_serialise( &tmpSeq->set, blob ); 
    }

    return isSuccess;
}

RBOOL
    rSequence_deserialise_from
    (
        rSequence* pSeq,
        RPU8 buffer,
        RU32 bufferSize,
        RU32* pBytesConsumed,
        RU32 from
    )
{
    RBOOL isSuccess = FALSE;

    _rSequence* tmpSeq = NULL;

    if( NULL != pSeq &&
        NULL != buffer &&
        0 != bufferSize )
    {
        *pSeq = rSequence_new_from( from );

        if( rpal_memory_isValid( *pSeq ) )
        {
            tmpSeq = (_rSequence*)*pSeq;
            isSuccess = set_deserialise( &tmpSeq->set, buffer, bufferSize, pBytesConsumed );
        }
    }

    return isSuccess;
}

rSequence
    rSequence_duplicate_from
    (
        rSequence original,
        RU32 from
    )
{
    _rSequence* newSeq  = NULL;
    _rSequence* pSeq = (_rSequence*)original;

    if( NULL != original )
    {
        newSeq = rpal_memory_alloc_from( sizeof( _rSequence ), from );

        if( NULL != newSeq )
        {
            if( !set_duplicate( &(pSeq->set), &(newSeq->set) ) )
            {
                rpal_memory_free( newSeq );
                newSeq = NULL;
            }
        }
    }

    return newSeq;
}
