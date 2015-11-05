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

#include <rpal/rpal_string.h>

#define RPAL_FILE_ID    9

#ifdef RPAL_PLATFORM_WINDOWS
#pragma warning( disable: 4996 ) // Disabling error on deprecated/unsafe
#endif

#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )

#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>

static char*
    strupr
    (
        char* s
    )
{
    char* p = s;
    if( NULL != s )
    {
        while ( 0 != ( *p = toupper( *p ) ) )
        {
            p++;
        }
    }
    return s;
}

static char*
    strlwr
    (
        char* s
    )
{
    char* p = s;
    if( NULL != s )
    {
        while( 0 != ( *p = tolower( *p ) ) )
        {
            p++;
        }
    }
    return s;
}

static RWCHAR*
    wcsupr
    (
        RWCHAR* s
    )
{
    RWCHAR* p = s;
    if( NULL != s )
    {
        while ( 0 != ( *p = towupper( *p ) ) )
        {
            p++;
        }
    }
    return s;
}

static RWCHAR*
    wcslwr
    (
        RWCHAR* s
    )
{
    RWCHAR* p = s;
    if( NULL != s )
    {
        while( 0 != ( *p = towlower( *p ) ) )
        {
            p++;
        }
    }
    return s;
}
#endif

static 
RU8 _hexToByteRef[ 0xFF ] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

static
RCHAR _byteToHexRef[ 0x10 ] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

RBOOL 
    rpal_string_isprint 
    (
        RCHAR ch
    )
{
    return isprint( ch );
}

RBOOL
    rpal_string_iswprint
    (
        RWCHAR ch
    )
{
    return iswprint( ch );
}


RU8
    rpal_string_str_to_byte
    (
        RPCHAR str
    )
{
    RU8 b = 0;

    if( NULL != str )
    {
        b = _hexToByteRef[ ((RU8*)str)[ 0 ] ] << 4;
        b |= _hexToByteRef[ ((RU8*)str)[ 1 ] ];
    }

    return b;
}

RVOID
    rpal_string_byte_to_str
    (
        RU8 b,
        RCHAR c[ 2 ]
    )
{
    c[ 0 ] = _byteToHexRef[ (b & 0xF0) >> 4 ];
    c[ 1 ] = _byteToHexRef[ (b & 0x0F) ];
}

RU32
    rpal_string_strlen
    (
        RPCHAR str
    )
{
    RU32 size = 0;

    if( NULL != str )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        // The glibc implementation is broken on non-alignmed pointers
        // which is not acceptable, FFS.
        while( 0 != *str )
        {
            size++;
            str++;
        }
#else
        size = (RU32)strlen( str );
#endif
    }

    return size;
}

RU32
    rpal_string_strlenw
    (
        RPWCHAR str
    )
{
    RU32 size = 0;

    if( NULL != str )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        // The glibc implementation is broken on non-alignmed pointers
        // which is not acceptable, FFS.
        while( 0 != *str )
        {
            size++;
            str++;
        }
#else
        size = (RU32)wcslen( str );
#endif
    }

    return size;
}

RU32
    rpal_string_strsize
    (
        RPCHAR str
    )
{
    RU32 size = 0;

    if( NULL != str )
    {
        size = (RU32)( rpal_string_strlen( str ) + 1 ) * sizeof( RCHAR );
    }

    return size;
}

RU32
    rpal_string_strsizew
    (
        RPWCHAR str
    )
{
    RU32 size = 0;

    if( NULL != str )
    {
        size = (RU32)( rpal_string_strlenw( str ) + 1 ) * sizeof( RWCHAR );
    }

    return size;
}

