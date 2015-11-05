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

#include <rpal/rpal.h>
#include <librpcm/librpcm.h>
#include "collectors.h"
#include <notificationsLib/notificationsLib.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID       80

#define _HISTORY_MAX_LENGTH     (1000)
#define _HISTORY_MAX_SIZE       (1024*1024*1)

rpcm_tag g_collected[] = { RP_TAGS_NOTIFICATION_NEW_PROCESS,
                           RP_TAGS_NOTIFICATION_TERMINATE_PROCESS,
                           RP_TAGS_NOTIFICATION_DNS_REQUEST,
                           RP_TAGS_NOTIFICATION_CODE_IDENTITY,
                           RP_TAGS_NOTIFICATION_NEW_TCP4_CONNECTION,
                           RP_TAGS_NOTIFICATION_NEW_UDP4_CONNECTION,
                           RP_TAGS_NOTIFICATION_HIDDEN_MODULE_DETECTED,
                           RP_TAGS_NOTIFICATION_MODULE_LOAD,
                           RP_TAGS_NOTIFICATION_FILE_CREATE,
                           RP_TAGS_NOTIFICATION_FILE_DELETE,
                           RP_TAGS_NOTIFICATION_NETWORK_SUMMARY };

static rMutex g_mutex = NULL;
static RU32 g_cur_size = 0;
static rSequence g_history[ _HISTORY_MAX_LENGTH ] = { 0 };
static RU32 g_history_head = 0;
static HbsState* g_state = NULL;

static
RVOID
    recordEvent
    (
        rpcm_tag notifId,
        rSequence notif
    )
{
    RU32 i = 0;

    UNREFERENCED_PARAMETER( notifId );

    if( rpal_memory_isValid( notif ) )
    {
        if( rMutex_lock( g_mutex ) )
        {
            if( g_history_head >= ARRAY_N_ELEM( g_history ) )
            {
                g_history_head = 0;
            }

            if( NULL != g_history[ g_history_head ] )
            {
                g_cur_size -= rSequence_getEstimateSize( g_history[ g_history_head ] );

                rSequence_free( g_history[ g_history_head ] );
                g_history[ g_history_head ] = NULL;
            }

            g_history[ g_history_head ] = rSequence_duplicate( notif );

            g_cur_size += rSequence_getEstimateSize( notif );

            i = g_history_head + 1;
            while( _HISTORY_MAX_SIZE < g_cur_size )
            {
                if( i >= ARRAY_N_ELEM( g_history ) )
                {
                    i = 0;
                }

                g_cur_size -= rSequence_getEstimateSize( g_history[ i ] );

                rSequence_free( g_history[ i ] );
                g_history[ i ] = NULL;
                i++;
            }

            g_history_head++;

            rpal_debug_info( "History size: %d KB", ( g_cur_size / 1024 ) );

            rMutex_unlock( g_mutex );
        }
    }
}

static 
RVOID
    dumpHistory
    (
        rpcm_tag notifId,
        rSequence notif
    )
{
    RU32 i = 0;
    rSequence tmp = NULL;
    UNREFERENCED_PARAMETER( notifId );
    UNREFERENCED_PARAMETER( notif );

    if( rMutex_lock( g_mutex ) )
    {
        for( i = 0; i < ARRAY_N_ELEM( g_history ); i++ )
        {
            if( rpal_memory_isValid( g_history[ i ] ) &&
                NULL != ( tmp = rSequence_duplicate( g_history[ i ] ) ) )
            {
                if( !rQueue_add( g_state->outQueue, tmp, 0 ) )
                {
                    rSequence_free( tmp );
                }
            }
        }

        rMutex_unlock( g_mutex );
    }
}


RBOOL
    collector_12_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    RU32 i = 0;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( NULL != ( g_mutex = rMutex_create() ) )
        {
            isSuccess = TRUE;

            rpal_memory_zero( g_history, sizeof( g_history ) );

            for( i = 0; i < ARRAY_N_ELEM( g_collected ); i++ )
            {
                if( !notifications_subscribe( g_collected[ i ], NULL, 0, NULL, recordEvent ) )
                {
                    isSuccess = FALSE;
                    break;
                }
            }

            if( !isSuccess )
            {
                for( i = i; i > 0; i-- )
                {
                    notifications_unsubscribe( g_collected[ i ], NULL, recordEvent );
                }
                notifications_unsubscribe( g_collected[ 0 ], NULL, recordEvent );

                rMutex_free( g_mutex );
                g_mutex = NULL;
            }
        }
    }

    return isSuccess;
}

RBOOL
    collector_12_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;
    RU32 i = 0;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        isSuccess = TRUE;
        g_state = hbsState;

        for( i = 0; i < ARRAY_N_ELEM( g_collected ); i++ )
        {
            if( notifications_unsubscribe( g_collected[ i ], NULL, recordEvent ) )
            {
                isSuccess = FALSE;
            }
        }

        if( rMutex_lock( g_mutex ) )
        {
            for( i = 0; i < ARRAY_N_ELEM( g_history ); i++ )
            {
                if( rpal_memory_isValid( g_history[ i ] ) )
                {
                    rSequence_free( g_history[ i ] );
                    g_history[ i ] = NULL;
                }
            }
        }

        rMutex_free( g_mutex );
        g_mutex = NULL;
    }

    return isSuccess;
}