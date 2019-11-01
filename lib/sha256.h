#ifndef LIB_SHA256_H
#define LIB_SHA256_H

#include <stdlib.h>

typedef struct mbedtls_sha256_context
{
    uint32_t total[2];          /*!< The number of Bytes processed.  */
    uint32_t state[8];          /*!< The intermediate digest state.  */
    unsigned char buffer[64];   /*!< The data block being processed. */
    int is224;                  /*!< Determines which function to use:
                                     0: Use SHA-256, or 1: Use SHA-224. */
} mbedtls_sha256_context;

void mbedtls_sha256_init( mbedtls_sha256_context *ctx );
int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 );
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen );
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32] );
int mbedtls_sha256_ret( const unsigned char *input, size_t ilen, unsigned char output[32], int is224 );

#endif /* LIB_SHA256_H */
