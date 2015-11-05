#ifndef _LIB_SHA1_H
#define _LIB_SHA1_H
#define SHA1_DIGEST_SIZE	20

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC_VERSION__
    #include <sys/types.h>
#else
    #define uint    unsigned int
#endif

#define uchar   unsigned char
#define ulong   unsigned long int

typedef struct
{
    ulong total[2];
    ulong state[5];
    uchar buffer[64];
}
sha1_context;

/*
 * Core SHA-1 functions
 */
void sha1_starts( sha1_context *ctx );
void sha1_update( sha1_context *ctx, uchar *input, uint length );
void sha1_finish( sha1_context *ctx, uchar digest[20] );

/*
 * Output SHA-1(buf)
 */
void sha1_csum( void *buf, uint buflen, uchar digest[20] );

/*
 * Output HMAC-SHA-1(key,buf)
 */
void sha1_hmac( uchar *key, uint keylen, uchar *buf, uint buflen,
                uchar digest[20] );

/*
 * Checkup routine
 */
int sha1_self_test( void );

#ifdef __cplusplus
}
#endif

#endif
