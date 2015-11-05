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

#include <rpal_blob.h>

#define RPAL_FILE_ID    2


typedef struct
{
	RPU8 pData;
	RU32 currentSize;
	RU32 sizeUsed;
	RU32 growBy;
	RU32 from;
} _rBlob, *_prBlob;

rBlob
	rpal_blob_create_from
	(
		RU32 initialSize,
		RU32 growBy,
		RU32 from
	)
{
	rBlob blob = NULL;

	blob = rpal_memory_alloc_from( sizeof( _rBlob ), from );

	if( rpal_memory_isValid( blob ) )
	{
		if( 0 != initialSize )
		{
			((_prBlob)blob)->pData = rpal_memory_alloc_from( initialSize + sizeof( RWCHAR ), from );

			if( !rpal_memory_isValid( ((_prBlob)blob)->pData ) )
			{
                rpal_memory_free( blob );
				blob = NULL;
			}
			else
			{
				((_prBlob)blob)->currentSize = initialSize;
                (((_prBlob)blob)->pData)[ initialSize ] = 0;
                (((_prBlob)blob)->pData)[ initialSize + 1 ] = 0;
			}
		}
        else
        {
            ((_prBlob)blob)->currentSize = 0;
            ((_prBlob)blob)->pData = NULL;
        }

		if( NULL != blob )
		{
			((_prBlob)blob)->sizeUsed = 0;
			((_prBlob)blob)->growBy = growBy;
			((_prBlob)blob)->from = from;
		}
	}
	else
	{
		blob = NULL;
	}

	return blob;
}

RVOID
	rpal_blob_free
	(
		rBlob blob
	)
{
	if( rpal_memory_isValid( blob ) )
	{
		if( rpal_memory_isValid( ((_prBlob)blob)->pData ) )
		{
			rpal_memory_free( ((_prBlob)blob)->pData );
		}

		rpal_memory_free( blob );
	}
}

RBOOL
	rpal_blob_add
	(
		rBlob blob,
		RPVOID pData,
		RU32 size
	)
{
	RBOOL isAdded = FALSE;

	if( rpal_memory_isValid( blob ) &&
		NULL != pData &&
		0 != size )
	{
		if( ((_prBlob)blob)->currentSize - ((_prBlob)blob)->sizeUsed < size )
		{
			((_prBlob)blob)->currentSize += ( ( 0 != ((_prBlob)blob)->growBy && size < ((_prBlob)blob)->growBy ) ? ((_prBlob)blob)->growBy : size );
			((_prBlob)blob)->pData = rpal_memory_realloc_from( ((_prBlob)blob)->pData,
                                                               ((_prBlob)blob)->currentSize + sizeof( RWCHAR ),
															   ((_prBlob)blob)->from );
		}

		if( rpal_memory_isValid( ((_prBlob)blob)->pData ) )
		{
			rpal_memory_memcpy( ((_prBlob)blob)->pData + ((_prBlob)blob)->sizeUsed, pData, size );

			((_prBlob)blob)->sizeUsed += size;
            
            // We allocate WCHAR more than we need always to ensure that if the buffer contains a wide string it will be terminated
            rpal_memory_zero( ((_prBlob)blob)->pData + ((_prBlob)blob)->sizeUsed, sizeof( RWCHAR ) );

			isAdded = TRUE;
		}
	}
    else if( NULL == pData ||
		     0 == size )
    {
        isAdded = TRUE;
    }

    if( isAdded )
    {
        if( !rpal_memory_isValid( ((_prBlob)blob)->pData ) )
        {
            isAdded = FALSE;
            rpal_debug_break();
        }
    }

	return isAdded;
}

RBOOL
	rpal_blob_insert
	(
		rBlob blob,
		RPVOID pData,
		RU32 size,
        RU32 offset
	)
{
	RBOOL isAdded = FALSE;

    _prBlob pBlob = (_prBlob)blob;

	if( rpal_memory_isValid( blob ) &&
		NULL != pData &&
		0 != size )
	{
		if( pBlob->currentSize - pBlob->sizeUsed < size )
		{
			pBlob->currentSize += ( 0 != pBlob->growBy ? pBlob->growBy : size );
			pBlob->pData = rpal_memory_realloc_from( pBlob->pData,
													 pBlob->currentSize + sizeof( RWCHAR ),
													 pBlob->from );
		}

		if( rpal_memory_isValid( pBlob->pData ) )
		{
            // Relocate existing data
            rpal_memory_memmove( pBlob->pData + offset + size, 
                                 pBlob->pData + offset, 
                                 pBlob->sizeUsed - offset );

			rpal_memory_memcpy( pBlob->pData + offset, pData, size );

			pBlob->sizeUsed += size;
            
            // We allocate WCHAR more than we need always to ensure that if the buffer contains a wide string it will be terminated
            rpal_memory_zero( pBlob->pData + pBlob->sizeUsed, sizeof( RWCHAR ) );

			isAdded = TRUE;
		}
	}
    else if( NULL == pData ||
		     0 == size )
    {
        isAdded = TRUE;
    }

    if( isAdded )
    {
        if( !rpal_memory_isValid( pBlob->pData ) )
        {
            isAdded = FALSE;
            rpal_debug_break();
        }
    }

	return isAdded;
}

