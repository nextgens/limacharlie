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

#ifndef _LIB_RPCM_H
#define _LIB_RPCM_H

#include <rpal/rpal.h>

//=============================================================================
// refractionPOINT Common Messaging
//=============================================================================
typedef RPVOID  rSequence;
typedef RPVOID  rList;
typedef RPVOID  rIterator;
typedef RU32    rpcm_tag;
typedef RU8     rpcm_type;

//=============================================================================
//  Types
//=============================================================================
#define RPCM_INVALID_TAG    0x00000000
#define RPCM_INVALID_TYPE   0x00
#define RPCM_RU8            0x01
#define RPCM_RU16           0x02
#define RPCM_RU32           0x03
#define RPCM_RU64           0x04
#define RPCM_STRINGA        0x05
#define RPCM_STRINGW        0x06
#define RPCM_BUFFER         0x07
#define RPCM_TIMESTAMP      0x08
#define RPCM_IPV4           0x09
#define RPCM_IPV6           0x0A
#define RPCM_POINTER_32     0x0B
#define RPCM_POINTER_64     0x0C
#define RPCM_TIMEDELTA      0x0D
#define RPCM_DOUBLE         0x0E

#define RPCM_COMPLEX_TYPES  0x80
#define RPCM_SEQUENCE       0x81
#define RPCM_LIST           0x82

//=============================================================================
//  API
//=============================================================================

// CREATION / DESTRUCTION
//-----------------------------------------------------------------------------
#define rSequence_new()     rSequence_new_from( RPAL_LINE_SUBTAG )
rSequence
    rSequence_new_from
    (
        RU32 from
    );

RVOID
    rSequence_free
    (
        rSequence seq
    );

RVOID
    rSequence_shallowFree
    (
        rSequence seq
    );

#define rList_new( elemTag, elemType )   rList_new_from( (elemTag), (elemType), RPAL_LINE_SUBTAG )
rList
    rList_new_from
    (
        rpcm_tag elemTag,
        rpcm_type elemType,
        RU32 from
    );

RVOID
    rList_free
    (
        rList list
    );

RVOID
    rList_shallowFree
    (
        rList list
    );

#define rIterator_new( listOrSequence )    rIterator_new_from( listOrSequence, RPAL_LINE_SUBTAG )
rIterator
    rIterator_new_from
    (
        RPVOID listOrSequence,
        RU32 from
    );

RVOID
    rIterator_free
    (
        rIterator ite
    );

RBOOL
    rIterator_next
    (
        rIterator ite,
        rpcm_tag* tag,
        rpcm_type* type,
        RPVOID* pElem,
        RU32* elemSize
    );

// ADD
//-----------------------------------------------------------------------------
RBOOL
    rSequence_addElement
    (
        rSequence seq,
        rpcm_tag tag,
        rpcm_type type,
        RPVOID pElem,
        RU32 elemSize
    );

RBOOL
    rSequence_addRU8
    (
        rSequence seq,
        rpcm_tag tag,
        RU8 val
    );

RBOOL
    rSequence_addRU16
    (
        rSequence seq,
        rpcm_tag tag,
        RU16 val
    );

RBOOL
    rSequence_addRU32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32 val
    );

RBOOL
    rSequence_addRU64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64 val
    );

RBOOL
    rSequence_addSTRINGA
    (
        rSequence seq,
        rpcm_tag tag,
        RPCHAR string
    );

RBOOL
    rSequence_addSTRINGW
    (
        rSequence seq,
        rpcm_tag tag,
        RPWCHAR string
    );

RBOOL
    rSequence_addBUFFER
    (
        rSequence seq,
        rpcm_tag tag,
        RPU8 buffer,
        RU32 bufferSize
    );

RBOOL
    rSequence_addTIMESTAMP
    (
        rSequence seq,
        rpcm_tag tag,
        RU64 val
    );

RBOOL
    rSequence_addIPV4
    (
        rSequence seq,
        rpcm_tag tag,
        RU32 val
    );

RBOOL
    rSequence_addIPV6
    (
        rSequence seq,
        rpcm_tag tag,
        RU8 val[ 16 ]
    );

RBOOL
    rSequence_addPOINTER32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32 val
    );

RBOOL
    rSequence_addPOINTER64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64 val
    );

RBOOL
    rSequence_addTIMEDELTA
    (
        rSequence seq,
        rpcm_tag tag,
        RU64 val
    );

RBOOL
    rSequence_addSEQUENCE
    (
        rSequence seq,
        rpcm_tag tag,
        rSequence newSeq
    );

