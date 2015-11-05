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
#include "rpal_context.h"
#include <rpal/master_only/rpal_privateHandleManager.h>

// Context instance
rpal_PContext   g_rpal_context = NULL;
rpal_private_module_context g_rpal_localContext = {0};
RBOOL           g_rpal_is_root_context = FALSE;
RU32            g_rpal_nrecursiveInit = 0;

rpal_PContext
	rpal_Context_get
	(

	)
{
	return g_rpal_context;
}

RU32
	rpal_Context_getIdentifier
	(

	)
{
    return g_rpal_localContext.identifier;
}

RVOID
    rpal_Context_cleanup
    (

    )
{
    RU32 index = 0;
    _handleMajorInfo* pMajor = NULL;
    RBOOL isDepFree = FALSE;

    if( 1 >= rInterlocked_get32( &g_rpal_nrecursiveInit ) )
    {
        if( NULL != g_rpal_localContext.local_handles )
        {
            // Unregister our context
            if( rpal_handleManager_close( g_rpal_localContext.hModule, &isDepFree ) )
            {
                if( !isDepFree )
                {
                    // Someone else has our handle open, wtf am I supposed TODO?
                }
            }

            rpal_btree_destroy( g_rpal_localContext.local_handles, FALSE );
        }

        if( g_rpal_is_root_context )
        {
            rRwLock_write_lock( g_handleMajorLock );

            for( index = 0; index < 0xFF; index++ )
            {
                pMajor = (_handleMajorInfo*)&(g_handleMajors[ index ]);

                if( NULL != pMajor->btree )
                {
                    rpal_btree_destroy( pMajor->btree, FALSE );
                }
            }

            rRwLock_free( g_handleMajorLock );
        }
    }
}

RVOID
    rpal_Context_deinitialize
    (

    )
{
    if( g_rpal_is_root_context )
    {
        if( 0 == rInterlocked_decrement32( &g_rpal_nrecursiveInit ) )
        {
            free( g_rpal_context );
        }
    }
}
