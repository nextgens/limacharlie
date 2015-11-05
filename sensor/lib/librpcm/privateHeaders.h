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

#ifndef _LIB_RPCM_PRIV_H
#define _LIB_RPCM_PRIV_H

#include <rpal/rpal.h>

//=============================================================================
//  Private Structures
//=============================================================================
#pragma pack(push)
#pragma pack(1)


typedef struct
{
    rpcm_tag    tag;
    rpcm_type   type;

} _ElemHeader, *_PElemHeader;

typedef struct
{
    _ElemHeader commonHeader;
    RU32        size;
    RU8         data[];

} _ElemVarHeader, *_PElemVarHeader;

typedef struct
{
    _ElemHeader     commonHeader;
    RU8             data[];

} _ElemSimpleHeader, *_PElemSimpleHeader;

typedef struct
{
    _ElemHeader     commonHeader;
    RPVOID          pComplexElement;

} _ElemComplexHeader, *_PElemComplexHeader;

typedef struct
{
    RU32        nElements;
    rpcm_tag    tag;
    rBlob       blob;
#ifdef RPAL_PLATFORM_DEBUG
    RBOOL       isReadTainted;
#endif

} _ElementSet, *_PElementSet;

typedef struct
{
    _ElementSet set;

} _rSequence;

typedef struct
{
    _ElementSet set;
    rpcm_tag    elemTags;
    rpcm_type   elemTypes;
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// The fix below should be applied at the next major release of HCP and HBS
// at the same time. Otherwise platforms with alignment issues may be problematic.
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    RU8         unusedPadding1;
//    RU16        unusedPadding2;
//#if defined(RPAL_PLATFORM_64_BIT) && defined(RPAL_PLATFORM_DEBUG)
//    RU32        unusedPadding3;
//#endif
    RPVOID      lastElem;

} _rList;

typedef struct
{
    RU32 index;
    _PElementSet set;

} _rIterator;


#pragma pack(pop)
//=============================================================================
//  Private Helpers
//=============================================================================
RBOOL
    isElemSimple
    (
        _PElemHeader header
    );

RBOOL
    isElemVariable
    (
        _PElemHeader header
    );

RBOOL
    isElemComplex
    (
        _PElemHeader header
    );

RU32
    getElemSize
    (
        _PElemHeader header,
        RBOOL isNetworkOrder
    );

RU32
    getHeaderSize
    (
        _PElemHeader header
    );

_PElemHeader
    nextElemInSet
    (
        _PElemHeader header,
        RBOOL isNetworkOrder
    );

RBOOL
    freeElement
    (
        _PElemHeader header
    );

_rIterator*
    newIterator
    (
        _PElementSet set
    );

RVOID
    freeIterator
    (
        _rIterator* ite
    );

_PElemHeader
    iteratorNext
    (
        _rIterator* ite
    );

RBOOL
    initSet
    (
        _PElementSet set,
		RU32 from
    );

RBOOL
    freeSet
    (
        _PElementSet set
    );

RBOOL
    freeShallowSet
    (
        _PElementSet set
    );

RBOOL
    set_addElement
    (
        _PElementSet set,
        rpcm_tag tag,
        rpcm_type type,
        RPVOID pElem,
        RU32 elemSize
    );

RBOOL
    set_getElement
    (
        _PElementSet set,
        rpcm_tag* tag,
        rpcm_type* type,
        RPVOID pElem,
        RU32* pElemSize,
        RPVOID* pAfterThis,
        RBOOL isGetRawPtr
    );

RBOOL
    set_removeElement
    (
        _PElementSet set,
        rpcm_tag tag,
        rpcm_type type
    );

RBOOL
    set_deserialise
    (
        _PElementSet set,
        RPU8 buffer,
        RU32 bufferSize,
        RU32* pBytesConsumed
    );

RBOOL
    set_serialise
    (
        _PElementSet set,
        rBlob blob
    );

RBOOL
    isTagInSet
    (
        _PElementSet set,
        rpcm_tag tag
    );


RBOOL
    set_duplicate
    (
        _PElementSet original,
        _PElementSet newSet
    );


#endif

