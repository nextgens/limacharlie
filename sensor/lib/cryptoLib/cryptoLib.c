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

#define RPAL_FILE_ID   33

#include <mbedtls/sha256.h>
#include <mbedtls/aes.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#include <cryptoLib/cryptoLib.h>


#pragma pack(push)
#pragma pack(1)
typedef struct
{
    RU8 symKey[ CRYPTOLIB_ASYM_2048_MIN_SIZE ];
    RU8 iv[ CRYPTOLIB_SYM_IV_SIZE ];
    RU8 data[];
} _CryptoLib_FastAsymBuffer;
#pragma pack(pop)


static rMutex g_mutex = NULL;
static mbedtls_entropy_context g_entropy = { 0 };
static mbedtls_ctr_drbg_context g_rng = { 0 };


RBOOL
    CryptoLib_init
    (

    )
{
    RBOOL isSuccess = FALSE;
    RCHAR perso[] = "rp-cryptolib-for-lc";
    if( NULL != ( g_mutex = rMutex_create() ) )
    {
        mbedtls_entropy_init( &g_entropy );
        mbedtls_ctr_drbg_init( &g_rng );
        mbedtls_ctr_drbg_seed( &g_rng,
                               mbedtls_entropy_func,
                               &g_entropy,
                               (const unsigned char*)perso,
                               sizeof( perso ) - sizeof( RCHAR ) );
        isSuccess = TRUE;
    }

    return isSuccess;
}

RVOID
    CryptoLib_deinit
    (

    )
{
    if( rMutex_lock( g_mutex ) )
    {
        rMutex_free( g_mutex );
    }
}

RBOOL
    CryptoLib_sign
    (
        RPVOID bufferToSign,
        RU32 bufferSize,
        RU8 privKey[ CRYPTOLIB_ASYM_KEY_SIZE_PRI ],
        RU8 pSignature[ CRYPTOLIB_SIGNATURE_SIZE ]
    )
{
    RBOOL isSuccess = FALSE;

    RU8 hash[ CRYPTOLIB_HASH_SIZE ] = {0};
    mbedtls_pk_context key = { 0 };
    mbedtls_rsa_context* rsa = NULL;
    
    if( NULL != bufferToSign &&
        0 != bufferSize &&
        NULL != privKey &&
        NULL != pSignature )
    {
        mbedtls_sha256( bufferToSign, bufferSize, hash, 0 );
        
        mbedtls_pk_init( &key );

        if( 0 == mbedtls_pk_parse_key( &key, privKey, CRYPTOLIB_ASYM_KEY_SIZE_PRI, NULL, 0 ) )
	    {
            if( NULL != ( rsa = mbedtls_pk_rsa( key ) ) )
            {
                if( 0 == mbedtls_rsa_pkcs1_encrypt( rsa,
                                                    mbedtls_ctr_drbg_random,
                                                    &g_rng,
                                                    MBEDTLS_RSA_PRIVATE,
                                                    sizeof( hash ),
                                                    hash,
                                                    pSignature ) )
                {
                    isSuccess = TRUE;
                }
            }
        }

        mbedtls_pk_free( &key );
    }

    return isSuccess;
}

