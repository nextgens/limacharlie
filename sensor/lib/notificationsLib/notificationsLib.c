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

#include <notificationsLib/notificationsLib.h>
#include <rpal/rpal.h>

#define RPAL_FILE_ID     42


typedef struct
{
    rSequence filter;
    RU32 flags;
    rQueue notifQueue;
    notifications_callback_func callback;

} _notificationSub;

static
RVOID
    _cleanup_notification_sub
    (
        _notificationSub* pNotif,
        RU32 dummy
    )
{

    UNREFERENCED_PARAMETER( dummy );

    if( rpal_memory_isValid( pNotif ) )
    {
        if( NULL != pNotif->filter )
        {
            rSequence_free( pNotif->filter );
        }

        rpal_memory_free( pNotif );
    }
}

static
RBOOL
    _cleanup_notifications
    (
        rCollection col
    )
{
    RBOOL isSuccess = FALSE;

    if( rpal_memory_isValid( col ) )
    {
        isSuccess = rpal_collection_freeWithFunc( col, (collection_free_func)_cleanup_notification_sub );
    }

    return isSuccess;
}


static
RBOOL
    _is_filter_match
    (
        rSequence notification,
        rSequence filter,
        RU32 flag
    )
{
    RBOOL isMatch = FALSE;

    rIterator ite = NULL;

    rpcm_tag tag = 0;
    rpcm_type type = 0;
    RPVOID pElem = NULL;
    RU32 elemSize = 0;

    RPVOID pNotifElem = NULL;
    RU32 notifElemSize = 0;

    if( NULL != notification &&
        NULL != filter )
    {
        if( NULL != ( ite = rIterator_new( filter ) ) )
        {
            if( NOTIFICATIONS_FLAG_MATCH_ALL == flag )
            {
                isMatch = TRUE;
            }

            while( rIterator_next( ite, &tag, &type, &pElem, &elemSize ) )
            {
                if( NOTIFICATIONS_FLAG_MATCH_ALL == flag )
                {
                    if( !rSequence_getElement( notification, &tag, &type, &pNotifElem, &notifElemSize ) ||
                        !( type >= RPCM_COMPLEX_TYPES ? 
                           _is_filter_match( pNotifElem, pElem, flag ) : 
                           rpcm_isElemEqual( type, pElem, elemSize, pNotifElem, notifElemSize ) ) )
                    {
                        isMatch = FALSE;
                        break;
                    }
                }
                else if( NOTIFICATIONS_FLAG_MATCH_ANY == flag )
                {
                    if( rSequence_getElement( notification, &tag, &type, &pNotifElem, &notifElemSize ) &&
                        ( type >= RPCM_COMPLEX_TYPES ? 
                          _is_filter_match( pNotifElem, pElem, flag ) : 
                          rpcm_isElemEqual( type, pElem, elemSize, pNotifElem, notifElemSize ) ) )
                    {
                        isMatch = TRUE;
                        break;
                    }
                }
                else if( NOTIFICATIONS_FLAG_MATCH_NONE == flag )
                {
                    if( rSequence_getElement( notification, &tag, &type, &pNotifElem, &notifElemSize ) &&
                        !( type >= RPCM_COMPLEX_TYPES ? 
                           _is_filter_match( pNotifElem, pElem, flag ) : 
                           rpcm_isElemEqual( type, pElem, elemSize, pNotifElem, notifElemSize ) ) )
                    {
                        isMatch = FALSE;
                        break;
                    }
                }
            }

            rIterator_free( ite );
        }
    }
    else
    {
        isMatch = TRUE;
    }

    return isMatch;
}


