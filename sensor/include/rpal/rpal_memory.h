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

#ifndef _RPAL_MEMORY_H
#define _RPAL_MEMORY_H

#include <rpal.h>


#define rpal_memory_allocEx(size,tag,subTag)        RPAL_API_CALL(rpal_memory_allocEx,(size),(tag),(subTag))
#define rpal_memory_free(ptr)                       RPAL_API_CALL(rpal_memory_free,(ptr))
#define rpal_memory_reAlloc(originalPtr,newSize)    RPAL_API_CALL(rpal_memory_realloc,(originalPtr),(newSize))
#define rpal_memory_isValid(ptr)                    RPAL_API_CALL(rpal_memory_isValid,(ptr))
#define rpal_memory_totalUsed()                     RPAL_API_CALL(rpal_memory_totalUsed,)
#define rpal_memory_printDetailedUsage()            RPAL_API_CALL(rpal_memory_printDetailedUsage,)

#define RPAL_LINE_SUBTAG                             ( ( RPAL_FILE_ID << 12 ) | __LINE__ )
#define rpal_memory_alloc_from( size, from )         ( rpal_memory_allocEx( (size), 0, from ) )
#define rpal_memory_alloc( size )                    ( rpal_memory_alloc_from( (size), RPAL_LINE_SUBTAG ) )
#define rpal_memory_duplicate( original, size )      ( rpal_memory_duplicate_from( (original), (size), RPAL_LINE_SUBTAG ) )
#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
#define rpal_memory_realloc(originalPtr,newSize)    rpal_memory_realloc_from( (originalPtr), (newSize), RPAL_LINE_SUBTAG )
#else
#define rpal_memory_realloc(originalPtr,newSize)    rpal_memory_reAlloc((originalPtr),(newSize))
#endif

#define FREE_AND_NULL(ptr)                          rpal_memory_free( (ptr) );\
                                                    (ptr) = NULL;

#define DESTROY_AND_NULL(ptr,destFunc)              destFunc(ptr);\
                                                    (ptr) = NULL;

RPVOID
    rpal_memory_realloc_from
    (
        RPVOID originalPtr,
        RSIZET newSize,
        RU32 from
    );

RPVOID
    rpal_memory_findMemory
    (

    );

RU32
	rpal_memory_memcmp
	(
		RPVOID	mem1,
		RPVOID	mem2,
		RSIZET	size
	);

RBOOL
	rpal_memory_simpleMemcmp
	(
		RPVOID	mem1,
		RPVOID	mem2,
		RSIZET	size
	);

RVOID
	rpal_memory_memcpy
	(
		RPVOID dest,
		RPVOID src,
		RSIZET size
	);

RVOID
	rpal_memory_memmove
	(
		RPVOID dest,
		RPVOID src,
		RSIZET size
	);

RVOID
	rpal_memory_zero
	(
		RPVOID ptr,
		RSIZET size
	);

RPVOID
	rpal_memory_memmem
	(
		RPVOID	haystack,
        RU32 haystackSize,
		RPVOID	needle,
        RU32 needleSize
	);

RPVOID 
    rpal_ULongToPtr
    (
        RU64 ptr
    );

RPVOID
    rpal_memory_duplicate_from
    (
        RPVOID original,
        RU32 originalSize,
        RU32 from
    );

#endif