RBOOL
    rpal_string_expand
    (
        RPCHAR  str,
        RPCHAR*  outStr
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != str &&
        NULL != outStr )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        RPCHAR tmp = NULL;
        RU32 size = 0;
        
        size = ExpandEnvironmentStringsA( str, NULL, 0 );
        
        if( 0 != size )
        {
            tmp = rpal_memory_alloc( ( size + 1 ) * sizeof( RCHAR ) );

            if( rpal_memory_isValid( tmp ) )
            {
                size = ExpandEnvironmentStringsA( str, tmp, size );

                if( 0 != size &&
                    rpal_memory_isValid( tmp ) )
                {
                    isSuccess = TRUE;

                    *outStr = tmp;
                }
                else
                {
                    rpal_memory_free( tmp );
                }
            }
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
RPAL_PLATFORM_TODO(String expansion in NIX environment requires custom solution)
        if( NULL != ( *outStr = rpal_string_strdupa( str ) ) )
        {
            isSuccess = TRUE;
        }
#endif
    }

    return isSuccess;
}


RBOOL
    rpal_string_expandw
    (
        RPWCHAR  str,
        RPWCHAR*  outStr
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != str &&
        NULL != outStr )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        RPWCHAR tmp = NULL;
        RU32 size = 0;
        RU32 len = 0;
        
        if( _WCH( '"' ) == str[ 0 ] )
        {
            len = rpal_string_strlenw( str );

            if( _WCH( '"' == str[ len - 1 ] ) )
            {
                str[ len - 1 ] = 0;
            }

            str++;
        }

        size = ExpandEnvironmentStringsW( str, NULL, 0 );
        
        if( 0 != size )
        {
            tmp = rpal_memory_alloc( ( size + 1 ) * sizeof( RWCHAR ) );

            if( rpal_memory_isValid( tmp ) )
            {
                size = ExpandEnvironmentStringsW( str, tmp, size );

                if( 0 != size &&
                    rpal_memory_isValid( tmp ) )
                {
                    isSuccess = TRUE;

                    *outStr = tmp;
                }
                else
                {
                    rpal_memory_free( tmp );
                }
            }
        }
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
RPAL_PLATFORM_TODO(String expansion in NIX environment requires custom solution)
        if( NULL != ( *outStr = rpal_string_strdupw( str ) ) )
        {
            isSuccess = TRUE;
        }
#endif
    }

    return isSuccess;
}

RPWCHAR
    rpal_string_atow
    (
        RPCHAR str
    )
{
    RPWCHAR wide = NULL;
    RU32 nChar = 0;
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPCHAR tmp = NULL;
#endif

    if( NULL != str )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        nChar = MultiByteToWideChar( CP_UTF8, 
                                     0, 
                                     str, 
                                     -1, 
                                     NULL, 
                                     0 );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        // glibc is broken on unaligned wide strings so we must
        // fix their shit for them.
        if( ! IS_PTR_ALIGNED( str ) )
        {
            // Got to realign, let's waste time and mmeory
            tmp = rpal_string_strdupa( str );
        }
        nChar = (RU32)mbstowcs( NULL, NULL != tmp ? tmp : str, 0 );
#endif
        if( 0 != nChar &&
            (RU32)(-1) != nChar )
        {
            wide = rpal_memory_alloc( ( nChar + 1 ) * sizeof( RWCHAR ) );

            if( NULL != wide )
            {
                if( 
#ifdef RPAL_PLATFORM_WINDOWS
                    0 == MultiByteToWideChar( CP_UTF8,
                                              0, 
                                              str, 
                                              -1, 
                                              wide, 
                                              nChar + 1 )
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                    (-1) == mbstowcs( wide, NULL != tmp ? tmp : str, nChar + 1 )
#endif
                    )
                {
                    rpal_memory_free( wide );
                    wide = NULL;
                }
            }
        }
    }
    
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    if( NULL != tmp )
    {
        rpal_memory_free( tmp );
    }
#endif

    return wide;
}

