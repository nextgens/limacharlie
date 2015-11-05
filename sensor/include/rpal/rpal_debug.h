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

#ifndef _RPAL_DEBUG_H
#define _RPAL_DEBUG_H

#include <rpal.h>

#ifdef RPAL_PLATFORM_DEBUG
#ifndef RPAL_PLATFORM_DEBUG_LOG
#define RPAL_PLATFORM_DEBUG_LOG
#endif
#else
#define RPAL_PLATFORM_DEBUG_LOG_CRIT
#define RPAL_PLATFORM_DEBUG_LOG_ERR
//#define RPAL_PLATFORM_DEBUG_LOG_WARN
//#define RPAL_PLATFORM_DEBUG_LOG_INFO
#endif

RVOID
    rpal_debug_printBuffer
    (
        RPU8 buffer,
        RU32 bufferSize,
        RU32 nWrap
    );

#ifdef RPAL_PLATFORM_WINDOWS

#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_CRIT )
#define rpal_debug_critical(format,...)   rpal_debug_print( "CRITICAL !!!!! %s: %d %s() %d - " ##format##"\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), __VA_ARGS__ )
#else
#define rpal_debug_critical(format,...)
#endif
#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_ERR )
#define rpal_debug_error(format,...)      rpal_debug_print( "ERROR ++++++++ %s: %d %s() %d - " ##format##"\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), __VA_ARGS__ )
#else
#define rpal_debug_error(format,...)
#endif
#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_WARN )
#define rpal_debug_warning(format,...)    rpal_debug_print( "WARNING ====== %s: %d %s() %d - " ##format##"\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), __VA_ARGS__ )
#else
#define rpal_debug_warning(format,...)
#endif
#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_INFO )
#define rpal_debug_info(format,...)       rpal_debug_print( "INFO --------- %s: %d %s() %d - " ##format##"\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), __VA_ARGS__ )
#else
#define rpal_debug_info(format,...)
#endif

#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <stdarg.h>
#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_CRIT )
#define rpal_debug_critical(format,...)   rpal_debug_print( "CRITICAL !!!!! %s: %d %s() %d - " #format "\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), ##__VA_ARGS__ )
#else
#define rpal_debug_critical(format,...)
#endif
#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_ERR )
#define rpal_debug_error(format,...)      rpal_debug_print( "ERROR ++++++++ %s: %d %s() %d - " #format "\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), ##__VA_ARGS__ )
#else
#define rpal_debug_error(format,...)
#endif
#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_WARN )
#define rpal_debug_warning(format,...)    rpal_debug_print( "WARNING ====== %s: %d %s() %d - " #format "\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), ##__VA_ARGS__ )
#else
#define rpal_debug_warning(format,...)
#endif
#if defined( RPAL_PLATFORM_DEBUG_LOG ) || defined( RPAL_PLATFORM_DEBUG_LOG_INFO )
#define rpal_debug_info(format,...)       rpal_debug_print( "INFO --------- %s: %d %s() %d - " #format "\n", __FILE__, __LINE__, __FUNCTION__, rpal_time_getLocal(), ##__VA_ARGS__ )
#else
#define rpal_debug_info(format,...)
#endif
#endif


RVOID
    rpal_debug_print
    (
        RPCHAR format,
        ...
    );

#ifdef RPAL_PLATFORM_DEBUG
    #ifdef RPAL_PLATFORM_WINDOWS
        #define rpal_debug_break()  DebugBreak()
    #elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        #include <signal.h>
        #define rpal_debug_break() raise(SIGTRAP)
    #endif
    #define RPAL_ASSERT(val) if( !(val) ){ rpal_debug_break(); }
#else
    #define RPAL_ASSERT(val)
    #define rpal_debug_break()
#endif

#define rpal_debug_not_implemented() ( rpal_debug_error( "API not implemented: %s", __FUNCTION__ ) )

#endif
