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

#define RPAL_FILE_ID    36

#ifdef RPAL_PLATFORM_WINDOWS
#pragma warning( disable: 4127 ) // Disabling error on constant expression in condition
#endif

//=============================================================================
//  Private Macros
//=============================================================================
#define RPCM_IPV6_SIZE  16

//=============================================================================
//  Private Prototypes
//=============================================================================

//=============================================================================
//  Housekeeping
//=============================================================================
RBOOL
    isElemSimple
    (
        _PElemHeader header
    )
{
    RBOOL isExpected = FALSE;

    if( NULL != header )
    {
        switch( header->type )
        {
            case RPCM_RU8:
            case RPCM_RU16:
            case RPCM_RU32:
            case RPCM_RU64:
            case RPCM_POINTER_32:
            case RPCM_POINTER_64:
            case RPCM_TIMEDELTA:
            case RPCM_TIMESTAMP:
            case RPCM_IPV4:
            case RPCM_IPV6:
                isExpected = TRUE;
                break;
        }
    }

    return isExpected;
}

RBOOL
    isElemVariable
    (
        _PElemHeader header
    )
{
    RBOOL isExpected = FALSE;

    if( NULL != header )
    {
        switch( header->type )
        {
            case RPCM_STRINGA:
            case RPCM_STRINGW:
            case RPCM_BUFFER:
                isExpected = TRUE;
                break;
        }
    }

    return isExpected;
}

RBOOL
    isElemComplex
    (
        _PElemHeader header
    )
{
    RBOOL isExpected = FALSE;

    if( NULL != header )
    {
        if( header->type >= RPCM_COMPLEX_TYPES )
        {
            isExpected = TRUE;
        }
    }

    return isExpected;
}

RU32
    getElemSize
    (
        _PElemHeader header,
        RBOOL isNetworkOrder
    )
{
    RU32 size = 0;

    if( NULL != header )
    {
        if( isElemComplex( header ) )
        {
            size = 0;
        }
        else
        {
            switch( header->type )
            {
                case RPCM_RU8:
                    size = sizeof(RU8);
                    break;
                case RPCM_RU16:
                    size = sizeof(RU16);
                    break;
                case RPCM_RU32:
                    size = sizeof(RU32);
                    break;
                case RPCM_RU64:
                    size = sizeof(RU64);
                    break;
                case RPCM_POINTER_32:
                    size = sizeof(RU32);
                    break;
                case RPCM_POINTER_64:
                case RPCM_TIMESTAMP:
                case RPCM_TIMEDELTA:
                    size = sizeof(RU64);
                    break;
                case RPCM_IPV4:
                    size = sizeof(RU32);
                    break;
                case RPCM_IPV6:
                    size = RPCM_IPV6_SIZE;
                    break;
                case RPCM_STRINGA:
                case RPCM_STRINGW:
                case RPCM_BUFFER:
                    if( isNetworkOrder )
                    {
                        size += rpal_ntoh32( ((_PElemVarHeader)header)->size );
                    }
                    else
                    {
                        size += ((_PElemVarHeader)header)->size;
                    }
                    break;
            }
        }
    }

    return size;
}

RU32
    getHeaderSize
    (
        _PElemHeader header
    )
{
    RU32 size = 0;

    if( NULL != header )
    {
        if( isElemSimple( header ) )
        {
            size = sizeof(_ElemSimpleHeader);
        }
        else if( isElemVariable( header ) )
        {
            size = sizeof(_ElemVarHeader);
        }
        else if( isElemComplex( header ) )
        {
            size = sizeof(_ElemComplexHeader);
        }
    }

    return size;
}

_PElemHeader
    nextElemInSet
    (
        _PElemHeader header,
        RBOOL isNetworkOrder
    )
{
    RPU8 newHeader = NULL;
    RU32 elemSize = 0;

    if( NULL != header )
    {
        elemSize = getHeaderSize( header );

        if( 0 != elemSize )
        {
            elemSize += getElemSize( header, isNetworkOrder );
            newHeader = (RPU8)header + elemSize;
        }
    }

    return (_PElemHeader)newHeader;
}

RBOOL
    freeElement
    (
        _PElemHeader header
    )
{
    RBOOL isClean = FALSE;

    if( NULL != header )
    {
        isClean = TRUE;

        if( isElemComplex( header ) )
        {
            isClean = freeSet( ((_PElemComplexHeader)header)->pComplexElement );

            if( isClean )
            {
                rpal_memory_free( ((_PElemComplexHeader)header)->pComplexElement );
            }
        }
    }

    return isClean;
}


#define newIterator( set )    newIterator_from( set, RPAL_LINE_SUBTAG )
_rIterator*
    newIterator_from
    (
        _PElementSet set,
        RU32 from
    )
{
    _rIterator* ite = NULL;

    if( NULL != set )
    {
        ite = rpal_memory_alloc_from( sizeof( _rIterator ), from );

        if( rpal_memory_isValid( ite ) )
        {
            ite->index = 0;
            ite->set = set;
        }
    }

    return ite;
}

RVOID
    freeIterator
    (
        _rIterator* ite
    )
{
    if( rpal_memory_isValid( ite ) )
    {
        rpal_memory_free( ite );
    }
}