RPCHAR
    rpal_string_wtoa
    (
        RPWCHAR str
    )
{
    RPCHAR ascii = NULL;
    RU32 size = 0;
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    RPWCHAR tmp = NULL;
#endif

    if( NULL != str )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        size = WideCharToMultiByte( CP_UTF8, 
                                    0, 
                                    str, 
                                    -1, 
                                    NULL, 
                                    0, 
                                    NULL, 
                                    NULL );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        // glibc is broken on unaligned wide strings so we must
        // fix their shit for them.
        if( ! IS_PTR_ALIGNED( str ) )
        {
            // Got to realign, let's waste time and mmeory
            tmp = rpal_string_strdupw( str );
        }
        
        size = (RU32)wcstombs( NULL, NULL != tmp ? tmp : str, 0 );
#endif
        if( 0 != size &&
            (RU32)(-1) != size )
        {
            ascii = rpal_memory_alloc( ( size + 1 ) * sizeof( RCHAR ) );

            if( 
#ifdef RPAL_PLATFORM_WINDOWS
                0 == WideCharToMultiByte( CP_UTF8, 
                                          0, 
                                          str, 
                                          -1, 
                                          ascii, 
                                          size + 1, 
                                          NULL, 
                                          NULL )
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                (-1) == wcstombs( ascii, NULL != tmp ? tmp : str, size + 1 )
#endif
                )
            {
                rpal_memory_free( ascii );
                ascii = NULL;
            }
        }
    }
    
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    if( NULL != tmp )
    {
        rpal_memory_free( tmp );
    }
#endif

    return ascii;
}

RPCHAR
    rpal_string_strcata
    (
        RPCHAR str,
        RPCHAR toAdd
    )
{
    RPCHAR out = NULL;
    RU32 originalSize = 0;
    RU32 toAddSize = 0;

    if( NULL != str &&
        NULL != toAdd )
    {
        originalSize = rpal_string_strlen( str ) * sizeof( RCHAR );
        toAddSize = rpal_string_strlen( toAdd ) * sizeof( RCHAR );
        
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        // glibc is broken on unaligned strings so we must
        // fix their shit for them.
        if( ! IS_PTR_ALIGNED( str ) )
        {
            rpal_memory_memcpy( str + originalSize, toAdd, toAddSize );
            out = str;
        }
        else
        {
#endif
            out = strcat( str, toAdd );
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        }
#endif
        
        if( NULL != out )
        {
            out[ ( originalSize + toAddSize ) / sizeof( RCHAR ) ] = 0;
        }
    }

    return out;
}

RPWCHAR
    rpal_string_strcatw
    (
        RPWCHAR str,
        RPWCHAR toAdd
    )
{
    RPWCHAR out = NULL;
    RU32 originalSize = 0;
    RU32 toAddSize = 0;

    if( NULL != str &&
        NULL != toAdd )
    {
        originalSize = rpal_string_strlenw( str ) * sizeof( RWCHAR );
        toAddSize = rpal_string_strlenw( toAdd ) * sizeof( RWCHAR );
        
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        // glibc is broken on unaligned strings so we must
        // fix their shit for them.
        if( ! IS_PTR_ALIGNED( str ) )
        {
            rpal_memory_memcpy( str + originalSize, toAdd, toAddSize );
            out = str;
        }
        else
        {
#endif
            out = wcscat( str, toAdd );
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        }
#endif
        if( NULL != out )
        {
            out[ ( originalSize + toAddSize ) / sizeof( RWCHAR ) ] = 0;
        }
    }

    return out;
}

RPCHAR
    rpal_string_strstr
    (
        RPCHAR haystack,
        RPCHAR needle
    )
{
    RPCHAR out = NULL;

    if( NULL != haystack &&
        NULL != needle )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations);
#endif
        out = strstr( haystack, needle );
    }

    return out;
}

RPWCHAR
    rpal_string_strstrw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    )
{
    RPWCHAR out = NULL;

    if( NULL != haystack &&
        NULL != needle )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations);
#endif
        out = wcsstr( haystack, needle );
    }

    return out;
}


RPWCHAR
    rpal_string_stristrw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    )
{
    RPWCHAR out = NULL;
    RU32 i = 0;
    RU32 hayLen = 0;
    RU32 neeLen = 0;

    if( NULL != haystack &&
        NULL != needle )
    {
        hayLen = rpal_string_strlenw( haystack );
        neeLen = rpal_string_strlenw( needle );
        
        if( hayLen >= neeLen )
        {
            for( i = 0; i <= ( hayLen - neeLen ); i++ )
            {
#ifdef RPAL_PLATFORM_WINDOWS
                if( 0 == wcsicmp( haystack + i, needle ) )
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
                RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations)
                if( 0 == wcscasecmp( haystack + i, needle ) )
#endif
                {
                    out = haystack + i;
                    break;
                }
            }
        }
    }

    return out;
}