RBOOL
    CryptoLib_verify
    (
        RPVOID bufferToVerify,
        RU32 bufferSize,
        RU8 pubKey[ CRYPTOLIB_ASYM_KEY_SIZE_PUB ],
        RU8 signature[ CRYPTOLIB_SIGNATURE_SIZE ]
    )
{
    RBOOL isSuccess = FALSE;

    RU8 hash[ CRYPTOLIB_ASYM_2048_MIN_SIZE ] = { 0 };
    RU8 actualHash[ CRYPTOLIB_HASH_SIZE ] = { 0 };
    mbedtls_pk_context key = { 0 };
    mbedtls_rsa_context* rsa = NULL;
    RSIZET outLength = 0;

    if( NULL != bufferToVerify &&
        0 != bufferSize &&
        NULL != pubKey &&
        NULL != signature )
    {
        mbedtls_sha256( bufferToVerify, bufferSize, actualHash, 0 );
        
        mbedtls_pk_init( &key );

        if( 0 == mbedtls_pk_parse_public_key( &key, pubKey, CRYPTOLIB_ASYM_KEY_SIZE_PUB ) )
	    {
            if( NULL != ( rsa = mbedtls_pk_rsa( key ) ) )
            {
                if( 0 == mbedtls_rsa_pkcs1_decrypt( rsa, 
                                                    mbedtls_ctr_drbg_random, 
                                                    &g_rng,
                                                    MBEDTLS_RSA_PUBLIC, 
                                                    &outLength, 
                                                    signature, 
                                                    hash,
                                                    sizeof( hash ) ) )
                {
                    if( rpal_memory_simpleMemcmp( hash, actualHash, sizeof( actualHash ) ) )
                    {
                        isSuccess = TRUE;
                    }
                }
            }
        }

        mbedtls_pk_free( &key );
    }

    return isSuccess;
}

RBOOL
    CryptoLib_symEncrypt
    (
        RPVOID bufferToEncrypt,
        RU32 bufferSize,
        RU8 key[ CRYPTOLIB_SYM_KEY_SIZE ],
        RU8 iv[ CRYPTOLIB_SYM_IV_SIZE ],
        RU32* pEncryptedSize
    )
{
    RBOOL isSuccess = FALSE;

    mbedtls_aes_context aes = { 0 };
    RU8 emptyIv[ CRYPTOLIB_SYM_IV_SIZE ] = {0};
    RU8* tmp = NULL;
    RU32 nPadding = 0;
    RU32 index = 0;

    if( NULL != bufferToEncrypt &&
        0 != bufferSize &&
        NULL != key &&
        NULL != pEncryptedSize )
    {
        nPadding = ( ( CRYPTOLIB_SYM_MOD_SIZE - ( bufferSize % CRYPTOLIB_SYM_MOD_SIZE ) ) ? 
                     ( CRYPTOLIB_SYM_MOD_SIZE - ( bufferSize % CRYPTOLIB_SYM_MOD_SIZE ) ) : 
                     CRYPTOLIB_SYM_MOD_SIZE );
        *pEncryptedSize = bufferSize + nPadding;

        tmp = bufferToEncrypt;

        for( index = bufferSize; index < bufferSize + nPadding; index++ )
        {
            tmp[ index ] = (RU8)nPadding;
        }

        mbedtls_aes_init( &aes );

        if( 0 == mbedtls_aes_setkey_enc( &aes, key, 256 ) )
        {
            if( 0 == mbedtls_aes_crypt_cbc( &aes, 
                                            MBEDTLS_AES_ENCRYPT, 
                                            *pEncryptedSize, 
                                            NULL != iv ? iv : emptyIv, 
                                            bufferToEncrypt, 
                                            bufferToEncrypt ) )
            {
                isSuccess = TRUE;
            }
        }

        mbedtls_aes_free( &aes );
    }

    return isSuccess;
}

