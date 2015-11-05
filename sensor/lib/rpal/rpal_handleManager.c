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

#include <rpal_handleManager.h>
#include <rpal/master_only/rpal_privateHandleManager.h>
#include <rpal/master_only/rpal_privateContext.h>

#ifdef RPAL_PLATFORM_WINDOWS
#pragma warning( disable: 4127 ) // Disabling error on constant expression in condition
#endif

#define rpal_handleManager_create_global(major,minor,val,cleanup)       RPAL_API_CALL(rpal_handleManager_create_global,(major),(minor),(val),(cleanup))
#define rpal_handleManager_open_global(handle,pVal)                     RPAL_API_CALL(rpal_handleManager_open_global,(handle),(pVal))
#define rpal_handleManager_openEx_global(major,minor,pVal)              RPAL_API_CALL(rpal_handleManager_openEx_global,(major),(minor),(pVal))
#define rpal_handleManager_close_global(handle,pIsDestroyed)            RPAL_API_CALL(rpal_handleManager_close_global,(handle),(pIsDestroyed))
#define rpal_handleManager_getValue_global(handle,pValue)               RPAL_API_CALL(rpal_handleManager_getValue_global,(handle),(pValue))


// Global tracking
_handleMajorInfo g_handleMajors[ 0xFF ] = {{0}};
rRwLock g_handleMajorLock = NULL;

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


RPAL_DEFINE_API
(
rHandle,
    rpal_handleManager_create_global,
        RU8 major,
        RU32 minor,
        RPVOID val,
        rpal_handleManager_cleanup_f cleanup
)
{
    rHandle newHandle = RPAL_HANDLE_INIT;
    volatile _handleMajorInfo* pMajor = NULL;
    _handleInfo tmpInfo = {{0}};
    tmpInfo.h.h = (RU64)RPAL_HANDLE_INVALID;

    do
    {
        if( ((RU8)RPAL_HANDLE_INVALID) != major )
        {
            pMajor = &(g_handleMajors[ major ]);

            if( rRwLock_read_lock( g_handleMajorLock ) )
            {
                if( NULL == pMajor->btree )
                {
                    rRwLock_read_unlock( g_handleMajorLock );

                    if( rRwLock_write_lock( g_handleMajorLock ) )
                    {
                        if( NULL == pMajor->btree )
                        {
                            pMajor->btree = rpal_btree_create( sizeof( _handleInfo ), 
                                                               (rpal_btree_comp_f)_findHandle,
                                                               NULL );
                        }

                        rRwLock_write_unlock( g_handleMajorLock );
                    }
                }
                else
                {
                    rRwLock_read_unlock( g_handleMajorLock );
                }

                if( NULL == pMajor->btree )
                {
                    break;
                }
            }

            tmpInfo.refCount = rRefCount_create( NULL, val, sizeof( _handleInfo ) );
            tmpInfo.val = val;
            tmpInfo.freeFunc = cleanup;
            tmpInfo.h.info.major = major;
            
            if( rRwLock_read_lock( g_handleMajorLock ) )
            {
                if( NULL != tmpInfo.refCount )
                {
                    if( RPAL_HANDLE_INVALID == minor )
                    {
                        // An anonymous handle is requested, we just go one-up
                        tmpInfo.h.info.minor = rInterlocked_increment32( &(pMajor->nextMinor) );
                        newHandle.h = tmpInfo.h.h;

                        if( !rpal_btree_add( pMajor->btree, &tmpInfo, FALSE ) )
                        {
                            rRefCount_destroy( tmpInfo.refCount );
                            newHandle.h = (RU64)RPAL_HANDLE_INVALID;
                            rInterlocked_decrement32( &(pMajor->nextMinor) );
                            rRwLock_read_unlock( g_handleMajorLock );
                            break;
                        }
                    }
                    else
                    {
                        // This is a "named" handle
                        tmpInfo.h.info.minor = minor;
                        newHandle.h = tmpInfo.h.h;

                        if( rpal_btree_search( pMajor->btree, &newHandle, NULL, FALSE ) )
                        {
                            // Already exists!
                            newHandle.h = (RU64)RPAL_HANDLE_INVALID;
                            rRefCount_destroy( tmpInfo.refCount );
                            rRwLock_read_unlock( g_handleMajorLock );
                            break;
                        }
                        else
                        {
                            if( !rpal_btree_add( pMajor->btree, &tmpInfo, FALSE ) )
                            {
                                newHandle.h = (RU64)RPAL_HANDLE_INVALID;
                                rRefCount_destroy( tmpInfo.refCount );
                                rRwLock_read_unlock( g_handleMajorLock );
                                break;
                            }
                        }
                    }
                }
                else
                {
                    newHandle.h = (RU64)RPAL_HANDLE_INVALID;
                    rRwLock_read_unlock( g_handleMajorLock );
                    break;
                }

                rRwLock_read_unlock( g_handleMajorLock );
            }
        }
    } while( FALSE );

    return newHandle;
}

