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

#include <rpal/rpal_endianness.h>


RU64
    rpal_ntoh64
    (
        RU64 n
    )
{
#ifndef RPAL_PLATFORM_BIGENDIAN
    RU32 n1 = (RU32)( ( n & LITERAL_64_BIT(0xFFFFFFFF00000000) ) >> 32 );
    RU32 n2 = (RU32)( n & LITERAL_64_BIT(0x00000000FFFFFFFF) );
    n = ( (RU64)rpal_ntoh32( n2 ) << 32 ) | ( (RU64)rpal_ntoh32( n1 ) );
#endif

    return n;
}

RU32
	rpal_ntoh32
	(
		RU32 n
	)
{
	return ntohl( n );
}

RU16
	rpal_ntoh16
	(
		RU16 n
	)
{
	return ntohs( n );
}

RU64
    rpal_hton64
    (
        RU64 n
    )
{
#ifndef RPAL_PLATFORM_BIGENDIAN
    RU32 n1 = (RU32)( ( n & LITERAL_64_BIT(0xFFFFFFFF00000000) ) >> 32 );
    RU32 n2 = (RU32)( n & LITERAL_64_BIT(0x00000000FFFFFFFF) );
    n = ( (RU64)rpal_hton32( n2 ) << 32 ) | ( (RU64)rpal_hton32( n1 ) );
#endif

    return n;
}

RU32
	rpal_hton32
	(
		RU32 n
	)
{
	return htonl( n );
}

RU16
	rpal_hton16
	(
		RU16 n
	)
{
	return htons( n );
}

