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

#ifndef _RPAL_PRIV_HANDLE_MANAGER_H
#define _RPAL_PRIV_HANDLE_MANAGER_H

#include <rpal.h>


typedef struct
{
    rHandle h;
    rRefCount refCount;
    RPVOID val;
    rpal_handleManager_cleanup_f freeFunc;

} _handleInfo;

typedef struct
{
    RU32 nextMinor;
    rBTree btree;

} _handleMajorInfo;

extern _handleMajorInfo g_handleMajors[ 0xFF ];
extern rRwLock g_handleMajorLock;

RPAL_DECLARE_API
( 
rHandle, 
    rpal_handleManager_create_global,
        RU8 major,
        RU32 minor,
        RPVOID val,
        rpal_handleManager_cleanup_f cleanup
);

RPAL_DECLARE_API
( 
RBOOL, 
    rpal_handleManager_open_global,
        rHandle handle,
        RPVOID* pVal
);

RPAL_DECLARE_API
( 
RBOOL, 
    rpal_handleManager_openEx_global,
        RU8 major,
        RU32 minor,
        RPVOID* pVal
);

RPAL_DECLARE_API
( 
RBOOL,
    rpal_handleManager_close_global,
        rHandle handle,
        RBOOL* pIsDestroyed
);

RPAL_DECLARE_API
( 
RBOOL,
    rpal_handleManager_getValue_global,
        rHandle handle,
        RPVOID* pValue
);

#endif
