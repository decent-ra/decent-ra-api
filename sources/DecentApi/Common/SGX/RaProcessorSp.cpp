#include "RaProcessorSp.h"

#include <cstdint>
#include <cstring>

#include <vector>
#include <map>

#include <sgx_key_exchange.h>

#include <cppcodec/base64_default_rfc4648.hpp>
#include <mbedtls/md.h>

#include "../Common.h"
#include "../make_unique.h"
#include "../consttime_memequal.h"

#include "../Tools/Crypto.h"

#include "../MbedTls/Kdf.h"
#include "../MbedTls/Drbg.h"
#include "../MbedTls/EcKey.h"
#include "../MbedTls/Hasher.h"
#include "../MbedTls/X509Cert.h"

#include "../SGX/IasReportCert.h"

#include "sgx_structs.h"
#include "IasReport.h"
#include "IasConnector.h"
#include "SgxCryptoConversions.h"
#include "RuntimeError.h"

using namespace Decent;
using namespace Decent::Sgx;
using namespace Decent::Ias;
using namespace Decent::Tools;
using namespace Decent::MbedTlsObj;

namespace
{
	static std::string ConstructNonce(size_t size)
	{
		size_t dataSize = (size / 4) * 3;
		std::vector<uint8_t> randData(dataSize);

#ifndef SIMULATING_ENCLAVE
		Drbg drbg;
		drbg.RandContainer(randData);
#else
		std::fill_n(randData.begin(), randData.size(), 0);
#endif // SIMULATING_ENCLAVE

		return cppcodec::base64_rfc4648::encode(randData);
	}

}

const RaProcessorSp::SgxReportDataVerifier RaProcessorSp::sk_defaultRpDataVrfy([](const sgx_report_data_t& initData, const sgx_report_data_t& expected) -> bool
{
#ifndef SIMULATING_ENCLAVE
	return consttime_memequal(&initData, &expected, sizeof(sgx_report_data_t)) == 1;
#else
	return true;
#endif // SIMULATING_ENCLAVE
});

RaProcessorSp::RaProcessorSp(const void* const iasConnectorPtr, std::shared_ptr<const MbedTlsObj::EcKeyPair<EcKeyType::SECP256R1> > mySignKey, std::shared_ptr<const sgx_spid_t> spidPtr,
	SgxReportDataVerifier rpDataVrfy, SgxQuoteVerifier quoteVrfy, const sgx_ra_config& raConfig) :
	m_raConfig(raConfig),
	m_spid(spidPtr),
	m_iasConnectorPtr(iasConnectorPtr),
	m_mySignKey(mySignKey),
	m_encrKeyPair(),
	m_myEncrKey(),
	m_peerEncrKey(),
	m_nonce(),
	m_smk(),
	m_mk(),
	m_sk(),
	m_vk(),
	m_rpDataVrfy(rpDataVrfy),
	m_quoteVrfy(quoteVrfy),
	m_iasReport(),
	m_isAttested(false),
	m_iasReportStr(),
	m_reportCert(),
	m_reportSign()
{
}

RaProcessorSp::~RaProcessorSp()
{
}

RaProcessorSp::RaProcessorSp(RaProcessorSp && other) :
	m_raConfig(std::move(other.m_raConfig)),
	m_spid(std::move(other.m_spid)),
	m_iasConnectorPtr(other.m_iasConnectorPtr),
	m_mySignKey(std::move(other.m_mySignKey)),
	m_encrKeyPair(std::move(other.m_encrKeyPair)),
	m_myEncrKey(std::move(other.m_myEncrKey)),
	m_peerEncrKey(std::move(other.m_peerEncrKey)),
	m_nonce(std::move(other.m_nonce)),
	m_smk(std::move(other.m_smk)),
	m_mk(std::move(other.m_mk)),
	m_sk(std::move(other.m_sk)),
	m_vk(std::move(other.m_vk)),
	m_rpDataVrfy(std::move(other.m_rpDataVrfy)),
	m_quoteVrfy(std::move(other.m_quoteVrfy)),
	m_iasReport(std::move(other.m_iasReport)),
	m_isAttested(other.m_isAttested),
	m_iasReportStr(std::move(other.m_iasReportStr)),
	m_reportCert(std::move(other.m_reportCert)),
	m_reportSign(std::move(other.m_reportSign))
{
	other.m_iasConnectorPtr = nullptr;
	other.m_isAttested = false;
}

