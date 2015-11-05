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
#include <librpcm/librpcm.h>
#include "collectors.h"
#include <notificationsLib/notificationsLib.h>
#include <libOs/libOs.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID          71

#ifdef RPAL_PLATFORM_WINDOWS
#include <windows_undocumented.h>
#pragma warning( disable: 4214 )
#include <WinDNS.h>

static HMODULE hDnsApi = NULL;
static DnsGetCacheDataTable_f getCache = NULL;
static DnsFree_f freeCacheEntry = NULL;
#endif

typedef struct
{
    RPWCHAR name;
    RU16 type;
    RU32 flags;

} _dnsRecord;


static
RVOID
    _freeRecords
    (
        rBlob recs
    )
{
    RU32 i = 0;
    _dnsRecord* pRec = NULL;

    if( NULL != recs )
    {
        i = 0;
        while( NULL != ( pRec = rpal_blob_arrElem( recs, sizeof( *pRec ), i++ ) ) )
        {
            if( NULL != pRec->name )
            {
                rpal_memory_free( pRec->name );
            }
        }
    }
}

static 
RPVOID
    dnsDiffThread
    (
        rEvent isTimeToStop,
        RPVOID ctx
    )
{
    RU32 nThLoop = 0;
    RU32 currentTimeout = 0;
    rSequence notif = NULL;
    rBlob snapCur = NULL;
    rBlob snapPrev = NULL;
    _dnsRecord rec = { 0 };
    _dnsRecord* pCurRec = NULL;
    _dnsRecord* pPrevRec = NULL;
    RU32 i = 0;
    RU32 j = 0;
    RBOOL isNew = FALSE;
    
#ifdef RPAL_PLATFORM_WINDOWS
    PDNSCACHEENTRY pDnsEntry = NULL;
    PDNSCACHEENTRY pPrevDnsEntry = NULL;
#endif

    UNREFERENCED_PARAMETER( ctx );

    while( !rEvent_wait( isTimeToStop, currentTimeout ) )
    {
        if( NULL != ( snapCur = rpal_blob_create( 0, 10 * sizeof( rec ) ) ) )
        {
#ifdef RPAL_PLATFORM_WINDOWS
            if( TRUE == getCache( &pDnsEntry ) )
            {
                while( NULL != pDnsEntry )
                {
                    rec.flags = pDnsEntry->dwFlags;
                    rec.type = pDnsEntry->wType;
                    if( NULL != ( rec.name = rpal_string_strdupw( pDnsEntry->pszName ) ) )
                    {
                        rpal_blob_add( snapCur, &rec, sizeof( rec ) );
                    }

                    pPrevDnsEntry = pDnsEntry;
                    pDnsEntry = pDnsEntry->pNext;

                    freeCacheEntry( pPrevDnsEntry->pszName, DnsFreeFlat );
                    freeCacheEntry( pPrevDnsEntry, DnsFreeFlat );
                }
            }
#endif

            // Do a general diff of the snapshots to find new entries.
            if( NULL != snapPrev )
            {
                i = 0;
                while( NULL != ( pCurRec = rpal_blob_arrElem( snapCur, sizeof( rec ), i++ ) ) )
                {
                    isNew = TRUE;
                    j = 0;
                    while( NULL != ( pPrevRec = rpal_blob_arrElem( snapPrev, sizeof( rec ), j++ ) ) )
                    {
                        if( pCurRec->flags == pPrevRec->flags &&
                            pCurRec->type == pPrevRec->type &&
                            0 == rpal_string_strcmpw( pCurRec->name, pPrevRec->name ) )
                        {
                            isNew = FALSE;
                            break;
                        }
                    }

                    if( isNew &&
                        !rEvent_wait( isTimeToStop, 0 ) )
                    {
                        if( NULL != ( notif = rSequence_new() ) )
                        {
                            rSequence_addSTRINGW( notif, RP_TAGS_DOMAIN_NAME, pCurRec->name );
                            rSequence_addRU16( notif, RP_TAGS_DNS_TYPE, pCurRec->type );
                            rSequence_addRU32( notif, RP_TAGS_DNS_FLAGS, pCurRec->flags );
                            rSequence_addTIMESTAMP( notif, RP_TAGS_TIMESTAMP, rpal_time_getGlobal() );

                            notifications_publish( RP_TAGS_NOTIFICATION_DNS_REQUEST, notif );

                            rSequence_free( notif );
                        }
                    }
                }
            }
        }

        if( NULL != snapPrev )
        {
            _freeRecords( snapPrev );
            rpal_blob_free( snapPrev );
            snapPrev = NULL;
        }

        snapPrev = snapCur;
        snapCur = NULL;

        nThLoop++;
        if( 0 == nThLoop % 20 )
        {
            currentTimeout = libOs_getUsageProportionalTimeout( MSEC_FROM_SEC( 10 ) ) + MSEC_FROM_SEC( 5 );
        }

        rpal_thread_sleep( currentTimeout );
    }

    if( NULL != snapPrev )
    {
        _freeRecords( snapPrev );
        rpal_blob_free( snapPrev );
        snapPrev = NULL;
    }

    return NULL;
}

RBOOL
    collector_2_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;
    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        RWCHAR apiName[] = _WCH( "dnsapi.dll" );
        RCHAR funcName1[] = "DnsGetCacheDataTable";
        RCHAR funcName2[] = "DnsFree";

        if( NULL != ( hDnsApi = LoadLibraryW( (RPWCHAR)&apiName ) ) )
        {
            if( NULL != ( getCache = (DnsGetCacheDataTable_f)GetProcAddress( hDnsApi, (RPCHAR)&funcName1 ) ) &&
                NULL != ( freeCacheEntry = (DnsFree_f)GetProcAddress( hDnsApi, (RPCHAR)&funcName2 ) ) )
            {
                isSuccess = TRUE;
            }
            else
            {
                rpal_debug_warning( "failed to get dns undocumented function" );
                FreeLibrary( hDnsApi );
            }
        }
        else
        {
            rpal_debug_warning( "failed to load dns api" );
        }
#endif
        if( isSuccess )
        {
            isSuccess = FALSE;

            if( rThreadPool_task( hbsState->hThreadPool, dnsDiffThread, NULL ) )
            {
                isSuccess = TRUE;
            }
        }
    }

    return isSuccess;
}

RBOOL
    collector_2_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        if( NULL != hDnsApi )
        {
            getCache = NULL;
            freeCacheEntry = NULL;
            FreeLibrary( hDnsApi );
        }
#endif
        isSuccess = TRUE;
    }

    return isSuccess;
}