RBOOL
    rSequence_addLIST
    (
        rSequence seq,
        rpcm_tag tag,
        rList newList
    );

RBOOL
    rSequence_addSEQUENCEdup
    (
        rSequence seq,
        rpcm_tag tag,
        rSequence newSeq
    );

RBOOL
    rSequence_addLISTdup
    (
        rSequence seq,
        rpcm_tag tag,
        rList newList
    );

RBOOL
    rList_addElement
    (
        rList list,
        rpcm_type type,
        RPVOID pElem,
        RU32 elemSize
    );

RBOOL
    rList_addRU8
    (
        rList list,
        RU8 val
    );

RBOOL
    rList_addRU16
    (
        rList list,
        RU16 val
    );

RBOOL
    rList_addRU32
    (
        rList list,
        RU32 val
    );

RBOOL
    rList_addRU64
    (
        rList list,
        RU64 val
    );

RBOOL
    rList_addSTRINGA
    (
        rList list,
        RPCHAR string
    );

RBOOL
    rList_addSTRINGW
    (
        rList list,
        RPWCHAR string
    );

RBOOL
    rList_addBUFFER
    (
        rList list,
        RPU8 buffer,
        RU32 bufferSize
    );

RBOOL
    rList_addTIMESTAMP
    (
        rList list,
        RU64 val
    );

RBOOL
    rList_addIPV4
    (
        rList list,
        RU32 val
    );

RBOOL
    rList_addIPV6
    (
        rList list,
        RU8 val[ 16 ]
    );

RBOOL
    rList_addPOINTER32
    (
        rList list,
        RU32 val
    );

RBOOL
    rList_addPOINTER64
    (
        rList list,
        RU64 val
    );

RBOOL
    rList_addTIMEDELTA
    (
        rList list,
        RU64 val
    );

RBOOL
    rList_addSEQUENCE
    (
        rList list,
        rSequence newSeq
    );

RBOOL
    rList_addSEQUENCEdup
    (
        rList list,
        rSequence newSeq
    );

RBOOL
    rList_addLIST
    (
        rList list,
        rList newList
    );

RBOOL
    rList_addLISTdup
    (
        rList list,
        rList newList
    );

// GET
//-----------------------------------------------------------------------------
RBOOL
    rSequence_getElement
    (
        rSequence seq,
        rpcm_tag* tag,
        rpcm_type* type,
        RPVOID pElem,
        RU32* elemSize
    );

RBOOL
    rSequence_getRU8
    (
        rSequence seq,
        rpcm_tag tag,
        RU8* pVal
    );

RBOOL
    rSequence_getRU16
    (
        rSequence seq,
        rpcm_tag tag,
        RU16* pVal
    );

RBOOL
    rSequence_getRU32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32* pVal
    );

RBOOL
    rSequence_getRU64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rSequence_getDOUBLE
    (
        rSequence seq,
        rpcm_tag tag,
        RDOUBLE* pVal
    );

RBOOL
    rSequence_getSTRINGA
    (
        rSequence seq,
        rpcm_tag tag,
        RPCHAR* pVal
    );

RBOOL
    rSequence_getSTRINGW
    (
        rSequence seq,
        rpcm_tag tag,
        RPWCHAR* pVal
    );

RBOOL
    rSequence_getBUFFER
    (
        rSequence seq,
        rpcm_tag tag,
        RPU8* pVal,
        RU32* pValSize
    );

RBOOL
    rSequence_getTIMESTAMP
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rSequence_getIPV4
    (
        rSequence seq,
        rpcm_tag tag,
        RU32* pVal
    );

RBOOL
    rSequence_getIPV6
    (
        rSequence seq,
        rpcm_tag tag,
        RU8* pVal
    );

RBOOL
    rSequence_getPOINTER32
    (
        rSequence seq,
        rpcm_tag tag,
        RU32* pVal
    );

