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

#include <rpal_time.h>
#include <rpal_synchronization.h>
#include <time.h>

#if defined( RPAL_PLATFORM_MACOSX )
    #include <mach/mach_time.h>
    #include <sys/time.h>
#endif

// This resource is not protected from multi-threading
// To do so would be the *right* thing, but a hassle
// for now. Since the impact of a race condition should
// be trivial, no protection for now...
static RU64 g_rpal_time_globalOffset = 0;

RU64
    rpal_time_getLocal
    (

    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return _time64( NULL );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    return time( NULL );
#endif
}

RPAL_DEFINE_API
( 
RU64, 
    rpal_time_getGlobal, 
)
{
    RU64 time = 0;

    time = rpal_time_getLocal() + g_rpal_time_globalOffset;

    return time;
}


RPAL_DEFINE_API
( 
RU64, 
    rpal_time_setGlobalOffset,
        RU64 offset
)
{
    RU64 oldOffset = 0;

    oldOffset = g_rpal_time_globalOffset;

    g_rpal_time_globalOffset = offset;

    return oldOffset;
}


RBOOL
    rpal_timer_init_interval
    (
        rpal_timer* timer,
        RU64 intervalSec,
        RBOOL isRandomStartTime
    )
{
    RBOOL isSuccess = FALSE;

    RU64 curTime = rpal_time_getGlobal();

    if( NULL != timer )
    {
        timer->isAbsolute = FALSE;
        timer->isReady = FALSE;

        if( isRandomStartTime )
        {
            timer->nextTime = ( rpal_rand() % intervalSec ) + curTime;
        }
        else
        {
            timer->nextTime = curTime + intervalSec;
        }

        timer->timeValue = intervalSec;
        isSuccess = TRUE;
    }

    return isSuccess;
}

RBOOL
    rpal_timer_init_onetime
    (
        rpal_timer* timer,
        RU64 timeStamp
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != timer )
    {
        timer->isAbsolute = TRUE;
        timer->isReady = FALSE;
        timer->nextTime = timeStamp;
        timer->timeValue = timeStamp;
        isSuccess = TRUE;
    }

    return isSuccess;
}


RU64
    rpal_timer_nextWait
    (
        rpal_timer* timers[]
    )
{
    RU64 i = 0;
    
    RU64 minTime = (RU64)(-1);
    RU64 curTime = rpal_time_getGlobal();

    if( NULL != timers )
    {
        while( NULL != timers[ i ] )
        {
            if( 0 != timers[ i ]->nextTime &&
                minTime > timers[ i ]->nextTime )
            {
                minTime = timers[ i ]->nextTime;
            }

            i++;
        }

        if( (RU64)(-1) != minTime )
        {
            if( curTime <= minTime )
            {
                minTime = minTime - curTime;
            }
            else
            {
                minTime = 0;
            }
        }
    }

    return minTime;
}

RU32
    rpal_timer_update
    (
        rpal_timer* timers[]
    )
{
    RU64 i = 0;
    RU64 curTime = rpal_time_getGlobal();

    RU32 nReady = FALSE;

    if( NULL != timers )
    {
        while( NULL != timers[ i ] )
        {
            if( 0 != timers[ i ]->nextTime &&
                curTime >= timers[ i ]->nextTime )
            {
                // Ready
                nReady++;

                if( timers[ i ]->isAbsolute )
                {
                    timers[ i ]->nextTime = 0;
                }
                else
                {
                    timers[ i ]->nextTime = curTime + timers[ i ]->timeValue;
                }

                timers[ i ]->isReady = TRUE;
            }
            else
            {
                timers[ i ]->isReady = FALSE;
            }

            i++;
        }
    }

    return nReady;
}

