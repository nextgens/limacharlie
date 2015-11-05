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

#include <liburl/liburl.h>
#include <libb64/libb64.h>

#define RPAL_FILE_ID  39

#pragma warning( disable: 4996 ) // Disabling error on deprecated/unsafe

RBOOL
    isValidUrlChar
    (
        CHAR ch
    )
{
    RBOOL isValid = FALSE;

    if( ( '0' <= ch && '9' >= ch ) ||
        ( 'a' <= ch && 'z' >= ch ) ||
        ( 'A' <= ch && 'Z' >= ch ) )
    {
        isValid = TRUE;
    }

    return isValid;
}

RPCHAR
    liburl_encode
    (
        RPCHAR str
    )
{
    RPCHAR encoded = NULL;

    RU32 index = 0;
    RU32 numToEncode = 0;
    RU32 len = 0;
    RCHAR escaped[ 3 ] = {0};

    if( NULL != str )
    {
        len = rpal_string_strlen( str );

        for( index = 0; index < len; index++ )
        {
            if( !isValidUrlChar( *(str + index) ) )
            {
                numToEncode++;
            }
        }

        encoded = rpal_memory_alloc( len + sizeof(RCHAR) + numToEncode );

        numToEncode = 0;

        if( rpal_memory_isValid( encoded ) )
        {
            for( index = 0; index < len; index++ )
            {
                if( !isValidUrlChar( *(str + index) ) )
                {
                    sprintf( (RPCHAR)&escaped, "%02X", *(str + index) );

                    *(encoded + index + numToEncode) = escaped[ 0 ];
                    numToEncode++;
                    *(encoded + index + numToEncode) = escaped[ 1 ];
                }
                else
                {
                    *(encoded + index + numToEncode) = *(str + index);
                }
            }
        }
    }

    return encoded;
}

rUrl
    liburl_create
    (

    )
{
    return (rUrl)rpal_stringbuffer_new( 0, 0, FALSE );
}

RVOID
    liburl_free
    (
        rUrl url
    )
{
    rpal_stringbuffer_free( (rString)url );
}

RBOOL
    liburl_add_param_str
    (
        rUrl url,
        RPCHAR paramName,
        RPCHAR paramVal
    )
{
    RBOOL isSuccess = FALSE;

    RPCHAR goodPName = NULL;
    RPCHAR goodPVal = NULL;

    if( rpal_memory_isValid( url ) &&
        NULL != paramName &&
        NULL != paramVal )
    {
        isSuccess = TRUE;

        if( 0 != rpal_string_strlen( rpal_stringbuffer_getString( (rString)url ) ) )
        {
            if( !rpal_stringbuffer_add( (rString)url, "&" ) )
            {
                isSuccess = FALSE;
            }
        }

        if( isSuccess )
        {
            isSuccess = FALSE;

            if( NULL != ( goodPName = liburl_encode( paramName ) ) &&
                NULL != ( goodPVal = liburl_encode( paramVal ) ) )
            {
                if( rpal_stringbuffer_add( (rString)url, goodPName ) &&
                    rpal_stringbuffer_add( (rString)url, "=" ) &&
                    rpal_stringbuffer_add( (rString)url, goodPVal ) )
                {
                    isSuccess = TRUE;
                }
            }

            rpal_memory_free( goodPName );
            rpal_memory_free( goodPVal );
        }
    }

    return isSuccess;
}

RBOOL
    liburl_add_param_buff
    (
        rUrl url,
        RPCHAR paramName,
        RPU8 paramVal,
        RU32 paramValSize
    )
{
    RBOOL isSuccess = FALSE;

    RPCHAR encBuff = NULL;

    if( NULL != url &&
        NULL != paramName &&
        NULL != paramVal &&
        0 != paramValSize )
    {
        if( base64_encode( paramVal, paramValSize, &encBuff, FALSE ) )
        {
            isSuccess = liburl_add_param_str( url, paramName, encBuff );

            rpal_memory_free( encBuff );
        }
    }

    return isSuccess;
}
