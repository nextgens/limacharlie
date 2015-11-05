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

#ifndef _RPAL_ARRAY
#define _RPAL_ARRAY

#include <rpal/rpal.h>

//=============================================================================
//  PUBLIC STRUCTURES
//=============================================================================
typedef RPVOID rCollection;
typedef RPVOID rCollectionIterator;

typedef RBOOL (*collection_compare_func)( RPVOID buffer, RU32 bufferSize, RPVOID like );
typedef RVOID (*collection_free_func)( RPVOID buffer, RU32 bufferSize );


typedef RPVOID rCircularBuffer;
typedef RVOID( *circularbuffer_free_func )( RPVOID elem );

//=============================================================================
//  PUBLIC API
//=============================================================================
RBOOL
    rpal_collection_create
    (
        rCollection* pCollection,
        collection_free_func destroyFunc
    );

RBOOL
    rpal_collection_free
    (
        rCollection collection
    );

RBOOL
	rpal_collection_freeWithFunc
	(
		rCollection collection, 
		collection_free_func func
	);

RBOOL
    rpal_collection_add
    (
        rCollection collection,
        RPVOID buffer,
        RU32 bufferSize
    );

RBOOL
    rpal_collection_remove
    (
        rCollection collection,
        RPVOID* buffer,
        RU32* bufferSize,
        collection_compare_func compFunc,
        RPVOID compObj
    );

RBOOL
    rpal_collection_get
    (
        rCollection collection,
        RPVOID* buffer,
        RU32* bufferSize,
        collection_compare_func compFunc,
        RPVOID compObj
    );

RBOOL
    rpal_collection_isPresent
    (
        rCollection collection,
        collection_compare_func compFunc,
        RPVOID compObj
    );

RBOOL
    rpal_collection_numOfMatches
    (
        rCollection collection,
        collection_compare_func compFunc,
        RPVOID compObj,
        RU32* numOfMatches
    );

RU32
    rpal_collection_getSize
    (
        rCollection collection
    );




rCircularBuffer
    rpal_circularbuffer_new
    (
        RU32 nElements,
        RU32 elemSize,
        circularbuffer_free_func optFreeFunc
    );

RVOID
    rpal_circularbuffer_free
    (
        rCircularBuffer cb
    );

RBOOL
    rpal_circularbuffer_add
    (
        rCircularBuffer cb,
        RPVOID pElem
    );

RPVOID
    rpal_circularbuffer_get
    (
        rCircularBuffer cb,
        RU32 index
    );

RPVOID
    rpal_circularbuffer_last
    (
        rCircularBuffer cb
    );

//=============================================================================
// Iterators
//=============================================================================
RBOOL
    rpal_collection_createIterator
    (
        rCollection collection,
        rCollectionIterator* iterator
    );

RVOID
    rpal_collection_freeIterator
    (
        rCollectionIterator iterator
    );

RBOOL
    rpal_collection_next
    (
        rCollectionIterator iterator,
        RPVOID* buffer,
        RU32* bufferSize
    );

RVOID
    rpal_collection_resetIterator
    (
        rCollectionIterator iterator
    );

#endif