RPAL_DEFINE_API
(
RBOOL,
    rpal_handleManager_open_global,
        rHandle handle,
        RPVOID* pVal
)
{
    RBOOL isSuccess = FALSE;
    _handleMajorInfo* pMajor = NULL;
    _handleInfo pInfo = {{0}};

    if( ((RU8)RPAL_HANDLE_INVALID) != handle.info.major &&
        RPAL_HANDLE_INVALID != handle.info.minor )
    {
        pMajor = &(g_handleMajors[ handle.info.major ]);

        if( NULL != pMajor->btree )
        {
            if( rpal_btree_search( pMajor->btree, &handle, &pInfo, FALSE ) )
            {
                if( rRefCount_acquire( pInfo.refCount ) )
                {
                    if( NULL != pVal )
                    {
                        *pVal = pInfo.val;
                    }

                    isSuccess = TRUE;
                }
            }
        }
    }

    return isSuccess;
}

RPAL_DEFINE_API
(
RBOOL,
    rpal_handleManager_openEx_global,
        RU8 major,
        RU32 minor,
        RPVOID* pVal
 )
{
    RBOOL isSuccess = FALSE;

    rHandle tmpHandle = RPAL_HANDLE_INIT;

    tmpHandle.info.major = major;
    tmpHandle.info.minor = minor;

    isSuccess = rpal_handleManager_open_global( tmpHandle, pVal );

    return isSuccess;
}

RPAL_DEFINE_API
(
RBOOL,
    rpal_handleManager_close_global,
        rHandle handle,
        RBOOL* pIsDestroyed
)
{
    RBOOL isSuccess = FALSE;
    _handleMajorInfo* pMajor = NULL;
    _handleInfo info = {{0}};
    RBOOL isReleased = FALSE;

    if( ((RU8)RPAL_HANDLE_INVALID) != handle.info.major &&
        RPAL_HANDLE_INVALID != handle.info.minor )
    {
        pMajor = &(g_handleMajors[ handle.info.major ]);

        if( NULL != pMajor->btree )
        {
            if( rpal_btree_manual_lock( pMajor->btree ) )
            {
                if( rpal_btree_search( pMajor->btree, &handle, &info, TRUE ) )
                {
                    if( rRefCount_release( info.refCount, &isReleased ) )
                    {
                        isSuccess = TRUE;

                        if( isReleased )
                        {
                            if( !rpal_btree_remove( pMajor->btree, &handle, NULL, TRUE ) )
                            {
                                isSuccess = FALSE;
                            }
                            else
                            {
                                if( NULL != info.freeFunc )
                                {
                                    isSuccess = info.freeFunc( info.val );
                                }
                            }
                        }

                        if( NULL != pIsDestroyed )
                        {
                            *pIsDestroyed = isReleased;
                        }
                    }
                }

                rpal_btree_manual_unlock( pMajor->btree );
            }
        }
    }

    return isSuccess;
}

RPAL_DEFINE_API
(
RBOOL,
    rpal_handleManager_getValue_global,
        rHandle handle,
        RPVOID* pValue
)
{
    RBOOL isSuccess = FALSE;
    _handleMajorInfo* pMajor = NULL;
    _handleInfo info = {{0}};

    if( ((RU8)RPAL_HANDLE_INVALID) != handle.info.major &&
        RPAL_HANDLE_INVALID != handle.info.minor &&
        NULL != pValue )
    {
        pMajor = &(g_handleMajors[ handle.info.major ]);

        if( NULL != pMajor->btree )
        {
            if( 0 == rpal_btree_search( pMajor->btree, &handle, &info, FALSE ) )
            {
                *pValue = info.val;
                isSuccess = TRUE;
            }
        }
    }

    return isSuccess;
}


//=============================================================================
//  LOCAL API
//=============================================================================


rHandle
    rpal_handleManager_create
    (
        RU8 major,
        RU32 minor,
        RPVOID val,
        rpal_handleManager_cleanup_f cleanup
    )
{
    rHandle ret = RPAL_HANDLE_INIT;

    ret = rpal_handleManager_create_global( major, minor, val, cleanup );

    if( RPAL_HANDLE_INVALID != ret.h )
    {
        rpal_btree_add( g_rpal_localContext.local_handles, &ret, FALSE );
    }

    return ret;
}

RBOOL
    rpal_handleManager_open
    (
        rHandle handle,
        RPVOID* pVal
    )
{
    RBOOL isSuccess = FALSE;

    isSuccess = rpal_handleManager_open_global( handle, pVal );

    return isSuccess;
}

RBOOL
    rpal_handleManager_openEx
    (
        RU8 major,
        RU32 minor,
        RPVOID* pVal
    )
{
    RBOOL isSuccess = FALSE;

    isSuccess = rpal_handleManager_openEx_global( major, minor, pVal );

    return isSuccess;
}

RBOOL
    rpal_handleManager_close
    (
        rHandle handle,
        RBOOL* pIsDestroyed
    )
{
    RBOOL isSuccess = FALSE;
    RBOOL isDestroyed = FALSE;

    isSuccess = rpal_handleManager_close_global( handle, &isDestroyed );

    if( isSuccess &&
        isDestroyed )
    {
        rpal_btree_remove( g_rpal_localContext.local_handles, &handle, NULL, FALSE );

        if( NULL != pIsDestroyed )
        {
            *pIsDestroyed = isDestroyed;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_handleManager_getValue
    (
        rHandle handle,
        RPVOID* pValue
    )
{
    return rpal_handleManager_getValue_global( handle, pValue );
}