void RaProcessorSp::Init()
{
	m_encrKeyPair = Tools::make_unique<MbedTlsObj::EcKeyPair<EcKeyType::SECP256R1> >(Tools::make_unique<Drbg>());

	m_nonce = ConstructNonce(IAS_REQUEST_NONCE_SIZE);

	m_iasReport = Tools::make_unique<sgx_ias_report_t>();

	if (!m_encrKeyPair || !m_mySignKey)
	{
		throw RuntimeException("Failed to init RaProcessorSp, because keys are invalid.");
	}

	m_encrKeyPair->ToPublicBinary(m_myEncrKey.x, m_myEncrKey.y);

	if (!Ias::CheckRaConfigValidaty(m_raConfig))
	{
		throw RuntimeException("RA config given to RaProcessorSp::Init is invalid.");
	}

	if (!CheckKeyDerivationFuncId(m_raConfig.ckdf_id))
	{
		throw RuntimeException("Key derivation function ID in RA config given to RaProcessorSp::Init is invalid.");
	}
}

void RaProcessorSp::ProcessMsg0(const sgx_ra_msg0s_t & msg0s, sgx_ra_msg0r_t & msg0r)
{
	if (!CheckExGrpId(msg0s.extended_grp_id))
	{
		throw RuntimeException("RA extension group ID given by the RA responder is not supported.");
	}

	GetMsg0r(msg0r);
}

void RaProcessorSp::ProcessMsg1(const sgx_ra_msg1_t & msg1, std::vector<uint8_t>& msg2)
{
	SetPeerEncrPubKey(SgxEc256Type2General(msg1.g_a));

	msg2.resize(sizeof(sgx_ra_msg2_t));
	sgx_ra_msg2_t& msg2Ref = *reinterpret_cast<sgx_ra_msg2_t*>(msg2.data());

	memcpy(&(msg2Ref.g_b), &m_myEncrKey, sizeof(sgx_ec256_public_t));
	memcpy(&(msg2Ref.spid), m_spid.get(), sizeof(sgx_spid_t));
	msg2Ref.quote_type = m_raConfig.linkable_sign;
	msg2Ref.kdf_id = m_raConfig.ckdf_id;

	General256Hash hashToBeSigned;
	Hasher<HashType::SHA256>().Batched(hashToBeSigned,
		std::array<DataListItem, 1>{{&m_myEncrKey, 2 * sizeof(sgx_ec256_public_t)}});

	Drbg drbg;
	m_mySignKey->Sign<HashType::SHA256>(hashToBeSigned, msg2Ref.sign_gb_ga.x, msg2Ref.sign_gb_ga.y, drbg);

	uint32_t cmac_size = offsetof(sgx_ra_msg2_t, mac);

	CMACer<CipherType::AES, GENERAL_128BIT_16BYTE_SIZE, CipherMode::ECB>(m_smk).Batched(msg2Ref.mac, std::array<DataListItem, 1>{ {&(msg2Ref.g_b), cmac_size}});

	std::string revcList;
	if (!StatConnector::GetRevocationList(m_iasConnectorPtr, msg1.gid, revcList))
	{
		throw RuntimeException("RaProcessorSp::ProcessMsg1 failed to get revocation list.");
	}

	std::vector<uint8_t> revcListBin = cppcodec::base64_rfc4648::decode(revcList);

	msg2Ref.sig_rl_size = static_cast<uint32_t>(revcListBin.size());
	msg2.insert(msg2.end(), revcListBin.begin(), revcListBin.end());
}

