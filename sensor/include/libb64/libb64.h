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

#ifndef _LIB_B64_H
#define _LIB_B64_H

#include <rpal/rpal.h>

#define BASE64_TERMINATOR_MAX_SIZE  4


#define base64_decode( iB, pOB, pOBS, iUE )   base64_decode_from( iB, pOB, pOBS, iUE, RPAL_LINE_SUBTAG )
RBOOL
    base64_decode_from
    (
        RPCHAR inputBuffer,
        RPVOID* pOutputBuffer,
        RPU32 pOutputBufferSize,
        RBOOL isUrlEncoded,
        RU32 from
    );


#define base64_encode( iB, iBS, pOB, iUE )    base64_encode_from( iB, iBS, pOB, iUE, RPAL_LINE_SUBTAG )
RBOOL
    base64_encode_from
    (
        RPVOID inputBuffer,
        RU32 inputBufferSize,
        RPCHAR* pOutputBuffer,
        RBOOL isUrlEncode,
        RU32 from
    );

#endif

