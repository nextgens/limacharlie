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

#include <rpal_debug.h>

RVOID
    rpal_debug_print
    (
        RPCHAR format,
        ...
    )
{
#ifdef RPAL_PLATFORM_WINDOWS
    va_list args;

    RCHAR tmp[ 1024 ] = {0};
    
    va_start( args, format );

    vsprintf( (RPCHAR)&tmp, format, args );

    OutputDebugString( tmp );

    va_end( args );
#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )
    va_list args;

    va_start( args, format );

    vfprintf( stderr, format, args );
    fflush( stderr );

    va_end( args );
#endif
}


RVOID
    rpal_debug_printBuffer
    (
        RPU8 buffer,
        RU32 bufferSize,
        RU32 nWrap
    )
{
    RU64 i = 0;
    
    for( i = 0; i < bufferSize; i++ )
    {
        printf( "%02X", buffer[ i ] );
        
        if( 0 == ( ( i + 1 ) % nWrap ) )
        {
            printf( "\n" );
        }
        else
        {
            printf( " " );
        }
    }
    printf( "\n" );
}