RPVOID
	rpal_blob_getBuffer
	(
		rBlob blob
	)
{
	RPVOID pData = NULL;

	if( rpal_memory_isValid( blob ) &&
        rpal_memory_isValid( ((_prBlob)blob)->pData ) )
	{
		pData = ((_prBlob)blob)->pData;
	}

	return pData;
}

RU32
	rpal_blob_getSize
	(
		rBlob blob
	)
{
	RU32 size = 0;

	if( rpal_memory_isValid( blob ) )
	{
		size = ((_prBlob)blob)->sizeUsed;
	}

	return size;
}

RPVOID
    rpal_blob_arrElem
    (
        rBlob blob,
        RU32 elemSize,
        RU32 elemIndex
    )
{
    RPVOID pElem = NULL;
    _prBlob b = (_prBlob)blob;
    RU32 offset = 0;

    if( rpal_memory_isValid( b ) )
    {
        offset = elemSize * elemIndex;

        if( rpal_memory_isValid( b->pData ) &&
            IS_WITHIN_BOUNDS( b->pData + ( offset ), elemSize, b->pData, b->sizeUsed ) )
        {
            pElem = b->pData + offset;
        }
    }

    return pElem;
}

RBOOL
    rpal_blob_remove
    (
        rBlob blob,
        RU32 startOffset,
        RU32 size
    )
{
    RBOOL isSuccess = FALSE;
    _prBlob pBlob = (_prBlob)blob;
    
    if( NULL != pBlob &&
        IS_WITHIN_BOUNDS( pBlob->pData + startOffset, size, pBlob->pData, pBlob->sizeUsed ) &&
        rpal_memory_isValid( pBlob->pData ) )
    {
        rpal_memory_memcpy( pBlob->pData + startOffset, pBlob->pData + startOffset + size, pBlob->sizeUsed - size - startOffset );
        pBlob->sizeUsed -= size;

        if( rpal_memory_isValid( pBlob->pData ) )
        {
            pBlob->pData = rpal_memory_realloc_from( pBlob->pData, pBlob->sizeUsed + sizeof( RWCHAR ), pBlob->from );
            pBlob->currentSize = pBlob->sizeUsed;

            if( NULL != pBlob->pData )
            {
                // We allocate WCHAR more than we need always to ensure that if the buffer contains a wide string it will be terminated
                rpal_memory_zero( pBlob->pData + pBlob->sizeUsed, sizeof( RWCHAR ) );
                isSuccess = TRUE;
            }
        }
    }

    return isSuccess;
}

rBlob
    rpal_blob_duplicate_from
    (
        rBlob original,
		RU32 from
    )
{
    _prBlob newBlob = NULL;
    _prBlob originalBlob = (_prBlob)original;

    if( NULL != original )
    {
        if( NULL != ( newBlob = rpal_memory_alloc_from( sizeof( _rBlob ), from ) ) )
        {
            newBlob->currentSize = originalBlob->currentSize;
            newBlob->growBy = originalBlob->growBy;
            newBlob->sizeUsed = originalBlob->sizeUsed;
			newBlob->from = from;

            if( 0 != originalBlob->currentSize &&
                NULL != originalBlob->pData )
            {
                newBlob->pData = rpal_memory_alloc_from( newBlob->currentSize + sizeof( RWCHAR ), from );
            
                if( NULL != newBlob->pData )
                {
                    rpal_memory_memcpy( newBlob->pData, originalBlob->pData, newBlob->currentSize + sizeof( RWCHAR ) );

                    if( !rpal_memory_isValid( newBlob->pData ) )
                    {
                        rpal_memory_free( newBlob );
                        newBlob = NULL;
                        rpal_debug_break();
                    }
                }
                else
                {
                    rpal_memory_free( newBlob );
                    newBlob = NULL;
                    rpal_debug_break();
                }
            }
            else
            {
                newBlob->pData = NULL;
            }
        }
    }

    return newBlob;
}

RPVOID
    rpal_blob_freeWrapperOnly
    (
        rBlob blob
    )
{
    RPVOID buffer = NULL;
    
    _prBlob pBlob = (_prBlob)blob;
    
    if( rpal_memory_isValid( pBlob ) )
    {
	buffer = pBlob->pData;
	rpal_memory_free( pBlob );
    }
    
    return buffer;
}

