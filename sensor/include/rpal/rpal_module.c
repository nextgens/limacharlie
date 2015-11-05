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

#include <rpal/rpal_module.h>
#define RPAL_FILE_ID      13


//=============================================================================
//  This file is included in each project and must be compile with the actual
//  project's pre-processor so that real-functions are not referenced and
//  therefore linked into "slave" projects.
//=============================================================================
// #define RPAL_MODE_MASTER
// #define RPAL_MODE_SLAVE


#ifdef RPAL_MODE_MASTER
#include <rpal/master_only/rpal_privateContext.h>
#include <rpal/master_only/rpal_privateMemory.h>
#include <rpal/master_only/rpal_privateTime.h>
#include <rpal/master_only/rpal_privateHandleManager.h>
#elif !defined( RPAL_MODE_SLAVE )
#define RPAL_MODE_SLAVE
#endif

// Contex should exist from code rpal lib.
extern rpal_PContext    g_rpal_context;
extern RBOOL            g_rpal_is_root_context;
extern rpal_private_module_context g_rpal_localContext;
extern RU32             g_rpal_nrecursiveInit;

static
RS32
    _findHandle
    (
        rHandle* hToFind,
        rHandle* hCurr
    )
{
    RS32 ret = 0;

    if( NULL != hToFind &&
        NULL != hCurr )
    {
        ret = (RS32)rpal_memory_memcmp( hToFind, hCurr, sizeof( *hToFind ) );
    }

    return ret;
}

RBOOL
	rpal_initialize
	(
		rpal_PContext	context,
		RU32			logicalIdentifier
	)
{
	RBOOL isSuccess = FALSE;

    rpal_private_module_context* tmpContext = NULL;
    rHandle hCurrentModule = RPAL_HANDLE_INIT;

    g_rpal_localContext.identifier = logicalIdentifier;

    rpal_srand( (RU32)rpal_time_getLocal() );

	if( NULL == context )
	{
#ifdef RPAL_MODE_MASTER
        if( NULL == g_rpal_context )
        {
            g_rpal_context = malloc( sizeof( *g_rpal_context ) );

            if( NULL != g_rpal_context )
            {

                g_rpal_is_root_context = TRUE;
                g_rpal_context->version = RPAL_VERSION_CURRENT;

		        // This is the core library.

		        // Setup the Memory Management
                RPAL_API_REF( rpal_memory_allocEx )     = _rpal_memory_allocEx;
                RPAL_API_REF( rpal_memory_free )        = _rpal_memory_free;
                RPAL_API_REF( rpal_memory_isValid )     = _rpal_memory_isValid;
                RPAL_API_REF( rpal_memory_realloc )     = _rpal_memory_realloc;
                RPAL_API_REF( rpal_memory_totalUsed )   = _rpal_memory_totalUsed;
                RPAL_API_REF( rpal_time_getGlobal )     = _rpal_time_getGlobal;
                RPAL_API_REF( rpal_time_setGlobalOffset ) = _rpal_time_setGlobalOffset;
                RPAL_API_REF( rpal_handleManager_create_global ) = _rpal_handleManager_create_global;
                RPAL_API_REF( rpal_handleManager_open_global ) = _rpal_handleManager_open_global;
                RPAL_API_REF( rpal_handleManager_openEx_global ) = _rpal_handleManager_openEx_global;
                RPAL_API_REF( rpal_handleManager_close_global ) = _rpal_handleManager_close_global;
                RPAL_API_REF( rpal_handleManager_getValue_global ) = _rpal_handleManager_getValue_global;
#ifdef RPAL_FEATURE_MEMORY_ACCOUNTING
                RPAL_API_REF( rpal_memory_printDetailedUsage )   = _rpal_memory_printDetailedUsage;
#endif

                if( NULL != ( g_handleMajorLock = rRwLock_create() ) )
                {
                    rInterlocked_increment32( &g_rpal_nrecursiveInit );
		            isSuccess = TRUE;
                }
                else
                {
                    free( g_rpal_context );
                    g_rpal_context = NULL;
                }
            }
        }
        else
        {
            rInterlocked_increment32( &g_rpal_nrecursiveInit );
            isSuccess = TRUE;
        }
#endif
	}
#ifdef RPAL_MODE_SLAVE
    else if( RPAL_VERSION_CURRENT <= context->version )
	{
		// We bind this instance to the core.
		g_rpal_context = context;
		isSuccess = TRUE;
	}
#endif

    if( isSuccess )
    {
        // If the local_handles is already create it means that
        // someone local already initialised rpal so no need to proceed...
        if( NULL == g_rpal_localContext.local_handles )
        {
            // Let's setup the local context
            g_rpal_localContext.local_handles = rpal_btree_create( sizeof( rHandle ), 
                                                                   (rpal_btree_comp_f)_findHandle,
                                                                   NULL );
            if( NULL == g_rpal_localContext.local_handles )
            {
                isSuccess = FALSE;
            }
            else
            {
                hCurrentModule.info.major = RPAL_HANDLES_MAJOR_MODULECONTEXT;
                hCurrentModule.info.minor = logicalIdentifier;

                // Let's register our local context
                // Is our component registered?
                if( rpal_handleManager_open( hCurrentModule, 
                                             (RPVOID)&tmpContext ) )
                {
                    // Seems our component is registered, is it just a double-registration
                    // or is it a different instance?
                    if( tmpContext != &g_rpal_localContext )
                    {
                        // Different instance, this is bad, bail out!
                        isSuccess = FALSE;
                    }
                    else
                    {
                        // Double init... all good, ignore.
                    }

                    // Either way, release the handle
                    rpal_handleManager_close( hCurrentModule, NULL );
                }
                else
                {
                    // Our component is not registered, do it.
                    hCurrentModule = rpal_handleManager_create( RPAL_HANDLES_MAJOR_MODULECONTEXT,
                                                                logicalIdentifier,
                                                                &g_rpal_localContext,
                                                                NULL );
                    if( RPAL_HANDLE_INVALID == hCurrentModule.h )
                    {
                        // There was an error registering, bail out.
                        isSuccess = FALSE;
                    }
                    else
                    {
                        g_rpal_localContext.hModule.h = hCurrentModule.h;
                    }
                }
            }


            if( !isSuccess )
            {
                if( NULL != g_rpal_localContext.local_handles )
                {
                    rpal_btree_destroy( g_rpal_localContext.local_handles, FALSE );
                }

                if( g_rpal_is_root_context )
                {
                    free( g_rpal_context );
                    g_rpal_context = NULL;
                    g_rpal_is_root_context = FALSE;
                }
            }
        }

#ifdef RPAL_MODE_MASTER
        if( !isSuccess && g_rpal_is_root_context )
        {
            rRwLock_free( g_handleMajorLock );
            free( g_rpal_context );
            g_rpal_context = NULL;
        }
#endif
    }

	return isSuccess;
}

