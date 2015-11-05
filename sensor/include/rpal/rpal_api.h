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

#ifndef _RPAL_API_H
#define _RPAL_API_H

#include <rpal.h>


typedef struct
{
    rHandle hApi;
    RU32 version;
    RU32 nFunctions;
    // RPVOID functions[];

} rpal_apiHeader;


RBOOL
    rpal_api_register
    (
        RU32 componentId,
        rpal_apiHeader* api,
        rpal_handleManager_cleanup_f cleanup
    );

RBOOL
    rpal_api_acquire
    (
        RU32 componentId,
        rpal_apiHeader** pApi
    );

RBOOL
    rpal_api_release
    (
        rpal_apiHeader* api
    );

#endif