RBOOL
    CryptoLib_symDecrypt
    (
        RPVOID bufferToDecrypt,
        RU32 bufferSize,
        RU8 key[ CRYPTOLIB_SYM_KEY_SIZE ],
        RU8 iv[ CRYPTOLIB_SYM_IV_SIZE ],
        RU32* pDecryptedSize
    )
{
    RBOOL isSuccess = FALSE;

    mbedtls_aes_context aes = { 0 };
    RU8 emptyIv[ CRYPTOLIB_SYM_IV_SIZE ] = {0};
    RU8* tmp = NULL;
    RU8 nPadding = 0;
    RU32 index = 0;

    if( NULL != bufferToDecrypt &&
        0 != bufferSize &&
        NULL != pDecryptedSize &&
        NULL != key )
    {
        tmp = bufferToDecrypt;

        mbedtls_aes_init( &aes );

        if( 0 == mbedtls_aes_setkey_dec( &aes, key, 256 ) )
        {
            if( 0 == mbedtls_aes_crypt_cbc( &aes,
                                            MBEDTLS_AES_DECRYPT,
                                            bufferSize,
                                            NULL != iv ? iv : emptyIv,
                                            bufferToDecrypt,
                                            bufferToDecrypt ) )
            {
                nPadding = tmp[ bufferSize - 1 ];

                if( nPadding < bufferSize &&
                    CRYPTOLIB_SYM_MOD_SIZE >= nPadding )
                {
                    isSuccess = TRUE;

                    for( index = bufferSize - 1; index > ( bufferSize - nPadding ); index-- )
                    {
                        if( tmp[ index ] != nPadding )
                        {
                            isSuccess = FALSE;
                            break;
                        }
                    }

                    if( isSuccess )
                    {
                        *pDecryptedSize = bufferSize - nPadding;
                    }
                }
            }
        }

        mbedtls_aes_free( &aes );
    }

    return isSuccess;
}

RBOOL
    CryptoLib_asymEncrypt
    (
        RPVOID bufferToEncrypt,
        RU32 bufferSize,
        RU8 pubKey[ CRYPTOLIB_ASYM_KEY_SIZE_PUB ],
        RPU8* pEncryptedBuffer,
        RU32* pEncryptedSize
    )
{
    RBOOL isSuccess = FALSE;

    mbedtls_pk_context key = { 0 };
    
    RSIZET outSize = 0;
    RPU8 outBuff = NULL;

    if( NULL != bufferToEncrypt &&
        0 != bufferSize &&
        NULL != pubKey &&
        NULL != pEncryptedBuffer &&
        NULL != pEncryptedSize )
    {
        mbedtls_pk_init( &key );

        if( 0 == mbedtls_pk_parse_public_key( &key, pubKey, CRYPTOLIB_ASYM_KEY_SIZE_PUB ) )
	    {
            outSize = MBEDTLS_MPI_MAX_SIZE;

            if( 0 < outSize )
            {
                if( bufferSize > outSize )
                {
                    outSize = bufferSize;
                }

                outBuff = rpal_memory_alloc( outSize );

                if( rpal_memory_isValid( outBuff ) )
                {
                    if( 0 == mbedtls_pk_encrypt( &key, 
                                                 bufferToEncrypt, 
                                                 bufferSize, 
                                                 outBuff, 
                                                 &outSize, 
                                                 outSize, 
                                                 mbedtls_ctr_drbg_random, 
                                                 &g_rng ) )
                    {
                        *pEncryptedBuffer = outBuff;
                        *pEncryptedSize = (RU32)outSize;

                        isSuccess = TRUE;
                    }
                    else
                    {
                        rpal_memory_free( outBuff );
                    }
                }
            }
        }

        mbedtls_pk_free( &key );
    }

    return isSuccess;
}

RBOOL
    CryptoLib_asymDecrypt
    (
        RPVOID bufferToDecrypt,
        RU32 bufferSize,
        RU8 priKey[ CRYPTOLIB_ASYM_KEY_SIZE_PRI ],
        RPU8* pDecryptedBuffer,
        RU32* pDecryptedSize
    )
{
    RBOOL isSuccess = FALSE;

    mbedtls_pk_context key = { 0 };

    RSIZET outSize = 0;
    RPU8 outBuff = NULL;

    if( NULL != bufferToDecrypt &&
        0 != bufferSize &&
        NULL != priKey &&
        NULL != pDecryptedBuffer &&
        NULL != pDecryptedSize )
    {
        mbedtls_pk_init( &key );

        if( 0 == mbedtls_pk_parse_key( &key, priKey, CRYPTOLIB_ASYM_KEY_SIZE_PRI, NULL, 0 ) )
	    {
            outBuff = rpal_memory_alloc( bufferSize );

            if( rpal_memory_isValid( outBuff ) )
            {
                if( 0 == mbedtls_pk_decrypt( &key,
                                             bufferToDecrypt,
                                             bufferSize,
                                             outBuff,
                                             &outSize,
                                             outSize,
                                             mbedtls_ctr_drbg_random,
                                             &g_rng ) )
                {
                    *pDecryptedBuffer = (RU8*)outBuff;
                    *pDecryptedSize = (RU32)outSize;

                    isSuccess = TRUE;
                }
                else
                {
                    rpal_memory_free( outBuff );
                }
            }
        }

        mbedtls_pk_free( &key );
    }

    return isSuccess;
}

