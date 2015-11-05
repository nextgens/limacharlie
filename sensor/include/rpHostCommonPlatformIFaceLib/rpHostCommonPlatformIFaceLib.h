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

#ifndef _RP_HCP_IFACE_H
#define _RP_HCP_IFACE_H

#include <rpal/rpal.h>
#include <rpHostCommonPlatformLib/rpHCP_Common.h>


//=============================================================================
//  Module Must Implement
//=============================================================================
// extern RpHcp_ModuleId g_Current_Module_id;
// extern RU32 (RPAL_THREAD_FUNC *RpHcpI_mainThread)( rEvent isTimeToStop );

//=============================================================================
//  API For rpHcpLib
//=============================================================================
RU32
RPAL_EXPORT
RPAL_THREAD_FUNC
    rpHcpI_entry
    (
        rpHCPModuleContext* moduleContext
    );

//=============================================================================
//  API For Module
//=============================================================================
RBOOL
    rpHcpI_beaconHome
    (
        rList requests,
        rList* responses
    );

RBOOL
    rpHcpI_getId
    (
        rpHCPId* pId
    );

rSequence
    rpHcpI_hcpIdToSeq
    (
        rpHCPId id
    );

rpHCPId
    rpHcpI_seqToHcpId
    (
        rSequence seq
    );

#endif
