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

#include <rpal_blob.h>

#define RPAL_FILE_ID   8

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	rBlob blob;
    RU32 nElements;
    RU32 elemSize;
    rMutex lock;

} _rStack, *_prStack;

#pragma pack(pop)

rStack
    rStack_new
    (
        RU32 elemSize
    )
{
    _prStack stack = NULL;

    stack = rpal_memory_alloc( sizeof( _rStack ) );

    if( NULL != stack )
    {
        stack->blob = rpal_blob_create( 0, 0 );

        if( NULL != stack->blob )
        {
            stack->nElements = 0;
            stack->elemSize = elemSize;

            if( NULL == ( stack->lock = rMutex_create() ) )
            {
                rpal_blob_free( stack->blob );
                rpal_memory_free( stack );
                stack = NULL;
            }
        }
        else
        {
            rpal_memory_free( stack );
            stack = NULL;
        }
    }

    return stack;
}

RBOOL
    rStack_free
    (
        rStack stack,
        rStack_freeFunc freeFunc
    )
{
    RBOOL isSuccess = FALSE;
    _prStack pStack = (_prStack)stack;
    RPVOID tmpElem = NULL;
    RPVOID tmpBuff = NULL;
    RU32 tmpSize = 0;
    RU32 i = 0;

    if( NULL != stack )
    {
        if( rMutex_lock( pStack->lock ) )
        {
            isSuccess = TRUE;

            tmpElem = rpal_blob_getBuffer( pStack->blob );
            tmpBuff = tmpElem;
            tmpSize = rpal_blob_getSize( pStack->blob );

            for( i = 0; i < pStack->nElements; i++ )
            {
                tmpElem = (RPU8)tmpElem + ( i * pStack->elemSize );

                if( IS_WITHIN_BOUNDS( tmpElem, pStack->elemSize, tmpBuff, tmpSize ) )
                {
                    if( NULL != freeFunc )
                    {
                        if( !freeFunc( tmpElem ) )
                        {
                            isSuccess = FALSE;
                        }
                    }
                }
            }

            rpal_blob_free( pStack->blob );

            rMutex_free( pStack->lock );
            rpal_memory_free( pStack );
        }
    }

    return isSuccess;
}

RBOOL
    rStack_push
    (
        rStack stack,
        RPVOID elem
    )
{
    RBOOL isSuccess = FALSE;
    _prStack pStack = (_prStack)stack;

    if( NULL != stack &&
        NULL != elem )
    {
        if( rMutex_lock( pStack->lock ) )
        {
            isSuccess = rpal_blob_add( pStack->blob, elem, pStack->elemSize );

            if( isSuccess )
            {
                pStack->nElements++;
            }

            rMutex_unlock( pStack->lock );
        }
    }

    return isSuccess;
}

RBOOL
    rStack_pop
    (
        rStack stack,
        RPVOID pOutElem
    )
{
    RBOOL isSuccess = FALSE;
    _prStack pStack = (_prStack)stack;
    RPVOID tmpElem = NULL;

    if( NULL != stack &&
        NULL != pOutElem &&
        0 < pStack->nElements )
    {
        if( rMutex_lock( pStack->lock ) )
        {
            tmpElem = rpal_blob_arrElem( pStack->blob, pStack->elemSize, pStack->nElements - 1 );

            if( NULL != tmpElem )
            {
                rpal_memory_memcpy( pOutElem, tmpElem, pStack->elemSize );

                if( rpal_blob_remove( pStack->blob, 
                                      pStack->elemSize * ( pStack->nElements - 1 ), 
                                      pStack->elemSize ) )
                {
                    isSuccess = TRUE;
                    pStack->nElements--;
                }
            }

            rMutex_unlock( pStack->lock );
        }
    }

    return isSuccess;
}

RBOOL
    rStack_isEmpty
    (
        rStack stack
    )
{
    RBOOL isEmpty = TRUE;
    _prStack pStack = (_prStack)stack;

    if( NULL != stack )
    {
        if( rMutex_lock( pStack->lock ) )
        {
            if( 0 != pStack->nElements )
            {
                isEmpty = FALSE;
            }

            rMutex_unlock( pStack->lock );
        }
    }

    return isEmpty;
}

RBOOL
    rStack_removeWith
    (
        rStack stack,
        rStack_compareFunc compFunc,
        RPVOID ref,
        RPVOID pElem
    )
{
    RBOOL isRemoved = FALSE;
    _prStack pStack = (_prStack)stack;
    RU32 i = 0;
    RPVOID tmpElem = NULL;

    if( NULL != stack &&
        NULL != compFunc )
    {
        if( rMutex_lock( pStack->lock ) )
        {
            for( i = 0; i < ( rpal_blob_getSize( pStack->blob ) / pStack->elemSize ); i++ )
            {
                if( NULL != ( tmpElem = rpal_blob_arrElem( pStack->blob, pStack->elemSize, i ) ) )
                {
                    if( compFunc( tmpElem, ref ) )
                    {
                        if( NULL != pElem )
                        {
                            rpal_memory_memcpy( pElem, tmpElem, pStack->elemSize );
                        }

                        if( rpal_blob_remove( pStack->blob, i * pStack->elemSize, pStack->elemSize ) )
                        {
                            pStack->nElements--;

                            isRemoved = TRUE;
                        }
                        break;
                    }
                }
            }

            rMutex_unlock( pStack->lock );
        }
    }

    return isRemoved;
}