void RaProcessorSp::ProcessMsg3(const sgx_ra_msg3_t & msg3, size_t msg3Len, std::vector<uint8_t> & msg4Pack,
	sgx_report_data_t * outOriRD)
{
	if (!consttime_memequal(&(m_peerEncrKey), &msg3.g_a, sizeof(sgx_ec256_public_t)))
	{
		throw RuntimeException("RaProcessorSp::ProcessMsg3 failed to verify MSG 3.");
	}
	
	//Make sure that msg3_size is bigger than sgx_mac_t.
	uint32_t mac_size = static_cast<uint32_t>(msg3Len - sizeof(sgx_mac_t));
	const uint8_t *p_msg3_cmaced = reinterpret_cast<const uint8_t*>(&msg3) + sizeof(sgx_mac_t);

	std::array<uint8_t, GENERAL_128BIT_16BYTE_SIZE> calcMac;
	CMACer<CipherType::AES, GENERAL_128BIT_16BYTE_SIZE, CipherMode::ECB>(m_smk).Batched(calcMac, std::array<DataListItem, 1>{ {p_msg3_cmaced, mac_size}});
	if (!consttime_memequal(calcMac.data(), msg3.mac, sizeof(msg3.mac)))
	{
		throw RuntimeException("RaProcessorSp::ProcessMsg3 failed to verify MSG 3.");
	}

	//std::memcpy(&spCTX.m_secProp, &msg3.ps_sec_prop, sizeof(sgx_ps_sec_prop_desc_t));

	//const sgx_quote_t* p_quote = reinterpret_cast<const sgx_quote_t *>(msg3->quote);

	sgx_report_data_t report_data = { 0 };

	// Verify the report_data in the Quote matches the expected value.
	// The first 32 bytes of report_data are SHA256 HASH of {ga|gb|vk}.
	// The second 32 bytes of report_data are set to zero.
	General256Hash reportDataHash;
	Hasher<HashType::SHA256>().Batched(reportDataHash,
		std::array<DataListItem, 3>
		{
			DataListItem{&(m_peerEncrKey), sizeof(sgx_ec256_public_t)},
			DataListItem{&(m_myEncrKey), sizeof(sgx_ec256_public_t)},
			DataListItem{&(m_vk), sizeof(sgx_ec_key_128bit_t)},
		});

	std::copy(reportDataHash.begin(), reportDataHash.end(), report_data.d);

	if (outOriRD)
	{
		std::memcpy(outOriRD, &report_data, sizeof(sgx_report_data_t));
	}

	const sgx_quote_t& quoteInMsg3 = reinterpret_cast<const sgx_quote_t&>(msg3.quote);

	do
	{
		if (!StatConnector::GetQuoteReport(m_iasConnectorPtr, msg3, msg3Len, m_nonce, m_raConfig.enable_pse,
			m_iasReportStr, m_reportSign, m_reportCert))
		{
			break;
		}

		X509Cert trustedIasCert(Ias::gsk_IasReportCert);
		X509Cert reportCertChain(m_reportCert);

		reportCertChain.ShrinkChain(trustedIasCert);

		m_reportCert = reportCertChain.GetPemChain();

		if (!CheckIasReport(*m_iasReport, m_iasReportStr, m_reportCert, m_reportSign, report_data) ||
#ifndef SIMULATING_ENCLAVE
			!consttime_memequal(&quoteInMsg3, &m_iasReport->m_quote, sizeof(sgx_quote_t) - sizeof(sgx_quote_t::signature_len))
#else
			false
#endif // !SIMULATING_ENCLAVE
			)
		{
			break;
		}

		if (m_raConfig.enable_pse)
		{
			General256Hash pseHash;
			Hasher<HashType::SHA256>().Calc(pseHash, msg3.ps_sec_prop.sgx_ps_sec_prop_desc);

			if (!consttime_memequal(pseHash.data(), &m_iasReport->m_pse_hash, pseHash.size()))
			{
				break;
			}
		}

		m_isAttested = true;
	} while (false);

	sgx_ra_msg4_t msg4;
	std::memcpy(&msg4.report, m_iasReport.get(), sizeof(*m_iasReport));
	msg4.is_accepted = m_isAttested ? 1 : 0;

	std::vector<uint8_t> msg4Bin(sizeof(msg4));
	std::memcpy(msg4Bin.data(), &msg4, msg4Bin.size());

	msg4Pack = QuickAesGcmPack(GetSK().m_key, std::array<uint8_t, 0>(), std::array<uint8_t, 0>(), msg4Bin, GetMK().m_key, nullptr, 1024);

	if (!m_isAttested)
	{
		throw RuntimeException("RaProcessorSp::ProcessMsg3 rejects the quote; SGX RA failed.");
	}
}