RU32
    rpal_time_getMilliSeconds
    (

    )
{
    RU32 t = 0;
#ifdef RPAL_PLATFORM_WINDOWS
    t = GetTickCount();
#elif defined( RPAL_PLATFORM_LINUX )
    struct timespec abs_time;
    clock_gettime( CLOCK_REALTIME, &abs_time );
    t = ( abs_time.tv_sec * 1000 ) + ( abs_time.tv_nsec / 1000000 );
#elif defined( RPAL_PLATFORM_MACOSX )
    const int64_t kOneMillion = 1000 * 1000;
    static mach_timebase_info_data_t s_timebase_info;
    
    if ( s_timebase_info.denom == 0 )
    {
        (void) mach_timebase_info( &s_timebase_info );
    }
    
    // mach_absolute_time() returns billionth of seconds,
    // so divide by one million to get milliseconds
    t = (RU32)( ( mach_absolute_time() * s_timebase_info.numer ) / ( kOneMillion * s_timebase_info.denom ) );
#endif
    return t;
}

RU64
    rpal_time_elapsedMilliSeconds
    (
        RU32 start
    )
{
    RU64 total = 0;
#ifdef RPAL_PLATFORM_WINDOWS
    RU32 now = 0;
    now = GetTickCount();

    if( start <= now )
    {
        total = now - start;
    }
    else
    {
        // Overflow
        total = ( 0xFFFFFFFF - start ) + now;
    }
#elif defined( RPAL_PLATFORM_LINUX )
    struct timespec abs_time;
    RU32 t = 0;
    clock_gettime( CLOCK_REALTIME, &abs_time );
    t = ( abs_time.tv_sec * 1000 ) + ( abs_time.tv_nsec / 1000000 );
    total = t - start;
#elif defined( RPAL_PLATFORM_MACOSX )
    RU32 t = 0;
    const int64_t kOneMillion = 1000 * 1000;
    static mach_timebase_info_data_t s_timebase_info;
    
    if ( s_timebase_info.denom == 0 )
    {
        (void) mach_timebase_info( &s_timebase_info );
    }
    
    // mach_absolute_time() returns billionth of seconds,
    // so divide by one million to get milliseconds
    t = (RU32)( ( mach_absolute_time() * s_timebase_info.numer ) / ( kOneMillion * s_timebase_info.denom ) );
    total = t - start;
#endif
    return total;
}

#ifdef RPAL_PLATFORM_WINDOWS
RU64 
    rpal_winFileTimeToTs
    (
        FILETIME ft
    )
{
    RU64 epochAsFiletime = 116444736000000000;
    RU64 hundredOfNanosecondes = 10000000;

    RU64 time = ( (RU64)(ft.dwHighDateTime) << 32 ) | ft.dwLowDateTime;

    return ( time - epochAsFiletime ) / hundredOfNanosecondes;
}
#endif

RBOOL
    rpal_time_hires_timestamp_metadata_init
    (
        rpal_hires_timestamp_metadata* metadata
    )
{
    RBOOL isSuccess = FALSE;

#ifdef RPAL_PLATFORM_WINDOWS
    LARGE_INTEGER freq;

    metadata->ticks_per_second = 0ull;
    
    if ( QueryPerformanceFrequency( &freq ) )
    {
        metadata->ticks_per_second = freq.QuadPart;
        isSuccess = TRUE;
    }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    isSuccess = TRUE;
#endif

    return isSuccess;
}


RU64
    rpal_time_get_hires_timestamp
    (
        rpal_hires_timestamp_metadata* metadata
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    LARGE_INTEGER timestamp_ticks;

    if ( metadata->ticks_per_second == 0ull )
    {
        return 0ull;
    }

    if ( !QueryPerformanceCounter( &timestamp_ticks ) )
    {
        return 0ull;
    }
    return ( timestamp_ticks.QuadPart * 1000000ull ) / metadata->ticks_per_second;

#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )

    struct timeval tv = { 0 };
    if ( 0 != gettimeofday( &tv, NULL ) )
    {
        return 0ull;
    }
    return ( ( RU64 )tv.tv_sec ) * 1000000ull + ( ( RU64 )tv.tv_usec );

#endif
}
