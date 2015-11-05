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

#ifndef _RPAL_QUEUE
#define _RPAL_QUEUE

#include <rpal/rpal.h>
#include <rpal/rpal_synchronization.h>

//=============================================================================
//  PUBLIC STRUCTURES
//=============================================================================
typedef RPVOID rQueue;

typedef RVOID (*queue_free_func)( RPVOID buffer, RU32 bufferSize );



//=============================================================================
//  PUBLIC API
//=============================================================================
RBOOL
    rQueue_create
    (
        rQueue* pQueue,
        queue_free_func destroyFunc,
        RU32 nMaxItems  // 0 for unlimited, otherwise behaves like a circular buffer
    );

RBOOL
    rQueue_free
    (
        rQueue queue
    );

RBOOL
    rQueue_add
    (
        rQueue queue,
        RPVOID buffer,
        RU32 bufferSize
    );

RBOOL
    rQueue_addEx
    (
        rQueue queue,
        RPVOID buffer,
        RU32 bufferSize,
        RBOOL isAllowOverflow
    );

RBOOL
    rQueue_remove
    (
        rQueue queue,
        RPVOID* buffer,
        RU32* bufferSize,
        RU32 miliSecTimeout
    );

RBOOL
    rQueue_isEmpty
    (
        rQueue queue
    );

RBOOL
    rQueue_isFull
    (
        rQueue queue
    );

rEvent
    rQueue_getNewElemEvent
    (
        rQueue queue
    );

RBOOL
    rQueue_getSize
    (
        rQueue queue,
        RU32* pSize
    );

#endif
