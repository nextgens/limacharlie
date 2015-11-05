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
#include <networkLib/networkLib.h>
#include <libOs/libOs.h>
#include <rpHostCommonPlatformLib/rTags.h>

#define RPAL_FILE_ID        79

static
RBOOL
    isTcpEqual
    (
        NetLib_Tcp4TableRow* row1,
        NetLib_Tcp4TableRow* row2
    )
{
    RBOOL isEqual = FALSE;

    if( NULL != row1 &&
        NULL != row2 &&
        row1->destIp == row2->destIp &&
        row2->destPort == row2->destPort &&
        row1->sourceIp == row2->sourceIp &&
        row1->sourcePort == row2->sourcePort )
    {
        isEqual = TRUE;
    }

    return isEqual;
}

static
RBOOL
    isUdpEqual
    (
        NetLib_UdpTableRow* row1,
        NetLib_UdpTableRow* row2
    )
{
    RBOOL isEqual = FALSE;

    if( NULL != row1 &&
        NULL != row2 &&
        row1->localIp == row2->localIp &&
        row2->localPort == row2->localPort )
    {
        isEqual = TRUE;
    }

    return isEqual;
}

static 
RPVOID
    networkDiffThread
    (
        rEvent isTimeToStop,
        RPVOID ctx
    )
{
    NetLib_Tcp4Table* currentTcp4Table = NULL;
    NetLib_Tcp4Table* oldTcp4Table = NULL;

    NetLib_UdpTable* currentUdpTable = NULL;
    NetLib_UdpTable* oldUdpTable = NULL;

    RU32 i = 0;
    RU32 j = 0;
    RBOOL isFound = FALSE;

    rSequence notif = NULL;
    rSequence comp = NULL;

    RU32 timeout = 100;
    RU32 nThLoop = 0;

    UNREFERENCED_PARAMETER( ctx );

    while( rpal_memory_isValid( isTimeToStop ) &&
           !rEvent_wait( isTimeToStop, 0 ) )
    {
        if( NULL != oldTcp4Table )
        {
            rpal_memory_free( oldTcp4Table );
            oldTcp4Table = NULL;
        }

        if( NULL != oldUdpTable )
        {
            rpal_memory_free( oldUdpTable );
            oldUdpTable = NULL;
        }

        // Swap the old snapshot for the (prev) new one
        oldTcp4Table = currentTcp4Table;
        oldUdpTable = currentUdpTable;
        currentTcp4Table = NULL;
        currentUdpTable = NULL;

        // Generate new tables
        currentTcp4Table = NetLib_getTcp4Table();
        currentUdpTable = NetLib_getUdpTable();

        // Diff TCP snapshots for new entries
        if( rpal_memory_isValid( currentTcp4Table ) &&
            rpal_memory_isValid( oldTcp4Table ) )
        {
            for( i = 0; i < currentTcp4Table->nRows; i++ )
            {
                isFound = FALSE;

                if( rEvent_wait( isTimeToStop, 0 ) )
                {
                    break;
                }

                for( j = 0; j < oldTcp4Table->nRows; j++ )
                {
                    if( isTcpEqual( &currentTcp4Table->rows[ i ], &oldTcp4Table->rows[ j ] ) )
                    {
                        isFound = TRUE;
                        break;
                    }
                }

                if( !isFound )
                {
                    if( NULL != ( notif = rSequence_new() ) )
                    {
                        if( rSequence_addRU32( notif, 
                                               RP_TAGS_STATE, 
                                               currentTcp4Table->rows[ i ].state ) &&
                            rSequence_addRU32( notif, 
                                               RP_TAGS_PROCESS_ID, 
                                               currentTcp4Table->rows[ i ].pid ) &&
                            rSequence_addTIMESTAMP( notif, 
                                                    RP_TAGS_TIMESTAMP, 
                                                    rpal_time_getGlobal() ) )
                        {
                            if( NULL != ( comp = rSequence_new() ) )
                            {
                                // Add the destination components
                                if( !rSequence_addIPV4( comp, 
                                                        RP_TAGS_IP_ADDRESS, 
                                                        currentTcp4Table->rows[ i ].destIp ) ||
                                    !rSequence_addRU16( comp, 
                                                        RP_TAGS_PORT, 
                                                        rpal_ntoh16( (RU16)currentTcp4Table->rows[ i ].destPort ) ) ||
                                    !rSequence_addSEQUENCE( notif, 
                                                            RP_TAGS_DESTINATION, 
                                                            comp ) )
                                {
                                    rSequence_free( comp );
                                    comp = NULL;
                                }
                            }

                            if( NULL != ( comp = rSequence_new() ) )
                            {
                                // Add the source components
                                if( !rSequence_addIPV4( comp, 
                                                        RP_TAGS_IP_ADDRESS, 
                                                        currentTcp4Table->rows[ i ].sourceIp ) ||
                                    !rSequence_addRU16( comp, 
                                                        RP_TAGS_PORT, 
                                                        rpal_ntoh16( (RU16)currentTcp4Table->rows[ i ].sourcePort ) ) ||
                                    !rSequence_addSEQUENCE( notif, 
                                                            RP_TAGS_SOURCE, 
                                                            comp ) )
                                {
                                    rSequence_free( comp );
                                    comp = NULL;
                                }
                            }

                            rpal_debug_info( "new tcp connection: 0x%08X = 0x%08X:0x%04X ---> 0x%08X:0x%04X -- 0x%08X.",
                                currentTcp4Table->rows[ i ].state,
                                currentTcp4Table->rows[ i ].sourceIp,
                                currentTcp4Table->rows[ i ].sourcePort,
                                currentTcp4Table->rows[ i ].destIp,
                                currentTcp4Table->rows[ i ].destPort,
                                currentTcp4Table->rows[ i ].pid );
                            notifications_publish( RP_TAGS_NOTIFICATION_NEW_TCP4_CONNECTION, notif );
                        }

                        rSequence_free( notif );
                    }
                }
            }
        }
        else if( 0 != nThLoop )
        {
            rpal_debug_warning( "could not get tcp connections table." );
        }


        // Diff TCP snapshots for new entries
        if( NULL != currentUdpTable &&
            NULL != oldUdpTable )
        {
            for( i = 0; i < currentUdpTable->nRows; i++ )
            {
                isFound = FALSE;

                if( rEvent_wait( isTimeToStop, 0 ) )
                {
                    break;
                }

                for( j = 0; j < oldUdpTable->nRows; j++ )
                {
                    if( isUdpEqual( &currentUdpTable->rows[ i ], &oldUdpTable->rows[ j ] ) )
                    {
                        isFound = TRUE;
                        break;
                    }
                }

                if( !isFound )
                {
                    if( NULL != ( notif = rSequence_new() ) )
                    {
                        if( !rSequence_addIPV4( notif, 
                                                RP_TAGS_IP_ADDRESS, 
                                                currentUdpTable->rows[ i ].localIp ) ||
                            !rSequence_addRU16( notif, 
                                                RP_TAGS_PORT, 
                                                rpal_ntoh16( (RU16)currentUdpTable->rows[ i ].localPort ) ) ||
                            !rSequence_addRU32( notif, 
                                                RP_TAGS_PROCESS_ID, 
                                                currentUdpTable->rows[ i ].pid ) ||
                            !rSequence_addTIMESTAMP( notif, 
                                                     RP_TAGS_TIMESTAMP, 
                                                     rpal_time_getGlobal() ) )
                        {
                            notif = NULL;
                        }
                        else
                        {
                            rpal_debug_info( "new udp connection: 0x%08X:0x%04X -- 0x%08X.",
                                currentUdpTable->rows[ i ].localIp,
                                currentUdpTable->rows[ i ].localPort,
                                currentUdpTable->rows[ i ].pid );
                            notifications_publish( RP_TAGS_NOTIFICATION_NEW_UDP4_CONNECTION, notif );
                        }

                        rSequence_free( notif );
                    }
                }
            }
        }
        else if( 0 != nThLoop )
        {
            rpal_debug_warning( "could not get udp connections table." );
        }

        nThLoop++;
        if( 0 == nThLoop % 10 )
        {
            timeout = libOs_getUsageProportionalTimeout( 1000 ) + 500;
        }

        rpal_thread_sleep( timeout );
    }

    rpal_memory_free( currentTcp4Table );
    rpal_memory_free( currentUdpTable );
    rpal_memory_free( oldTcp4Table );
    rpal_memory_free( oldUdpTable );

    return NULL;
}

RBOOL
    collector_4_init
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;
    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        if( rThreadPool_task( hbsState->hThreadPool, networkDiffThread, NULL ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    collector_4_cleanup
    (
        HbsState* hbsState,
        rSequence config
    )
{
    RBOOL isSuccess = FALSE;

    UNREFERENCED_PARAMETER( config );

    if( NULL != hbsState )
    {
        isSuccess = TRUE;
    }

    return isSuccess;
}