RPCHAR
    rpal_string_itoa
    (
        RU32 num,
        RPCHAR outBuff,
        RU32 radix
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return _itoa( num, outBuff, radix );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    if( 10 == radix )
    {
        if( 0 < sprintf( outBuff, "%d", num ) )
        {
            return outBuff;
        }
    }
    else if( 16 == radix )
    {
        if( 0 < sprintf( outBuff, "%X", num ) )
        {
            return outBuff;
        }
    }
    
    return NULL;
#endif
}


RPWCHAR
    rpal_string_itow
    (
        RU32 num,
        RPWCHAR outBuff,
        RU32 radix
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    return _itow( num, outBuff, radix );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    if( 10 == radix )
    {
        if( 0 < swprintf( outBuff, (RSIZET)(-1), _WCH("%d"), num ) )
        {
            return outBuff;
        }
    }
    else if( 16 == radix )
    {
        if( 0 < swprintf( outBuff, (RSIZET)(-1), _WCH("%X"), num ) )
        {
            return outBuff;
        }
    }

    return NULL;
#endif
}


RPCHAR
    rpal_string_strdupa
    (
        RPCHAR strA
    )
{
    RPCHAR out = NULL;
    RU32 len = 0;

    if( NULL != strA )
    {
        len = rpal_string_strlen( strA );

        out = rpal_memory_alloc( len + sizeof( RCHAR ) );

        if( rpal_memory_isValid( out ) )
        {
            rpal_memory_memcpy( out, strA, len );
            out[ len ] = 0;
        }
    }

    return out;
}


RPWCHAR
    rpal_string_strdupw
    (
        RPWCHAR strW
    )
{
    RPWCHAR out = NULL;
    RU32 len = 0;

    if( NULL != strW )
    {
        len = rpal_string_strlenw( strW );

        out = rpal_memory_alloc( ( len + 1 ) * sizeof( RWCHAR ) );

        if( rpal_memory_isValid( out ) )
        {
            rpal_memory_memcpy( out, strW, len * sizeof( RWCHAR ) );
            out[ len ] = 0;
        }
    }

    return out;
}

RBOOL
    rpal_string_match
    (
        RPCHAR pattern,
        RPCHAR str
    )
{
    // Taken and modified from:
    // http://www.codeproject.com/Articles/19694/String-Wildcard-Matching-and

    enum State {
        Exact,      	// exact match
        Any,        	// ?
        AnyRepeat,    	// *
        AnyAtLeastOne,  // +
        Escaped         // [backslash] 
    };

    RCHAR *s = str;
    RCHAR *p = pattern;
    RCHAR *q = 0;
    RU32 state = 0;

    RBOOL match = TRUE;

    while( match && *p )
    {
        if( *p == '*' )
        {
            state = AnyRepeat;
            q = p+1;
        }
        else if( *p == '?' )
        {
            state = Any;
        }
        else if( *p == '\\' )
        {
            state = Exact;
            p++;
        }
        else if( *p == '+' )
        {
            state = AnyRepeat;

            if( 0 != *s )
            {
                q = p+1;
                s++;
            }
            else
            {
                // Ensures we essentially fail the match
                q = p;
            }
        }
        else
        {
            state = Exact;
        }

        if( *s == 0 ){ break; }

        switch( state )
        {
            case Exact:
                match = *s == *p;
                s++;
                p++;
                break;

            case Any:
                match = TRUE;
                s++;
                p++;
                break;

            case AnyRepeat:
                match = TRUE;
                s++;

                if( *s == *q )
                {
                    if( rpal_string_match( q, s ) )
                    {
                        p++;
                    }
                }
                break;
        }
    }

    if( state == AnyRepeat )
    {
        return ( *s == *q );
    }
    else if( state == Any )
    {
        return ( *s == *p );
    }
    else
    {
        return match && ( *s == *p );
    }
}