_PElemHeader
    iteratorNext
    (
        _rIterator* ite
    )
{
    RPU8 buff = NULL;
    RU32 buffSize = 0;
    _PElemHeader next = NULL;

    if( NULL != ite )
    {
        buff = rpal_blob_getBuffer( ite->set->blob );
        buffSize = rpal_blob_getSize( ite->set->blob );

        next = (_PElemHeader)( buff + ite->index );

        if( IS_WITHIN_BOUNDS( next, sizeof(_ElemHeader), buff, buffSize ) &&
            0 != getHeaderSize( next ) )
        {
            if( IS_WITHIN_BOUNDS( next, 
                                  getHeaderSize( next ) + getElemSize( next, FALSE ),
                                  buff,
                                  buffSize ) )
            {
                
                ite->index = (RU32)((RPU8)nextElemInSet( next, FALSE ) - buff);
            }
            else
            {
                next = NULL;
            }
        }
        else
        {
            next = NULL;
        }
    }

    return next;
}

RBOOL
    initSet
    (
        _PElementSet set,
		RU32 from
    )
{
    RBOOL isInit = FALSE;

    if( NULL != set )
    {
        set->nElements = 0;
        set->tag = RPCM_INVALID_TAG;
        set->blob = rpal_blob_create_from( 0, 0, from );
#ifdef RPAL_PLATFORM_DEBUG
        set->isReadTainted = FALSE;
#endif
        if( rpal_memory_isValid( set->blob ) )
        {
            isInit = TRUE;
        }
    }

    return isInit;
}


RBOOL
    _freeSet
    (
        _PElementSet set,
        RBOOL isShallow
    )
{
    RBOOL isClean = FALSE;

    RU32 numFreed = 0;
    _rIterator* ite = NULL;
    _PElemHeader tmpHeader = NULL;

    if( NULL != set )
    {
        if( NULL != ( ite = newIterator( set ) ) )
        {
            isClean = TRUE;

            while( NULL != ( tmpHeader = iteratorNext( ite ) ) )
            {
                if( !isShallow ||
                    !isElemComplex( tmpHeader ) )
                {
                    if( !freeElement( tmpHeader ) )
                    {
                        isClean = FALSE;
                        break;
                    }
                }

                numFreed++;
            }

            if( numFreed != set->nElements )
            {
                isClean = FALSE;
                rpal_debug_break();
            }

            freeIterator( ite );
        }
        
        if( isClean )
        {
            rpal_blob_free( set->blob );
        }
    }

    return isClean;
}


RBOOL
    freeSet
    (
        _PElementSet set
    )
{
    return _freeSet( set, FALSE );
}


RBOOL
    freeShallowSet
    (
        _PElementSet set
    )
{
    return _freeSet( set, TRUE );
}


