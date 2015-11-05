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

#define RPAL_FILE_ID       94

HbsState* g_state = NULL;

static RVOID
    exfilFunc
    (
        rpcm_tag notifId,
        rSequence notif
    )
{
    rSequence wrapper = NULL;
    rSequence tmpNotif = NULL;

    if( rpal_memory_isValid( notif ) &&
        NULL != g_state )
    {
        if( NULL != ( wrapper = rSequence_new() ) )
        {
            if( NULL != ( tmpNotif = rSequence_duplicate( notif ) ) )
            {
                if( rSequence_addSEQUENCE( wrapper, notifId, tmpNotif ) )
                {
                    if( !rQueue_add( g_state->outQueue, wrapper, 0 ) )
                    {
                        rSequence_free( wrapper );
                    }
                }
                else
                {
                    rSequence_free( wrapper );
                    rSequence_free( tmpNotif );
                }
            }
        }
    }
}


RBOOL
    collector_0_init
    ( 
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    rList subscribed = NULL;
    rpcm_tag notifId = 0;

    if( NULL != hbsState &&
        rpal_memory_isValid( config ) )
    {
        if( rSequence_getLIST( config, RP_TAGS_HBS_LIST_NOTIFICATIONS, &subscribed ) )
        {
            isSuccess = TRUE;
            g_state = hbsState;

            while( rList_getRU32( subscribed, RP_TAGS_HBS_NOTIFICATION_ID, &notifId ) )
            {
                if( !notifications_subscribe( notifId, NULL, 0, NULL, exfilFunc ) )
                {
                    isSuccess = FALSE;
                }
            }
        }
    }

    return isSuccess;
}

RBOOL 
    collector_0_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    rList subscribed = NULL;
    rpcm_tag notifId = 0;

    if( NULL != hbsState &&
        rpal_memory_isValid( config ) )
    {
        if( rSequence_getLIST( config, RP_TAGS_HBS_LIST_NOTIFICATIONS, &subscribed ) )
        {
            isSuccess = TRUE;

            while( rList_getRU32( subscribed, RP_TAGS_HBS_NOTIFICATION_ID, &notifId ) )
            {
                if( notifications_unsubscribe( notifId, NULL, exfilFunc ) )
                {
                    isSuccess = FALSE;
                }
            }
        }
    }

    return isSuccess;
}