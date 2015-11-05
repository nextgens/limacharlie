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

#ifndef _RPAL_BLOB
#define _RPAL_BLOB

#include <rpal/rpal.h>

typedef RPVOID	rBlob;

#define rpal_blob_create( initialSize, growBy )    rpal_blob_create_from( initialSize, growBy, RPAL_LINE_SUBTAG )
rBlob
	rpal_blob_create_from
	(
		RU32 initialSize,
		RU32 growBy,
    RU32 from
	);

RVOID
	rpal_blob_free
	(
		rBlob blob
	);

RBOOL
	rpal_blob_add
	(
		rBlob blob,
		RPVOID pData,
		RU32 size
	);

RBOOL
    rpal_blob_insert
    (
        rBlob blob,
        RPVOID pData,
        RU32 size,
        RU32 offset
    );

RPVOID
	rpal_blob_getBuffer
	(
		rBlob blob
	);

RU32
	rpal_blob_getSize
	(
		rBlob blob
	);

RPVOID
    rpal_blob_arrElem
    (
        rBlob blob,
        RU32 elemSize,
        RU32 elemIndex
    );

RBOOL
    rpal_blob_remove
    (
        rBlob blob,
        RU32 startOffset,
        RU32 size
    );

#define rpal_blob_duplicate( original )     rpal_blob_duplicate_from( original, RPAL_LINE_SUBTAG )
rBlob
    rpal_blob_duplicate_from
    (
        rBlob original,
		RU32 from
    );

RPVOID
    rpal_blob_freeWrapperOnly
    (
        rBlob blob
    );

#endif
