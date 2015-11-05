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

#ifndef _RPAL_DATATYPES_H
#define _RPAL_DATATYPES_H

#include <rpal_platform.h>

/*
 *
 * WINDOWS
 *
 */
#ifdef RPAL_PLATFORM_WINDOWS
    #define  _WIN32_WINNT 0x0500
    #include <windows.h>
    #include <windows_undocumented.h>
    #include <string.h>
    #include <stdlib.h>
    #include <stdio.h>
    #ifndef RPAL_PLATFORM_WINDOWS_32
        #include <varargs.h>
    #endif
    typedef BOOL		RBOOL;
    typedef BOOL*		RPBOOL;

    typedef BYTE		RU8;
    typedef BYTE*		RPU8;

    typedef WORD		RU16;
    typedef WORD*		RPU16;

    typedef UINT32		RU32;
    typedef UINT32*		RPU32;

    typedef LONG            RS32;
    typedef LONG*           RPS32;

    typedef UINT64	        RU64;
    typedef UINT64*	        RPU64;

    typedef INT64           RS64;
    typedef INT64*          RPS64;

    typedef	CHAR		RCHAR;
    typedef CHAR*		RPCHAR;

    typedef WCHAR		RWCHAR;
    typedef WCHAR*		RPWCHAR;

    typedef VOID		RVOID;
    typedef PVOID		RPVOID;

    typedef size_t		RSIZET;

    typedef RU64            RTIME;

    typedef float           RFLOAT;
    typedef double          RDOUBLE;

#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    #include <stdint.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <errno.h>
    #include <string.h>

    typedef int             RBOOL;
    typedef int*		RPBOOL;

    typedef uint8_t         RU8;
    typedef uint8_t*        RPU8;

    typedef uint16_t	RU16;
    typedef uint16_t*       RPU16;

    typedef uint32_t        RU32;
    typedef uint32_t*       RPU32;

    typedef int32_t         RS32;
    typedef int32_t*        RPS32;

    typedef uint64_t        RU64;
    typedef uint64_t*	RPU64;

    typedef	char		RCHAR;
    typedef char*		RPCHAR;

    typedef wchar_t		RWCHAR;
    typedef wchar_t*	RPWCHAR;

    typedef void		RVOID;
    typedef void*		RPVOID;

    typedef size_t		RSIZET;

    typedef RU64            RTIME;

    typedef float           RFLOAT;
    typedef double          RDOUBLE;
#endif

// Common values
#ifndef NULL
    #define NULL 0
#endif

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define RINFINITE ((RU32)(-1))

#define RPAL_MAX_PATH   (260)

#define _WCH(str) L ## str

#define NUMBER_TO_PTR(num)  ((RPVOID)(RSIZET)(num))
#define PTR_TO_NUMBER(ptr)  ((RSIZET)(ptr))

#define LITERAL_64_BIT(i)   (i ## LL)

#ifndef UNREFERENCED_PARAMETER
    #define UNREFERENCED_PARAMETER(p) (p=p)
#endif

#define IS_PTR_ALIGNED(ptr) (0 == (RSIZET)(ptr) % sizeof(RU32))

// Export Visibility Control
#ifdef RPAL_PLATFORM_WINDOWS
    #define RPAL_EXPORT         __declspec(dllexport)
    #define RPAL_DONT_EXPORT
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    #define RPAL_EXPORT         __attribute__((visibility("default")))
    #define RPAL_DONT_EXPORT    __attribute__((visibility("hidden")))
#endif

//=============================================================================
//  These data types must be included here instead of their own headers
//  since they are part of the core API, ie. centralized functions...
//=============================================================================

//=============================================================================
//  Handle Manager
typedef union
{
    RU64 h;
    struct
    {
        RU8 major;
        RU8 reserved1;
        RU16 reserved2;
        RU32 minor;
    } info;
} rHandle;

#define RPAL_HANDLE_INVALID             ((-1))
#define RPAL_HANDLE_INIT                {(RU64)(LITERAL_64_BIT(0xFFFFFFFFFFFFFFFF))}
typedef RBOOL (*rpal_handleManager_cleanup_f)( RPVOID val );

//=============================================================================
//  rBTree
typedef RPVOID rBTree;
typedef RS32 (*rpal_btree_comp_f)( RPVOID, RPVOID );
typedef RVOID (*rpal_btree_free_f)( RPVOID );


#endif