RBOOL
    CryptoLib_genRandomBytes
    (
        RPU8 pRandBytes,
        RU32  bytesRequired
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != pRandBytes &&
        0 != bytesRequired )
    {
        if( 0 == mbedtls_ctr_drbg_random( &g_rng, pRandBytes, bytesRequired ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    CryptoLib_fastAsymEncrypt
    (
        RPVOID bufferToEncrypt,
        RU32 bufferSize,
        RU8 pubKey[ CRYPTOLIB_ASYM_KEY_SIZE_PUB ],
        RPU8* pEncryptedBuffer,
        RU32* pEncryptedSize
    )
{
    RBOOL isSuccess = FALSE;

    RU8 symKey[ CRYPTOLIB_SYM_KEY_SIZE ] = {0};
    RU8 iv[ CRYPTOLIB_SYM_IV_SIZE ] = {0};

    mbedtls_pk_context key = { 0 };

    _CryptoLib_FastAsymBuffer* asymBuffer = NULL;
    RU32 asymSize = 0;
    RSIZET tmpLength = 0;

    RU32 symSize = 0;

    if( NULL != bufferToEncrypt &&
        0 != bufferSize &&
        NULL != pubKey &&
        NULL != pEncryptedBuffer &&
        NULL != pEncryptedSize )
    {
        mbedtls_pk_init( &key );

        if( 0 == mbedtls_pk_parse_public_key( &key, pubKey, CRYPTOLIB_ASYM_KEY_SIZE_PUB ) )
	    {
            if( CryptoLib_genRandomBytes( symKey, sizeof( symKey ) ) &&
                CryptoLib_genRandomBytes( iv, sizeof( iv ) ) )
            {
                asymSize = sizeof( _CryptoLib_FastAsymBuffer ) + bufferSize;

                asymBuffer = rpal_memory_alloc( asymSize + CRYPTOLIB_SYM_MOD_SIZE ); // 16 because of block round-up

                if( rpal_memory_isValid( asymBuffer ) )
                {
                    if( 0 == mbedtls_pk_encrypt( &key, 
                                                 symKey, 
                                                 sizeof( symKey ), 
                                                 asymBuffer->symKey, 
                                                 &tmpLength, 
                                                 sizeof( asymBuffer->symKey ), 
                                                 mbedtls_ctr_drbg_random, 
                                                 &g_rng ) )
                    {
						rpal_memory_memcpy( asymBuffer->iv, iv, sizeof( iv ) );

                        if( CryptoLib_symEncrypt( bufferToEncrypt, bufferSize, symKey, iv, &symSize ) )
                        {
                            rpal_memory_memcpy( asymBuffer->data , bufferToEncrypt, symSize );

                            *pEncryptedBuffer = (RPU8)asymBuffer;
                            *pEncryptedSize = asymSize - bufferSize + symSize;

                            isSuccess = TRUE;
                        }
                        else
                        {
                            rpal_memory_free( asymBuffer );
                        }
                    }
                    else
                    {
                        rpal_memory_free( asymBuffer );
                    }
                }

                rpal_memory_zero( symKey, sizeof( symKey ) );
                rpal_memory_zero( iv, sizeof( iv ) );
            }
        }

        mbedtls_pk_free( &key );
    }

    return isSuccess;
}

RBOOL
    CryptoLib_fastAsymDecrypt
    (
        RPVOID bufferToDecrypt,
        RU32 bufferSize,
        RU8 priKey[ CRYPTOLIB_ASYM_KEY_SIZE_PRI ],
        RPU8* pDecryptedBuffer,
        RU32* pDecryptedSize
    )
{
    RBOOL isSuccess = FALSE;

    _CryptoLib_FastAsymBuffer* asymBuffer = NULL;
    RPU8 symKey = NULL;
    RU32 symSize = 0;
    RU32 decryptedSize = 0;

    if( NULL != bufferToDecrypt &&
        sizeof( _CryptoLib_FastAsymBuffer ) < bufferSize &&
        NULL != priKey &&
        NULL != pDecryptedBuffer &&
        NULL != pDecryptedSize )
    {
        asymBuffer = bufferToDecrypt;

        if( CryptoLib_asymDecrypt( bufferToDecrypt, 
                                   CRYPTOLIB_ASYM_2048_MIN_SIZE, 
                                   priKey, 
                                   &symKey, 
                                   &symSize ) )
        {
            if( CryptoLib_symDecrypt( asymBuffer->data, 
                                      bufferSize - sizeof( _CryptoLib_FastAsymBuffer ), 
                                      symKey, 
                                      asymBuffer->iv, 
                                      &decryptedSize ) )
            {
                *pDecryptedSize = decryptedSize;
                *pDecryptedBuffer = asymBuffer->data;

                isSuccess = TRUE;
            }

            rpal_memory_free( symKey );
        }
    }

    return isSuccess;
}

RBOOL
    CryptoLib_hash
    (
        RPVOID buffer,
        RU32 bufferSize,
        RU8 pHash[ CRYPTOLIB_HASH_SIZE ]
    )
{
    RBOOL isSuccess = FALSE;

    if( NULL != buffer &&
        0 != bufferSize &&
        NULL != pHash )
    {
        mbedtls_sha256( buffer, bufferSize, pHash, 0 );
        isSuccess = TRUE;
    }

    return isSuccess;
}


RBOOL
    CryptoLib_hashFileW
    (
        RPWCHAR fileName,
        RU8 pHash[ CRYPTOLIB_HASH_SIZE ],
        RBOOL isAvoidTimestamps
    )
{
    RBOOL isSuccess = FALSE;

    mbedtls_sha256_context ctx = { 0 };
    RU8 buff[1024] = {0};
    rFile f = NULL;
    RU32 read = 0;

    if( NULL != fileName &&
        NULL != pHash )
    {
        if( rFile_open( fileName, &f, RPAL_FILE_OPEN_READ |
                                      RPAL_FILE_OPEN_EXISTING |
                                      ( isAvoidTimestamps ? RPAL_FILE_OPEN_AVOID_TIMESTAMPS : 0 ) ) )
        {
            mbedtls_sha256_init( &ctx );
            mbedtls_sha256_starts( &ctx, 0 );
            while( ( read = rFile_readUpTo( f, sizeof( buff ), buff ) ) > 0 )
            {
                mbedtls_sha256_update( &ctx, buff, read );
            }
            mbedtls_sha256_finish( &ctx, pHash );
            mbedtls_sha256_free( &ctx );
            isSuccess = TRUE;

            rFile_close( f );
        }
    }

    return isSuccess;
}

RBOOL
    CryptoLib_hashFileA
    (
        RPCHAR fileName,
        RU8 pHash[ CRYPTOLIB_HASH_SIZE ],
        RBOOL isAvoidTimestamps
    )
{
    RBOOL isSuccess = FALSE;

    RPWCHAR wFile = NULL;

    if( NULL != fileName &&
        NULL != pHash )
    {
	    if( NULL != ( wFile = rpal_string_atow( fileName ) ) )
	    {
            isSuccess = CryptoLib_hashFileW( wFile, pHash, isAvoidTimestamps );
	    
	        rpal_memory_free( wFile );
	    }
    }

    return isSuccess;
}

