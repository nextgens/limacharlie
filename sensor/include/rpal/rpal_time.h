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

#ifndef _RPAL_TIME_H
#define _RPAL_TIME_H

#include <rpal.h>


#define rpal_time_getGlobal()                     RPAL_API_CALL(rpal_time_getGlobal,)
#define rpal_time_setGlobalOffset(offset)         RPAL_API_CALL(rpal_time_setGlobalOffset,(offset))

#define SEC_PER_HOUR                                (60*60)
#define SEC_PER_DAY                                 (SEC_PER_HOUR * 24)
#define MSEC_FROM_SEC(sec)                          ((sec)*1000)

typedef struct
{
    RBOOL isReady;
    RBOOL isAbsolute;
    RU64 timeValue;
    RU64 nextTime;

} rpal_timer;

RU64
    rpal_time_getLocal
    (

    );

RBOOL
    rpal_timer_init_interval
    (
        rpal_timer* timer,
        RU64 intervalSec,
        RBOOL isRandomStartTime
    );
    
RBOOL
    rpal_timer_init_onetime
    (
        rpal_timer* timer,
        RU64 timeStamp
    );

RU64
    rpal_timer_nextWait
    (
        rpal_timer* timers[]
    );

RU32
    rpal_timer_update
    (
        rpal_timer* timers[]
    );

RU32
    rpal_time_getMilliSeconds
    (

    );

RU64
    rpal_time_elapsedMilliSeconds
    (
        RU32 start
    );

#ifdef RPAL_PLATFORM_WINDOWS
RU64
    rpal_winFileTimeToTs
    (
        FILETIME ft
    );
#endif


// The following functions allow the generation of a sequence of monotonically
// increasing timestamps, in microseconds. These timestamps are relative to
// their neighbours in the sequence: if the (N+1)th timestamp is higher than
// the Nth by x, then approximately x microseconds have elapsed between the
// two. Timestamps have no meaning in terms of the current "absolute" time --
// they only relate to the previous and next in the sequence.

typedef struct
{
#ifdef RPAL_PLATFORM_WINDOWS
    RU64 ticks_per_second;
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#endif
} rpal_hires_timestamp_metadata;

RBOOL
    rpal_time_hires_timestamp_metadata_init
    (
        rpal_hires_timestamp_metadata* metadata
    );

RU64
    rpal_time_get_hires_timestamp
    (
        rpal_hires_timestamp_metadata* metadata
    );
#endif