RBOOL
    set_addElement
    (
        _PElementSet set,
        rpcm_tag tag,
        rpcm_type type,
        RPVOID pElem,
        RU32 elemSize
    )
{
    RBOOL isSuccess = FALSE;

    _ElemHeader header = {0};
    _ElemSimpleHeader simpleHeader = {0};
    _ElemVarHeader varHeader = {0};
    _ElemComplexHeader complexHeader = {0};

    RU16 tmpRU16 = 0;
    RU32 tmpRU32 = 0;
    RU64 tmpRU64 = 0;

    if( NULL != set &&
        NULL != pElem )
    {
        header.tag = tag;
        header.type = type;

        if( isElemSimple( &header ) )
        {
#ifdef RPAL_PLATFORM_DEBUG
            if( set->isReadTainted )
            {
                rpal_debug_critical( "ADD operation of an RPCM structure that was READ from before. This is potentially invalidating previously acquired pointers." );
            }
#endif
            simpleHeader.commonHeader = header;
            
            if( rpal_blob_add( set->blob, &simpleHeader, sizeof( simpleHeader ) ) )
            {
                switch( type )
                {
                    case RPCM_RU8:
                        isSuccess = rpal_blob_add( set->blob, pElem, sizeof( RU8 ) );
                        break;
                    case RPCM_RU16:
                        tmpRU16 = *(RPU16)pElem;
                        isSuccess = rpal_blob_add( set->blob, &tmpRU16, sizeof( RU16 ) );
                        break;
                    case RPCM_RU32:
                    case RPCM_IPV4:
                    case RPCM_POINTER_32:
                        tmpRU32 = *(RPU32)pElem;
                        isSuccess = rpal_blob_add( set->blob, &tmpRU32, sizeof( RU32 ) );
                        break;
                    case RPCM_RU64:
                    case RPCM_TIMESTAMP:
                    case RPCM_POINTER_64:
                    case RPCM_TIMEDELTA:
                        tmpRU64 = *(RPU64)pElem;
                        isSuccess = rpal_blob_add( set->blob, &tmpRU64, sizeof( RU64 ) );
                        break;
                    case RPCM_IPV6:
                        isSuccess = rpal_blob_add( set->blob, pElem, RPCM_IPV6_SIZE );
                        break;
                }
            }
        }
        else if( isElemVariable( &header ) )
        {
#ifdef RPAL_PLATFORM_DEBUG
            if( set->isReadTainted )
            {
                rpal_debug_critical( "ADD operation of an RPCM structure that was READ from before. This is potentially invalidating previously acquired pointers." );
            }
#endif
            varHeader.commonHeader = header;

            switch( type )
            {
                case RPCM_STRINGA:
                    varHeader.size = ( rpal_string_strlen( (RPCHAR)pElem ) + 1 ) * sizeof( RCHAR );
                    break;
                case RPCM_STRINGW:
                    varHeader.size = ( rpal_string_strlenw( (RPWCHAR)pElem ) + 1 ) * sizeof( RWCHAR );
                    break;
                case RPCM_BUFFER:
                    varHeader.size = elemSize;
                    break;
            }

            if( rpal_blob_add( set->blob, &varHeader, sizeof( varHeader ) ) &&
                rpal_blob_add( set->blob, pElem, varHeader.size ) )
            {
                isSuccess = TRUE;
            }
        }
        else if( isElemComplex( &header ) )
        {
            complexHeader.commonHeader = header;
            complexHeader.pComplexElement = pElem;
            
            if( rpal_blob_add( set->blob, &complexHeader, sizeof( complexHeader ) ) )
            {
                isSuccess = TRUE;
            }
        }

        if( isSuccess )
        {
            set->nElements++;
        }
    }

    return isSuccess;
}


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
    )
{
    RBOOL isSuccess = FALSE;

    _rIterator* ite = NULL;
    _PElemHeader elem = NULL;
    _PElemSimpleHeader simpleElem = NULL;
    _PElemVarHeader varElem = NULL;
    _PElemComplexHeader complexElem = NULL;

    if( NULL != set )
    {
        ite = newIterator( set );

        if( rpal_memory_isValid( ite ) )
        {
            while( NULL != ( elem = iteratorNext( ite ) ) )
            {
                if( ( NULL == tag || RPCM_INVALID_TAG == *tag || *tag == elem->tag ) &&
                    ( ( ( NULL == type || RPCM_INVALID_TYPE == *type ) && ( isGetRawPtr || NULL == pElem ) ) || *type == elem->type ) )
                {
                    isSuccess = TRUE;

                    if( NULL != tag )
                    {
                        *tag = elem->tag;
                    }

                    if( NULL != type )
                    {
                        *type = elem->type;
                    }

                    if( isElemSimple( elem ) )
                    {
                        simpleElem = (_PElemSimpleHeader)elem;

                        if( NULL == pAfterThis ||
                            NULL == *pAfterThis )
                        {
                            if( !isGetRawPtr )
                            {
                                // I just want to get the data copied to a local variable
                                switch( simpleElem->commonHeader.type )
                                {
                                    case RPCM_RU8:
                                        if( NULL != pElem )
                                        {
                                            *((RU8*)pElem) = *(RU8*)simpleElem->data;
                                        }
                                        break;
                                    case RPCM_RU16:
                                        if( NULL != pElem )
                                        {
                                            *((RU16*)pElem) = *(RU16*)simpleElem->data;
                                        }
                                        break;
                                    case RPCM_RU32:
                                    case RPCM_IPV4:
                                    case RPCM_POINTER_32:
                                        if( NULL != pElem )
                                        {
                                            *((RU32*)pElem) = *(RU32*)simpleElem->data ;
                                        }
                                        break;
                                    case RPCM_RU64:
                                    case RPCM_TIMESTAMP:
                                    case RPCM_POINTER_64:
                                    case RPCM_TIMEDELTA:
                                        if( NULL != pElem )
                                        {
                                            *((RU64*)pElem) = *(RU64*)simpleElem->data;
                                        }
                                        break;
                                    case RPCM_IPV6:
                                        if( NULL != pElem )
                                        {
                                            rpal_memory_memcpy( pElem, simpleElem->data, RPCM_IPV6_SIZE );
                                        }
                                        break;
                                }
                            }
                            else if( NULL != pElem )
                            {
                                // I want the raw pointer
                                *(RPVOID*)pElem = simpleElem->data;

#ifdef RPAL_PLATFORM_DEBUG
                                set->isReadTainted = TRUE;
#endif
                            }

                            if( NULL != pAfterThis )
                            {
                                *pAfterThis = simpleElem->data;
                            }
                        }
                        else if( *pAfterThis == simpleElem->data )
                        {
                            *pAfterThis = NULL;
                            isSuccess = FALSE;
                        }
                        else
                        {
                            isSuccess = FALSE;
                        }
                    }
                    else if( isElemVariable( elem ) )
                    {
                        varElem = (_PElemVarHeader)elem;

                        if( NULL == pAfterThis ||
                            NULL == *pAfterThis )
                        {
                            if( NULL != pElem )
                            {
                                *(RPCHAR*)pElem = (RPCHAR)varElem->data;
#ifdef RPAL_PLATFORM_DEBUG
                                set->isReadTainted = TRUE;
#endif
                            }

                            if( NULL != pAfterThis )
                            {
                                *pAfterThis = varElem->data;
                            }
                        }
                        else if( *pAfterThis == varElem->data )
                        {
                            //*pAfterThis = varElem->data;
                            *pAfterThis = NULL;
                            isSuccess = FALSE;
                        }
                        else
                        {
                            isSuccess = FALSE;
                        }
                    }
                    else if( isElemComplex( elem ) )
                    {
                        complexElem = (_PElemComplexHeader)elem;

                        if( NULL == pAfterThis ||
                            NULL == *pAfterThis )
                        {
                            if( NULL != pElem )
                            {
                                *(RPVOID*)pElem = (RPCHAR)complexElem->pComplexElement;
                            }

                            if( NULL != pAfterThis )
                            {
                                *pAfterThis = complexElem->pComplexElement;
                            }
                        }
                        else if( *pAfterThis == complexElem->pComplexElement )
                        {
                            *pAfterThis = NULL;
                            isSuccess = FALSE;
                        }
                        else
                        {
                            isSuccess = FALSE;
                        }
                    }
                    else
                    {
                        isSuccess = FALSE;
                        break;
                    }

                    if( isSuccess )
                    {
                        if( NULL != pElemSize )
                        {
                            *pElemSize = getElemSize( elem, FALSE );
                        }
                        break;
                    }
                }
            }

            freeIterator( ite );
        }
    }

    return isSuccess;
}