RBOOL
    notifications_publish
    (
        rpcm_tag notifType,
        rSequence notifData
    )
{
    RBOOL isSuccess = FALSE;

    rCollection col = NULL;
    rCollectionIterator ite = NULL;
    _notificationSub* sub = NULL;

    rSequence dupSeq = NULL;

    rHandle hSub = RPAL_HANDLE_INIT;

    if( NULL == notifData )
    {
        notifData = rSequence_new();
    }

    if( NULL != notifData )
    {
        hSub.info.major = RPAL_HANDLES_MAJOR_NOTIFICATIONS;
        hSub.info.minor = notifType;

        if( rpal_handleManager_open( hSub, &col ) )
        {
            if( rpal_collection_createIterator( col, &ite ) )
            {
                isSuccess = TRUE;

                while( rpal_collection_next( ite, (RPVOID*)&sub, NULL ) )
                {
                    if( _is_filter_match( notifData, sub->filter, sub->flags ) )
                    {
                        // We have a match
                        if( NULL != sub->notifQueue &&
                            NULL != ( dupSeq = rSequence_duplicate( notifData ) ) )
                        {
                            if( !rQueue_add( sub->notifQueue, dupSeq, sizeof( dupSeq ) ) )
                            {
                                rSequence_free( dupSeq );
                            }
                        }

                        if( NULL != sub->callback )
                        {
                            sub->callback( notifType, notifData );
                        }
                    }
                }

                rpal_collection_freeIterator( ite );
            }

            rpal_handleManager_close( hSub, NULL );
        }
        else
        {
            // Nobody is registered, so we'll say all is ok
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}



static
RBOOL
    _is_sub_elem
    (
        _notificationSub* colSub,
        RU32 dummy,
        _notificationSub* compSub
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( dummy );

    if( NULL != colSub &&
        NULL != compSub )
    {
        if( colSub->callback == compSub->callback &&
            colSub->notifQueue == compSub->notifQueue )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}


RBOOL
    notifications_subscribe
    (
        rpcm_tag notifType,
        rSequence filter,
        RU32 flags,
        rQueue notifQueue,                      // OPTIONAL
        notifications_callback_func callback    // OPTIONAL
        // One of pNotifQueue or callback MUST be specified as notification method
        // User is responsible for freeing the notification sequence when received through
        // the a notification queue.
    )
{
    RBOOL isSuccess = FALSE;

    rCollection col = NULL;
    rHandle hNotif = {0};
    _notificationSub* sub = NULL;
    _notificationSub compSub = {0};

    if( NULL != notifQueue ||
        NULL != callback )
    {
        if( !rpal_handleManager_openEx( RPAL_HANDLES_MAJOR_NOTIFICATIONS, notifType, &col ) )
        {
            col = NULL;

            // The notif type doesn't exist yet so we'll create it
            if( rpal_collection_create( &col, (collection_free_func)_cleanup_notification_sub ) )
            {
                hNotif = rpal_handleManager_create( RPAL_HANDLES_MAJOR_NOTIFICATIONS, notifType, col, _cleanup_notifications );

                if( RPAL_HANDLE_INVALID == hNotif.h )
                {
                    rpal_collection_free( col );
                    col = NULL;
                }
            }
            else
            {
                col = NULL;
            }
        }

        if( NULL != col )
        {
            compSub.callback = callback;
            compSub.notifQueue = notifQueue;

            // Is it already registered?
            if( !rpal_collection_isPresent( col, (collection_compare_func)_is_sub_elem, &compSub ) )
            {
                sub = rpal_memory_alloc( sizeof( *sub ) );

                if( NULL != sub )
                {
                    sub->callback = callback;
                    sub->filter = filter;
                    sub->flags = flags;
                    sub->notifQueue = notifQueue;

                    if( rpal_collection_add( col, sub, sizeof( *sub ) ) )
                    {
                        isSuccess = TRUE;
                    }
                    else
                    {
                        rpal_memory_free( sub );
                    }
                }
            }
            else
            {
                isSuccess = TRUE;
            }
        }
    }

    return isSuccess;
}


RBOOL
    notifications_unsubscribe
    (
        rpcm_tag notifType,
        rQueue notifQueue,
        notifications_callback_func callback
    )
{
    RBOOL isSuccess = FALSE;

    rCollection notifCollection = NULL;
    _notificationSub* sub = NULL;
    _notificationSub compSub = {0};
    rHandle hSub = RPAL_HANDLE_INIT;

    if( NULL != notifQueue ||
        NULL != callback )
    {
        compSub.callback = callback;
        compSub.notifQueue = notifQueue;

        hSub.info.major = RPAL_HANDLES_MAJOR_NOTIFICATIONS;
        hSub.info.minor = notifType;

        if( rpal_handleManager_open( hSub, &notifCollection ) )
        {
            if( rpal_collection_remove( notifCollection, (RPVOID*)&sub, NULL, (collection_compare_func)_is_sub_elem, &compSub ) )
            {
                _cleanup_notification_sub( sub, 0 );
                isSuccess = TRUE;
                // We do another close on the handle since we do an open on every Subscribe
                // that way the handle gets magically deleted once there are no more subscribers
                rpal_handleManager_close( hSub, NULL );
            }

            rpal_handleManager_close( hSub, NULL );
        }
    }

    return isSuccess;
}


RVOID
    notifications_free
    (
        rSequence notification,
        RU32 unusedSize
    )
{
    UNREFERENCED_PARAMETER( unusedSize );

    if( NULL != notification )
    {
        rSequence_free( notification );
    }
}

