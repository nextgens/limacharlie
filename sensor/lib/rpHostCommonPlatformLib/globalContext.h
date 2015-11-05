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

#ifndef _RP_HCP_GLOBAL_CONTEXT_H
#define _RP_HCP_GLOBAL_CONTEXT_H

#include <rpal/rpal.h>
#include <cryptoLib/cryptoLib.h>
#include <MemoryModule/MemoryModule.h>
#include <rpHostCommonPlatformLib/rpHCP_Common.h>
#include <librpcm/librpcm.h>

#pragma pack(push)
#pragma pack(1)

#define RP_HCP_CONTEXT_MAX_MODULES      10
#ifdef RPAL_PLATFORM_DEBUG
#define RP_HCP_CONTEXT_MODULE_TIMEOUT   (3600*1000)
#else
#define RP_HCP_CONTEXT_MODULE_TIMEOUT   (30*1000)
#endif


typedef struct
{
    RpHcp_ModuleId id;
    rThread hThread;
    rEvent isTimeToStop;
    HMEMORYMODULE hModule;
    rpHCPModuleContext context;
    RU8 hash[ CRYPTOLIB_HASH_SIZE ];
    RBOOL isOsLoaded;

} rpHCPModuleInfo;


typedef struct
{
    rpHCPId agentId;
    RU32 enrollmentTokenSize;
    RU8 enrollmentToken[];

} rpHCPIdentStore;


typedef struct
{
    // Global State
    RU32 isRunning;

    // Global Info
    rpHCPId currentId;

    // Beacon Management
    rEvent isBeaconTimeToStop;
    rThread hBeaconThread;
    RU64 beaconTimeout;
    RPCHAR primaryUrl;
    RPCHAR secondaryUrl;

    // Modules Management
    rpHCPModuleInfo modules[ RP_HCP_CONTEXT_MAX_MODULES ];

    // Ident token
    RPU8 enrollmentToken;
    RU32 enrollmentTokenSize;

} rpHCPContext;


extern rpHCPContext g_hcpContext;


rSequence
    hcpIdToSeq
    (
        rpHCPId id
    );

rpHCPId
    seqToHcpId
    (
        rSequence
    );


#pragma pack(pop)

#endif