RBOOL
    rpal_string_matchw
    (
        RPWCHAR pattern,
        RPWCHAR str
    )
{
    // Taken and modified from:
    // http://www.codeproject.com/Articles/19694/String-Wildcard-Matching-and

    enum State {
        Exact,      	// exact match
        Any,        	// ?
        AnyRepeat,    	// *
        AnyAtLeastOne,  // +
        Escaped         // [backslash]
    };

    RWCHAR *s = str;
    RWCHAR *p = pattern;
    RWCHAR *q = 0;
    RU32 state = 0;

    RBOOL match = TRUE;

    if( NULL == pattern ||
        NULL == str )
    {
        return FALSE;
    }

    while( match && *p )
    {
        if( *p == _WCH('*') )
        {
            state = AnyRepeat;
            q = p+1;
        }
        else if( *p == _WCH('?') )
        {
            state = Any;
        }
        else if( *p == _WCH('\\') )
        {
            state = Exact;
            // Only escape if the next character is a wildcard
            // otherwise it's just a normal backslash, avoids
            // some common problems when user forgets to double
            // backslash a path seperator for example.
            if( *(p+1) == _WCH('?') ||
                *(p+1) == _WCH('*') ||
                *(p+1) == _WCH('+') ||
                *(p+1) == _WCH('\\') )
            {
                p++;
            }
        }
        else if( *p == _WCH('+') )
        {
            state = AnyRepeat;

            if( 0 != *s )
            {
                q = p+1;
                s++;
            }
            else
            {
                // Ensures we essentially fail the match
                q = p;
            }
        }
        else
        {
            state = Exact;
        }

        if( *s == 0 ){ break; }

        switch( state )
        {
            case Exact:
                match = *s == *p;
                s++;
                p++;
                break;

            case Any:
                match = TRUE;
                s++;
                p++;
                break;

            case AnyRepeat:
                match = TRUE;
                s++;

                if( *s == *q )
                {
                    if( rpal_string_matchw( q, s ) )
                    {
                        p++;
                    }
                }
                break;
        }
    }

    if( state == AnyRepeat )
    {
        return ( *s == *q );
    }
    else if( state == Any )
    {
        return ( *s == *p );
    }
    else
    {
        return match && ( *s == *p );
    }
}

RPCHAR
    rpal_string_strcatExA
    (
        RPCHAR strToExpand,
        RPCHAR strToCat
    )
{
    RPCHAR res = NULL;
    RU32 finalSize = 0;

    if( NULL != strToCat )
    {
        finalSize = ( rpal_string_strlen( strToExpand ) + 
                      rpal_string_strlen( strToCat ) + 1 ) * sizeof( RCHAR );

        strToExpand = rpal_memory_realloc( strToExpand, finalSize );

        if( NULL != strToExpand )
        {
            res = rpal_string_strcata( strToExpand, strToCat );

            if( !rpal_memory_isValid( res ) )
            {
                res = NULL;
            }
        }
    }

    return res;
}

RPWCHAR
    rpal_string_strcatExW
    (
        RPWCHAR strToExpand,
        RPWCHAR strToCat
    )
{
    RPWCHAR res = NULL;
    RU32 finalSize = 0;

    if( NULL != strToCat )
    {
        finalSize = ( rpal_string_strlenw( strToExpand ) + 
                      rpal_string_strlenw( strToCat ) + 1 ) * sizeof( RWCHAR );

        strToExpand = rpal_memory_realloc( strToExpand, finalSize );

        if( NULL != strToExpand )
        {
            res = rpal_string_strcatw( strToExpand, strToCat );

            if( !rpal_memory_isValid( res ) )
            {
                res = NULL;
            }
        }
    }

    return res;
}

