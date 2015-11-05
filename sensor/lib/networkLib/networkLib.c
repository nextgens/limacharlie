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

#include <networkLib/networkLib.h>

#define RPAL_FILE_ID    41


#ifdef RPAL_PLATFORM_WINDOWS
#include <windows_undocumented.h>

static HMODULE hIpHlpApi = NULL;
static GetTcpTable_f GetTcpTable = NULL;
static GetUdpTable_f GetUdpTable = NULL;
static GetExtendedTcpTable_f GetExtendedTcpTable = NULL;
static GetExtendedUdpTable_f GetExtendedUdpTable = NULL;

static
RBOOL
    loadNetApi
    (

    )
{
    RBOOL isLoaded = FALSE;

    RCHAR importDll[] = "iphlpapi.dll";
    RCHAR import1[] = "GetTcpTable";
    RCHAR import2[] = "GetUdpTable";
    RCHAR import3[] = "GetExtendedTcpTable";
    RCHAR import4[] = "GetExtendedUdpTable";

    if( ( NULL == GetTcpTable ||
          NULL == GetUdpTable ) &&
          NULL == hIpHlpApi )
    {
        // Some potential weird race condition, but extremelyt unlikely
        if( NULL == ( hIpHlpApi = LoadLibraryA( (RPCHAR)&importDll ) ) )
        {
            return FALSE;
        }
    }

    if( NULL == GetTcpTable )
    {
        GetTcpTable = (GetTcpTable_f)GetProcAddress( hIpHlpApi, (RPCHAR)&import1 );
    }

    if( NULL == GetUdpTable )
    {
        GetUdpTable = (GetUdpTable_f)GetProcAddress( hIpHlpApi, (RPCHAR)&import2 );
    }

    if( NULL == GetExtendedTcpTable )
    {
        GetExtendedTcpTable = (GetExtendedTcpTable_f)GetProcAddress( hIpHlpApi, (RPCHAR)&import3 );
    }

    if( NULL == GetExtendedUdpTable )
    {
        GetExtendedUdpTable = (GetExtendedUdpTable_f)GetProcAddress( hIpHlpApi, (RPCHAR)&import4 );
    }

    if( NULL != GetTcpTable &&
        NULL != GetUdpTable )
    {
        isLoaded = TRUE;
    }

    return isLoaded;
}

#endif

NetLib_Tcp4Table*
    NetLib_getTcp4Table
    (

    )
{
    NetLib_Tcp4Table* table = NULL;
#ifdef RPAL_PLATFORM_WINDOWS
    PMIB_TCPTABLE winTable = NULL;
    RU32 size = 0;
    RU32 error = 0;
    RBOOL isFinished = FALSE;
    RU32 i = 0;

    if( loadNetApi() )
    {
        while( !isFinished )
        {
            if( NULL != GetExtendedTcpTable )
            {
                error = GetExtendedTcpTable( winTable, (DWORD*)&size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0 );
            }
            else
            {
                error = GetTcpTable( winTable, (PDWORD)&size, FALSE );
            }

            if( ERROR_INSUFFICIENT_BUFFER == error &&
                0 != size )
            {
                if( NULL == ( winTable = rpal_memory_realloc( winTable, size ) ) )
                {
                    isFinished = TRUE;
                }
            }
            else if( ERROR_SUCCESS != error )
            {
                rpal_memory_free( winTable );
                winTable = NULL;
                isFinished = TRUE;
            }
            else
            {
                isFinished = TRUE;
            }
        }

        if( NULL != winTable )
        {
            if( NULL != ( table = rpal_memory_alloc( sizeof( NetLib_Tcp4Table ) + 
                                                     ( winTable->dwNumEntries * sizeof( NetLib_Tcp4TableRow ) ) ) ) )
            {
                table->nRows = winTable->dwNumEntries;

                for( i = 0; i < winTable->dwNumEntries; i++ )
                {
                    if( NULL == GetExtendedTcpTable )
                    {
                        table->rows[ i ].destIp = winTable->table[ i ].dwRemoteAddr;
                        table->rows[ i ].destPort = (RU16)winTable->table[ i ].dwRemotePort;
                        table->rows[ i ].sourceIp = winTable->table[ i ].dwLocalAddr;
                        table->rows[ i ].sourcePort = (RU16)winTable->table[ i ].dwLocalPort;
                        table->rows[ i ].state = winTable->table[ i ].dwState;
                        table->rows[ i ].pid = 0;
                    }
                    else
                    {
                        table->rows[ i ].destIp = ((PMIB_TCPROW_OWNER_PID)winTable->table)[ i ].dwRemoteAddr;
                        table->rows[ i ].destPort = (RU16)((PMIB_TCPROW_OWNER_PID)winTable->table)[ i ].dwRemotePort;
                        table->rows[ i ].sourceIp = ((PMIB_TCPROW_OWNER_PID)winTable->table)[ i ].dwLocalAddr;
                        table->rows[ i ].sourcePort = (RU16)((PMIB_TCPROW_OWNER_PID)winTable->table)[ i ].dwLocalPort;
                        table->rows[ i ].state = ((PMIB_TCPROW_OWNER_PID)winTable->table)[ i ].dwState;
                        table->rows[ i ].pid = ((PMIB_TCPROW_OWNER_PID)winTable->table)[ i ].dwOwningPid;
                    }
                }
            }

            rpal_memory_free( winTable );
        }
    }
#else
    rpal_debug_not_implemented();
#endif
    return table;
}

