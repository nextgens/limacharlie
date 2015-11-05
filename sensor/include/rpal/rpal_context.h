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

#ifndef _RPAL_CONTEXT_H
#define _RPAL_CONTEXT_H

#include <rpal.h>

#define RPAL_VERSION_1          1
#define RPAL_VERSION_CURRENT    RPAL_VERSION_1

// Helpers to define API Calls into RPAL
#define RPAL_API_REF(funcName)                  g_rpal_context->_##funcName
#define RPAL_API_CALL(funcName,...)             (RPAL_API_REF(funcName)(__VA_ARGS__))
#define RPAL_DECLARE_API(retType,funcName,...)  retType _##funcName(__VA_ARGS__)
#define RPAL_DEFINE_API(retType,funcName,...)   retType _##funcName(__VA_ARGS__)
#define RPAL_API_PTR(retType,funcName,...)      retType (*_##funcName)(__VA_ARGS__)

// RPAL context definition
typedef struct
{
	RU8		version;

	// Memory Management
    RPAL_API_PTR( RPVOID,   rpal_memory_allocEx,    RSIZET size, RU32 tag, RU32 subTag );
	RPAL_API_PTR( RVOID,    rpal_memory_free,       RPVOID ptr );
	RPAL_API_PTR( RPVOID,   rpal_memory_realloc,    RPVOID ptr, RSIZET newSize );
	RPAL_API_PTR( RBOOL,    rpal_memory_isValid,    RPVOID ptr );
    RPAL_API_PTR( RU32,     rpal_memory_totalUsed, );
    RPAL_API_PTR( RU64,     rpal_time_getGlobal, );
    RPAL_API_PTR( RU64,     rpal_time_setGlobalOffset, RU64 offset );
    RPAL_API_PTR( rHandle,  rpal_handleManager_create_global,   RU8 major,      RU32 minor, RPVOID val,     rpal_handleManager_cleanup_f cleanup );
    RPAL_API_PTR( RBOOL,    rpal_handleManager_open_global,     rHandle handle, RPVOID* pVal );
    RPAL_API_PTR( RBOOL,    rpal_handleManager_openEx_global,   RU8 major,      RU32 minor, RPVOID* pVal );
    RPAL_API_PTR( RBOOL,    rpal_handleManager_close_global,    rHandle handle, RBOOL* isDestroyed );
    RPAL_API_PTR( RBOOL,    rpal_handleManager_getValue_global, rHandle handle, RPVOID* pValue );
#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
    RPAL_API_PTR( RVOID,     rpal_memory_printDetailedUsage, );
#endif
    

} rpal_Context, *rpal_PContext;


typedef struct
{
    RU32 identifier;
    rHandle hModule;
    rBTree local_handles;

} rpal_private_module_context;




// API
rpal_PContext
	rpal_Context_get
	(

	);

RU32
	rpal_Context_getIdentifier
	(

	);

// Initialization of rpal is handled by rpal_module.c/h which must be
// be manually included into every project to provide centralized API.

// This must be called prior to memory leak detection if it is used.
RVOID
    rpal_Context_cleanup
    (

    );

// This must be called at the very end of execution, after memory leak
// detection if it is used.
RVOID
    rpal_Context_deinitialize
    (

    );


#endif
