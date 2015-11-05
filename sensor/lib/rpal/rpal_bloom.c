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

#include <rpal/rpal_bloom.h>
#include <math.h>

#define RPAL_FILE_ID    3


struct bloom
{
  // These fields are part of the public interface of this structure.
  // Client code may read these values if desired. Client code MUST NOT
  // modify any of these.
  RS32 entries;
  RDOUBLE error;
  RS32 bits;
  RS32 bytes;
  RU32 hashes;

  // Fields below are private to the implementation. These may go away or
  // change incompatibly at any moment. Client code MUST NOT access or rely
  // on these.
  RDOUBLE bpe;
  RU8 * bf;
  RS32 ready;
};

typedef struct
{
    RU32 nExpectedEntries;
    RDOUBLE errorRate;
    RU32 nEntries;
    struct bloom rawBloom;

} _rBloom;


//=============================================================================
//  PRIVATE FUNCTIONS
//  Internals taken from https://github.com/jvirkki/libbloom/blob/master/bloom.c
//  and modified for our purpose.
//=============================================================================
static RU32 
    murmurhash2
    ( 
        const void * key, 
        RS32 len, 
        const RU32 seed 
    )
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const RU32 m = 0x5bd1e995;
    const RS32 r = 24;

    // Initialize the hash to a 'random' value

    RU32 h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const RU8 * data = (const RU8 *)key;

    while( len >= 4 )
    {
        RU32 k = *(RU32 *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch( len )
    {
        case 3: h ^= data[ 2 ] << 16;
        case 2: h ^= data[ 1 ] << 8;
        case 1: h ^= data[ 0 ];
            h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

static RS32 
    bloom_check_add
    ( 
        struct bloom * bloom,
        const void * buffer, 
        RS32 len, 
        RS32 add
    )
{
    RU32 hits = 0;
    register RU32 a = murmurhash2( buffer, len, 0x9747b28c );
    register RU32 b = murmurhash2( buffer, len, a );
    register RU32 x;
    register RU32 i;
    register RU32 byte;
    register RU32 mask;
    register RU8 c;

    if( bloom->ready == 0 ) {
#ifdef RPAL_PLATFORM_DEBUG
        ( void )printf( "bloom at %p not initialized!\n", (void *)bloom );
#endif
        return -1;
    }


    for( i = 0; i < bloom->hashes; i++ )
    {
        x = (a + i*b) % bloom->bits;
        byte = x >> 3;
        c = bloom->bf[ byte ];        // expensive memory access
        mask = 1 << (x % 8);

        if( c & (RU8)mask )
        {
            hits++;
        }
        else
        {
            if( add )
            {
                bloom->bf[ byte ] = c | (RU8)mask;
            }
            else
            {
                break;
            }
        }
    }

    if( hits == bloom->hashes )
    {
        return 1;                   // 1 == element already in (or collision)
    }

    return 0;
}

static RS32 
    bloom_init
    ( 
        struct bloom * bloom, 
        RS32 entries, 
        RDOUBLE error
    )
{
    RDOUBLE num = 0;
    RDOUBLE denom = 0;
    RDOUBLE dentries = 0;

    bloom->ready = 0;

    if( entries < 1 || error == 0 )
    {
        return 1;
    }

    bloom->entries = entries;
    bloom->error = error;

    num = log( bloom->error );
    denom = 0.480453013918201; // ln(2)^2
    bloom->bpe = -( num / denom );

    dentries = (RDOUBLE)entries;
    bloom->bits = (int)(RU64)( dentries * bloom->bpe );

    if( bloom->bits % 8 )
    {
        bloom->bytes = (bloom->bits / 8) + 1;
    }
    else
    {
        bloom->bytes = bloom->bits / 8;
    }

    bloom->hashes = (int)(RU64)ceil( 0.693147180559945 * bloom->bpe );  // ln(2)

    bloom->bf = (RU8 *)rpal_memory_alloc( bloom->bytes );
    if( bloom->bf == NULL )
    {
        return 1;
    }

    bloom->ready = 1;
    return 0;
}

static RS32 
    bloom_check
    ( 
        struct bloom * bloom, 
        const void * buffer, 
        RS32 len
    )
{
    return bloom_check_add( bloom, buffer, len, 0 );
}

static RS32 
    bloom_add
    ( 
        struct bloom * bloom, 
        const void * buffer, 
        RS32 len
    )
{
    return bloom_check_add( bloom, buffer, len, 1 );
}

#ifdef RPAL_PLATFORM_DEBUG
static void 
    bloom_print
    ( 
        struct bloom * bloom 
    )
{
    (void)printf( "bloom at %p\n", (void *)bloom );
    (void)printf( " ->entries = %d\n", bloom->entries );
    (void)printf( " ->error = %f\n", bloom->error );
    (void)printf( " ->bits = %d\n", bloom->bits );
    (void)printf( " ->bits per elem = %f\n", bloom->bpe );
    (void)printf( " ->bytes = %d\n", bloom->bytes );
    (void)printf( " ->hash functions = %d\n", bloom->hashes );
}
#endif

static void
    bloom_free
    ( 
        struct bloom * bloom 
    )
{
    if( bloom->ready )
    {
        rpal_memory_free( bloom->bf );
    }
    bloom->ready = 0;
}

//=============================================================================
//  PUBLIC API
//=============================================================================

rBloom
    rpal_bloom_create
    (
        RU32 nExpectedEntries,
        RDOUBLE errorRate
    )
{
    _rBloom* pB = NULL;

    if( NULL != ( pB = rpal_memory_alloc( sizeof( _rBloom ) ) ) )
    {
        pB->errorRate = errorRate;
        pB->nExpectedEntries = nExpectedEntries;
        pB->nEntries = 0;
        if( 0 != bloom_init( &pB->rawBloom, nExpectedEntries, errorRate ) )
        {
            rpal_memory_free( pB );
            pB = NULL;
        }
        else
        {
#ifdef RPAL_PLATFORM_DEBUG
            bloom_print( &pB->rawBloom );
#endif
        }
    }

    return (rBloom)pB;
}

RVOID
    rpal_bloom_destroy
    (
        rBloom bloom
    )
{
    if( rpal_memory_isValid( bloom ) )
    {
        bloom_free( &((_rBloom*)bloom)->rawBloom );
        rpal_memory_free( bloom );
    }
}

RBOOL
    rpal_bloom_present
    (
        rBloom bloom,
        RPVOID pBuff,
        RU32 size
    )
{
    RBOOL isPresent = FALSE;
    _rBloom* pB = (_rBloom*)bloom;

    if( rpal_memory_isValid( pB ) &&
        NULL != pBuff &&
        0 != size )
    {
        if( 1 == bloom_check( &pB->rawBloom, pBuff, size ) )
        {
            isPresent = TRUE;
        }
    }

    return isPresent;
}

RBOOL
    rpal_bloom_add
    (
        rBloom bloom,
        RPVOID pBuff,
        RU32 size
    )
{
    RBOOL isSuccess = FALSE;
    _rBloom* pB = (_rBloom*)bloom;
    RU32 ret = 0;

    if( rpal_memory_isValid( pB ) &&
        NULL != pBuff &&
        0 != size )
    {
        ret = bloom_add( &pB->rawBloom, pBuff, size );

        if( 0 == ret )
        {
            pB->nEntries++;
            isSuccess = TRUE;
        }
        else if( 1 == ret )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RU32
    rpal_bloom_getNumEntries
    (
        rBloom bloom
    )
{
    RU32 n = 0;
    _rBloom* pB = (_rBloom*)bloom;

    if( rpal_memory_isValid( pB ) )
    {
        n = pB->nEntries;
    }

    return n;
}

RBOOL
    rpal_bloom_addIfNew
    (
        rBloom bloom,
        RPVOID pBuff,
        RU32 size
    )
{
    RBOOL isNew = FALSE;
    _rBloom* pB = (_rBloom*)bloom;

    if( rpal_memory_isValid( pB ) &&
        NULL != pBuff &&
        0 != size )
    {
        if( 0 == bloom_add( &pB->rawBloom, pBuff, size ) )
        {
            pB->nEntries++;
            isNew = TRUE;
        }
    }

    return isNew;
}

RBOOL
    rpal_bloom_reset
    (
        rBloom bloom
    )
{
    RBOOL isSuccess = FALSE;

    _rBloom* pB = (_rBloom*)bloom;

    if( rpal_memory_isValid( pB ) )
    {
        rpal_memory_zero( pB->rawBloom.bf, pB->rawBloom.bytes );
        pB->nEntries = 0;
        isSuccess = TRUE;
    }

    return isSuccess;
}

RBOOL
    rpal_bloom_serialize
    (
        rBloom bloom,
        RPU8* pBuff,
        RU32* pBuffSize
    )
{
    RBOOL isSuccess = FALSE;

    _rBloom* pB = (_rBloom*)bloom;

    if ( rpal_memory_isValid( bloom ) &&
         NULL != pBuff &&
         NULL != pBuffSize )
    {
        *pBuffSize = sizeof( _rBloom ) + pB->rawBloom.bytes;
        if( NULL != ( *pBuff = rpal_memory_alloc( *pBuffSize ) ) )
        {
            rpal_memory_memcpy( *pBuff, pB, sizeof( _rBloom ) );
            rpal_memory_memcpy( *pBuff + sizeof( _rBloom ), pB->rawBloom.bf, pB->rawBloom.bytes );
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

rBloom
    rpal_bloom_deserialize
    (
        RPU8 pBuff,
        RU32 buffSize
    )
{
    _rBloom* pB = NULL;

    if( NULL != pBuff &&
        sizeof( _rBloom ) < buffSize )
    {
        if( NULL != ( pB = rpal_memory_alloc( sizeof( _rBloom ) ) ) )
        {
            rpal_memory_memcpy( pB, pBuff, sizeof( _rBloom ) );

            if( pB->rawBloom.bytes == (RS32)( buffSize - sizeof( _rBloom ) ) &&
                NULL != ( pB->rawBloom.bf = rpal_memory_alloc( pB->rawBloom.bytes ) ) )
            {
                rpal_memory_memcpy( pB->rawBloom.bf, pBuff + sizeof( _rBloom ), pB->rawBloom.bytes );
            }
            else
            {
                // Mistatch between expected bloom filter size and buffer, or
                // error allocating bloom filter.
                rpal_memory_free( pB );
                pB = NULL;
            }
        }
    }


    return (rBloom)pB;
}
