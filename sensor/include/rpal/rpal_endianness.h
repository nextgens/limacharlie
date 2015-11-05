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

#ifndef _RPAL_ENDIANNESS_H
#define _RPAL_ENDIANNESS_H

#include <rpal.h>

RU64
    rpal_ntoh64
    (
        RU64 n
    );

RU32
	rpal_ntoh32
	(
		RU32 n
	);

RU16
	rpal_ntoh16
	(
		RU16 n
	);

RU64
    rpal_hton64
    (
        RU64 n
    );

RU32
	rpal_hton32
	(
		RU32 n
	);

RU16
	rpal_hton16
	(
		RU16 n
	);


#endif
