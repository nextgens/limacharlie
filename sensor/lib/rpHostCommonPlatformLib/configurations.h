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

#ifndef _RP_HCP_CONFIG_H
#define _RP_HCP_CONFIG_H

#include "obfuscated.h"
#include <obfuscationLib/obfuscationLib.h>
#include "deployments.h"

#define RP_HCP_CONFIG_DEFAULT_BEACON_TIMEOUT        (1*60*60) // In seconds
#define RP_HCP_CONFIG_DEFAULT_BEACON_TIMEOUT_INIT   (1*10) // In seconds

// Undefine this for release since it allows the local and manual load of modules
// bypassing the crypto.
#ifdef RPAL_PLATFORM_DEBUG
#ifndef RP_HCP_LOCAL_LOAD
#define RP_HCP_LOCAL_LOAD
#endif
#endif

#define DEFAULT_AGENT_ID_ORG        1
#define DEFAULT_AGENT_ID_SUBNET     1

#endif