RPCHAR
    rpal_string_strtoka
    (
        RPCHAR str,
        RCHAR token,
        RPCHAR* state
    )
{
    RPCHAR nextToken = NULL;

    if( NULL != state &&
        LOGICAL_XOR( NULL == str, NULL == *state ) )
    {
        if( NULL != str )
        {
            nextToken = str;
            *state = str;
        }
        else
        {
            nextToken = *state;
            nextToken++;
            **state = token;
            (*state)++;
        }

        while( 0 != **state &&
                token != **state )
        {
            (*state)++;
        }

        if( token == **state )
        {
            **state = 0;
        }
        else
        {
            *state = NULL;
        }
    }

    return nextToken;
}

RPWCHAR
    rpal_string_strtokw
    (
        RPWCHAR str,
        RWCHAR token,
        RPWCHAR* state
    )
{
    RPWCHAR nextToken = NULL;

    if( NULL != state &&
        LOGICAL_XOR( NULL == str, NULL == *state ) )
    {
        if( NULL != str )
        {
            nextToken = str;
            *state = str;
        }
        else
        {
            nextToken = *state;
            nextToken++;
            **state = token;
            (*state)++;
        }

        while( 0 != **state &&
                token != **state )
        {
            (*state)++;
        }

        if( token == **state )
        {
            **state = 0;
        }
        else
        {
            *state = NULL;
        }
    }

    return nextToken;
}

RS32
    rpal_string_strcmpw
    (
        RPWCHAR str1,
        RPWCHAR str2
    )
{
    RS32 res = (-1);

    if( NULL != str1 &&
        NULL != str2 )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations);
#endif
        res = wcscmp( str1, str2 );
    }

    return res;
}

RS32
    rpal_string_strcmpa
    (
        RPCHAR str1,
        RPCHAR str2
    )
{
    RS32 res = (-1);

    if( NULL != str1 &&
        NULL != str2 )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations);
#endif
        res = strcmp( str1, str2 );
    }

    return res;
}


RS32
    rpal_string_stricmpa
    (
        RPCHAR str1,
        RPCHAR str2
    )
{
    RS32 res = (-1);

    if( NULL != str1 &&
        NULL != str2 )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        res = stricmp( str1, str2 );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations)
        res = strcasecmp( str1, str2 );
#endif
    }

    return res;
}

RS32
    rpal_string_stricmpw
    (
        RPWCHAR str1,
        RPWCHAR str2
    )
{
    RS32 res = (-1);

    if( NULL != str1 &&
        NULL != str2 )
    {
#ifdef RPAL_PLATFORM_WINDOWS
        res = wcsicmp( str1, str2 );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
        RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations)
        res = wcscasecmp( str1, str2 );
#endif
    }

    return res;
}

RPCHAR
    rpal_string_touppera
    (
        RPCHAR str
    )
{
    RPCHAR ret = NULL;

    if( NULL != str )
    {
        ret = strupr( str );
    }

    return ret;
}

RPWCHAR
    rpal_string_toupperw
    (
        RPWCHAR str
    )
{
    RPWCHAR ret = NULL;

    if( NULL != str )
    {
        ret = wcsupr( str );
    }

    return ret;
}

RPCHAR
    rpal_string_tolowera
    (
        RPCHAR str
    )
{
    RPCHAR ret = NULL;

    if( NULL != str )
    {
        ret = strlwr( str );
    }

    return ret;
}

RPWCHAR
    rpal_string_tolowerw
    (
        RPWCHAR str
    )
{
    RPWCHAR ret = NULL;

    if( NULL != str )
    {
        ret = wcslwr( str );
    }

    return ret;
}

RPCHAR
    rpal_string_strcpya
    (
        RPCHAR dst,
        RPCHAR src
    )
{
    RPCHAR res = NULL;

    if( NULL != dst &&
        NULL != src )
    {
        res = dst;

        while( 0 != *src )
        {
            *dst = *src;
            src++;
            dst++;
        }

        *dst = 0;
    }

    return res;
}

RPWCHAR
    rpal_string_strcpyw
    (
        RPWCHAR dst,
        RPWCHAR src
    )
{
    RPWCHAR res = NULL;
    
    if( NULL != dst &&
        NULL != src )
    {
        res = dst;

        while( 0 != *src )
        {
            *dst = *src;
            src++;
            dst++;
        }

        *dst = 0;
    }

    return res;
}

