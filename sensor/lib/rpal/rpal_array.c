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

#include <rpal/rpal_array.h>

#define RPAL_FILE_ID    1


typedef struct _CollectionRecord
{
    RPVOID buffer;
    RU32 bufferSize;

    struct _CollectionRecord* next;
    struct _CollectionRecord* previous;

} _rCollectionRecord, *_rPCollectionRecord;

typedef struct
{
    RU32 numRecords;
    _rPCollectionRecord records;
    collection_free_func freeFunc;
    rRwLock lock;
    _rPCollectionRecord tail;

} _rCollection, *_rPCollection;

typedef struct
{
    _rPCollection collection;
    RPVOID lastBuffer;
    RBOOL isLastBufferPrevious;

} _rCollectionIterator, *_rPCollectionIterator;



typedef struct
{
    RU32 nElements;
    RU32 elemSize;
    RU32 curHead;
    RBOOL isFull;
    circularbuffer_free_func optFreeFunc;
    RU8 buff[];

} _rCircularBuffer;

RBOOL
    rpal_collection_create
    (
        rCollection* pCollection,
        collection_free_func destroyFunc
    )
{
    RBOOL isSuccess = FALSE;

    _rPCollection col = NULL;

    if( NULL != pCollection )
    {
        *pCollection = rpal_memory_alloc( sizeof(_rCollection) );

        if( NULL != *pCollection )
        {
            col = *pCollection;

            col->numRecords = 0;
            col->records = NULL;
            col->tail = NULL;

            col->freeFunc = destroyFunc;

            if( NULL != ( col->lock = rRwLock_create() ) )
            {
                isSuccess = TRUE;
            }
            else
            {
                rpal_memory_free( col );
                *pCollection = NULL;
            }
        }
    }

    return isSuccess;
}

RBOOL
    rpal_collection_free
    (
        rCollection collection
    )
{
    RBOOL isSuccess = FALSE;

    _rPCollection col = NULL;
    _rPCollectionRecord rec = NULL;
    _rPCollectionRecord tmp = NULL;
    
    col = collection;

    if( rpal_memory_isValid( collection ) )
    {
        if( rRwLock_write_lock( col->lock ) )
        {
            if( 0 != col->numRecords )
            {
                rec = col->records;

                while( NULL != rec )
                {
                    if( NULL != col->freeFunc )
                    {
                        col->freeFunc( rec->buffer, rec->bufferSize );
                    }

                    tmp = rec->next;

                    rpal_memory_free( rec );

                    rec = tmp;
                }

                isSuccess = TRUE;
            }

            rRwLock_free( col->lock );
            rpal_memory_free( collection );
        }
    }

    return isSuccess;
}


RBOOL
    rpal_collection_freeWithFunc
    (
        rCollection collection,
		collection_free_func func
    )
{
    RBOOL isSuccess = FALSE;
    
    if( rpal_memory_isValid( collection ) &&
		NULL != func )
    {
        if( rRwLock_write_lock( ((_rCollection*)collection)->lock ) )
        {
            ((_rCollection*)collection)->freeFunc = func;

            rRwLock_write_unlock( ((_rCollection*)collection)->lock );

            isSuccess = rpal_collection_free( collection );
        }
    }

    return isSuccess;
}

