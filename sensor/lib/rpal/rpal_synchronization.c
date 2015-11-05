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

#include <rpal.h>

#define RPAL_FILE_ID    11

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <pthread.h>
#include <time.h>
#endif

#if defined( RPAL_PLATFORM_MACOSX )
#include <sys/time.h>
#endif

typedef struct
{
#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE hMutex;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    pthread_mutex_t hMutex;
#endif
} _rMutex;

typedef struct
{
    rMutex mutex;
    RU32 count;
    rRefCount_freeFunc freeFunc;
    RPVOID pElem;
    RU32 elemSize;

} _rRefCount;

typedef struct
{
#ifdef RPAL_PLATFORM_WINDOWS
    HANDLE hEvent;
    volatile RU32 waitCount;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    pthread_mutex_t hMutex;
    pthread_cond_t hCond;
    RBOOL isManualReset;
    RBOOL isOn;
#endif

} _rEvent;

typedef struct
{
    rEvent evtCanRead;
    rEvent evtCanWrite;
    rMutex stateLock;
    RU32 readCount;

} _rRwLock;


//=============================================================================
//  rMutex API
//=============================================================================
rMutex
    rMutex_create
    (

    )
{
    rMutex mutex = NULL;

    mutex = rpal_memory_alloc( sizeof( _rMutex ) );

    if( rpal_memory_isValid( mutex ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        ((_rMutex*)mutex)->hMutex = CreateMutex( NULL, FALSE, NULL );

        if( NULL == ((_rMutex*)mutex)->hMutex )
        {
            rpal_memory_free( mutex );
            mutex = NULL;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        pthread_mutex_init( &((_rMutex*)mutex)->hMutex, NULL );
#endif
    }

    return mutex;
}

RVOID
    rMutex_free
    (
        rMutex mutex
    )
{
    if( NULL != mutex )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( NULL != ((_rMutex*)mutex)->hMutex )
        {
            CloseHandle( ((_rMutex*)mutex)->hMutex );
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        pthread_mutex_destroy( &((_rMutex*)mutex)->hMutex );
#endif
        rpal_memory_free( mutex );
    }
}

RBOOL
    rMutex_trylock
    (
        rMutex mutex,
        RU32 timeout
    )
{
    RBOOL isSuccess = FALSE;

    if( rpal_memory_isValid( mutex ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( WAIT_OBJECT_0 == WaitForSingleObject( ((_rMutex*)mutex)->hMutex, timeout ) )
        {
            isSuccess = TRUE;
        }
#elif defined( RPAL_PLATFORM_LINUX )
        struct timespec abs_time;
        
        if( RINFINITE != timeout )
        {
            clock_gettime( CLOCK_REALTIME, &abs_time );
            
            abs_time.tv_sec += ( timeout / 1000 );
            abs_time.tv_nsec += ( ( timeout % 1000 ) * 1000000 );
            
            if( 0 == pthread_mutex_timedlock( &((_rMutex*)mutex)->hMutex, &abs_time ) )
            {
                isSuccess = TRUE;
            }
        }
        else
        {
            if( 0 == pthread_mutex_lock( &((_rMutex*)mutex)->hMutex ) )
            {
                isSuccess = TRUE;
            }
        }
#elif defined( RPAL_PLATFORM_MACOSX ) || defined( RPAL_PLATFORM_IOS ) || defined( RPAL_PLATFORM_ANDROID )
        // These platforms lack the required APIs, there is no real elegant solution
        // so we do the best we can, currently a 10ms poll. This looks like OSS, not good.
        if( RINFINITE != timeout )
        {
            RU32 timeStart = rpal_time_getMilliSeconds();
            
            isSuccess = TRUE;
            
            while( EBUSY == pthread_mutex_trylock( &((_rMutex*)mutex)->hMutex) )
            {
                if( timeout <= (RU32)rpal_time_elapsedMilliSeconds( (RU32)timeStart ) )
                {
                    isSuccess = FALSE;
                    break;
                }
                
                rpal_thread_sleep( 10 );
            }
        }
        else
        {
            if( 0 == pthread_mutex_lock( &((_rMutex*)mutex)->hMutex ) )
            {
                isSuccess = TRUE;
            }
        }
#endif
    }
    
    return isSuccess;
}

RBOOL
    rMutex_lock
    (
        rMutex mutex
    )
{
    return rMutex_trylock( mutex, RINFINITE );
}

RVOID
    rMutex_unlock
    (
        rMutex mutex
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    ReleaseMutex( ((_rMutex*)mutex)->hMutex );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    pthread_mutex_unlock( &((_rMutex*)mutex)->hMutex );
#endif
}


//=============================================================================
//  rRefCount API
//=============================================================================
rRefCount
    rRefCount_create
    (
        rRefCount_freeFunc freeFunc,
        RPVOID pElem,
        RU32 elemSize
    )
{
    _rRefCount* ref = NULL;

    ref = rpal_memory_alloc( sizeof( *ref ) );

    if( rpal_memory_isValid( ref ) )
    {
        ref->mutex = rMutex_create();

        if( rpal_memory_isValid( ref->mutex ) )
        {
            ref->count = 1;
            ref->freeFunc = freeFunc;
            ref->pElem = pElem;
            ref->elemSize = elemSize;
        }
        else
        {
            rpal_memory_free( ref );
            ref = NULL;
        }
    }

    return (rRefCount)ref;
}

RBOOL
    rRefCount_acquire
    (
        rRefCount ref
    )
{
    RBOOL isSuccess = FALSE;

    if( rpal_memory_isValid( ref ) )
    {
        if( rMutex_lock( ((_rRefCount*)ref)->mutex ) )
        {
            ((_rRefCount*)ref)->count++;
            isSuccess = TRUE;

            rMutex_unlock( ((_rRefCount*)ref)->mutex );
        }
    }

    return isSuccess;
}

RBOOL
    rRefCount_getElem
    (
        rRefCount ref,
        RPVOID* pElem
    )
{
    RBOOL isSuccess = FALSE;

    if( rpal_memory_isValid( ref ) &&
        NULL != pElem )
    {
        if( rMutex_lock( ((_rRefCount*)ref)->mutex ) )
        {
            *pElem = ((_rRefCount*)ref)->pElem;
            isSuccess = TRUE;

            rMutex_unlock( ((_rRefCount*)ref)->mutex );
        }
    }

    return isSuccess;
}

RBOOL
    rRefCount_release
    (
        rRefCount ref,
        RBOOL* pIsReleased
    )
{
    RBOOL isSuccess = FALSE;
    RBOOL isReleased = FALSE;
    _rRefCount* pRef = NULL;

    if( rpal_memory_isValid( ref ) )
    {
        pRef = (_rRefCount*)ref;

        if( rMutex_lock( pRef->mutex ) )
        {
            if( 0 == --(pRef->count) )
            {
                if( NULL != pRef->freeFunc )
                {
                    pRef->freeFunc( pRef->pElem, pRef->elemSize );
                }

                rMutex_free( pRef->mutex );
                rpal_memory_free( pRef );

                isReleased = TRUE;
            }
            else
            {
                rMutex_unlock( pRef->mutex );
            }

            if( NULL != pIsReleased )
            {
                *pIsReleased = isReleased;
            }

            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RVOID
    rRefCount_destroy
    (
        rRefCount ref
    )
{
    _rRefCount* pRef = (_rRefCount*)ref;

    if( rpal_memory_isValid( ref ) )
    {
        rMutex_lock( pRef->mutex );
        rMutex_free( pRef->mutex );
        rpal_memory_free( pRef );
    }
}

//=============================================================================
//  rEvent API
//=============================================================================
rEvent
    rEvent_create
    (
        RBOOL isManualReset
    )
{
    _rEvent* evt = NULL;

    evt = rpal_memory_alloc( sizeof( *evt ) );

    if( rpal_memory_isValid( evt ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        evt->waitCount = 0;
        
        evt->hEvent = CreateEvent( NULL, isManualReset, FALSE, NULL );
        
        if( NULL == evt->hEvent )
        {
            rpal_memory_free( evt );
            evt = NULL;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        pthread_mutex_init( &evt->hMutex, NULL );
        pthread_cond_init( &evt->hCond, NULL );
        evt->isManualReset = isManualReset;
        evt->isOn = FALSE;
#endif
    }

    return (rEvent)evt;
}

RVOID
    rEvent_free
    (
        rEvent ev
    )
{
    if( rpal_memory_isValid( ev ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        CloseHandle( ((_rEvent*)ev)->hEvent );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        ((_rEvent*)ev)->isOn = FALSE;
        pthread_cond_destroy( &((_rEvent*)ev)->hCond );
        pthread_mutex_destroy( &((_rEvent*)ev)->hMutex );
#endif
        rpal_memory_free( ev );
    }
}

RBOOL
    rEvent_wait
    (
        rEvent ev,
        RU32 timeout
    )
{
    RBOOL isSuccess = FALSE;
    _rEvent* volatile evt = (_rEvent*)ev;

    if( rpal_memory_isValid( ev ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        rInterlocked_increment32( &evt->waitCount );

        if( WAIT_OBJECT_0 == WaitForSingleObject( evt->hEvent, timeout ) )
        {
            isSuccess = TRUE;
        }
        
        rInterlocked_decrement32( &evt->waitCount );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        struct timespec abs_time;
        struct timeval cur_time;
        struct timespec ts;
        int err = 0;
        gettimeofday( &cur_time, &ts );
        
        abs_time.tv_sec = cur_time.tv_sec;
        abs_time.tv_nsec = cur_time.tv_usec / 1000;
        
        abs_time.tv_sec += ( timeout / 1000 );
        abs_time.tv_nsec += ( ( timeout % 1000 ) * 1000000 );
        
        if( 0 == pthread_mutex_lock( &evt->hMutex ) )
        {
            if( evt->isOn )
            {
                if( !evt->isManualReset )
                {
                    evt->isOn = FALSE;
                }
                isSuccess = TRUE;
            }
            else
            {
                if( ( RINFINITE != timeout && ( 0 == ( err = pthread_cond_timedwait( &evt->hCond, &evt->hMutex, &abs_time ) ) ) ) ||
                    ( RINFINITE == timeout && ( 0 == ( err = pthread_cond_wait( &evt->hCond, &evt->hMutex ) ) ) ) )
                {
                    if( !evt->isManualReset )
                    {
                        evt->isOn = FALSE;
                    }
                    isSuccess = TRUE;
                }
            }
            
            pthread_mutex_unlock( &evt->hMutex );
        }
#endif
    }

    return isSuccess;
}

RBOOL
    rEvent_set
    (
        rEvent ev
    )
{
    RBOOL isSuccess = FALSE;
    _rEvent* evt = (_rEvent*)ev;

    if( rpal_memory_isValid( ev ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        isSuccess = SetEvent( evt->hEvent );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( 0 == pthread_mutex_lock( &evt->hMutex ) )
        {
            evt->isOn = TRUE;
            if( evt->isManualReset )
            {
                pthread_cond_broadcast( &evt->hCond );
            }
            else
            {
                pthread_cond_signal( &evt->hCond );
            }
            
            
            pthread_mutex_unlock( &evt->hMutex );
            isSuccess = TRUE;
        }
#endif
    }

    return isSuccess;
}

RBOOL
    rEvent_unset
    (
        rEvent ev
    )
{
    RBOOL isSuccess = FALSE;
    _rEvent* evt = (_rEvent*)ev;

    if( rpal_memory_isValid( ev ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        isSuccess = ResetEvent( evt->hEvent );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( 0 == pthread_mutex_lock( &evt->hMutex ) )
        {
            evt->isOn = FALSE;
            
            pthread_mutex_unlock( &evt->hMutex );
            
            isSuccess = TRUE;
        }
#endif
    }

    return isSuccess;
}

RBOOL
    rEvent_pulse
    (
        rEvent ev
    )
{
    RBOOL isSuccess = FALSE;
    _rEvent* evt = (_rEvent*)ev;

    if( rpal_memory_isValid( ev ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        RU32 waited = 500;
        
        do
        {
            if( !PulseEvent( evt->hEvent ) )
            {
                break;
            }

            if( 0 != evt->waitCount )
            {
                Sleep( 1 );
                waited--;
            }

        } while( 0 != evt->waitCount || waited != 0 );

        if( 0 != waited )
        {
            isSuccess = TRUE;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( 0 == pthread_mutex_lock( &evt->hMutex ) )
        {
            pthread_cond_broadcast( &evt->hCond );
            
            pthread_mutex_unlock( &evt->hMutex );
        }
#endif
    }

    return isSuccess;
}

//=============================================================================
//  rRwLock API
//=============================================================================
rRwLock
    rRwLock_create
    (

    )
{
    _rRwLock* lock;

    lock = rpal_memory_alloc( sizeof( *lock ) );

    if( rpal_memory_isValid( lock ) )
    {
        lock->evtCanRead = rEvent_create( TRUE );
        lock->evtCanWrite = rEvent_create( TRUE );
        lock->stateLock = rMutex_create();
        lock->readCount = 0;

        if( !rpal_memory_isValid( lock->evtCanRead ) ||
            !rpal_memory_isValid( lock->evtCanWrite ) ||
            !rpal_memory_isValid( lock->stateLock ) )
        {
            rEvent_free( lock->evtCanRead );
            rEvent_free( lock->evtCanWrite );
            rMutex_free( lock->stateLock );
            rpal_memory_free( lock );
            lock = NULL;
        }
        else
        {
            rEvent_set( lock->evtCanRead );
        }
    }

    return (rRwLock)lock;
}

RVOID
    rRwLock_free
    (
        rRwLock lock
    )
{
    _rRwLock* lck = (_rRwLock*)lock;

    if( rpal_memory_isValid( lock ) )
    {
        rEvent_free( lck->evtCanRead );
        rEvent_free( lck->evtCanWrite );
        rMutex_free( lck->stateLock );

        rpal_memory_free( lock );
    }
}

RBOOL
    rRwLock_read_lock
    (
        rRwLock lock
    )
{
    RBOOL isSuccess = FALSE;
    _rRwLock* lck = (_rRwLock*)lock;
    RBOOL isReady = FALSE;

    if( rpal_memory_isValid( lock ) )
    {
        // Wait to get permission to read atomically
        while( !isReady )
        {
            if( rMutex_lock( lck->stateLock ) )
            {
                if( rEvent_wait( lck->evtCanRead, 0 ) )
                {
                    isReady = TRUE;
                    lck->readCount++;
                }

                rMutex_unlock( lck->stateLock );

                isSuccess = TRUE;
            }

            if( !isReady )
            {
                rEvent_wait( lck->evtCanRead, RINFINITE );
            }
        }

        // Now we're safe to read
    }

    return isSuccess;
}

RBOOL
    rRwLock_read_unlock
    (
        rRwLock lock
    )
{
    RBOOL isSuccess = FALSE;
    _rRwLock* lck = (_rRwLock*)lock;

    if( rpal_memory_isValid( lock ) )
    {
        if( rMutex_lock( lck->stateLock ) )
        {
            lck->readCount--;

            if( 0 == lck->readCount )
            {
                rEvent_set( lck->evtCanWrite );
            }

            rMutex_unlock( lck->stateLock );

            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rRwLock_write_lock
    (
        rRwLock lock
    )
{
    RBOOL isSuccess = FALSE;
    _rRwLock* lck = (_rRwLock*)lock;
    RBOOL isReady = FALSE;

    if( rpal_memory_isValid( lock ) )
    {
        while( !isReady )
        {
            if( rMutex_lock( lck->stateLock ) )
            {
                if( 0 == lck->readCount )
                {
                    isReady = TRUE;
                    rEvent_unset( lck->evtCanWrite );
                }
                else
                {
                    rEvent_unset( lck->evtCanRead );
                    rMutex_unlock( lck->stateLock );
                }

                isSuccess = TRUE;
            }

            if( !isReady )
            {
                rEvent_wait( lck->evtCanWrite, RINFINITE );
            }
        }
    }

    return isSuccess;
}

RBOOL
    rRwLock_write_unlock
    (
        rRwLock lock
    )
{
    RBOOL isSuccess = FALSE;
    _rRwLock* lck = (_rRwLock*)lock;

    if( rpal_memory_isValid( lock ) )
    {
        rEvent_set( lck->evtCanRead );
        rMutex_unlock( lck->stateLock );
        isSuccess = TRUE;
    }

    return isSuccess;
}

//=============================================================================
//  Interlocked
//=============================================================================
RU32
    rInterlocked_increment32
    (
        volatile RU32* pRu32
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return InterlockedIncrement( (LONG*)pRu32 );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return __sync_add_and_fetch( pRu32, 1 );
#endif
}

RU32
    rInterlocked_decrement32
    (
        volatile RU32* pRu32
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return InterlockedDecrement( (LONG*)pRu32 );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return __sync_sub_and_fetch( pRu32, 1 );
#endif
}

RU32
    rInterlocked_add32
    (
        volatile RU32* pRu32,
        RU32 toAdd
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return InterlockedExchangeAdd( (LONG*)pRu32, toAdd );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return __sync_add_and_fetch( pRu32, toAdd );
#endif
}

RU32
    rInterlocked_set32
    (
        volatile RU32* pRu32,
        RU32 value
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return InterlockedExchange( (LONG*)pRu32, value );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return __sync_lock_test_and_set( pRu32, value );
#endif
}

#ifdef RPAL_PLATFORM_64_BIT
RU64
    rInterlocked_set64
    (
        volatile RU64* pRu64,
        RU64 value
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return InterlockedExchange64( (LONG64*)pRu64, value );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return __sync_lock_test_and_set( pRu64, value );
#endif
}
#endif

RU32
    rInterlocked_get32
    (
        volatile RU32* pRu32
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return InterlockedCompareExchange( (LONG*)pRu32, *pRu32, *pRu32 );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return __sync_fetch_and_or( pRu32, 0 );
#endif
}

#ifdef RPAL_PLATFORM_64_BIT
RU64
    rInterlocked_get64
    (
        volatile RU64* pRu64
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return InterlockedCompareExchange64( (LONG64*)pRu64, *pRu64, *pRu64 );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return __sync_fetch_and_or( pRu64, 0 );
#endif
}
#endif

