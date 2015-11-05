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

#ifndef _RP_HCP_COMMON_H
#define _RP_HCP_COMMON_H

#include <rpal/rpal.h>
#include <librpcm/librpcm.h>

#pragma pack(push)
#pragma pack(1)

typedef union
{
    struct
    {
        RU8 orgId;
        RU8 subnetId;
        RU8 platformId;
        RU8 configId;
        RU32 uniqueId;

    } id;
    
    RU64 raw;
} rpHCPId;


/*
 *
 *  HCP Platform Scheme
 *
 * Total 8 bits
 * [ 2: CPU Arch | 3: Platform Major | 3: Platform Minor ]
 *
 * CPU Architecture: 2 bits
 * - 0: *RESERVED* (Maybe Sparc?)
 * - 1: x86 (32bit)
 * - 2: x64 (64bit)
 * - 3: *MASK*
 *
 * OS Major: 3 bits
 * - 0: *UNKNOWN*
 * - 1: Windows
 * - 2: OSX
 * - 3: IOS
 * - 4: Android
 * - 5: Linux
 * - 6: *RESERVED*
 * - 7: *MASK*
 *
 * OS Minor: 3 bits
 * (OS Dependant)
 * - Linux: 1-Ubuntu 2-CentOS5 3-CentOS6
 *
 * Examples: Windows 64 bit: 10-001-000 = 0x84
 *           Windows 32 bit: 01-001-000 = 0x44
 *           All Windows: 11-001-000 = 0xB4
 *                        OR
 *                        11-001-111 = 0xBF (Since Windows has no minor for now)
 *           Linux Ubuntu 32 bit: 01-101-001 = 0x69
 *           Linux Ubuntu 64 bit: 10-101-001 = 0xA9
 */

#define RP_HCP_PLATFORM_CPU_ARCH(plat)      ((RU8)(((plat) & 0xB0)>>6))
#define RP_HCP_PLATFORM_MAJOR(plat)         ((RU8)(((plat) & 0x38)>>3))
#define RP_HCP_PLATFORM_MINOR(plat)         ((RU8)((plat) & 0x06))

#define RP_HCP_PLATFORM_ARCH_UNKNOWN        0x00
#define RP_HCP_PLATFORM_ARCH_X86            0x01
#define RP_HCP_PLATFORM_ARCH_X64            0x02
#define RP_HCP_PLATFORM_ARCH_ANY            0x03

#define RP_HCP_PLATFORM_MAJOR_ANY           0x07
#define RP_HCP_PLATFORM_MINOR_ANY           0x07

#define RP_HCP_PLATFORM_MINOR_UNKNOWN       0x00

#define RP_HCP_PLATFORM_MAJOR_WINDOWS       0x01

#define RP_HCP_PLATFORM_MAJOR_MACOSX        0x02

#define RP_HCP_PLATFORM_MAJOR_IOS           0x03

#define RP_HCP_PLATFORM_MAJOR_ANDROID       0x04

#define RP_HCP_PLATFORM_MAJOR_LINUX         0x05
#define RP_HCP_PLATFORM_MINOR_LINUX_UBUNTU_12       0x01
#define RP_HCP_PLATFORM_MINOR_LINUX_CENTOS_5        0x02
#define RP_HCP_PLATFORM_MINOR_LINUX_CENTOS_6        0x03
#define RP_HCP_PLATFORM_MINOR_LINUX_UBUNTU_14       0x04

// Current Platform
#ifndef RP_HCP_PLATFORM_CURRENT_ARCH
#ifdef RPAL_PLATFORM_64_BIT
#define RP_HCP_PLATFORM_CURRENT_CPU RP_HCP_PLATFORM_ARCH_X64
#else
#define RP_HCP_PLATFORM_CURRENT_CPU RP_HCP_PLATFORM_ARCH_X86
#endif
#endif

#ifndef RP_HCP_PLATFORM_CURRENT_MAJOR

#ifdef RPAL_PLATFORM_WINDOWS
#define RP_HCP_PLATFORM_CURRENT_MAJOR       RP_HCP_PLATFORM_MAJOR_WINDOWS
#elif defined( RPAL_PLATFORM_ANDROID )  /* Make it precede over Linux, as Android also defines RPAL_PLATFORM_LINUX. */
#define RP_HCP_PLATFORM_CURRENT_MAJOR       RP_HCP_PLATFORM_MAJOR_ANDROID
#elif defined( RPAL_PLATFORM_LINUX )
#define RP_HCP_PLATFORM_CURRENT_MAJOR       RP_HCP_PLATFORM_MAJOR_LINUX
#elif defined( RPAL_PLATFORM_MACOSX )
#define RP_HCP_PLATFORM_CURRENT_MAJOR       RP_HCP_PLATFORM_MAJOR_MACOSX
#elif defined( RPAL_PLATFORM_IOS )
#define RP_HCP_PLATFORM_CURRENT_MAJOR       RP_HCP_PLATFORM_MAJOR_IOS
#endif

#endif

#ifndef RP_HCP_PLATFORM_CURRENT_MINOR
#define RP_HCP_PLATFORM_CURRENT_MINOR RP_HCP_PLATFORM_MINOR_UNKNOWN
#endif


#define RP_HCP_ID_MAKE_PLATFORM(cpu,major,minor)   ((RU8)( ((RU8)(cpu)<<6) | ((RU8)(major)<<3) | ((RU8)(minor)) ))

typedef RU8 RpHcp_ModuleId;
#define RP_HCP_MODULE_ID_BOOTSTRAP          0
#define RP_HCP_MODULE_ID_HCP                1
#define RP_HCP_MODULE_ID_HBS                2
#define RP_HCP_MODULE_ID_TEST               3
#define RP_HCP_MODULE_ID_AAD                4

typedef struct
{
    rpal_PContext rpalContext;
    rpHCPId* pCurrentId;
    RBOOL (*func_beaconHome)( RpHcp_ModuleId sourceModuleId, rList toSend, rList* pReceived );
    rEvent isTimeToStop;

} rpHCPModuleContext;

#pragma pack(pop)
#endif
