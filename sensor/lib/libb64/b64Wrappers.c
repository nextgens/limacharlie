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

#include <libb64/libb64.h>
#include "cencode.h"
#include "cdecode.h"

#define RPAL_FILE_ID   34

#define IS_VALID( val ) (NULL != (val))
#define IS_NOT_ZERO( val ) (0 != (val))
#define MAKE_ZERO( val ) ((val) = 0)
#define MAKE_NULL( val ) ((val) = NULL)
#pragma warning( disable: 4305 ) // Disabling error on truncate cast, it all works

RBOOL
    base64_encode_from
    (
        RPVOID inputBuffer,
        RU32 inputBufferSize,
        RPCHAR* pOutputBuffer,
        RBOOL isUrlEncode,
        RU32 from
    )
{
    RBOOL isSuccess = FALSE;

    base64_encodestate state = {0};
    RU32 encodedSize = 0;
    RU32 requiredBufferSize = 0;
    RU32 i = 0;
    RCHAR tmpEsc[ 4 ] = { '%', 0, 0, 0 };

    if( IS_VALID( inputBuffer ) &&
        IS_VALID( pOutputBuffer ) &&
        IS_NOT_ZERO( inputBufferSize ) )
    {
        MAKE_NULL( *pOutputBuffer );
        base64_init_encodestate(&state);

        requiredBufferSize = ((inputBufferSize * 4) / 3) + ( inputBufferSize % 3 ? ( 3 - ( inputBufferSize % 3 ) ) : 0 ) + ( 2 * sizeof( RCHAR ) );

        *pOutputBuffer = (RPCHAR)rpal_memory_alloc_from( requiredBufferSize, from );

        if( IS_VALID( *pOutputBuffer ) )
        {
            memset( *pOutputBuffer, 0, requiredBufferSize );

            encodedSize = (RU32)(RSIZET)base64_encode_block( (const char*)inputBuffer, 
                                                      inputBufferSize, 
                                                      *pOutputBuffer, 
                                                      &state );

            if( IS_NOT_ZERO( encodedSize ) )
            {
                encodedSize += (RU32)(RSIZET)base64_encode_blockend( *pOutputBuffer + encodedSize, &state );

                (*pOutputBuffer)[ encodedSize ] = 0;

                if( isUrlEncode )
                {
                    for( i = encodedSize - 1; -1 != i && NULL != *pOutputBuffer; i-- )
                    {
                        switch( (*pOutputBuffer)[ i ] )
                        {
                            case '=':
                            case '+':
                            case '/':
                                encodedSize += 2;
                                if( NULL != ( *pOutputBuffer = rpal_memory_realloc( *pOutputBuffer, encodedSize + ( 2 * sizeof( RCHAR ) ) ) ) )
				{
				    rpal_memory_memmove( *pOutputBuffer + i + ( 3 * sizeof( RCHAR ) ), *pOutputBuffer + i + sizeof( RCHAR ), encodedSize - i - sizeof( RCHAR ) );
				    rpal_string_byte_to_str( (*pOutputBuffer)[ i ], (RPCHAR)&(tmpEsc[1]) );
				    memcpy( *pOutputBuffer + i, &tmpEsc, 3 );
				}
                                break;
                        }
                    }
                }

                isSuccess = TRUE;
            }
            else
            {
                rpal_memory_free( *pOutputBuffer );
                MAKE_NULL( *pOutputBuffer );
            }

        }
    }

    if( isSuccess )
    {
        if( !rpal_memory_isValid( *pOutputBuffer ) )
        {
            isSuccess = FALSE;
        }
    }

    return isSuccess;
}

RBOOL
    base64_decode_from
    (
        RPCHAR inputBuffer,
        RPVOID* pOutputBuffer,
        RPU32 pOutputBufferSize,
        RBOOL isUrlEncoded,
        RU32 from
    )
{
    RBOOL isSuccess = FALSE;

    base64_decodestate state;
    RU32 inputLength = 0;
    RU32 requiredSize = 0;
    RPCHAR correctedInput = NULL;
    RU32 correctedSize = 0;
    RU32 i = 0;

    if( IS_VALID( inputBuffer ) &&
        IS_VALID( pOutputBuffer ) &&
        IS_VALID( pOutputBufferSize ) )
    {
        base64_init_decodestate(&state);

        MAKE_NULL( *pOutputBuffer );
        MAKE_ZERO( *pOutputBufferSize );

        inputLength = (RU32)rpal_string_strlen( inputBuffer );

        if( IS_NOT_ZERO( inputLength ) )
        {
            requiredSize = inputLength;

            *pOutputBuffer = (RU8*)rpal_memory_alloc_from( requiredSize, from );

            if( IS_VALID( *pOutputBuffer ) )
            {
                if( isUrlEncoded )
                {
                    correctedInput = rpal_memory_alloc( inputLength );

                    if( rpal_memory_isValid( correctedInput ) )
                    {
                        rpal_memory_memcpy( correctedInput, inputBuffer, inputLength );
                        correctedSize = inputLength;

                        for( i = 0; i < (correctedSize - 2); i++ )
                        {
                            if( '%' == correctedInput[ i ] )
                            {
                                correctedInput[ i ] = rpal_string_str_to_byte( correctedInput + i + 1 );
                                rpal_memory_memcpy( correctedInput + i + 1, correctedInput + i + 3, inputLength - i - 2 );
                                correctedSize -= 2;
                            }
                        }
                    }
                }
                else
                {
                    correctedInput = inputBuffer;
                    correctedSize = inputLength;
                }

                if( NULL != correctedInput &&
                    0 != correctedSize )
                {
                    *pOutputBufferSize = base64_decode_block( correctedInput, 
                                                              correctedSize, 
                                                              (char*)*pOutputBuffer, 
                                                              &state );
                }

                if( IS_NOT_ZERO( *pOutputBufferSize ) )
                {
                    isSuccess = TRUE;
                }
                else
                {
                    rpal_memory_free( *pOutputBuffer );
                    MAKE_NULL( *pOutputBuffer );
                    MAKE_ZERO( *pOutputBufferSize );
                }
            }
            else
            {
                MAKE_NULL( *pOutputBuffer );
                MAKE_ZERO( *pOutputBufferSize );
            }
        }
    }

    if( isSuccess )
    {
        if( !rpal_memory_isValid( *pOutputBuffer ) )
        {
            isSuccess = FALSE;
        }
    }

    return isSuccess;
}

