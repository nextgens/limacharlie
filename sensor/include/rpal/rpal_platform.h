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

#ifndef _RPAL_PLATFORM_H
#define _RPAL_PLATFORM_H

/*
 *
 * CURRENT PLATFORM
 *
 */

// The platform can be specified through a compiler define by setting the appropriate platform
// and the define below.
#ifndef RPAL_PLATFORM_DISABLE_DETECTION

#ifdef WIN32
    #ifdef WIN64
        #define RPAL_PLATFORM_WINDOWS
        #define RPAL_PLATFORM_WINDOWS_64
                #define RPAL_PLATFORM_64_BIT
    #else
        #define RPAL_PLATFORM_WINDOWS
        #define RPAL_PLATFORM_WINDOWS_32
                #define RPAL_PLATFORM_32_BIT
    #endif

    #define RPAL_PLATFORM_LITTLE_ENDIAN
#elif defined( __linux )
    #define RPAL_PLATFORM_LINUX
    
    #if __x86_64__ || __ppc64__
        #define RPAL_PLATFORM_LINUX_64
        #define RPAL_PLATFORM_64_BIT
    #else
        #define RPAL_PLATFORM_LINUX_32
        #define RPAL_PLATFORM_32_BIT
    #endif

    #if __BYTE_ORDER == __LITTLE_ENDIAN
        #define RPAL_PLATFORM_LITTLE_ENDIAN
    #else
        #define RPAL_PLATFORM_BIG_ENDIAN
    #endif
#elif defined( __APPLE__ )
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define RPAL_PLATFORM_IOS
    #elif TARGET_OS_MAC
        #define RPAL_PLATFORM_MACOSX
    #endif

    #if TARGET_CPU_X86
        #define RPAL_PLATFORM_32_BIT
    #elif TARGET_CPU_X86_64
        #define RPAL_PLATFORM_64_BIT
    #endif
#else
    #define RPAL_PLATFORM_UNKNOWN
    #error Platform could not be detected.
#endif

#endif

#ifdef _DEBUG
    #define RPAL_PLATFORM_DEBUG
#endif

// This is purely advisory
#define DO_PRAGMA(x) _Pragma (#x)
#ifdef RPAL_PLATFORM_DEBUG
#define RPAL_PLATFORM_TODO(x) DO_PRAGMA(message ("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\nPLATFORM TODO - " #x "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"))
#else
#define RPAL_PLATFORM_TODO(x)
#endif

#endif