RBOOL
    set_removeElement
    (
        _PElementSet set,
        rpcm_tag tag,
        rpcm_type type
    )
{
    RBOOL isSuccess = FALSE;

    _rIterator* ite = NULL;
    _PElemHeader elem = NULL;
    _PElemSimpleHeader simpleElem = NULL;
    _PElemVarHeader varElem = NULL;
    _PElemComplexHeader complexElem = NULL;
    RU32 sizeToRemove = 0;

    if( NULL != set )
    {
        ite = newIterator( set );

        if( rpal_memory_isValid( ite ) )
        {
            while( NULL != ( elem = iteratorNext( ite ) ) )
            {
                if( ( RPCM_INVALID_TAG == tag || tag == elem->tag ) &&
                    ( RPCM_INVALID_TYPE == type || type == elem->type ) )
                {
                    isSuccess = TRUE;

                    if( isElemSimple( elem ) )
                    {
                        simpleElem = (_PElemSimpleHeader)elem;

                        switch( simpleElem->commonHeader.type )
                        {
                            case RPCM_RU8:
                                sizeToRemove = sizeof( RU8 ) + sizeof( *simpleElem );
                                break;
                            case RPCM_RU16:
                                sizeToRemove = sizeof( RU16 ) + sizeof( *simpleElem );
                                break;
                            case RPCM_RU32:
                            case RPCM_IPV4:
                            case RPCM_POINTER_32:
                                sizeToRemove = sizeof( RU32 ) + sizeof( *simpleElem );
                                break;
                            case RPCM_RU64:
                            case RPCM_TIMESTAMP:
                            case RPCM_POINTER_64:
                            case RPCM_TIMEDELTA:
                                sizeToRemove = sizeof( RU64 ) + sizeof( *simpleElem );
                                break;
                            case RPCM_IPV6:
                                sizeToRemove = RPCM_IPV6_SIZE + sizeof( *simpleElem );
                                break;
                        }
                    }
                    else if( isElemVariable( elem ) )
                    {
                        varElem = (_PElemVarHeader)elem;

                        sizeToRemove = varElem->size + sizeof( *varElem );
                    }
                    else if( isElemComplex( elem ) )
                    {
                        complexElem = (_PElemComplexHeader)elem;

                        sizeToRemove = sizeof( *complexElem );

                        if( RPCM_LIST == complexElem->commonHeader.type )
                        {
                            rList_free( complexElem->pComplexElement );
                        }
                        else if( RPCM_COMPLEX_TYPES <= complexElem->commonHeader.type )
                        {
                            rSequence_free( complexElem->pComplexElement );
                        }
                    }
                    else
                    {
                        isSuccess = FALSE;
                        break;
                    }

                    if( isSuccess )
                    {
                        set->nElements--;
                        isSuccess = rpal_blob_remove( set->blob, (RU32)((RPU8)elem - (RPU8)rpal_blob_getBuffer( set->blob )), sizeToRemove );
                        break;
                    }
                }
            }

            freeIterator( ite );
        }
    }

    return isSuccess;
}