RBOOL
    rpal_string_atoi
    (
        RPCHAR str,
        RU32* pNum
    )
{
    RBOOL isSuccess = FALSE;
    RPCHAR tmp = 0;

    if( NULL != str &&
        NULL != pNum )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations)
#endif
        *pNum = (RU32)strtol( str, &tmp, 10 );
        
        if( NULL != tmp &&
            0 == *tmp )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_string_wtoi
    (
        RPWCHAR str,
        RU32* pNum
    )
{
    RBOOL isSuccess = FALSE;
    RPWCHAR tmp = 0;

    if( NULL != str &&
        NULL != pNum )
    {
#if defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
RPAL_PLATFORM_TODO(Confirm GLIBC doesnt break this with optimizations)
#endif
        *pNum = (RU32)wcstol( str, &tmp, 10 );
        if( NULL != tmp &&
            0 == *tmp )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_string_filla
    (
        RPCHAR str,
        RU32 nChar,
        RCHAR fillWith
    )
{
    RBOOL isFilledSomerthing = FALSE;
    RU32 i = 0;

    if( NULL != str )
    {
        for( i = 0; i < nChar; i++ )
        {
            if( 0 == str[ i ] )
            {
                isFilledSomerthing = TRUE;
                str[ i ] = fillWith;
            }
        }
    }

    return isFilledSomerthing;
}

RBOOL
    rpal_string_fillw
    (
        RPWCHAR str,
        RU32 nChar,
        RWCHAR fillWith
    )
{
    RBOOL isFilledSomerthing = FALSE;
    RU32 i = 0;

    if( NULL != str )
    {
        for( i = 0; i < nChar; i++ )
        {
            if( 0 == str[ i ] )
            {
                isFilledSomerthing = TRUE;
                str[ i ] = fillWith;
            }
        }
    }

    return isFilledSomerthing;
}

RBOOL
    rpal_string_startswitha
    (
        RPCHAR haystack,
        RPCHAR needle
    )
{
    RBOOL isStartsWith = FALSE;
    
    if( NULL != haystack &&
        NULL != needle )
    {
        if( 0 == rpal_memory_memcmp( haystack, needle, rpal_string_strlen( needle ) * sizeof( RCHAR ) ) )
        {
            isStartsWith = TRUE;
        }
    }
    
    return isStartsWith;
}


RBOOL
    rpal_string_startswithw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    )
{
    RBOOL isStartsWith = FALSE;
    
    if( NULL != haystack &&
        NULL != needle )
    {
        if( 0 == rpal_memory_memcmp( haystack, needle, rpal_string_strlenw( needle ) * sizeof( RWCHAR ) ) )
        {
            isStartsWith = TRUE;
        }
    }
    
    return isStartsWith;
}

RBOOL
    rpal_string_startswithia
    (
        RPCHAR haystack,
        RPCHAR needle
    )
{
    RBOOL isStartsWith = FALSE;
    
    RPCHAR tmpHaystack = NULL;
    RPCHAR tmpNeedle = NULL;
    
    if( NULL != haystack &&
        NULL != needle )
    {
        if( NULL != ( tmpHaystack = rpal_string_strdupa( haystack ) ) )
        {
            if( NULL != ( tmpNeedle = rpal_string_strdupa( needle ) ) )
            {
                tmpHaystack = rpal_string_tolowera( tmpHaystack );
                tmpNeedle = rpal_string_tolowera( tmpNeedle );
                
                if( 0 == rpal_memory_memcmp( tmpHaystack, tmpNeedle, rpal_string_strlen( tmpNeedle ) * sizeof( RCHAR ) ) )
                {
                    isStartsWith = TRUE;
                }
                
                rpal_memory_free( tmpNeedle );
            }
            
            rpal_memory_free( tmpHaystack );
        }
    }
    
    return isStartsWith;
}


RBOOL
    rpal_string_startswithiw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    )
{
    RBOOL isStartsWith = FALSE;
    
    RPWCHAR tmpHaystack = NULL;
    RPWCHAR tmpNeedle = NULL;
    
    if( NULL != haystack &&
        NULL != needle )
    {
        if( NULL != ( tmpHaystack = rpal_string_strdupw( haystack ) ) )
        {
            if( NULL != ( tmpNeedle = rpal_string_strdupw( needle ) ) )
            {
                tmpHaystack = rpal_string_tolowerw( tmpHaystack );
                tmpNeedle = rpal_string_tolowerw( tmpNeedle );
                
                if( 0 == rpal_memory_memcmp( tmpHaystack, tmpNeedle, rpal_string_strlenw( tmpNeedle ) * sizeof( RWCHAR ) ) )
                {
                    isStartsWith = TRUE;
                }
                
                rpal_memory_free( tmpNeedle );
            }
            
            rpal_memory_free( tmpHaystack );
        }
    }
    
    return isStartsWith;
}