RBOOL
    rpal_collection_add
    (
        rCollection collection,
        RPVOID buffer,
        RU32 bufferSize
    )
{
    RBOOL isSuccess = FALSE;
    _rPCollection col = (_rPCollection)collection;
    _rPCollectionRecord record = NULL;
    _rPCollectionRecord tmpRecord = NULL;

    if( rpal_memory_isValid( collection ) &&
        NULL != buffer )
    {
        record = rpal_memory_alloc( sizeof(_rCollectionRecord) );

        if( rpal_memory_isValid( record ) )
        {
            record->buffer = buffer;
            record->bufferSize = bufferSize;
            record->next = NULL;
            record->previous = NULL;

            if( rRwLock_write_lock( col->lock ) )
            {
                tmpRecord = col->tail;
                if( rpal_memory_isValid( tmpRecord ) )
                {
                    col->tail = record;
                    record->previous = tmpRecord;
                    record->next = NULL;
                    tmpRecord->next = record;
                }
                else
                {
                    col->records = record;
                    col->tail = record;
                }

                col->numRecords++;

                rRwLock_write_unlock( col->lock );
            }

            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_collection_remove
    (
        rCollection collection,
        RPVOID* buffer,
        RU32* bufferSize,
        collection_compare_func compFunc,
        RPVOID compObj
    )
{
    RBOOL isSuccess = FALSE;
    _rPCollection col = (_rPCollection)collection;

    _rPCollectionRecord record = NULL;

    if( rpal_memory_isValid( collection ) )
    {
        if( NULL != buffer )
        {
            *buffer = NULL;
        }
        
        if( NULL != bufferSize )
        {
            *bufferSize = 0;
        }

        if( rRwLock_write_lock( col->lock ) )
        {
            if( NULL != compFunc )
            {
                record = col->records;

                while( rpal_memory_isValid( record ) )
                {
                    if( TRUE == compFunc( record->buffer, record->bufferSize, compObj ) )
                    {
                        break;
                    }

                    record = record->next;
                }
            }
            else
            {
                record = col->records;
            }

            if( rpal_memory_isValid( record ) )
            {
                if( rpal_memory_isValid( record->previous ) )
                {
                    record->previous->next = record->next;
                }

                if( rpal_memory_isValid( record->next ) )
                {
                    record->next->previous = record->previous;
                }

                if( record == col->records )
                {
                    col->records = record->next;
                }

                if( record == col->tail )
                {
                    col->tail = record->previous;
                }

                col->numRecords--;

                if( NULL != buffer )
                {
                    *buffer = record->buffer;
                }

                if( NULL != bufferSize )
                {
                    *bufferSize = record->bufferSize;
                }
            
                rpal_memory_free( record );

                isSuccess = TRUE;
            }

            rRwLock_write_unlock( col->lock );
        }
    }

    return isSuccess;
}


RBOOL
    _rpal_collection_get
    (
        rCollection collection,
        RPVOID* buffer,
        RU32* bufferSize,
        collection_compare_func compFunc,
        RPVOID compObj
    )
{
    RBOOL isSuccess = FALSE;
    _rPCollection col = (_rPCollection)collection;

    _rPCollectionRecord record = NULL;

    if( rpal_memory_isValid( collection ) )
    {
        if( NULL != buffer )
        {
            *buffer = NULL;
        }

        if( NULL != bufferSize )
        {
            *bufferSize = 0;
        }

        if( NULL != compFunc )
        {
            record = col->records;

            while( rpal_memory_isValid( record ) )
            {
                if( TRUE == compFunc( record->buffer, record->bufferSize, compObj ) )
                {
                    break;
                }

                record = record->next;
            }
        }
        else
        {
            record = col->records;
        }

        if( rpal_memory_isValid( record ) )
        {
            if( NULL != buffer )
            {
                *buffer = record->buffer;
            }

            if( NULL != bufferSize )
            {
                *bufferSize = record->bufferSize;
            }

            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_collection_get
    (
        rCollection collection,
        RPVOID* buffer,
        RU32* bufferSize,
        collection_compare_func compFunc,
        RPVOID compObj
    )
{
    RBOOL isSuccess = FALSE;
    
    _rPCollection col = (_rPCollection)collection;

    if( rpal_memory_isValid( col ) &&
        rRwLock_read_lock( col->lock ) )
    {
        isSuccess = _rpal_collection_get( collection, buffer, bufferSize, compFunc, compObj );

        rRwLock_read_unlock( col->lock );
    }

    return isSuccess;
}

RBOOL
    rpal_collection_isPresent
    (
        rCollection collection,
        collection_compare_func compFunc,
        RPVOID compObj
    )
{
    RBOOL isSuccess = FALSE;
    _rPCollection col = (_rPCollection)collection;

    _rPCollectionRecord record = NULL;

    if( rpal_memory_isValid( collection ) &&
        NULL != compFunc )
    {
        if( rRwLock_read_lock( col->lock ) )
        {
            record = col->records;

            while( rpal_memory_isValid( record ) )
            {
                if( TRUE == compFunc( record->buffer, record->bufferSize, compObj ) )
                {
                    isSuccess = TRUE;
                    break;
                }

                record = record->next;
            }

            rRwLock_read_unlock( col->lock );
        }
    }

    return isSuccess;
}



RBOOL
    rpal_collection_numOfMatches
    (
        rCollection collection,
        collection_compare_func compFunc,
        RPVOID compObj,
        RU32* numOfMatches
    )
{
    RBOOL isSuccess = FALSE;
    _rPCollection col = (_rPCollection)collection;

    _rPCollectionRecord record = NULL;

    if( rpal_memory_isValid( collection ) &&
        NULL != compFunc &&
        NULL != numOfMatches )
    {
        isSuccess = TRUE;
        (*numOfMatches) = 0;

        if( rRwLock_read_lock( col->lock ) )
        {
            record = col->records;

            while( rpal_memory_isValid( record ) )
            {
                if( TRUE == compFunc( record->buffer, record->bufferSize, compObj ) )
                {
                    (*numOfMatches)++;
                }

                record = record->next;
            }

            rRwLock_read_unlock( col->lock );
        }
    }

    return isSuccess;
}

RU32
    rpal_collection_getSize
    (
        rCollection collection
    )
{
    RU32 size = 0;
    _rPCollection col = (_rPCollection)collection;

    if( rpal_memory_isValid( collection ) )
    {
        if( rRwLock_read_lock( col->lock ) )
        {
            size = col->numRecords;

            rRwLock_read_unlock( col->lock );
        }
    }

    return size;
}






rCircularBuffer
    rpal_circularbuffer_new
    (
        RU32 nElements,
        RU32 elemSize,
        circularbuffer_free_func optFreeFunc
    )
{
    _rCircularBuffer* cb = NULL;

    if( 0 != nElements && 0 != elemSize )
    {
        if( NULL != ( cb = (_rCircularBuffer*)rpal_memory_alloc( sizeof( _rCircularBuffer ) 
                                                                 + ( nElements * elemSize ) ) ) )
        {
            cb->nElements = nElements;
            cb->elemSize = elemSize;
            cb->curHead = 0;
            cb->isFull = FALSE;
            cb->optFreeFunc = optFreeFunc;
            rpal_memory_zero( cb->buff, nElements * elemSize );
        }
    }

    return (rCircularBuffer)cb;
}

RVOID
    rpal_circularbuffer_free
    (
        rCircularBuffer cb
    )
{
    _rCircularBuffer* pCb = (_rCircularBuffer*)cb;
    RU32 i = 0;
    
    if( rpal_memory_isValid( cb ) )
    {
        if( NULL != ( pCb->optFreeFunc ) )
        {
            for( i = 0; i < ( pCb->isFull ? pCb->nElements : pCb->curHead ); i++ )
            {
                pCb->optFreeFunc( pCb->buff + ( i * pCb->elemSize ) );
            }
        }

        rpal_memory_free( cb );
    }
}

RBOOL
    rpal_circularbuffer_add
    (
        rCircularBuffer cb,
        RPVOID pElem
    )
{
    RBOOL isSuccess = FALSE;

    _rCircularBuffer* pCb = (_rCircularBuffer*)cb;

    if( rpal_memory_isValid( cb ) )
    {
        if( pCb->isFull &&
            NULL != pCb->optFreeFunc )
        {
            pCb->optFreeFunc( pCb->buff + ( pCb->curHead * pCb->elemSize ) );
        }

        rpal_memory_memcpy( pCb->buff + ( pCb->curHead * pCb->elemSize ), pElem, pCb->elemSize );
        pCb->curHead++;
        if( pCb->curHead >= pCb->nElements )
        {
            pCb->curHead = 0;
            
            if( !pCb->isFull )
            {
                pCb->isFull = TRUE;
            }
        }

        isSuccess = TRUE;
    }

    return isSuccess;
}

RPVOID
    rpal_circularbuffer_get
    (
        rCircularBuffer cb,
        RU32 index
    )
{
    RPVOID pOut = NULL;

    _rCircularBuffer* pCb = (_rCircularBuffer*)cb;

    if( rpal_memory_isValid( cb ) )
    {
        if( index < pCb->nElements )
        {
            if( pCb->isFull || ( index < pCb->curHead ) )
            {
                pOut = pCb->buff + ( pCb->elemSize * index );
            }
        }
    }

    return pOut;
}

RPVOID
    rpal_circularbuffer_last
    (
        rCircularBuffer cb
    )
{
    RPVOID pOut = NULL;

    _rCircularBuffer* pCb = (_rCircularBuffer*)cb;
    RU32 lastIndex = 0;

    if( rpal_memory_isValid( cb ) )
    {
        if( pCb->isFull || 0 < pCb->curHead )
        {
            if( 0 != pCb->curHead )
            {
                lastIndex = pCb->curHead - 1;
            }
            else
            {
                lastIndex = pCb->nElements - 1;
            }

            pOut = pCb->buff + ( pCb->elemSize * lastIndex );
        }
    }

    return pOut;
}


//=============================================================================
// Iterators
//=============================================================================
RBOOL
    rpal_collection_iterator_func
    (
        RPVOID record,
        RU32 recordSize,
        RPVOID iterator
    )
{
    RBOOL isTheOne = FALSE;

    _rPCollectionIterator iter;

    UNREFERENCED_PARAMETER( recordSize );

    if( NULL != record &&
        NULL != iterator )
    {
        iter = (_rPCollectionIterator)iterator;

        if( iter->isLastBufferPrevious || iter->lastBuffer == NULL )
        {
            isTheOne = TRUE;
            iter->isLastBufferPrevious = FALSE;
        }
        else if( iter->lastBuffer == record )
        {
            iter->isLastBufferPrevious = TRUE;
        }
    }

    return isTheOne;
}

RBOOL
    rpal_collection_createIterator
    (
        rCollection collection,
        rCollectionIterator* iterator
    )
{
    RBOOL isSuccess = FALSE;
    _rPCollectionIterator ite = NULL;
    _rPCollection col = (_rPCollection)collection;

    if( rpal_memory_isValid( collection ) &&
        NULL != iterator )
    {
        *iterator = rpal_memory_alloc( sizeof(*ite) );

        if( NULL != *iterator )
        {
            ite = *iterator;

            ite->collection = collection;
            ite->lastBuffer = NULL;
            ite->isLastBufferPrevious = FALSE;

            if( rRwLock_read_lock( col->lock ) )
            {
                isSuccess = TRUE;
            }
            else
            {
                rpal_memory_free( *iterator );
                *iterator = NULL;
            }
        }
    }

    return isSuccess;
}

RVOID
    rpal_collection_freeIterator
    (
        rCollectionIterator iterator
    )
{
    _rPCollectionIterator ite = (_rPCollectionIterator)iterator;

    if( NULL != iterator )
    {
        rRwLock_read_unlock( (_rCollection*)ite->collection->lock );

        ite->collection = NULL;
        ite->lastBuffer = NULL;

        rpal_memory_free( ite );
    }
}

RBOOL
    rpal_collection_next
    (
        rCollectionIterator iterator,
        RPVOID* buffer,
        RU32* bufferSize
    )
{
    RBOOL isSuccess = FALSE;
    _rPCollectionIterator ite = (_rPCollectionIterator)iterator;
    
    RPVOID tmpBuffer = NULL;

    if( NULL != iterator )
    {
        // Call the internal function so it doesn't attempt to read-lock.
        if( _rpal_collection_get( ite->collection, 
                                  &tmpBuffer, 
                                  bufferSize, 
                                  rpal_collection_iterator_func,
                                  iterator ) )
        {
            ite->lastBuffer = tmpBuffer;

            if( NULL != buffer )
            {
                *buffer = tmpBuffer;
            }

            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RVOID
    rpal_collection_resetIterator
    (
        rCollectionIterator iterator
    )
{
    _rPCollectionIterator ite = (_rPCollectionIterator)iterator;

    if( rpal_memory_isValid( iterator ) )
    {
        ite->isLastBufferPrevious = FALSE;
        ite->lastBuffer = NULL;
    }
}

