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

#define RPAL_FILE_ID     0

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
#include <signal.h>
#endif

//=============================================================================
//  Global Accounting
//=============================================================================
#pragma pack(push)
#pragma pack(1)

#ifdef RPAL_FEATURE_MEMORY_SECURITY
typedef struct
{
	RU32			moduleId;
	RU32			tag;
	RU32			subTag;
	RSIZET			size;
    RU32            magic;

} rpal_memStub, *rpal_pMemStub;

#define RPAL_MEMORY_STUB_MAGIC  0x07f0adde

#define RPAL_MEMORY_GET_STUB(ptr) ( (rpal_pMemStub)((RPU8)(ptr) - sizeof( rpal_memStub )) )

static volatile RU32 g_rpal_memory_totalBytes = 0;


//=============================================================================
//  Detailed Memory Accounting
//=============================================================================
#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
RU32 g_rpal_memory_subTagBytes[ 1 << 20 ] = {0};
#endif

#endif

//=============================================================================
//  API
//=============================================================================

RPVOID
    rpal_memory_findMemory
    (

    )
{
    RPVOID found = NULL;

#ifdef RPAL_PLATFORM_WINDOWS
    MEMORY_BASIC_INFORMATION memInfo = {0};
    RPU8 ptr = NULL;
    RPVOID lastQueried = 0;

    while( 0 != VirtualQueryEx( GetCurrentProcess(), lastQueried, &memInfo, sizeof( memInfo ) ) )
    {
        if( MEM_FREE != memInfo.State && MEM_RESERVE != memInfo.State )
        {
            ptr = memInfo.BaseAddress;
            while( ptr < ( (RPU8)memInfo.BaseAddress + memInfo.RegionSize - sizeof( RU32 ) ) )
            {
                if( *(RPU32)ptr == RPAL_MEMORY_STUB_MAGIC )
                {
                    ptr += sizeof( RU32 );

    #ifdef RPAL_PLATFORM_DEBUG
                    ptr -= sizeof( rpal_memStub );
                    rpal_debug_info( "\nTag: %d\nSubTag: %d\nModuleId: %d\nSize: %d\nAddr: 0x%016X\n", 
                                     ((rpal_pMemStub)ptr)->tag,
                                     ((rpal_pMemStub)ptr)->subTag,
                                     ((rpal_pMemStub)ptr)->moduleId,
                                     ((rpal_pMemStub)ptr)->size,
                                     ptr + sizeof( rpal_memStub ) );
                    ptr += sizeof( rpal_memStub );
    #endif
                }

                ptr++;
            }
        }

        lastQueried = (RPU8)memInfo.BaseAddress + memInfo.RegionSize;
    }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
RPAL_PLATFORM_TODO( Finding memory leaks content in memory )
#endif

    return found;
}

RVOID
	_rpal_memory_warning
	(
		RPVOID ptr
	)
{
	UNREFERENCED_PARAMETER( ptr );
#ifdef RPAL_PLATFORM_WINDOWS
	DebugBreak();
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
	raise(SIGTRAP);
#endif
}


/*
 *
 *	Exported Memory Functions
 *
 */
RPAL_DEFINE_API
( 
RPVOID, 
    rpal_memory_allocEx, 
        RSIZET size, 
        RU32 tag, 
        RU32 subTag
)
{
	RPVOID ptr = NULL;

#ifndef RPAL_FEATURE_MEMORY_SECURITY
	UNREFERENCED_PARAMETER( tag );
	UNREFERENCED_PARAMETER( subTag );

	ptr = malloc( size );

	if( NULL == ptr )
	{
		_rpal_memory_warning( ptr );
	}
#else
	ptr = malloc( size + ( 2* sizeof( rpal_memStub ) ) );

	if( NULL != ptr )
	{
        ptr = (RPU8)ptr + sizeof( rpal_memStub );

		RPAL_MEMORY_GET_STUB( ptr )->moduleId = rpal_Context_getIdentifier();
		RPAL_MEMORY_GET_STUB( ptr )->size = size;
		RPAL_MEMORY_GET_STUB( ptr )->tag = tag;
		RPAL_MEMORY_GET_STUB( ptr )->subTag = subTag;
        RPAL_MEMORY_GET_STUB( ptr )->magic = RPAL_MEMORY_STUB_MAGIC;

		rpal_memory_memcpy( (RPU8)ptr + RPAL_MEMORY_GET_STUB( ptr )->size, 
							RPAL_MEMORY_GET_STUB( ptr ), 
							sizeof( rpal_memStub ) );

        rInterlocked_add32( &g_rpal_memory_totalBytes, (RS32)size );

#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
        rInterlocked_add32( &g_rpal_memory_subTagBytes[ subTag % ARRAY_N_ELEM( g_rpal_memory_subTagBytes ) ], (RS32)size );
#endif

        rpal_memory_zero( ptr, size );
	}
	else
	{
		_rpal_memory_warning( ptr );
	}
#endif

	return ptr;
}