RBOOL
    set_deserialise
    (
        _PElementSet set,
        RPU8 buffer,
        RU32 bufferSize,
        RU32* pBytesConsumed
    )
{
    RBOOL isSuccess = FALSE;

    _PElemHeader header = NULL;
    _PElemSimpleHeader simpleHeader = NULL;
    _PElemVarHeader varHeader = NULL;
    _PElemComplexHeader complexHeader = NULL;

    _rSequence* tmpSeq = NULL;
    _rList* tmpList = NULL;

    RU16 tmp16 = 0;
    RU32 tmp32 = 0;
    RU64 tmp64 = 0;

    RPWCHAR tmpWStr = NULL;

    RU32 numElement = 0;
    RU32 curElem = 0;

    RU32 bytesConsumed = 0;

    if( NULL != set &&
        NULL != buffer &&
        sizeof( RU32 ) <= bufferSize )
    {
        numElement = rpal_ntoh32( *(RU32*)buffer );

        header = (_PElemHeader)(buffer + sizeof( RU32 ));

        if( 0 == numElement )
        {
            // Empty Sets
            isSuccess = TRUE;
        }

        while( IS_WITHIN_BOUNDS( header, sizeof(*header), buffer, bufferSize ) &&
               curElem < numElement )
        {
            curElem++;
            bytesConsumed = 0;

            if( isElemSimple( header ) )
            {
                simpleHeader = (_PElemSimpleHeader)header;

                switch( header->type )
                {
                    case RPCM_RU8:
                        isSuccess = set_addElement( set,
                                                    rpal_ntoh32( simpleHeader->commonHeader.tag ),
                                                    simpleHeader->commonHeader.type,
                                                    (RU8*)simpleHeader->data,
                                                    sizeof( RU8 ) );
                        break;
                    case RPCM_RU16:
                        tmp16 = rpal_ntoh16( *(RU16*)simpleHeader->data );
                        isSuccess = set_addElement( set,
                                                    rpal_ntoh32( simpleHeader->commonHeader.tag ),
                                                    simpleHeader->commonHeader.type,
                                                    &tmp16,
                                                    sizeof( RU16 ) );
                        break;
                    case RPCM_RU32:
                    case RPCM_IPV4:
                    case RPCM_POINTER_32:
                        tmp32 = rpal_ntoh32( *(RU32*)simpleHeader->data );
                        isSuccess = set_addElement( set,
                                                    rpal_ntoh32( simpleHeader->commonHeader.tag ),
                                                    simpleHeader->commonHeader.type,
                                                    &tmp32,
                                                    sizeof( RU32 ) );
                        break;
                    case RPCM_RU64:
                    case RPCM_TIMESTAMP:
                    case RPCM_POINTER_64:
                    case RPCM_TIMEDELTA:
                        tmp64 = rpal_ntoh64( *(RU64*)simpleHeader->data );
                        isSuccess = set_addElement( set,
                                                    rpal_ntoh32( simpleHeader->commonHeader.tag ),
                                                    simpleHeader->commonHeader.type,
                                                    &tmp64,
                                                    sizeof( RU64 ) );
                        break;
                    case RPCM_IPV6:
                        isSuccess = set_addElement( set,
                                                    rpal_ntoh32( simpleHeader->commonHeader.tag ),
                                                    simpleHeader->commonHeader.type,
                                                    (RU8*)simpleHeader->data,
                                                    RPCM_IPV6_SIZE );
                        break;
                }
            }
            else if( isElemVariable( header ) )
            {
                varHeader = (_PElemVarHeader)header;

                if( RPCM_STRINGW == varHeader->commonHeader.type )
                {
                    // Wide strings are a special case, they are serialised as a utf-8 string
                    // so we need to convert them back to wide before adding to the set.
                    tmpWStr = rpal_string_atow( (RPCHAR)varHeader->data );

                    if( NULL != tmpWStr )
                    {
                        isSuccess = set_addElement( set, 
                                                    rpal_ntoh32( varHeader->commonHeader.tag ),
                                                    varHeader->commonHeader.type,
                                                    tmpWStr,
                                                    ( rpal_string_strlenw( tmpWStr ) + 1 ) * sizeof( RWCHAR ) );

                        rpal_memory_free( tmpWStr );
                    }
                    else
                    {
                        isSuccess = FALSE;
                    }
                }
                else
                {
                    isSuccess = set_addElement( set, 
                                                rpal_ntoh32( varHeader->commonHeader.tag ),
                                                varHeader->commonHeader.type,
                                                varHeader->data,
                                                rpal_ntoh32( varHeader->size ) );
                }
            }
            else if( isElemComplex( header ) )
            {
                complexHeader = (_PElemComplexHeader)header;

                if( RPCM_SEQUENCE == complexHeader->commonHeader.type )
                {
                    isSuccess = rSequence_deserialise( (rSequence*)&tmpSeq, 
                                                       (RPU8)&(complexHeader->pComplexElement), 
                                                       (RU32)((buffer + bufferSize) - (RPU8)&(complexHeader->pComplexElement)),
                                                       &bytesConsumed );

                    if( isSuccess )
                    {
                        isSuccess = set_addElement( set,
                                                    rpal_ntoh32( complexHeader->commonHeader.tag ), 
                                                    RPCM_SEQUENCE, 
                                                    tmpSeq, 
                                                    sizeof( tmpSeq ) );
                    }
                }
                else if( RPCM_LIST == complexHeader->commonHeader.type )
                {
                    isSuccess = rList_deserialise( (rList*)&tmpList, 
                                                   (RPU8)&(complexHeader->pComplexElement), 
                                                   (RU32)((buffer + bufferSize) - (RPU8)&(complexHeader->pComplexElement)),
                                                   &bytesConsumed );

                    if( isSuccess )
                    {
                        isSuccess = set_addElement( set, 
                                                    rpal_ntoh32( complexHeader->commonHeader.tag ), 
                                                    RPCM_LIST, 
                                                    tmpList, 
                                                    sizeof( tmpList ) );
                    }
                }
            }
            else
            {
                isSuccess = FALSE;
                break;
            }

            if( 0 == bytesConsumed )
            {
                header = nextElemInSet( header, TRUE );
            }
            else
            {
                header = (_PElemHeader)((RPU8)header + bytesConsumed + sizeof(_ElemHeader) );
            }
        }

        if( !isSuccess )
        {
            freeSet( set );
        }
        else
        {
            if( NULL != pBytesConsumed )
            {
                *pBytesConsumed = (RU32)((RPU8)header - buffer);
            }
        }
    }

    return isSuccess;
}

