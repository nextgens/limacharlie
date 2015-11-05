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

#ifndef _LIB_URL_H
#define _LIB_URL_H

#include <rpal/rpal.h>

typedef rString rUrl;

RPCHAR
    liburl_encode
    (
        RPCHAR str
    );

rUrl
    liburl_create
    (

    );

RVOID
    liburl_free
    (
        rUrl url
    );

RBOOL
    liburl_add_param_str
    (
        rUrl url,
        RPCHAR paramName,
        RPCHAR paramVal
    );

RBOOL
    liburl_add_param_buff
    (
        rUrl url,
        RPCHAR paramName,
        RPU8 paramVal,
        RU32 paramValSize
    );

#endif
