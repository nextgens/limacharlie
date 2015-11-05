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

#ifndef _RP_HCP_CRYPTO_H
#define _RP_HCP_CRYPTO_H

#include <rpal/rpal.h>
#include <cryptoLib/cryptoLib.h>

RVOID
    setC2PublicKey
    (
        RPU8 key
    );

RVOID
    setRootPublicKey
    (
        RPU8 key
    );

RVOID
    freeKeys
    (

    );

RPU8
    getC2PublicKey
    (

    );

RPU8
    getRootPublicKey
    (

    );


RBOOL
    verifyC2Signature
    (
        RPU8 buffer,
        RU32 bufferSize,
        RU8 signature[ CRYPTOLIB_SIGNATURE_SIZE ]
    );

#endif
