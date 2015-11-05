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

#ifndef _RP_HCP_H
#define _RP_HCP_H

#include <rpal/rpal.h>

RBOOL
    rpHostCommonPlatformLib_launch
    (
        RU8 configHint,
        RPCHAR primaryHomeUrl,
        RPCHAR secondaryHomeUrl
    );

RBOOL
    rpHostCommonPlatformLib_stop
    (

    );

RBOOL
    rpHostCommonPlatformLib_load
    (
        RPWCHAR modulePath
    );

RBOOL
    rpHostCommonPlatformLib_unload
    (
        RU8 moduleId
    );

#endif

