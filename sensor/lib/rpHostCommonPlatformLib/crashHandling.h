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

#ifndef RP_HCP_CRASH_H
#define RP_HCP_CRASH_H
#include <rpal/rpal.h>
#include <librpcm/librpcm.h>

RBOOL
    acquireCrashContextPresent
    (
        RPU8* pCrashContext,
        RU32* pCrashContextSize
    );

RBOOL
    setCrashContext
    (
        RPU8 crashContext,
        RU32 crashContextSize
    );

RBOOL
    cleanCrashContext
    (

    );

RU32
    getCrashContextSize
    (

    );

RBOOL
    setGlobalCrashHandler
    (

    );

void
    forceCrash
    (

    );

#endif
