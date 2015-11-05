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

#ifndef _OBFUSCATION_H
#define _OBFUSCATION_H

#include <rpal/rpal.h>

#define OBFUSCATIONLIB_COMPILE(val)
#define OBFUSCATIONLIB_DECLARE(varName,obfValue) RU8 varName[] = obfValue
#define OBFUSCATIONLIB_TOGGLE(pBuffer) (obfuscationLib_toggle((RPU8)(pBuffer),sizeof(pBuffer),(RPU8)OBFUSCATIONLIB_KEY,sizeof(OBFUSCATIONLIB_KEY)-1))


RPVOID
    obfuscationLib_toggle
    (
        RPVOID pBuffer,
        RU32 bufferSize,
        RPU8 pKey,
        RU32 keySize
    );

#endif

