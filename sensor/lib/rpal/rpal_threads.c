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

#include "rpal_threads.h"
#include "rpal_synchronization.h"

#define RPAL_FILE_ID    12

#if defined( RPAL_PLATFORM_MACOSX )
#include <unistd.h>
#include <mach/mach_init.h>
#include <mach/mach_port.h>
#elif defined( RPAL_PLATFORM_LINUX )
#include <unistd.h>
#include <sys/syscall.h>
#endif

#if defined( RPAL_PLATFORM_WINDOWS )
    #pragma warning( disable: 4127 ) // Disabling error on constant expression in condition
    typedef struct
    {
        HANDLE hThread;
    } _rThread;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    #include <time.h>
    #include <pthread.h>
    typedef struct
    {
        pthread_t hThread;
        rMutex hWaitMutex;
        rMutex hStateMutex;
        RPVOID context;
        rpal_thread_func pFunc;
        RBOOL isCanBeFreed;
        RBOOL isHasStarted;
    } _rThread;
#endif

//=============================================================================
//  Threads
//=============================================================================

RVOID
    _rpal_thread_free
    (
        _rThread* pThread
    )
{
    if( rpal_memory_isValid( pThread ) )
    {
#if defined( RPAL_PLATFORM_WINDOWS )
        CloseHandle( pThread->hThread );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        rMutex_free( pThread->hStateMutex );
        rMutex_free( pThread->hWaitMutex );
        pthread_detach( pThread->hThread );
#endif
        rpal_memory_free( pThread );
    }
}

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
RU32
    _rpal_thread_stub
    (
        RPVOID context
    )
{
    RU32 ret = -1;
    _rThread* pThread = (_rThread*)context;
    RBOOL isShouldFree = FALSE;
    
    if( rpal_memory_isValid( pThread ) )
    {
        if( rMutex_lock( pThread->hWaitMutex ) )
        {
            pThread->isHasStarted = TRUE;
            ret = pThread->pFunc( pThread->context );
            rMutex_unlock( pThread->hWaitMutex );
        }
        
        if( rMutex_lock( pThread->hStateMutex ) )
        {
            if( pThread->isCanBeFreed )
            {
                isShouldFree = TRUE;
            }
            else
            {
                pThread->isCanBeFreed = TRUE;
            }
            rMutex_unlock( pThread->hStateMutex );
        }

        if( isShouldFree )
        {
            _rpal_thread_free( pThread );
        }
    }
    
    return ret;
}
#endif


#ifdef RPAL_PLATFORM_ANDROID

static RBOOL android_sigusr1_handler_installed = FALSE;

/*
 * Android does not yet implement pthread_cancel(). Until they get it, we can
 * hack something like it using thread-specific signals, for which the
 * following routine is the handler. To terminate a thread, we thus send it
 * the SIGUSR1 signal.
 *
 * Reference:
 * http://stackoverflow.com/questions/4610086/pthread-cancel-alternatives-in-android-ndk
 */
void
android_thread_exit_sig_handler( int sig )
{
    pthread_exit( 0 );
}

#endif


rThread
    rpal_thread_new
    (
        rpal_thread_func pStartFunction,
        RPVOID pContext
    )
{
    _rThread* hThread = NULL;

#ifdef RPAL_PLATFORM_ANDROID
    if ( !android_sigusr1_handler_installed )
    {
        struct sigaction action;
        memset( &action, 0, sizeof( action ) );
        sigemptyset( &action.sa_mask );
        action.sa_flags = 0;
        action.sa_handler = android_thread_exit_sig_handler;
        if ( sigaction( SIGUSR1, &action, NULL ) != 0 )
        {
            return NULL;
        }

        android_sigusr1_handler_installed = TRUE;
    }
#endif

    if( NULL != pStartFunction )
    {
        if( NULL != ( hThread = rpal_memory_alloc( sizeof( _rThread ) ) ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            RU32 threadId = 0;
            hThread->hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)pStartFunction, pContext, 0, (LPDWORD)&threadId );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
            hThread->context = pContext;
            hThread->hWaitMutex = rMutex_create();
            hThread->hStateMutex = rMutex_create();
            hThread->pFunc = pStartFunction;
            hThread->isCanBeFreed = FALSE;
            hThread->isHasStarted = FALSE;
            pthread_create( &hThread->hThread, NULL, (void*)&_rpal_thread_stub, hThread );
#endif
        }
    }

    return (rThread)hThread;
}