RBOOL
    rSequence_getPOINTER64
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rSequence_getTIMEDELTA
    (
        rSequence seq,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rSequence_getSEQUENCE
    (
        rSequence seq,
        rpcm_tag tag,
        rSequence* pVal
    );

RBOOL
    rSequence_getLIST
    (
        rSequence seq,
        rpcm_tag tag,
        rList* pVal
    );

RBOOL
    rList_getElement
    (
        rList list,
        rpcm_tag* tag,
        rpcm_type* type,
        RPVOID pElem,
        RU32* elemSize
    );

RBOOL
    rList_getRU8
    (
        rList list,
        rpcm_tag tag,
        RU8* pVal
    );

RBOOL
    rList_getRU16
    (
        rList list,
        rpcm_tag tag,
        RU16* pVal
    );

RBOOL
    rList_getRU32
    (
        rList list,
        rpcm_tag tag,
        RU32* pVal
    );

RBOOL
    rList_getRU64
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rList_getDOUBLE
    (
        rList list,
        rpcm_tag tag,
        RDOUBLE* pVal
    );

RBOOL
    rList_getSTRINGA
    (
        rList list,
        rpcm_tag tag,
        RPCHAR* pVal
    );

RBOOL
    rList_getSTRINGW
    (
        rList list,
        rpcm_tag tag,
        RPWCHAR* pVal
    );

RBOOL
    rList_getBUFFER
    (
        rList list,
        rpcm_tag tag,
        RU8* pVal,
        RU32* pValSize
    );

RBOOL
    rList_getTIMESTAMP
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rList_getIPV4
    (
        rList list,
        rpcm_tag tag,
        RU32* pVal
    );

RBOOL
    rList_getIPV6
    (
        rList list,
        rpcm_tag tag,
        RU8* pVal
    );

RBOOL
    rList_getPOINTER32
    (
        rList list,
        rpcm_tag tag,
        RU32* pVal
    );

RBOOL
    rList_getPOINTER64
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rList_getTIMEDELTA
    (
        rList list,
        rpcm_tag tag,
        RU64* pVal
    );

RBOOL
    rList_getSEQUENCE
    (
        rList list,
        rpcm_tag tag,
        rSequence* pVal
    );

RBOOL
    rList_getLIST
    (
        rSequence seq,
        rpcm_tag tag,
        rList* pVal
    );

// REMOVE / OTHER
//-----------------------------------------------------------------------------
RBOOL
    rSequence_removeElement
    (
        rSequence seq,
        rpcm_tag tag,
        rpcm_type type
    );

RBOOL
    rList_resetIterator
    (
        rList list
    );

#define rSequence_duplicate( orig )     rSequence_duplicate_from( orig, RPAL_LINE_SUBTAG )
rSequence
    rSequence_duplicate_from
    (
        rSequence original,
        RU32 from
    );

#define rList_duplicate( orig )         rList_duplicate_from( orig, RPAL_LINE_SUBTAG )
rList
    rList_duplicate_from
    (
        rList original,
        RU32 from
    );

// SERIALIZE / DESERIALIZE
//-----------------------------------------------------------------------------
RBOOL
    rSequence_serialise
    (
        rSequence seq,
        rBlob blob
    );

#define rSequence_deserialise( pSeq, buffer, bufferSize, pBytesConsumed )   rSequence_deserialise_from( pSeq, buffer, bufferSize, pBytesConsumed, RPAL_LINE_SUBTAG )
RBOOL
    rSequence_deserialise_from
    (
        rSequence* pSeq,
        RPU8 buffer,
        RU32 bufferSize,
        RU32* pBytesConsumed,
        RU32 from
    );

RBOOL
    rList_serialise
    (
        rList list,
        rBlob blob
    );

#define rList_deserialise( pSeq, buffer, bufferSize, pBytesConsumed )   rList_deserialise_from( pSeq, buffer, bufferSize, pBytesConsumed, RPAL_LINE_SUBTAG )
RBOOL
    rList_deserialise_from
    (
        rList* pList,
        RPU8 buffer,
        RU32 bufferSize,
        RU32* pBytesConsumed,
        RU32 from
    );


// OTHER
//-----------------------------------------------------------------------------
RBOOL
    rList_setInfo
    (
        rList list,
        rpcm_tag elemTags,
        rpcm_type elemTypes
    );

RU32
    rList_getNumElements
    (
        rList list
    );

RBOOL
    rpcm_isElemEqual
    (
        rpcm_type type,
        RPVOID pElem1,
        RU32 elem1Size,
        RPVOID pElem2,
        RU32 elem2Size
    );

RBOOL
    rSequence_isEqual
    (
        rSequence seq1,
        rSequence seq2
    );

RBOOL
    rList_isEqual
    (
        rList list1,
        rList list2
    );

RVOID
    rSequence_unTaintRead
    (
        rSequence seq
    );

RVOID
    rList_unTaintRead
    (
        rList list
    );

RU32
    rSequence_getEstimateSize
    (
        rSequence seq
    );

RU32
    rList_getEstimateSize
    (
        rList list
    );

#endif
