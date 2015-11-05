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

#ifndef _RPAL_BLOOM
#define _RPAL_BLOOM

#include <rpal/rpal.h>

//=============================================================================
//  PUBLIC STRUCTURES
//=============================================================================
typedef RPVOID rBloom;


//=============================================================================
//  PUBLIC API
//=============================================================================
rBloom
    rpal_bloom_create
    (
        RU32 nExpectedEntries,
        RDOUBLE errorRate
    );

RVOID
    rpal_bloom_destroy
    (
        rBloom bloom
    );

RBOOL
    rpal_bloom_present
    (
        rBloom bloom,
        RPVOID pBuff,
        RU32 size
    );

RBOOL
    rpal_bloom_add
    (
        rBloom bloom,
        RPVOID pBuff,
        RU32 size
    );

RU32
    rpal_bloom_getNumEntries
    (
        rBloom bloom
    );

RBOOL
    rpal_bloom_addIfNew
    (
        rBloom bloom,
        RPVOID pBuff,
        RU32 size
    );

RBOOL
    rpal_bloom_reset
    (
        rBloom bloom
    );

// NOTE: Bloom Filter serialization is NOT
// platform independant.
RBOOL
    rpal_bloom_serialize
    (
        rBloom bloom,
        RPU8* pBuff,
        RU32* pBuffSize
    );

rBloom
    rpal_bloom_deserialize
    (
        RPU8 pBuff,
        RU32 pBuffSize
    );

#endif
