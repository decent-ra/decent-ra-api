/**
 * \file enclave_sub_func.h
 *
 * \brief This file contains the substitute functions (mainly used by mbed TLS) 
 *        for enclave programs.
 */

#ifndef MBEDTLS_SUB_FUNC_ENCLAVE_H
#define MBEDTLS_SUB_FUNC_ENCLAVE_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define fprintf_enclave(stream, format, ...) (0)

inline int printf_enclave(const char *format, ...)
{
	(void)(format);
	//Do nothing so we don't accidentally leak the internal 
	// state of the enclave program.
	return 0;
}

int snprintf_enclave(char *s, size_t n, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif //!MBEDTLS_SUB_FUNC_ENCLAVE_H
