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

#include <rpal/rpal_queue.h>

#define RPAL_FILE_ID  7

typedef struct
{
    rCollection col;
    rMutex mutex;
    rEvent newItems;
    RU32 nMaxItems;
    queue_free_func destroyFunc;

} _rQueue, *_rPQueue;

RBOOL
    rQueue_create
    (
        rQueue* pQueue,
        queue_free_func destroyFunc,
        RU32 nMaxItems
    )
{
    RBOOL isSuccess = FALSE;

    _rPQueue q = NULL;

    if( NULL != pQueue )
    {
        q = rpal_memory_alloc( sizeof( _rQueue ) );

        if( rpal_memory_isValid( q ) )
        {
            q->mutex = NULL;
            q->newItems = NULL;
            q->col = NULL;
            q->nMaxItems = nMaxItems;
            q->destroyFunc = destroyFunc;

            if( rpal_collection_create( &(q->col), destroyFunc ) &&
                NULL != ( q->mutex = rMutex_create() ) &&
                NULL != ( q->newItems = rEvent_create( TRUE ) ) )
            {
                isSuccess = TRUE;
            }

            if( !isSuccess )
            {
                rpal_collection_free( q->col );
                rMutex_free( q->mutex );
                rEvent_free( q->newItems );
                rpal_memory_free( q );
            }
            else
            {
                *pQueue = q;
            }
        }
    }

    return isSuccess;
}

RBOOL
    rQueue_free
    (
        rQueue queue
    )
{
    RBOOL isSuccess = FALSE;

    _rPQueue q = (_rPQueue)queue;

    if( NULL != queue )
    {
        if( rMutex_lock( q->mutex ) )
        {
            rpal_collection_free( q->col );

            rEvent_free( q->newItems );

            rMutex_free( q->mutex );

            isSuccess = TRUE;
        }

        rpal_memory_free( q );
    }

    return isSuccess;
}

RBOOL
    rQueue_addEx
    (
        rQueue queue,
        RPVOID buffer,
        RU32 bufferSize,
        RBOOL isAllowOverflow
    )
{
    RBOOL isSuccess = FALSE;

    _rPQueue q = (_rPQueue)queue;

    RPVOID removeBuff = NULL;
    RU32 removeSize = 0;

    if( NULL != queue )
    {
        if( rMutex_lock( q->mutex ) )
        {
            isSuccess = TRUE;

            if( 0 != q->nMaxItems &&
                q->nMaxItems == rpal_collection_getSize( q->col ) )
            {
                if( isAllowOverflow )
                {
                    if( rpal_collection_remove( q->col, &removeBuff, &removeSize, NULL, NULL ) )
                    {
                        if( NULL != q->destroyFunc )
                        {
                            q->destroyFunc( removeBuff, removeSize );
                        }
                    }
                    else
                    {
                        isSuccess = FALSE;
                    }
                }
                else
                {
                    isSuccess = FALSE;
                }
            }

            if( isSuccess )
            {
                isSuccess = rpal_collection_add( q->col, buffer, bufferSize );
            }

            if( isSuccess )
            {
                rEvent_set( q->newItems );
            }

            rMutex_unlock( q->mutex );
        }
    }

    return isSuccess;
}

RBOOL
    rQueue_add
    (
        rQueue queue,
        RPVOID buffer,
        RU32 bufferSize
    )
{
    return rQueue_addEx( queue, buffer, bufferSize, TRUE );
}

RBOOL
    rQueue_remove
    (
        rQueue queue,
        RPVOID* buffer,
        RU32* bufferSize,
        RU32 miliSecTimeout
    )
{
    RBOOL isSuccess = FALSE;

    _rPQueue q = (_rPQueue)queue;
    RU64 tmpTime = 0; // Oy I whish I had a WaitForMultipleObjects

    if( NULL != queue )
    {
        tmpTime = rpal_time_getLocal();

        do
        {
            if( 0 != miliSecTimeout &&
                miliSecTimeout < ( rpal_time_getLocal() - tmpTime ) )
            {
                // Since we can't wait for both the event and the mutex
                // there is a chance we get the event but by the time we get to the
                // mutex there are no more elements. So we must maintain an Absolute
                // number of time we are willing to wait for it...
                break;
            }

            if( rMutex_lock( q->mutex ) )
            {
                if( rpal_collection_remove( q->col, buffer, bufferSize, NULL, NULL ) )
                {
                    if( 0 == rpal_collection_getSize( q->col ) )
                    {
                        rEvent_unset( q->newItems );
                    }

                    isSuccess = TRUE;
                }

                rMutex_unlock( q->mutex );
            }
            else
            {
                break;
            }
        }
        while( 0 != miliSecTimeout && 
               !isSuccess && 
               rEvent_wait( q->newItems, miliSecTimeout ) );
    }

    return isSuccess;
}

RBOOL
    rQueue_isEmpty
    (
        rQueue queue
    )
{
    RBOOL isEmpty = TRUE;

    _rPQueue q = (_rPQueue)queue;

    if( NULL != queue )
    {
        if( rMutex_lock( q->mutex ) )
        {
            if( 0 != rpal_collection_getSize( q->col ) )
            {
                isEmpty = FALSE;
            }

            rMutex_unlock( q->mutex );
        }
    }

    return isEmpty;
}

RBOOL
    rQueue_isFull
    (
        rQueue queue
    )
{
    RBOOL isFull = FALSE;

    _rPQueue q = (_rPQueue)queue;

    if( NULL != queue )
    {
        if( rMutex_lock( q->mutex ) )
        {
            if( q->nMaxItems == rpal_collection_getSize( q->col ) )
            {
                isFull = TRUE;
            }

            rMutex_unlock( q->mutex );
        }
    }

    return isFull;
}

rEvent
    rQueue_getNewElemEvent
    (
        rQueue queue
    )
{
    rEvent evt = NULL;

    _rPQueue q = (_rPQueue)queue;

    if( NULL != queue )
    {
        if( rMutex_lock( q->mutex ) )
        {
            evt = q->newItems;

            rMutex_unlock( q->mutex );
        }
    }

    return evt;
}

RBOOL
    rQueue_getSize
    (
        rQueue queue,
        RU32* pSize
    )
{
    RBOOL isSuccess = FALSE;

    _rPQueue q = (_rPQueue)queue;

    if( NULL != queue &&
        NULL != pSize )
    {
        if( rMutex_lock( q->mutex ) )
        {
            *pSize = rpal_collection_getSize( q->col );

            isSuccess = TRUE;

            rMutex_unlock( q->mutex );
        }
    }

    return isSuccess;
}

