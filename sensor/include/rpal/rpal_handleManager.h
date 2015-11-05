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

#ifndef _RPAL_HANDLE_MANAGER_H
#define _RPAL_HANDLE_MANAGER_H

#include <rpal/rpal.h>


rHandle
    rpal_handleManager_create
    (
        RU8 major,
        RU32 minor,
        RPVOID val,
        rpal_handleManager_cleanup_f cleanup
    );

RBOOL
    rpal_handleManager_open
    (
        rHandle handle,
        RPVOID* pVal
    );

RBOOL
    rpal_handleManager_openEx
    (
        RU8 major,
        RU32 minor,
        RPVOID* pVal
    );

RBOOL
    rpal_handleManager_close
    (
        rHandle handle,
        RBOOL* pIsDestroyed
    );

RBOOL
    rpal_handleManager_getValue
    (
        rHandle handle,
        RPVOID* pValue
    );

#endif
