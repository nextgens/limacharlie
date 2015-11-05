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

#ifndef _NOTIFICATIONS_LIB_H
#define _NOTIFICATIONS_LIB_H


#include <rpal/rpal.h>
#include <librpcm/librpcm.h>


#define NOTIFICATIONS_FLAG_MATCH_ALL        0x00000001
#define NOTIFICATIONS_FLAG_MATCH_ANY        0x00000002
#define NOTIFICATIONS_FLAG_MATCH_NONE       0x00000003


typedef RVOID (*notifications_callback_func)( rpcm_tag notifType, rSequence notifData );


RBOOL
    notifications_publish
    (
        rpcm_tag notifType,
        rSequence notifData
    );

RBOOL
    notifications_subscribe
    (
        rpcm_tag notifType,
        rSequence filter,                       // Filter is freed by the lib
        RU32 flags,
        rQueue notifQueue,                      // OPTIONAL
        notifications_callback_func callback    // OPTIONAL
        // One of pNotifQueue or callback MUST be specified as notification method
        // User is responsible for freeing the notification sequence when received through
        // the a notification queue.
    );

RBOOL
    notifications_unsubscribe
    (
        rpcm_tag notifType,
        rQueue notifQueue,
        notifications_callback_func callback
    );

// To be used as cleanup function in notification queues
RVOID
    notifications_free
    (
        rSequence notification,
        RU32 unusedSize
    );

#endif
