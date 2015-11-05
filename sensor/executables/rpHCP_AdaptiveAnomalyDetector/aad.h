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

#ifndef _AAD_H
#define _AAD_H

#include <rpal.h>
#include <librpcm/librpcm.h>

#define AAD_TRAINING_MIN_RELATIONS          10
#define AAD_TRAINING_MAX_TIME               (60*60*24)

#define AAD_REL_PROCESS_MODULE              0
#define AAD_REL_MODULE_HASH                 1
#define AAD_REL_MODULE_PATH                 2
#define AAD_REL_PROCESS_PATH                3
#define AAD_REL_HASH_MODULE                 4

#define AAD_REL_MAX                         5

RBOOL
    aad_checkRelation_WCH_WCH
    (
        rBTree stash,
        RPWCHAR parentKey,
        RPWCHAR childKey
    );

RBOOL
    aad_checkRelation_WCH_HASH
    (
        rBTree stash,
        RPWCHAR parentKey,
        RPU8 childKey
    );

RBOOL
    aad_checkRelation_HASH_WCH
    (
        rBTree stash,
        RPU8 parentKey,
        RPWCHAR childKey
    );

RBOOL
    aad_enableParent_WCH
    (
        rBTree stash,
        RPWCHAR parentKey,
        RU32 nExpected,
        RDOUBLE nFPRatio
    );

RBOOL
    aad_enablePhase1Parent_WCH
    (
        rBTree stash,
        RPWCHAR parentKey
    );

RBOOL
    aad_enableParent_HASH
    (
        rBTree stash,
        RPU8 parentKey,
        RU32 nExpected,
        RDOUBLE nFPRatio
    );

RBOOL
    aad_loadStashFromBuffer
    (
        rBTree stash,
        RPU8 pBuffer,
        RU32 bufferSize
    );

RBOOL
    aad_dumpStashToBuffer
    (
        rBTree stash,
        RPU8* pOutBuffer,
        RU32* pOutBufferSize
    );

RBOOL
    aad_isParentEnabled_WCH
    (
        rBTree stash,
        RPWCHAR parentKey
    );

rBTree
    aad_initStash_phase_1
    (

    );

rBTree
    aad_initStash_phase_2
    (

    );

rSequence
    aad_newReport_WCH_WCH
    (
        RU32 relTypeId,
        RPWCHAR parentKey,
        RPWCHAR childKey
    );

rSequence
    aad_newReport_WCH_HASH
    (
        RU32 relTypeId,
        RPWCHAR parentKey,
        RPU8 childKey
    );

rSequence
    aad_newReport_HASH_WCH
    (
        RU32 relTypeId,
        RPU8 parentKey,
        RPWCHAR childKey
    );

#endif
