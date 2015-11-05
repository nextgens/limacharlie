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

#ifndef _RPAL_SYNCHRONIZATION_H
#define _RPAL_SYNCHRONIZATION_H

#include <rpal.h>

typedef RPVOID rMutex;

typedef RPVOID rRefCount;
typedef RVOID (*rRefCount_freeFunc)( RPVOID pElem, RU32 elemSize );

typedef RPVOID rEvent;

typedef RPVOID rRwLock;

rMutex
    rMutex_create
    (

    );

RVOID
    rMutex_free
    (
        rMutex mutex
    );

RBOOL
    rMutex_trylock
    (
        rMutex mutex,
        RU32 timeout
    );

RBOOL
    rMutex_lock
    (
        rMutex mutex
    );

RVOID
    rMutex_unlock
    (
        rMutex mutex
    );

rRefCount
    rRefCount_create
    (
        rRefCount_freeFunc freeFunc,
        RPVOID pElem,
        RU32 elemSize
    );

RBOOL
    rRefCount_acquire
    (
        rRefCount ref
    );

RBOOL
    rRefCount_getElem
    (
        rRefCount ref,
        RPVOID* pElem
    );

RBOOL
    rRefCount_release
    (
        rRefCount ref,
        RBOOL* pIsReleased
    );

RVOID
    rRefCount_destroy
    (
        rRefCount ref
    );

rEvent
    rEvent_create
    (
        RBOOL isManualReset
    );

RVOID
    rEvent_free
    (
        rEvent ev
    );

RBOOL
    rEvent_wait
    (
        rEvent ev,
        RU32 timeout
    );

RBOOL
    rEvent_set
    (
        rEvent ev
    );

RBOOL
    rEvent_unset
    (
        rEvent ev
    );

RBOOL
    rEvent_pulse
    (
        rEvent ev
    );

rRwLock
    rRwLock_create
    (

    );

RVOID
    rRwLock_free
    (
        rRwLock lock
    );

RBOOL
    rRwLock_read_lock
    (
        rRwLock lock
    );

RBOOL
    rRwLock_read_unlock
    (
        rRwLock lock
    );

RBOOL
    rRwLock_write_lock
    (
        rRwLock lock
    );

RBOOL
    rRwLock_write_unlock
    (
        rRwLock lock
    );

RU32
    rInterlocked_increment32
    (
        volatile RU32* pRu32
    );

RU32
    rInterlocked_decrement32
    (
        volatile RU32* pRu32
    );

RU32
    rInterlocked_add32
    (
        volatile RU32* pRu32,
        RU32 toAdd
    );

RU32
    rInterlocked_set32
    (
        volatile RU32* pRu32,
        RU32 value
    );

RU64
    rInterlocked_set64
    (
        volatile RU64* pRu64,
        RU64 value
    );

RU32
    rInterlocked_get32
    (
        volatile RU32* pRs32
    );

RU64
    rInterlocked_get64
    (
        volatile RU64* pRu64
    );

#endif