RPAL_DEFINE_API
( 
RVOID, 
    rpal_memory_free, 
        RPVOID ptr
)
{
	if( rpal_memory_isValid( ptr ) )
	{
        rInterlocked_add32( &g_rpal_memory_totalBytes, (RS32)0 - (RS32)(RPAL_MEMORY_GET_STUB( ptr )->size) );

#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
        rInterlocked_add32( &g_rpal_memory_subTagBytes[ RPAL_MEMORY_GET_STUB( ptr )->subTag % ARRAY_N_ELEM( g_rpal_memory_subTagBytes ) ], (RS32)0 - (RS32)(RPAL_MEMORY_GET_STUB( ptr )->size) );
#endif

        rpal_memory_zero( ptr, (RPAL_MEMORY_GET_STUB( ptr )->size) );
        rpal_memory_zero( (RPU8)ptr + RPAL_MEMORY_GET_STUB( ptr )->size, sizeof( rpal_memStub ) );
        rpal_memory_zero( RPAL_MEMORY_GET_STUB( ptr ), sizeof( rpal_memStub ) );

		free( RPAL_MEMORY_GET_STUB( ptr ) );
	}
}

RPVOID
    rpal_memory_realloc_from
    (
        RPVOID originalPtr,
        RSIZET newSize,
        RU32 from
    )
{
	RPVOID newPtr = NULL;

    if( NULL == originalPtr )
    {
        newPtr = rpal_memory_alloc_from( newSize, from );
    }
    else
    {
        newPtr = rpal_memory_reAlloc( originalPtr, newSize );
    }

    return newPtr;
}

RPAL_DEFINE_API
( 
RPVOID, 
    rpal_memory_realloc, 
        RPVOID  originalPtr, 
        RSIZET  newSize
)
{
	RPVOID newPtr = NULL;
	RSIZET originalSize = 0;

#ifndef RPAL_FEATURE_MEMORY_SECURITY
	UNREFERENCED_PARAMETER( originalSize );
	newPtr = realloc( originalPtr, newSize );
#else
    if( NULL == originalPtr )
    {
        newPtr = rpal_memory_alloc( newSize );
    }
    else if( rpal_memory_isValid( originalPtr ) )
    {
        newPtr = realloc( RPAL_MEMORY_GET_STUB( originalPtr ), 
                          newSize + ( 2 * sizeof( rpal_memStub ) ) );

	    if( NULL != newPtr )
	    {
            newPtr = (RPU8)newPtr + sizeof( rpal_memStub );

            originalSize = RPAL_MEMORY_GET_STUB( newPtr )->size;
            RPAL_MEMORY_GET_STUB( newPtr )->size = newSize;

            if( originalSize < newSize )
            {
                rpal_memory_zero( (RPU8)newPtr + originalSize, sizeof( rpal_memStub ) );
            }

            rInterlocked_add32( &g_rpal_memory_totalBytes, (RS32)( newSize - originalSize ) );

#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
            rInterlocked_add32( &g_rpal_memory_subTagBytes[ RPAL_MEMORY_GET_STUB( newPtr )->subTag % ARRAY_N_ELEM( g_rpal_memory_subTagBytes ) ], (RS32)( newSize - originalSize ) );
#endif

		    rpal_memory_memcpy( (RPU8)newPtr + RPAL_MEMORY_GET_STUB( newPtr )->size, 
							    RPAL_MEMORY_GET_STUB( newPtr ), 
							    sizeof( rpal_memStub ) );

            if( !rpal_memory_isValid( newPtr ) )
            {
                rpal_debug_break();
            }
	    }
    }
#endif

	return newPtr;
}

RPAL_DEFINE_API
( 
RBOOL, 
    rpal_memory_isValid, 
        RPVOID ptr
)
{
	RBOOL isValid = FALSE;

	if( NULL != ptr )
	{
#ifndef RPAL_FEATURE_MEMORY_SECURITY
		isValid = TRUE;
#else
        if( RPAL_MEMORY_STUB_MAGIC == RPAL_MEMORY_GET_STUB(ptr)->magic &&
            rpal_memory_simpleMemcmp( RPAL_MEMORY_GET_STUB(ptr), 
									  (RPU8)ptr + RPAL_MEMORY_GET_STUB(ptr)->size, 
									  sizeof( rpal_memStub ) ) )
		{
			isValid = TRUE;
		}
        else
        {
		    _rpal_memory_warning( ptr );
	    }
#endif
	}
	
	return isValid;
}

