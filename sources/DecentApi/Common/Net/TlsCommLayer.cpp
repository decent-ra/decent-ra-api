#include "TlsCommLayer.h"

#include <memory>

#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>

#include "../Common.h"
#include "../make_unique.h"

#include "../MbedTls/Session.h"
#include "../MbedTls/X509Cert.h"
#include "../MbedTls/TlsConfig.h"
#include "../MbedTls/AsymKeyBase.h"
#include "../MbedTls/MbedTlsException.h"

#include "NetworkException.h"
#include "ConnectionBase.h"

using namespace Decent::Net;
using namespace Decent::MbedTlsObj;

namespace
{
	static int MbedTlsSslSend(void *ctx, const unsigned char *buf, size_t len)
	{
		return ConnectionBase::SendRawCallback(ctx, buf, len);
	}

	static int MbedTlsSslRecv(void *ctx, unsigned char *buf, size_t len)
	{
		return ConnectionBase::RecvRawCallback(ctx, buf, len);
	}
}

TlsCommLayer::TlsCommLayer(ConnectionBase& cnt, std::shared_ptr<const TlsConfig> tlsConfig, bool reqPeerCert, std::shared_ptr<const Session> session) :
	m_sslCtx(Tools::make_unique<mbedtls_ssl_context>()),
	m_tlsConfig(tlsConfig)
{
	if (!tlsConfig)
	{
		throw Exception("The parameter given to the TLS Communication Layer is invalid.");
	}

	tlsConfig->NullCheck();

	mbedtls_ssl_init(m_sslCtx.get());

	int mbedRet = mbedtls_ssl_setup(m_sslCtx.get(), tlsConfig->Get());
	if (mbedRet != MBEDTLS_SUCCESS_RET)
	{
		mbedtls_ssl_free(m_sslCtx.get());
		throw Decent::MbedTlsObj::MbedTlsException("TlsCommLayer::TlsCommLayer::mbedtls_ssl_setup", mbedRet);
	}

	mbedtls_ssl_set_bio(m_sslCtx.get(), &cnt, &MbedTlsSslSend, &MbedTlsSslRecv, nullptr);
	mbedtls_ssl_set_hs_authmode(m_sslCtx.get(), reqPeerCert ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_NONE);

	if (session)
	{
		mbedRet = mbedtls_ssl_session_reset(m_sslCtx.get());
		if (mbedRet != MBEDTLS_SUCCESS_RET)
		{
			mbedtls_ssl_free(m_sslCtx.get());
			throw Decent::MbedTlsObj::MbedTlsException("TlsCommLayer::TlsCommLayer::mbedtls_ssl_set_session", mbedRet);
		}

		mbedRet = mbedtls_ssl_set_session(m_sslCtx.get(), session->Get()); 
		if (mbedRet != MBEDTLS_SUCCESS_RET)
		{
			mbedtls_ssl_free(m_sslCtx.get());
			throw Decent::MbedTlsObj::MbedTlsException("TlsCommLayer::TlsCommLayer::mbedtls_ssl_set_session", mbedRet);
		}
	}

	mbedRet = mbedtls_ssl_handshake(m_sslCtx.get());
	if (mbedRet != MBEDTLS_SUCCESS_RET)
	{
		mbedtls_ssl_free(m_sslCtx.get());
		throw Decent::MbedTlsObj::MbedTlsException("TlsCommLayer::TlsCommLayer::mbedtls_ssl_handshake", mbedRet);
	}
}

TlsCommLayer::TlsCommLayer(TlsCommLayer && other) :
	m_sslCtx(std::move(other.m_sslCtx)),
	m_tlsConfig(std::move(other.m_tlsConfig))
{
	other.m_sslCtx.reset();
	other.m_tlsConfig.reset();
}

TlsCommLayer::~TlsCommLayer()
{
	//try
	//{
	//	mbedtls_ssl_close_notify(m_sslCtx.get());
	//} catch (const std::exception&) { }

	mbedtls_ssl_free(m_sslCtx.get());
}

TlsCommLayer & TlsCommLayer::operator=(TlsCommLayer && other)
{
	if (this != &other)
	{
		m_sslCtx.swap(other.m_sslCtx);
		m_tlsConfig.swap(other.m_tlsConfig);
	}
	return *this;
}

size_t TlsCommLayer::SendRaw(const void * buf, const size_t size)
{
	if (!IsValid())
	{
		throw ConnectionNotEstablished();
	}
	if (!buf)
	{
		throw Net::Exception("Function called with null pointer.");
	}

	int mbedRet = MBEDTLS_ERR_SSL_WANT_WRITE;

	while (mbedRet == MBEDTLS_ERR_SSL_WANT_WRITE)
	{
		mbedRet = mbedtls_ssl_write(m_sslCtx.get(), static_cast<const uint8_t*>(buf), size); //Pure C function, assume nothrow;
		if (mbedRet < 0 && mbedRet != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			throw MbedTlsException("mbedtls_ssl_write", mbedRet);
		}
	}

	return static_cast<size_t>(mbedRet);
}

size_t TlsCommLayer::RecvRaw(void * buf, const size_t size)
{
	if (!IsValid())
	{
		throw ConnectionNotEstablished();
	}
	if (!buf)
	{
		throw Net::Exception("Function called with null pointer.");
	}

	int mbedRet = MBEDTLS_ERR_SSL_WANT_READ;

	while (mbedRet == MBEDTLS_ERR_SSL_WANT_READ)
	{
		mbedRet = mbedtls_ssl_read(m_sslCtx.get(), reinterpret_cast<uint8_t*>(buf), size);
		if (mbedRet < 0 && mbedRet != MBEDTLS_ERR_SSL_WANT_READ)
		{
			throw MbedTlsException("mbedtls_ssl_read", mbedRet);
		}
	}

	return static_cast<size_t>(mbedRet);
}

void TlsCommLayer::SetConnectionPtr(ConnectionBase& cnt)
{
	mbedtls_ssl_set_bio(m_sslCtx.get(), &cnt, &MbedTlsSslSend, &MbedTlsSslRecv, nullptr);
}

std::shared_ptr<Session> TlsCommLayer::GetSessionCopy() const
{
	std::shared_ptr<Session> res = std::make_shared<Session>();
	CALL_MBEDTLS_C_FUNC(mbedtls_ssl_get_session, m_sslCtx.get(), res->Get());
	return res;
}

std::string TlsCommLayer::GetPeerCertPem() const
{
	const mbedtls_x509_crt* crtPtr = mbedtls_ssl_get_peer_cert(m_sslCtx.get());
	
	if (!IsValid() || !crtPtr)
	{
		throw ConnectionNotEstablished();
	}
	//We just need the non-const pointer, and then we will return the PEM string.
	return X509Cert(*const_cast<mbedtls_x509_crt*>(crtPtr)).GetCurrPem();
}

std::string TlsCommLayer::GetPublicKeyPem() const
{
	const mbedtls_x509_crt* crtPtr = mbedtls_ssl_get_peer_cert(m_sslCtx.get());

	if (!IsValid() || !crtPtr)
	{
		throw ConnectionNotEstablished();
	}
	//We just need the non-const pointer, and then we will return the PEM string.
	
	return AsymKeyBase(const_cast<mbedtls_pk_context&>(crtPtr->pk)).GetPublicPem();
}

bool TlsCommLayer::IsValid() const
{
	return m_sslCtx != nullptr && m_tlsConfig && !m_tlsConfig->IsNull();
}
