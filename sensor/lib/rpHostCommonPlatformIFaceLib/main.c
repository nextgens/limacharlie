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

#include <rpHostCommonPlatformIFaceLib/rpHostCommonPlatformIFaceLib.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID     49

FORCE_LINK_THIS(HCP_IFACE);

#pragma warning( disable: 4127 ) // Disabling error on constant expression in condition

rpHCPModuleContext* g_Module_Context = NULL;
extern RpHcp_ModuleId g_current_Module_id;
extern RU32 (RPAL_THREAD_FUNC RpHcpI_mainThread)( rEvent isTimeToStop );

RU32
#ifdef RPAL_PLATFORM_WINDOWS
__declspec(dllexport)
#endif
RPAL_THREAD_FUNC
    rpHcpI_entry
    (
        rpHCPModuleContext* moduleContext
    )
{
    RU32 ret = (RU32)(-1);
    rThread hMain = 0;

    if( NULL != moduleContext )
    {
        g_Module_Context = moduleContext;

        if( rpal_initialize( moduleContext->rpalContext, g_current_Module_id ) )
        {
            ret = (RU32)(-2);

            if( 0 != ( hMain = rpal_thread_new( RpHcpI_mainThread, 
                                                g_Module_Context->isTimeToStop ) ) )
            {
                rpal_debug_info( "main module worker started" );
                ret = 0;

                while( TRUE )
                {
                    if( rpal_thread_wait( hMain, ( 1 * 1000 ) ) )
                    {
                        break;
                    }
                }
                
                rpal_debug_info( "main module worker finished" );

                rpal_thread_free( hMain );
            }
            else
            {
                rpal_debug_error( "failed spawning module main worker" );
            }

            rpal_Context_cleanup();

            rpal_Context_deinitialize();
        }
        else
        {
            rpal_debug_error( "failed IFace init" );
        }
    }

    return ret;
}

RBOOL
    rpHcpI_beaconHome
    (
        rList requests,
        rList* responses
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != g_Module_Context &&
        NULL != g_Module_Context->func_beaconHome )
    {
        isSuccess = g_Module_Context->func_beaconHome( g_current_Module_id, requests, responses );
    }

    return isSuccess;
}


RBOOL
    rpHcpI_getId
    (
        rpHCPId* pId
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != g_Module_Context &&
        NULL != g_Module_Context->pCurrentId &&
        NULL != pId )
    {
        isSuccess = TRUE;
        *pId = *g_Module_Context->pCurrentId;
    }

    return isSuccess;
}

rSequence
    rpHcpI_hcpIdToSeq
    (
        rpHCPId id
    )
{
    rSequence seq = NULL;

    if( NULL != ( seq = rSequence_new() ) )
    {
        if( !rSequence_addRU8( seq, RP_TAGS_HCP_ID_ORG, id.id.orgId ) ||
            !rSequence_addRU8( seq, RP_TAGS_HCP_ID_SUBNET, id.id.subnetId ) ||
            !rSequence_addRU32( seq, RP_TAGS_HCP_ID_UNIQUE, id.id.uniqueId ) ||
            !rSequence_addRU8( seq, RP_TAGS_HCP_ID_PLATFORM, id.id.platformId ) ||
            !rSequence_addRU8( seq, RP_TAGS_HCP_ID_CONFIG, id.id.configId ) )
        {
            DESTROY_AND_NULL( seq, rSequence_free );
        }
    }

    return seq;
}

rpHCPId
    rpHcpI_seqToHcpId
    (
        rSequence seq
    )
{
    rpHCPId id = {0};

    if( NULL != seq )
    {
        if( !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_ORG, 
                               &(id.id.orgId) ) ||
            !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_SUBNET, 
                               &(id.id.subnetId) ) ||
            !rSequence_getRU32( seq, 
                                RP_TAGS_HCP_ID_UNIQUE, 
                                &(id.id.uniqueId) ) ||
            !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_PLATFORM, 
                               &(id.id.platformId) ) ||
            !rSequence_getRU8( seq, 
                               RP_TAGS_HCP_ID_CONFIG, 
                               &(id.id.configId) ) )
        {
            id.raw = 0;
        }
    }

    return id;
}