rThreadID
    rpal_thread_self
    (
    )
{
#if defined( RPAL_PLATFORM_WINDOWS )
        return GetCurrentThreadId();
#elif defined( RPAL_PLATFORM_MACOSX )
        return pthread_mach_thread_np( pthread_self() );
#elif defined( RPAL_PLATFORM_LINUX )
        return syscall( SYS_gettid );
#else
#error "Thread ID undefined for this platform."
#endif
}


RBOOL
    rpal_thread_wait
    (
        rThread hThread,
        RU32 timeout
    )
{
    RBOOL isJoined = FALSE;
    _rThread* pThread = (_rThread*)hThread;

    if( rpal_memory_isValid( hThread ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( WAIT_OBJECT_0 == WaitForSingleObject( pThread->hThread, timeout ) )
        {
            isJoined = TRUE;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        RPVOID ret = NULL;

        if ( RINFINITE == timeout )
        {
            isJoined = ( 0 == pthread_join( pThread->hThread, &ret ) );
        }
        else
        {
            while( !pThread->isHasStarted )
            {
                // This is purely for a very small possibility of a race condition
                // so we don't mind polling very quickly since it should not be more
                // than a milisecond for the thread to actually start...
                rpal_thread_sleep( 0 );
            }
            
            if( rMutex_trylock( pThread->hWaitMutex, timeout ) )
            {
                isJoined = TRUE;
                rMutex_unlock( pThread->hWaitMutex );
                
                // Linux has a thread stub so we must wait for it on top of the normal wait
                // otherwise if we exit immediately the stub may still be running.
                pthread_join( pThread->hThread, &ret );
            }
        }
#endif
    }

    return isJoined;
}


RBOOL
    rpal_thread_terminate
    (
        rThread hThread
    )
{
    RBOOL isTerminated = FALSE;
    _rThread* pThread = (_rThread*)hThread;
    if( rpal_memory_isValid( hThread ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        isTerminated = TerminateThread( pThread->hThread, 0 );
#elif defined( RPAL_PLATFORM_ANDROID )
        if ( 0 == pthread_kill( pThread->hThread, SIGUSR1 ) )
        {
            isTerminated = TRUE;
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( 0 == pthread_cancel( pThread->hThread ) )
        {
            isTerminated = TRUE;
        }
#endif
    }
    return isTerminated;
}

RVOID
    rpal_thread_free
    (
        rThread hThread
    )
{
    RBOOL isShouldBeFreed = FALSE;
    _rThread* pThread = (_rThread*)hThread;
    
    if( rpal_memory_isValid( pThread ) )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        isShouldBeFreed = TRUE;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        if( rMutex_lock( pThread->hStateMutex ) )
        {
            if( pThread->isCanBeFreed )
            {
                isShouldBeFreed = TRUE;
            }
            else
            {
                pThread->isCanBeFreed = TRUE;
            }
            
            rMutex_unlock( pThread->hStateMutex );
        }
#endif
        if( isShouldBeFreed )
        {
            _rpal_thread_free( pThread );
        }
    }
}


RVOID
    rpal_thread_sleep
    (
        RU32 miliSec
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    Sleep( miliSec );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    usleep( miliSec * 1000 );
#endif
}


RVOID
    rpal_thread_sleepInSec
    (
        RU32 sec
    )
{
    rpal_thread_sleep( sec * 1000 );
}


//=============================================================================
// Thread Pools
//=============================================================================
typedef struct
{
    // Thread Pool Core
    rMutex counterMutex;
    RU32 nThreads;
    RU32 minThreads;
    RU32 maxThreads;
    rQueue taskQueue;
    RBOOL isProcessingTasks;
    RBOOL isAcceptingTasks;
    RU32 nCurrentlyProcessing;
    RU32 threadTtl;
    RU32 nTotalTasksInPool;

    // Scheduling Related
    rCollection timers;
    rMutex timerMutex;
    rEvent timerWakeup;
    rEvent timerTimeToStop;
    RU32 nScheduled;

    // Handles to Current Threads
    rStack hThreads;

} _rThreadPool;

typedef struct
{
    rpal_thread_pool_func taskFunction;
    RPVOID taskData;

} _rThreadPoolTask;

typedef struct
{
    rpal_timer timer;
    _rThreadPoolTask task;

} _rThreadPoolScheduledTask;

typedef struct
{
    rThread hThread;
    rEvent timeToStopEvent;
    _rThreadPool* pPool;
    RU64 timeEnteredIdle;

} _rThreadPoolThread;


static
RVOID
    _freePoolTask
    (
        _rThreadPoolTask* task,
        RU32 dummySize
    )
{
    UNREFERENCED_PARAMETER( dummySize );
    rpal_memory_free( task );
}


static
RBOOL
    _freePoolThreadNoWait
    (
        _rThreadPoolThread* t
    )
{
    RBOOL isClean = FALSE;

    if( NULL != t )
    {
        rEvent_set( t->timeToStopEvent );
        rpal_thread_free( t->hThread );
        rEvent_free( t->timeToStopEvent );
    }

    return isClean;
}

static
RBOOL
    _freePoolThread
    (
        _rThreadPoolThread* t,
        RU32 timeout
    )
{
    RBOOL isClean = FALSE;

    if( NULL != t )
    {
        rEvent_set( t->timeToStopEvent );
        if( !rpal_thread_wait( t->hThread, timeout ) )
        {
            rpal_debug_warning( "thread termination timed out", timeout );
        }
        rpal_thread_free( t->hThread );
        rEvent_free( t->timeToStopEvent );
    }

    return isClean;
}

static
RBOOL
    _isThreadEqual
    (
        _rThreadPoolThread* pThread,
        _rThreadPoolThread* ref
    )
{
    RBOOL isEqual = FALSE;

    if( NULL != pThread &&
        NULL != ref )
    {
        if( pThread->hThread == ref->hThread )
        {
            isEqual = TRUE;
        }
    }

    return isEqual;
}


static
RBOOL
    _cleanupPoolThread
    (
        _rThreadPoolThread* t
    )
{
    RBOOL isClean = FALSE;

    if( NULL != t )
    {
        if( rStack_removeWith( t->pPool->hThreads, (rStack_compareFunc)_isThreadEqual, t, NULL ) )
        {
            if( _freePoolThreadNoWait( t ) )
            {
                isClean = TRUE;
            }
        }
    }

    return isClean;
}



RU32
    _thread_pool_worker
    (
        _rThreadPoolThread* pThread
    )
{
    RU32 ret = 0;

    _rThreadPoolTask* task = NULL;
#ifdef RPAL_PLATFORM_WINDOWS
    RPVOID result = NULL;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPVOID result __attribute ( (unused) );
#endif
    _rThreadPoolThread thread = {0};
    RU32 qSize = 0;
    
    if( NULL != pThread )
    {
        
        thread = *pThread;
        
        rEvent_unset( thread.timeToStopEvent );
        
        while( !rEvent_wait( thread.timeToStopEvent, 0 ) )
        {
            if( thread.pPool->isProcessingTasks )
            {
                if( rQueue_remove( thread.pPool->taskQueue, (RPVOID*)&task, NULL, MSEC_FROM_SEC( 1 ) ) )
                {
                    if( rMutex_lock( thread.pPool->counterMutex ) )
                    {
                        ++thread.pPool->nCurrentlyProcessing;
                        rMutex_unlock( thread.pPool->counterMutex );
                    }

                    if( NULL != task->taskFunction )
                    {
                        rpal_debug_info( "worker task starting: %p", task->taskFunction );
                        result = task->taskFunction( thread.timeToStopEvent, task->taskData );
                        rpal_debug_info( "worker task ended: %p", task->taskFunction );
                    }

                    rpal_memory_free( task );
                    if( rMutex_lock( thread.pPool->counterMutex ) )
                    {
                        --thread.pPool->nCurrentlyProcessing;
                        --thread.pPool->nTotalTasksInPool;
                        rMutex_unlock( thread.pPool->counterMutex );
                    }
                    thread.timeEnteredIdle = rpal_time_getLocal();
                }
                else if( !rEvent_wait( thread.timeToStopEvent, 1 ) )
                {
                    // Let's look to see if we should throttle down the number of threads
                    if( rMutex_lock( thread.pPool->counterMutex ) )
                    {
                        RBOOL threadExiting = FALSE;
                        if( thread.pPool->minThreads < ( thread.pPool->nThreads - thread.pPool->nCurrentlyProcessing ) &&
                            rQueue_getSize( thread.pPool->taskQueue, &qSize ) &&
                            0 == qSize &&
                            rpal_time_getLocal() >= ( thread.timeEnteredIdle + thread.pPool->threadTtl ) )
                        {
                            // We will manually go remove THIS thread entry from the stack
                            rpal_debug_info( "thread pool decreasing because of IDLE time." );
                            --thread.pPool->nThreads;
                            _cleanupPoolThread( &thread );
                            threadExiting = TRUE;
                        }

                        rMutex_unlock( thread.pPool->counterMutex );
                        if( threadExiting )
                        {
                            break;
                        }
                    }
                }
            }
            else if( rEvent_wait( thread.timeToStopEvent, MSEC_FROM_SEC( 1 )  ) )
            {
                break;
            }
            else
            {
                // Waiting to see if the pool will start up again.
                rpal_debug_info( "worker waiting for pool to be processing" );
                rpal_thread_sleep( MSEC_FROM_SEC( 1 ) );
            }
        }
    }

    return ret;
}

static
RVOID
    _freeTimer
    (
        RPVOID ptr,
        RU32 size
    )
{
    UNREFERENCED_PARAMETER( size );
    rpal_memory_free( ptr );
}

static
RBOOL
    _isThreadPoolThreadReady
    (
        _rThreadPoolThread* pThread
    )
{
    RBOOL isReady = FALSE;

    RU32 i = 0;

    if( NULL != pThread )
    {
        // We give a newly creating thread 5 seconds
        // to register as a go, although it should always be much
        // quicker.
        for( i = 0; i < 500; i++ )
        {
            if( !rEvent_wait( pThread->timeToStopEvent, 0 ) )
            {
                isReady = TRUE;
                break;
            }

            rpal_thread_sleep( 10 );
        }
    }

    return isReady;
}

static
RBOOL
    _initNewThread
    (
        _rThreadPool* pool
    )
{
    RBOOL isSuccess = FALSE;

    _rThreadPoolThread tmpThread = {0};

    if( NULL != pool )
    {
        isSuccess = TRUE;

        tmpThread.hThread = 0;
        tmpThread.timeToStopEvent = NULL;
        tmpThread.pPool = pool;
        tmpThread.timeEnteredIdle = rpal_time_getLocal();

        // We SET the timeToQuitEvent before creating the thread because we use
        // it as a processing gate to know when the thread has copied its context
        // into its own stack space (since we are giving it a ptr to our stack thread).
        if( NULL == ( tmpThread.timeToStopEvent = rEvent_create( TRUE ) ) ||
            !rEvent_set( tmpThread.timeToStopEvent ) ||
            0 == ( tmpThread.hThread = rpal_thread_new( (rpal_thread_func)_thread_pool_worker, &tmpThread ) ) ||
            !_isThreadPoolThreadReady( &tmpThread ) ||
            !rStack_push( pool->hThreads, &tmpThread ) )
        {
            isSuccess = FALSE;
        }
    }

    return isSuccess;
}

rThreadPool
    rThreadPool_create
    (
        RU32 minThreads,
        RU32 maxThreads,
        RU32 threadTtl
    )
{
    _rThreadPool* pool = NULL;

    RU32 index = 0;
    RBOOL isSuccess = FALSE;

    _rThreadPoolThread tmpThread = {0};

    if( minThreads <= maxThreads )
    {
        pool = rpal_memory_alloc( sizeof( _rThreadPool ) );

        if( NULL != pool )
        {
            pool->maxThreads = maxThreads;
            pool->minThreads = minThreads;
            pool->nThreads = minThreads;
            pool->isProcessingTasks = TRUE;
            pool->isAcceptingTasks = TRUE;
            pool->timerTimeToStop = NULL;
            pool->nCurrentlyProcessing = 0;
            pool->threadTtl = threadTtl;
            
            if( NULL != ( pool->counterMutex = rMutex_create() ) &&
                rQueue_create( &(pool->taskQueue), (queue_free_func)_freePoolTask, 0 ) &&
                rpal_collection_create( &(pool->timers), _freeTimer ) &&
                NULL != ( pool->timerMutex = rMutex_create() ) &&
                NULL != ( pool->timerWakeup = rEvent_create( TRUE ) ) &&
                NULL != ( pool->hThreads = rStack_new( sizeof( _rThreadPoolThread ) ) ) )
            {
                isSuccess = TRUE;

                for( index = 0; index < pool->nThreads && isSuccess; index++ )
                {
                    isSuccess = _initNewThread( pool );
                }
            }

            if( !isSuccess )
            {
                while( rStack_pop( pool->hThreads, &tmpThread ) )
                {
                    rInterlocked_decrement32( &pool->nThreads );

                    _freePoolThread( &tmpThread, RINFINITE );
                }

                rQueue_free( pool->taskQueue );
                rpal_collection_free( pool->timers );
                rStack_free( pool->hThreads, NULL );
                rMutex_free( pool->timerMutex );
                rEvent_free( pool->timerWakeup );
                rMutex_free( pool->counterMutex );
                rpal_memory_free( pool );
                pool = NULL;
            }
        }
    }

    return pool;
}

RVOID
    rThreadPool_destroy
    (
        rThreadPool pool,
        RBOOL isWaitForTasksToFinish
    )
{
    _rThreadPool* p = (_rThreadPool*)pool;

    _rThreadPoolThread tmpThread = {0};

    if( rpal_memory_isValid( pool ) )
    {
        p->isAcceptingTasks = FALSE;

        if( isWaitForTasksToFinish )
        {
            while( !rQueue_isEmpty( p->taskQueue ) &&
                   !rThreadPool_isIdle( pool ) )
            {
                rpal_thread_sleep( 100 );
            }
        }

        rEvent_set( p->timerTimeToStop ); // This is just a copy of the actual thread's so we won't close.
        rEvent_set( p->timerWakeup );

        // Deactivate automatic thread throttling so we may free everything
        // ourselves. We subvert the throttling rule by faking that there are
        // no threads in the pool: therefore the number of available threads
        // becomes negative or zero, hence it never exceeds the set minimum.
        if ( rMutex_lock( p->counterMutex ) )
        {
            p->nThreads = 0;
            rMutex_unlock( p->counterMutex );
        }

        while( rStack_pop( p->hThreads, &tmpThread ) )
        {
            _freePoolThread( &tmpThread, isWaitForTasksToFinish ? RINFINITE : MSEC_FROM_SEC( 10 ) );
        }

        rQueue_free( p->taskQueue );
        rpal_collection_free( p->timers );
        rMutex_free( p->timerMutex );
        rEvent_free( p->timerWakeup );
        rStack_free( p->hThreads, NULL );
        rMutex_free( p->counterMutex );

        rpal_memory_free( pool );
    }
}

RU32
    rThreadPool_getNbThreads
    (
        rThreadPool pool
    )
{
    return rInterlocked_get32( &( (_rThreadPool*)pool )->nThreads );
}


RBOOL
    rThreadPool_getTaskQueueLength
    (
        rThreadPool pool,
        RU32* pLength
    )
{
    return rQueue_getSize( ( (_rThreadPool*)pool )->taskQueue, pLength );
}


RBOOL
    rThreadPool_taskIfRunning
    (
        rThreadPool pool,
        rpal_thread_pool_func taskFunction,
        RPVOID taskData
    )
{
    RBOOL isSuccess = FALSE;

    _rThreadPool* p = (_rThreadPool*)pool;

    if( NULL != pool )
    {
        if( p->isProcessingTasks )
        {
            isSuccess = rThreadPool_task( pool, taskFunction, taskData );
        }
    }

    return isSuccess;
}

RBOOL
    rThreadPool_task
    (
        rThreadPool pool,
        rpal_thread_pool_func taskFunction,
        RPVOID taskData
    )
{
    RBOOL isSuccess = FALSE;

    _rThreadPool* p = (_rThreadPool*)pool;
    _rThreadPoolTask* task = NULL;
    RU32 qSize = 0;
    
    if( NULL != pool &&
        NULL != taskFunction &&
        p->isAcceptingTasks )
    {
        task = rpal_memory_alloc( sizeof( *task ) );

        if( NULL != task )
        {
            task->taskData = taskData;
            task->taskFunction = taskFunction;
            
            if( rMutex_lock( p->counterMutex ) )
            {
                ++p->nTotalTasksInPool;
                rMutex_unlock( p->counterMutex );
            }

            if( rQueue_add( p->taskQueue, task, sizeof( *task ) ) )
            {
                isSuccess = TRUE;
            }
            else
            {
                if( rMutex_lock( p->counterMutex ) )
                {
                    --p->nTotalTasksInPool;
                    rMutex_unlock( p->counterMutex );
                }
            
                rpal_memory_free( task );
            }

            if( rMutex_lock( p->counterMutex ) )
            {
                rQueue_getSize( p->taskQueue, &qSize );
                rpal_debug_info( "thread pool tasking 0x%p, current stats: %d minThreads, %d curThread, %d curProcessing, %d qSize.", 
                                 taskFunction, p->minThreads, p->nThreads, p->nCurrentlyProcessing, qSize );

                // Check to see if we have the minimum of available threads
                // and no more than the maximum.
                while(
                        ( (RS32)p->minThreads ) > ( (RS32)p->nThreads - (RS32)p->nTotalTasksInPool ) &&
                        p->nThreads < p->maxThreads
                        )
                {
                    if( _initNewThread( p ) )
                    {
                        rpal_debug_info( "thread pool increasing to demand to %d, queue size is %d.", p->nThreads, qSize );
                        ++p->nThreads;
                    }
                    else
                    {
                        rpal_debug_error( "Unable to create thread mandated by pool." );
                        break;
                    }
                }

                rMutex_unlock( p->counterMutex );
            }
        }
    }

    return isSuccess;
}


static
RPVOID
    _rThreadPoolSchedulerThread
    (
        rEvent timeToStopEvent,
        _rThreadPool* pool
    )
{
    RPVOID ret = NULL;

    RU64 timeToWait = 0;
    rpal_timer** timers = NULL;
    rCollectionIterator ite = NULL;
    _rThreadPoolScheduledTask* task = NULL;
    RU32 nTimer = 0;
    RU32 i = 0;

    rpal_debug_info( "scheduling thread starting" );

    if( NULL != ( timers = rpal_memory_alloc( ( nTimer + 1 ) * sizeof( rpal_timer* ) ) ) )
    {
        timers[ 0 ] = NULL;

        pool->timerTimeToStop = timeToStopEvent;

        while( TRUE )
        {
            // We have been told we should wake up
            if( rEvent_wait( pool->timerWakeup, (RU32)timeToWait ) )
            {
                rpal_debug_info( "scheduled threads change occured" );
                
                // Is it because it's time to quit?
                if( rEvent_wait( timeToStopEvent, 0 ) )
                {
                    break;
                }
                else
                {
                    if( rMutex_lock( pool->timerMutex ) )
                    {
                        rEvent_unset( pool->timerWakeup );

                        // Nope, it must mean the timers have changed
                        nTimer = rpal_collection_getSize( pool->timers );

                        if( NULL != ( timers = rpal_memory_realloc( timers, ( nTimer + 1 ) * sizeof( rpal_timer* ) ) ) )
                        {
                            if( rpal_collection_createIterator( pool->timers, &ite ) )
                            {
                                i = 0;

                                while( i < nTimer &&
                                        rpal_collection_next( ite, (RPVOID*)&task, NULL ) )
                                {
                                    // The first part of the scheduled task is actually the timer
                                    // so we cheat a little bit to skip doing multiple allocations / lookups.
                                    timers[ i ] = (rpal_timer*)task;
                                    i++;
                                }

                                rpal_collection_freeIterator( ite );
                            }
                            timers[ nTimer ] = NULL;
                        }

                        rMutex_unlock( pool->timerMutex );
                    }
                }
            }

            if( rMutex_lock( pool->timerMutex ) )
            {
                // Is it time to task anything?
                if( 0 != rpal_timer_update( timers ) )
                {
                    // Find the timer that expired and task it
                    for( i = 0; i < nTimer; i++ )
                    {
                        if( NULL != timers[ i ] && timers[ i ]->isReady )
                        {
                            rThreadPool_task( pool, 
                                    ((_rThreadPoolScheduledTask*)timers[ i ])->task.taskFunction,
                                    ((_rThreadPoolScheduledTask*)timers[ i ])->task.taskData );
                        }
                    }
                }

                timeToWait = rpal_timer_nextWait( timers );

                // If it's an infinite wait don't multiply as you will overflow.
                if( RINFINITE != (RU32)timeToWait )
                {
                    timeToWait = MSEC_FROM_SEC(timeToWait);
                }

                rMutex_unlock( pool->timerMutex );
            }
        }

        rpal_memory_free( timers );
    }

    rpal_debug_info( "scheduling thread stopping" );

    return ret;
}


RBOOL
    _rThreadPool_schedule
    (
        rThreadPool pool,
        RBOOL isAbsolute,
        RU64 timeValue,
        rpal_thread_pool_func taskFunction,
        RPVOID taskData,
        RBOOL isRandomStartOffset
    )
{
    RBOOL isSuccess = FALSE;

    _rThreadPool* p = (_rThreadPool*)pool;
    _rThreadPoolScheduledTask* sTask = NULL;

    if( rpal_memory_isValid( p ) &&
        NULL != taskFunction &&
        0 != timeValue )
    {
        if( NULL != ( sTask = rpal_memory_alloc( sizeof( _rThreadPoolScheduledTask ) ) ) )
        {
            sTask->task.taskData = taskData;
            sTask->task.taskFunction = taskFunction;

            if( ( isAbsolute &&
                  rpal_timer_init_onetime( &(sTask->timer), timeValue ) ) ||
                  rpal_timer_init_interval( &(sTask->timer), timeValue, isRandomStartOffset ) )
            {
                if( isAbsolute )
                {
                    rpal_debug_info( "scheduling one time execution in %d", timeValue );
                }
                else
                {
                    rpal_debug_info( "scheduling recurring execution every %d, next %d", timeValue, sTask->timer.nextTime - rpal_time_getGlobal() );
                }
                if( rMutex_lock( p->timerMutex ) )
                {
                    if( rpal_collection_add( p->timers, sTask, sizeof( *sTask ) ) )
                    {
                        rEvent_set( p->timerWakeup );

                        if( 1 != rInterlocked_increment32( &p->nScheduled ) ||
                            rThreadPool_task( pool, (rpal_thread_pool_func)_rThreadPoolSchedulerThread, pool ) )
                        {
                            rpal_debug_info( "scheduling thread tasked" );
                            isSuccess = TRUE;
                        }
                    }

                    rMutex_unlock( p->timerMutex );
                }
            }

            if( !isSuccess )
            {
                rpal_memory_free( sTask );
            }
        }
    }

    return isSuccess;
}

RBOOL
    rThreadPool_scheduleOneTime
    (
        rThreadPool pool,
        RU64 absoluteTime,
        rpal_thread_pool_func taskFunction,
        RPVOID taskData
    )
{
    return _rThreadPool_schedule( pool, TRUE, absoluteTime, taskFunction, taskData, FALSE );
}

RBOOL
    rThreadPool_scheduleRecurring
    (
        rThreadPool pool,
        RU64 timeInterval,
        rpal_thread_pool_func taskFunction,
        RPVOID taskData,
        RBOOL isRandomStartOffset
    )
{
    return _rThreadPool_schedule( pool, FALSE, timeInterval, taskFunction, taskData, isRandomStartOffset );
}

RBOOL
    rThreadPool_pause
    (
        rThreadPool pool
    )
{
    RBOOL isSuccess = FALSE;

    _rThreadPool* p = (_rThreadPool*)pool;

    if( NULL != pool )
    {
        p->isProcessingTasks = FALSE;
    }

    return isSuccess;
}

RBOOL
    rThreadPool_resume
    (
        rThreadPool pool
    )
{
    RBOOL isSuccess = FALSE;

    _rThreadPool* p = (_rThreadPool*)pool;

    if( NULL != pool )
    {
        p->isProcessingTasks = TRUE;
    }

    return isSuccess;
}

RBOOL
    rThreadPool_isIdle
    (
        rThreadPool pool
    )
{
    RBOOL isSuccess = FALSE;

    _rThreadPool* p = (_rThreadPool*)pool;

    if( NULL != pool && rMutex_lock( p->counterMutex ) )
    {
        if( 0 == p->nCurrentlyProcessing )
        {
            isSuccess = TRUE;
        }
        rMutex_unlock( p->counterMutex );
    }

    return isSuccess;
}


/* EOF */