RBOOL
    rpal_string_endswitha
    (
        RPCHAR haystack,
        RPCHAR needle
    )
{
    RBOOL isEndsWith = FALSE;
    
    if( NULL != haystack &&
        NULL != needle )
    {
        if( 0 == rpal_memory_memcmp( haystack + rpal_string_strlen( haystack ) - 
                                                rpal_string_strlen( needle ), 
                                     needle, 
                                     rpal_string_strlen( needle ) * sizeof( RCHAR ) ) )
        {
            isEndsWith = TRUE;
        }
    }
    
    return isEndsWith;
}

RBOOL
    rpal_string_endswithw
    (
        RPWCHAR haystack,
        RPWCHAR needle
    )
{
    RBOOL isEndsWith = FALSE;
    
    if( NULL != haystack &&
        NULL != needle )
    {
        if( 0 == rpal_memory_memcmp( haystack + rpal_string_strlenw( haystack ) - 
                                                rpal_string_strlenw( needle ), 
                                     needle, 
                                     rpal_string_strlenw( needle ) * sizeof( RWCHAR ) ) )
        {
            isEndsWith = TRUE;
        }
    }
    
    return isEndsWith;
}


RBOOL
    rpal_string_trima
    (
        RPCHAR str,
        RPCHAR charsToTrim
    )
{
    RBOOL isSomethingTrimmed = FALSE;

    RS32 i = 0;
    RU32 j = 0;
    RU32 nChars = 0;

    if( NULL != str &&
        NULL != charsToTrim )
    {
        nChars = rpal_string_strlen( charsToTrim );

        if( 0 != nChars )
        {
            for( i = rpal_string_strlen( str ) - 1; i >= 0; i-- )
            {
                for( j = 0; j < nChars; j++ )
                {
                    if( charsToTrim[ j ] == str[ i ] )
                    {
                        isSomethingTrimmed = TRUE;
                        str[ i ] = 0;
                        break;
                    }
                }

                if( !isSomethingTrimmed )
                {
                    break;
                }
            }
        }
    }

    return isSomethingTrimmed;
}


RBOOL
    rpal_string_trimw
    (
        RPWCHAR str,
        RPWCHAR charsToTrim
    )
{
    RBOOL isSomethingTrimmed = FALSE;

    RS32 i = 0;
    RU32 j = 0;
    RU32 nChars = 0;

    if( NULL != str &&
        NULL != charsToTrim )
    {
        nChars = rpal_string_strlenw( charsToTrim );

        if( 0 != nChars )
        {
            for( i = rpal_string_strlenw( str ) - 1; i >= 0; i-- )
            {
                for( j = 0; j < nChars; j++ )
                {
                    if( charsToTrim[ j ] == str[ i ] )
                    {
                        isSomethingTrimmed = TRUE;
                        str[ i ] = 0;
                        break;
                    }
                }

                if( !isSomethingTrimmed )
                {
                    break;
                }
            }
        }
    }

    return isSomethingTrimmed;
}

RBOOL
    rpal_string_charIsAscii
    (
        RCHAR c
    )
{
    RBOOL isAscii = FALSE;

    if( ( 0x20 <= c && 0x7E >= c ) ||
        0x09 == c || 0x0D == c || 0x0A == c )
    {
        isAscii = TRUE;
    }

    return isAscii;
}
