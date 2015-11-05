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

#ifndef _RPAL_STACK
#define _RPAL_STACK

#include <rpal/rpal.h>

typedef RPVOID	rStack;
typedef RBOOL (*rStack_freeFunc)( RPVOID elem );
typedef RBOOL (*rStack_compareFunc)( RPVOID elem, RPVOID ref );

rStack
    rStack_new
    (
        RU32 elemSize
    );

RBOOL
    rStack_free
    (
        rStack stack,
        rStack_freeFunc freeFunc
    );

RBOOL
    rStack_push
    (
        rStack stack,
        RPVOID elem
    );

RBOOL
    rStack_pop
    (
        rStack stack,
        RPVOID pOutElem
    );

RBOOL
    rStack_isEmpty
    (
        rStack stack
    );

RBOOL
    rStack_removeWith
    (
        rStack stack,
        rStack_compareFunc compFunc,
        RPVOID ref,
        RPVOID pElem
    );

#endif