RPAL_DEFINE_API
( 
RU32, 
    rpal_memory_totalUsed, 
)
{
    return g_rpal_memory_totalBytes;
}

RU32
	rpal_memory_memcmp
	(
		RPVOID	mem1,
		RPVOID	mem2,
		RSIZET	size
	)
{
	if( NULL == mem1 )
	{
		_rpal_memory_warning( mem1 );
	}

	if( NULL == mem2 )
	{
		_rpal_memory_warning( mem2 );
	}

	return memcmp( mem1, mem2, size );
}

__inline
RBOOL
	rpal_memory_simpleMemcmp
	(
		RPVOID	mem1,
		RPVOID	mem2,
		RSIZET	size
	)
{
	RBOOL isEqual = FALSE;
	RSIZET i = 0;

	if( NULL == mem1 )
	{
		_rpal_memory_warning( mem1 );
	}

	if( NULL == mem2 )
	{
		_rpal_memory_warning( mem2 );
	}

	if( NULL != mem1 &&
		NULL != mem2 )
	{
		isEqual = TRUE;

		for( i = 0; i < size; i++ )
		{
			if( ((RPU8)mem1)[ i ] != ((RPU8)mem2)[ i ] )
			{
				isEqual = FALSE;
				break;
			}
		}
	}

	return isEqual;
}

RVOID
	rpal_memory_memcpy
	(
		RPVOID dest,
		RPVOID src,
		RSIZET size
	)
{
    if( NULL == dest )
	{
		_rpal_memory_warning( dest );
	}

	if( NULL == src )
	{
		_rpal_memory_warning( src );
	}

	memcpy( dest, src, size );
}

RVOID
	rpal_memory_memmove
	(
		RPVOID dest,
		RPVOID src,
		RSIZET size
	)
{
    if( NULL == dest )
	{
		_rpal_memory_warning( dest );
	}

	if( NULL == src )
	{
		_rpal_memory_warning( src );
	}

    memmove( dest, src, size );
}

RVOID
	rpal_memory_zero
	(
		RPVOID ptr,
		RSIZET size
	)
{
	if( NULL != ptr )
	{
		memset( ptr, 0, size );
	}
	else
	{
		_rpal_memory_warning( ptr );
	}
}

RPVOID
	rpal_memory_memmem
	(
		RPVOID	haystack,
        RU32 haystackSize,
		RPVOID	needle,
        RU32 needleSize
	)
{
    RPVOID found = NULL;

    RU32 index = 0;

    if( NULL != haystack &&
        0 != haystackSize &&
        NULL != needle &&
        0 != needleSize &&
        haystackSize >= needleSize )
    {
        for( index = 0; index < ( haystackSize - needleSize ); index++ )
        {
            if( rpal_memory_simpleMemcmp( (RPU8)haystack + index, needle, needleSize ) )
            {
                found = (RPU8)haystack + index;
                break;
            }
        }
    }

    return found;
}

#pragma pack(pop)


RPVOID 
    rpal_ULongToPtr
    (
        RU64 ptr
    )
{
    RPVOID ret = NULL;
#ifdef RPAL_PLATFORM_32_BIT
#ifdef RPAL_PLATFORM_WINDOWS
    ret = ULongToPtr( ptr );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    ret = (RPVOID)(RU32)ptr;
#endif
#else
    ret = (RPVOID) ptr;
#endif
    return ret;
}



RPVOID
    rpal_memory_duplicate_from
    (
        RPVOID original,
        RU32 originalSize,
        RU32 from
    )
{
    RPVOID res = NULL;

    if( NULL != original )
    {
        if( NULL != ( res = rpal_memory_alloc_from( originalSize, from ) ) )
        {
            rpal_memory_memcpy( res, original, originalSize );
        }
    }

    return res;
}

#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
RPAL_DEFINE_API
( 
RVOID, 
    rpal_memory_printDetailedUsage, 
)
{
    RU32 i = 0;
    for( i = 0; i < ARRAY_N_ELEM( g_rpal_memory_subTagBytes ); i++ )
    {
        if( 0 != g_rpal_memory_subTagBytes[ i ] )
        {
            RU32 fileId = ( ( i & 0xff000 ) >> 12 );
            RU32 line = ( i & 0xfff );
            rpal_debug_info( "File ID %3d, line %4d: %4d bytes", fileId, line, g_rpal_memory_subTagBytes[ i ] );
        }
    }
}
#endif