const sgx_ra_config & RaProcessorSp::GetRaConfig() const
{
	return m_raConfig;
}

bool RaProcessorSp::IsAttested() const
{
	return m_isAttested;
}

std::unique_ptr<sgx_ias_report_t> RaProcessorSp::ReleaseIasReport()
{
	return std::move(m_iasReport);
}

const G128BitSecretKeyWrap & RaProcessorSp::GetSK() const
{
	return m_sk;
}

const G128BitSecretKeyWrap & RaProcessorSp::GetMK() const
{
	return m_mk;
}

void RaProcessorSp::GetMsg0r(sgx_ra_msg0r_t & msg0r)
{
	msg0r.ra_config = m_raConfig;

	m_mySignKey->ToPublicBinary(msg0r.sp_pub_key.gx, msg0r.sp_pub_key.gy);
}

const std::string & RaProcessorSp::GetIasReportStr() const
{
	return m_iasReportStr;
}

const std::string & RaProcessorSp::GetIasReportCert() const
{
	return m_reportCert;
}

const std::string & RaProcessorSp::GetIasReportSign() const
{
	return m_reportSign;
}

bool RaProcessorSp::CheckExGrpId(const uint32_t id) const
{
	return id == 0;
}

bool RaProcessorSp::CheckKeyDerivationFuncId(const uint16_t id) const
{
	return id == SGX_DEFAULT_AES_CMAC_KDF_ID;
}

void RaProcessorSp::SetPeerEncrPubKey(const general_secp256r1_public_t & inEncrPubKey)
{
	m_peerEncrKey = inEncrPubKey;

	MbedTlsObj::EcPublicKey<EcKeyType::SECP256R1> peerEncrKey(inEncrPubKey.x, inEncrPubKey.y);

	G256BitSecretKeyWrap sharedKey;
	m_encrKeyPair->DeriveSharedKey(sharedKey.m_key, peerEncrKey, Tools::make_unique<MbedTlsObj::Drbg>());

	CKDF<CipherType::AES, GENERAL_128BIT_16BYTE_SIZE, CipherMode::ECB>(sharedKey, "SMK", m_smk);
	CKDF<CipherType::AES, GENERAL_128BIT_16BYTE_SIZE, CipherMode::ECB>(sharedKey, "MK", m_mk);
	CKDF<CipherType::AES, GENERAL_128BIT_16BYTE_SIZE, CipherMode::ECB>(sharedKey, "SK", m_sk);
	CKDF<CipherType::AES, GENERAL_128BIT_16BYTE_SIZE, CipherMode::ECB>(sharedKey, "VK", m_vk);
}

bool RaProcessorSp::CheckIasReport(sgx_ias_report_t & outIasReport,
	const std::string & iasReportStr, const std::string & reportCert, const std::string & reportSign, 
	const sgx_report_data_t & oriRD) const
{
	if (!m_rpDataVrfy || !m_quoteVrfy)
	{
		return false;
	}

	auto quoteVerifier = [this, oriRD](const sgx_ias_report_t & iasReport) -> bool
	{
		return m_rpDataVrfy(oriRD, iasReport.m_quote.report_body.report_data) &&
			m_quoteVrfy(iasReport.m_quote);
	};

	return ParseAndVerifyIasReport(outIasReport, iasReportStr, reportCert, reportSign, m_nonce.c_str(), m_raConfig, quoteVerifier);
}
