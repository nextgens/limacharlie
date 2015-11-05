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

#ifndef _NETWORK_LIB_H
#define _NETWORK_LIB_H

#include <rpal/rpal.h>

typedef struct
{
    RU32 state;

    RU32 sourceIp;
    RU16 sourcePort;

    RU32 destIp;
    RU16 destPort;

    RU32 pid;

} NetLib_Tcp4TableRow;

typedef struct
{
    RU32 nRows;
    NetLib_Tcp4TableRow rows[];

} NetLib_Tcp4Table;



typedef struct
{
    RU32 localIp;
    RU16 localPort;

    RU32 pid;

} NetLib_UdpTableRow;

typedef struct
{
    RU32 nRows;
    NetLib_UdpTableRow rows[];

} NetLib_UdpTable;


#define NETWORKLIB_TCP_STATE_CLOSED                 1
#define NETWORKLIB_TCP_STATE_LISTEN                 2
#define NETWORKLIB_TCP_STATE_SYN_SENT               3
#define NETWORKLIB_TCP_STATE_SYM_RECEIVED           4
#define NETWORKLIB_TCP_STATE_ESTABLISHED            5
#define NETWORKLIB_TCP_STATE_FIN_WAIT_1             6
#define NETWORKLIB_TCP_STATE_FIN_WAIT_2             7
#define NETWORKLIB_TCP_STATE_CLOSE_WAIT             8
#define NETWORKLIB_TCP_STATE_CLOSING                9
#define NETWORKLIB_TCP_STATE_LAST_ACK               10
#define NETWORKLIB_TCP_STATE_TIME_WAIT              11
#define NETWORKLIB_TCP_STATE_DELETE_STATE           12



NetLib_Tcp4Table*
    NetLib_getTcp4Table
    (

    );

NetLib_UdpTable*
    NetLib_getUdpTable
    (

    );

#endif
