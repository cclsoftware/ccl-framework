//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/base/security/cryptobox.cpp
// Description : Cryptographical Toolbox
//
//************************************************************************************************

#include "ccl/base/security/cryptobox.h"

#include "ccl/public/security/icryptoservice.h"

using namespace CCL;
using namespace Security;
using namespace Crypto;

//************************************************************************************************
// Crypto::RandomPool
//************************************************************************************************

bool RandomPool::generate (BlockRef block)
{
	return System::GetCryptoService ().RNG_generate (block) == kResultOk;
}

//************************************************************************************************
// Crypto::MD5
//************************************************************************************************

bool MD5::calculate (BlockRef digest, IStream& data, IProgressNotify* progress)
{
	return System::GetCryptoService ().MD5_calculate (digest, data, progress) == kResultOk;
}

//************************************************************************************************
// Crypto::SHA1
//************************************************************************************************

bool SHA1::calculate (BlockRef digest, IStream& data, IProgressNotify* progress)
{
	return System::GetCryptoService ().SHA1_calculate (digest, data, progress) == kResultOk;
}

//************************************************************************************************
// Crypto::SHA256
//************************************************************************************************

bool SHA256::calculate (BlockRef digest, IStream& data, IProgressNotify* progress)
{
	return System::GetCryptoService ().SHA256_calculate (digest, data, progress) == kResultOk;
}

//************************************************************************************************
// Crypto::HMAC_SHA1
//************************************************************************************************

bool HMAC_SHA1::sign (IStream& signature, BlockRef key, IStream& data)
{
	return System::GetCryptoService ().HMAC_SHA1_sign (signature, key, data) == kResultOk;
}

//************************************************************************************************
// Crypto::HMAC_SHA56
//************************************************************************************************

bool HMAC_SHA256::sign (IStream& signature, BlockRef key, IStream& data)
{
	return System::GetCryptoService ().HMAC_SHA256_sign (signature, key, data) == kResultOk;
}

//************************************************************************************************
// Crypto::HKDF
//************************************************************************************************

bool HKDF::deriveKey (IStream& derivedKey, uint32 derivedKeyLength, BlockRef secret, BlockRef salt, BlockRef info)
{
	return System::GetCryptoService ().HKDF_deriveKey (derivedKey, derivedKeyLength, secret, salt, info) == kResultOk;
}

//************************************************************************************************
// Crypto::AES
//************************************************************************************************

bool AES::encrypt (IStream& cipherData, BlockRef key, BlockRef iv, IStream& plainData, BlockCipherMode mode)
{
	return System::GetCryptoService ().AES_encrypt (cipherData, key, iv, plainData, mode) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AES::decrypt (IStream& plainData, BlockRef key, BlockRef iv, IStream& cipherData, BlockCipherMode mode)
{
	return System::GetCryptoService ().AES_decrypt (plainData, key, iv, cipherData, mode) == kResultOk;
}

//************************************************************************************************
// Crypto::RSA
//************************************************************************************************

bool RSA::generateKeyPair (IStream& privateKey, IStream& publicKey, BlockRef randomData, uint32 keyLength)
{
	return System::GetCryptoService ().RSA_generateKeyPair (privateKey, publicKey, keyLength, randomData) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RSA::encrypt (IStream& cipherData, IStream& publicKey, IStream& plainData, BlockRef randomData)
{
	return System::GetCryptoService ().RSA_encrypt (cipherData, publicKey, randomData, plainData) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RSA::decrypt (IStream& plainData, IStream& privateKey, IStream& cipherData)
{
	return System::GetCryptoService ().RSA_decrypt (plainData, privateKey, cipherData) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RSA::sign (IStream& signature, IStream& privateKey, IStream& data, Hash hash)
{
	return System::GetCryptoService ().RSA_sign (signature, privateKey, data, hash) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RSA::verify (IStream& data, IStream& publicKey, IStream& signature, Hash hash)
{
	return System::GetCryptoService ().RSA_verify (data, publicKey, signature, hash) == kResultOk;
}

//************************************************************************************************
// Crypto::BER
//************************************************************************************************

bool BER::decode (IASN1ContentHandler& reader, IStream& encodedData)
{
	return System::GetCryptoService ().BER_decode (reader, encodedData) == kResultOk;
}

//************************************************************************************************
// Crypto::DER
//************************************************************************************************

bool DER::encode (IStream& encodedData, int tag, IStream& content)
{
	return System::GetCryptoService ().DER_encode (encodedData, tag, content) == kResultOk;
}

//************************************************************************************************
// Crypto::PKCS7
//************************************************************************************************

bool PKCS7::decodeData (IASN1ContentHandler& reader, IStream& encodedData)
{
	return System::GetCryptoService ().PKCS7_decodeData (reader, encodedData) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PKCS7::getCertificates (IStream& certificate, IStream& encodedData)
{
	return System::GetCryptoService ().PKCS7_getCertificates (certificate, encodedData) == kResultOk;
}

//************************************************************************************************
// Crypto::IntegerStatics
//************************************************************************************************

IInteger* IntegerStatics::create ()
{
	return System::GetCryptoService ().Integer_create ();
}
