#include "Kdf.h"

#include <mbedtls/hkdf.h>

#include "Hasher.h"
#include "MbedTlsException.h"

using namespace Decent;
using namespace Decent::MbedTlsObj;

void detail::HKDF(HashType hashType, const void * inKey, const size_t inKeyLen, const void * label, const size_t labelLen, const void * inSalt, const size_t inSaltLen, void * outKey, const size_t outKeyLen)
{
	CALL_MBEDTLS_C_FUNC(mbedtls_hkdf, &GetMsgDigestInfo(hashType), static_cast<const unsigned char *>(inSalt), inSaltLen,
		static_cast<const unsigned char *>(inKey), inKeyLen,
		static_cast<const unsigned char *>(label), labelLen,
		static_cast<unsigned char *>(outKey), outKeyLen);
}
