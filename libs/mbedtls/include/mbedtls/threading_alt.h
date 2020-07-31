#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_THREADING_ALT)

typedef void* mbedtls_threading_mutex_t;

#endif

#endif /* MBEDTLS_THREADING_ALT_H */
