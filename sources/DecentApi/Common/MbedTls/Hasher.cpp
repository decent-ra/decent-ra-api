#include "Hasher.h"

#include <mbedtls/md.h>

#include "MbedTlsException.h"
#include "Internal/Hasher.h"

using namespace Decent;
using namespace Decent::MbedTlsObj;

const mbedtls_md_info_t& MbedTlsObj::GetMsgDigestInfo(HashType type)
{
	const mbedtls_md_info_t* res = mbedtls_md_info_from_type(detail::GetMsgDigestType(type));
	if (res)
	{
		return *res;
	}
	else
	{
		throw RuntimeException("mbedTLS msg digest info not found.");
	}
}

void MsgDigestBase::FreeObject(mbedtls_md_context_t * ptr)
{
	mbedtls_md_free(ptr);
	delete ptr;
}

MsgDigestBase::MsgDigestBase(const mbedtls_md_info_t & mdInfo, bool needHMac) :
	MsgDigestBase()
{
	static_assert(false == 0, "The value of false is different with the one expected in mbedTLS.");
	CALL_MBEDTLS_C_FUNC(mbedtls_md_setup, Get(), &mdInfo, needHMac);
}

MsgDigestBase::~MsgDigestBase()
{
}

MsgDigestBase::MsgDigestBase() :
	ObjBase(new mbedtls_md_context_t, &FreeObject)
{
	mbedtls_md_init(Get());
}

MsgDigestBase::MsgDigestBase(MsgDigestBase && rhs) :
	ObjBase(std::forward<ObjBase>(rhs))
{
}

HasherBase::~HasherBase()
{
}

HasherBase::HasherBase(const mbedtls_md_info_t & mdInfo) :
	MsgDigestBase(mdInfo, false)
{
	CALL_MBEDTLS_C_FUNC(mbedtls_md_starts, Get());
}

void HasherBase::Update(const void * data, const size_t dataSize)
{
	CALL_MBEDTLS_C_FUNC(mbedtls_md_update, Get(), static_cast<const unsigned char*>(data), dataSize);
}

void HasherBase::Finish(void * output)
{
	CALL_MBEDTLS_C_FUNC(mbedtls_md_finish, Get(), static_cast<unsigned char*>(output));
}

HMACerBase::~HMACerBase()
{
}

HMACerBase::HMACerBase(const mbedtls_md_info_t & mdInfo, const void * key, const size_t keySize) :
	MsgDigestBase(mdInfo, true)
{
	CALL_MBEDTLS_C_FUNC(mbedtls_md_hmac_starts, Get(), static_cast<const unsigned char*>(key), (keySize * BITS_PER_BYTE));
}

void HMACerBase::Update(const void * data, const size_t dataSize)
{
	if (dataSize > 0 && !data)
	{
		throw RuntimeException("Invalid parameter(s) given to HMACerBase::Update");
	}

	CALL_MBEDTLS_C_FUNC(mbedtls_md_hmac_update, Get(), static_cast<const unsigned char*>(data), dataSize);
}

void HMACerBase::Finish(void * output)
{
	if (!output)
	{
		throw RuntimeException("Invalid parameter(s) given to HMACerBase::Finish");
	}

	CALL_MBEDTLS_C_FUNC(mbedtls_md_hmac_finish, Get(), static_cast<unsigned char*>(output));
}
