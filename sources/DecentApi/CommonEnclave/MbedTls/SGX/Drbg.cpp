//#if ENCLAVE_PLATFORM_SGX

#include "../../../Common/MbedTls/Drbg.h"
#include "../../../Common/SGX/RuntimeError.h"

#include <sgx_trts.h>

using namespace Decent::MbedTlsObj;

Drbg::Drbg() :
	m_state(nullptr)
{
}

Drbg::~Drbg()
{
}

void Drbg::Rand(void* buf, const size_t size)
{
	DECENT_CHECK_SGX_FUNC_CALL_ERROR(sgx_read_rand, static_cast<unsigned char *>(buf), size);
}

//#endif //ENCLAVE_PLATFORM_SGX