NetLib_UdpTable*
    NetLib_getUdpTable
    (

    )
{
    NetLib_UdpTable* table = NULL;
#ifdef RPAL_PLATFORM_WINDOWS
    PMIB_UDPTABLE winTable = NULL;
    RU32 size = 0;
    RU32 error = 0;
    RBOOL isFinished = FALSE;
    RU32 i = 0;

    if( loadNetApi() )
    {
        while( !isFinished )
        {
            if( NULL != GetExtendedUdpTable )
            {
                error = GetExtendedUdpTable( winTable, (DWORD*)&size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0 );
            }
            else
            {
                error = GetUdpTable( winTable, (PDWORD)&size, FALSE );
            }

            if( ERROR_INSUFFICIENT_BUFFER == error &&
                0 != size )
            {
                if( NULL == ( winTable = rpal_memory_realloc( winTable, size ) ) )
                {
                    isFinished = TRUE;
                }
            }
            else if( ERROR_SUCCESS != error )
            {
                rpal_memory_free( winTable );
                winTable = NULL;
                isFinished = TRUE;
            }
            else
            {
                isFinished = TRUE;
            }
        }

        if( NULL != winTable )
        {
            if( NULL != ( table = rpal_memory_alloc( sizeof( NetLib_UdpTable ) + 
                                                     ( winTable->dwNumEntries * sizeof( NetLib_UdpTableRow ) ) ) ) )
            {
                table->nRows = winTable->dwNumEntries;

                for( i = 0; i < winTable->dwNumEntries; i++ )
                {
                    if( NULL == GetExtendedUdpTable )
                    {
                        table->rows[ i ].localIp = winTable->table[ i ].dwLocalAddr;
                        table->rows[ i ].localPort = (RU16)winTable->table[ i ].dwLocalPort;
                        table->rows[ i ].pid = 0;
                    }
                    else
                    {
                        table->rows[ i ].localIp = ((PMIB_UDPROW_OWNER_PID)winTable->table)[ i ].dwLocalAddr;
                        table->rows[ i ].localPort = (RU16)((PMIB_UDPROW_OWNER_PID)winTable->table)[ i ].dwLocalPort;
                        table->rows[ i ].pid = ((PMIB_UDPROW_OWNER_PID)winTable->table)[ i ].dwOwningPid;
                    }
                }
            }

            rpal_memory_free( winTable );
        }
    }
#else
    rpal_debug_not_implemented();
#endif
    return table;
}