RBOOL
    set_serialise
    (
        _PElementSet set,
        rBlob blob
    )
{
    RBOOL isSuccess = FALSE;

    _PElemHeader header = NULL;
    _PElemSimpleHeader simpleHeader = NULL;
    _PElemVarHeader varHeader = NULL;
    _PElemComplexHeader complexHeader = NULL;
    _rIterator* ite = NULL;

    RU32 tmp32_1 = 0;
    RU32 tmp32_2 = 0;

    RU16 tmpRU16 = 0;
    RU32 tmpRU32 = 0;
    RU64 tmpRU64 = 0;

    RPCHAR tmpStr = NULL;

    if( NULL != set &&
        NULL != blob )
    {
        ite = newIterator( set );

        if( rpal_memory_isValid( ite ) )
        {
            tmp32_1 = rpal_hton32( set->nElements );

            if( rpal_blob_add( blob, &(tmp32_1), sizeof( RU32 ) ) )
            {
                isSuccess = TRUE;

                while( NULL != ( header = iteratorNext( ite ) ) )
                {
                    tmp32_1 = rpal_hton32( header->tag );
                    
                    if( !rpal_blob_add( blob, &tmp32_1, sizeof(RU32) ) ||
                        !rpal_blob_add( blob, &(header->type), sizeof(header->type) ) )
                    {
                        isSuccess = FALSE;
                        break;
                    }

                    if( isElemSimple( header ) )
                    {
                        simpleHeader = (_PElemSimpleHeader)header;

                        switch( header->type )
                        {
                            case RPCM_RU8:
                                isSuccess = rpal_blob_add( blob, simpleHeader->data, sizeof( RU8 ) );
                                break;
                            case RPCM_RU16:
                                tmpRU16 = rpal_hton16( *(RPU16)simpleHeader->data );
                                isSuccess = rpal_blob_add( blob, &tmpRU16, sizeof( RU16 ) );
                                break;
                            case RPCM_RU32:
                            case RPCM_IPV4:
                            case RPCM_POINTER_32:
                                tmpRU32 = rpal_hton32( *(RPU32)simpleHeader->data );
                                isSuccess = rpal_blob_add( blob, &tmpRU32, sizeof( RU32 ) );
                                break;
                            case RPCM_RU64:
                            case RPCM_TIMESTAMP:
                            case RPCM_POINTER_64:
                            case RPCM_TIMEDELTA:
                                tmpRU64 = rpal_hton64( *(RPU64)simpleHeader->data );
                                isSuccess = rpal_blob_add( blob, &tmpRU64, sizeof( RU64 ) );
                                break;
                            case RPCM_IPV6:
                                isSuccess = rpal_blob_add( blob, simpleHeader->data, RPCM_IPV6_SIZE );
                                break;
                        }
                    }
                    else if( isElemVariable( header ) )
                    {
                        varHeader = (_PElemVarHeader)header;

                        if( RPCM_STRINGW == varHeader->commonHeader.type )
                        {
                            // Wide strings are a special case. They internally get formatted to 
                            // utf-8 to simplify cross-platform compatibility, no need to worry
                            // about endianness.
                            tmpStr = rpal_string_wtoa( (RPWCHAR)varHeader->data );

                            if( NULL != tmpStr )
                            {
                                tmp32_2 = ( rpal_string_strlen( tmpStr ) + 1 ) * sizeof( RCHAR );
                                tmp32_1 = rpal_hton32( tmp32_2 );

                                if( !rpal_blob_add( blob, &tmp32_1, sizeof( varHeader->size ) ) ||
                                    !rpal_blob_add( blob, tmpStr, tmp32_2 ) )
                                {
                                    isSuccess = FALSE;
                                    break;
                                }

                                rpal_memory_free( tmpStr );
                            }
                            else
                            {
                                isSuccess = FALSE;
                                break;
                            }
                        }
                        else
                        {
                            tmp32_1 = rpal_hton32( varHeader->size );

                            if( !rpal_blob_add( blob, &tmp32_1, sizeof( varHeader->size ) ) ||
                                !rpal_blob_add( blob, varHeader->data, varHeader->size ) )
                            {
                                isSuccess = FALSE;
                                break;
                            }
                        }
                    }
                    else if( isElemComplex( header ) )
                    {
                        complexHeader = (_PElemComplexHeader)header;

                        if( ( RPCM_SEQUENCE == header->type &&
                              !rSequence_serialise( complexHeader->pComplexElement, blob ) ) ||
                            ( RPCM_LIST == header->type &&
                              !rList_serialise( complexHeader->pComplexElement, blob )) )
                        {
                            isSuccess = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        isSuccess = FALSE;
                        break;
                    }
                }
            }

            freeIterator( ite );
        }
    }

    return isSuccess;
}

RBOOL
    isTagInSet
    (
        _PElementSet set,
        rpcm_tag tag
    )
{
    RBOOL isSuccess = FALSE;

    _rIterator ite = {0};
    _PElemHeader elem = NULL;

    if( NULL != set )
    {
        ite.index = 0;
        ite.set = set;

        while( NULL != ( elem = iteratorNext( &ite ) ) )
        {
            if( elem->tag == tag )
            {
                isSuccess = TRUE;
                break;
            }
        }
    }

    return isSuccess;
}

rIterator
    rIterator_new_from
    (
        RPVOID listOrSequence,
        RU32 from
    )
{
    return newIterator_from( listOrSequence, from );
}

RVOID
    rIterator_free
    (
        rIterator ite
    )
{
    freeIterator( ite );
}

RBOOL
    rIterator_next
    (
        rIterator ite,
        rpcm_tag* tag,
        rpcm_type* type,
        RPVOID* pElem,
        RU32* elemSize
    )
{
    RBOOL isSuccess = FALSE;

    _rIterator* pIte = NULL;
    _PElemHeader header = NULL;
    _PElemSimpleHeader simpleElem = NULL;
    _PElemVarHeader varElem = NULL;
    _PElemComplexHeader complexElem = NULL;

    if( rpal_memory_isValid( ite ) )
    {
        pIte = (_rIterator*)ite;

        if( NULL != ( header = iteratorNext( pIte ) ) )
        {
            isSuccess = TRUE;

            if( NULL != tag )
            {
                *tag = header->tag;
            }

            if( NULL != type )
            {
                *type = header->type;
            }

            if( isElemSimple( header ) )
            {
                simpleElem = (_PElemSimpleHeader)header;

                if( NULL != pElem )
                {
                    *pElem = simpleElem->data;
                }
            }
            else if( isElemVariable( header ) )
            {
                varElem = (_PElemVarHeader)header;

                if( NULL != pElem )
                {
                    *pElem = varElem->data;
                }
            }
            else if( isElemComplex( header ) )
            {
                complexElem = (_PElemComplexHeader)header;

                if( NULL != pElem )
                {
                    *pElem = complexElem->pComplexElement;
                }
            }
            else
            {
                isSuccess = FALSE;
            }

            if( isSuccess )
            {
                if( NULL != elemSize )
                {
                    *elemSize = getElemSize( header, FALSE );
                }
            }
        }
    }

    return isSuccess;
}

RBOOL
    set_duplicate
    (
        _PElementSet original,
        _PElementSet newSet
    )
{
    RBOOL isSuccess = FALSE;

    _rIterator* ite = NULL;
    _PElemHeader elem = NULL;
    _PElemComplexHeader complex = NULL;

    if( NULL != original &&
        NULL != newSet )
    {
        newSet->nElements = original->nElements;
        newSet->tag = original->tag;
        newSet->blob = rpal_blob_duplicate( original->blob );

        if( NULL != newSet->blob )
        {
            if( NULL != ( ite = newIterator( newSet ) ) )
            {
                isSuccess = TRUE;

                while( NULL != ( elem = iteratorNext( ite ) ) )
                {
                    complex = (_PElemComplexHeader)elem;

                    if( RPCM_LIST == elem->type )
                    {
                        complex->pComplexElement = rList_duplicate( complex->pComplexElement );

                        if( NULL == complex->pComplexElement )
                        {
                            isSuccess = FALSE;
                        }
                    }
                    else if( RPCM_COMPLEX_TYPES <= elem->type )
                    {
                        complex->pComplexElement = rSequence_duplicate( complex->pComplexElement );

                        if( NULL == complex->pComplexElement )
                        {
                            isSuccess = FALSE;
                        }
                    }
                }

                freeIterator( ite );

                if( !isSuccess )
                {
                    freeSet( newSet );
                }
            }
        }
    }

    return isSuccess;
}

RU32
    set_getEstimateSize
    (
        _PElementSet set
    )
{
    RU32 size = 0;
    _rIterator* ite = NULL;
    _PElemHeader elem = NULL;

    if( rpal_memory_isValid( set ) )
    {
        if( NULL != ( ite = newIterator( set ) ) )
        {
            while( NULL != ( elem = iteratorNext( ite ) ) )
            {
                if( RPCM_LIST == elem->type )
                {
                    size += rList_getEstimateSize( ( (_PElemComplexHeader)elem )->pComplexElement ) + sizeof( _ElemComplexHeader ) + sizeof( _rList );
                }
                else if( RPCM_COMPLEX_TYPES <= elem->type )
                {
                    size += rSequence_getEstimateSize( ( (_PElemComplexHeader)elem )->pComplexElement ) + sizeof( _ElemComplexHeader ) + sizeof( _rSequence );
                }
                else
                {
                    size += getElemSize( elem, FALSE ) + sizeof( _ElemSimpleHeader );
                }
            }

            freeIterator( ite );
        }
    }

    return size;
}

//=============================================================================
//  Public API
//=============================================================================
RBOOL
    set_isEqual
    (
        _PElementSet set1,
        _PElementSet set2
    )
{
    RBOOL isEqual = FALSE;
    rIterator ite = NULL;
    
    rpcm_tag tag = 0;
    rpcm_type type = 0;
    RPVOID p = NULL;
    RU32 s = 0;
    RPVOID elem = NULL;
    RU32 size = 0;
    RPVOID lastP = NULL;
    
    if( rpal_memory_isValid( set1 ) &&
        rpal_memory_isValid( set2 ) &&
        set1->nElements == set2->nElements )
    {
        if( NULL != ( ite = rIterator_new( set1 ) ) )
        {
            isEqual = TRUE;

            while( rIterator_next( ite, &tag, &type, &p, &s ) )
            {
                isEqual = FALSE;
                lastP = NULL;

                while( set_getElement( set2, &tag, &type, &elem, &size, &lastP, TRUE ) )
                {
                    if( rpcm_isElemEqual( type, p, s, elem, size ) )
                    {
                        isEqual = TRUE;
                        break;
                    }
                }

                if( !isEqual )
                {
                    break;
                }
            }

            rIterator_free( ite );

            if( isEqual )
            {
                isEqual = FALSE;

                if( NULL != ( ite = rIterator_new( set2 ) ) )
                {
                    isEqual = TRUE;

                    while( rIterator_next( ite, &tag, &type, &p, &s ) )
                    {
                        isEqual = FALSE;
                        lastP = NULL;

                        while( set_getElement( set1, &tag, &type, &elem, &size, &lastP, TRUE ) )
                        {
                            if( rpcm_isElemEqual( type, p, s, elem, size ) )
                            {
                                isEqual = TRUE;
                                break;
                            }
                        }

                        if( !isEqual )
                        {
                            break;
                        }
                    }

                    rIterator_free( ite );
                }
            }
        }
    }

    return isEqual;
}

RBOOL
    rpcm_isElemEqual
    (
        rpcm_type type,
        RPVOID pElem1,
        RU32 elem1Size,
        RPVOID pElem2,
        RU32 elem2Size
    )
{
    RBOOL isEqual = FALSE;

    if( NULL != pElem1 &&
        NULL != pElem2 )
    {
        switch( type )
        {
            case RPCM_RU8:
                if( *(RPU8)pElem1 == *(RPU8)pElem2 )
                {
                    isEqual = TRUE;
                }
                break;
            case RPCM_RU16:
                if( *(RPU16)pElem1 == *(RPU16)pElem2 )
                {
                    isEqual = TRUE;
                }
                break;
            case RPCM_RU32:
            case RPCM_POINTER_32:
            case RPCM_IPV4:
                if( *(RPU32)pElem1 == *(RPU32)pElem2 )
                {
                    isEqual = TRUE;
                }
                break;
            case RPCM_RU64:
            case RPCM_POINTER_64:
            case RPCM_TIMEDELTA:
            case RPCM_TIMESTAMP:
                if( *(RPU64)pElem1 == *(RPU64)pElem2 )
                {
                    isEqual = TRUE;
                }
                break;
            case RPCM_IPV6:
                if( 0 == rpal_memory_memcmp( pElem1, pElem2, RPCM_IPV6_SIZE ) )
                {
                    isEqual = TRUE;
                }
                break;
            case RPCM_STRINGA:
                if( 0 == rpal_string_strcmpa( (RPCHAR)pElem1, (RPCHAR)pElem2 ) )
                {
                    isEqual = TRUE;
                }
                break;
            case RPCM_STRINGW:
                if( 0 == rpal_string_strcmpw( (RPWCHAR)pElem1, (RPWCHAR)pElem2 ) )
                {
                    isEqual = TRUE;
                }
                break;
            case RPCM_BUFFER:
                if( elem1Size == elem2Size &&
                    0 == rpal_memory_memcmp( pElem1, pElem2, elem1Size) )
                {
                    isEqual = TRUE;
                }
                break;
        }

        if( RPCM_COMPLEX_TYPES <= type )
        {
            isEqual = set_isEqual( (_PElementSet)pElem1, (_PElementSet)pElem2 );
        }
    }

    return isEqual;
}

RBOOL
    rSequence_isEqual
    (
        rSequence seq1,
        rSequence seq2
    )
{
    return set_isEqual( &((_rSequence*)seq1)->set, &((_rSequence*)seq2)->set );
}

RBOOL
    rList_isEqual
    (
        rList list1,
        rList list2
    )
{
    return set_isEqual( &((_rList*)list1)->set, &((_rList*)list2)->set );
}

RVOID
    rSequence_unTaintRead
    (
        rSequence seq
    )
{
#ifdef RPAL_PLATFORM_DEBUG
    ((_rSequence*)seq)->set.isReadTainted = FALSE;
#else
    UNREFERENCED_PARAMETER( seq );
#endif
    return;
}

RVOID
    rList_unTaintRead
    (
        rList list
    )
{
#ifdef RPAL_PLATFORM_DEBUG
    ((_rList*)list)->set.isReadTainted = FALSE;
#else
    UNREFERENCED_PARAMETER( list );
#endif
    return;
}

RU32
    rSequence_getEstimateSize
    (
        rSequence seq
    )
{
    return set_getEstimateSize( &( (_rSequence*)seq )->set );
}

RU32
    rList_getEstimateSize
    (
        rList list
    )
{
    return set_getEstimateSize( &( (_rList*)list )->set );
}
