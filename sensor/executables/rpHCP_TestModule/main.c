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

#include <rpal/rpal.h>
#include <rpHostCommonPlatformIFaceLib/rpHostCommonPlatformIFaceLib.h>
//=============================================================================
//  RP HCP Module Requirements
//=============================================================================
RpHcp_ModuleId g_current_Module_id = 42;


RU32
RPAL_THREAD_FUNC
    RpHcpI_mainThread
    (
        rEvent isTimeToStop
    )
{
    RU32 ret = 0;

    FORCE_LINK_THAT(HCP_IFACE);
    

    while( !rEvent_wait( isTimeToStop, (10*1000) ) )
    {
        rpal_debug_info("RP HCP Test Module Running...\n");
    }

    return ret;
}

