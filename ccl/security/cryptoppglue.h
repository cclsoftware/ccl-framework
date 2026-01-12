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
// Filename    : ccl/security/cryptoppglue.h
// Description : Crypto++ Glue Code
//
//************************************************************************************************

#ifndef _ccl_cryptoppglue_h
#define _ccl_cryptoppglue_h

#include "core/public/coretypes.h"

namespace Core {
namespace IO {
class Stream;
class Buffer;
class MemoryStream; }}

namespace CryptoPP {
class Integer;
class StreamTransformation;
class StreamTransformationFilter;
class SHA1;
class SHA256;
class OID;
class BERSequenceDecoder;
class BERSetDecoder;
class ReusableSink;
class ReusableSource;

namespace Weak1 {
class MD5; }

//************************************************************************************************
// ASN1ContentHandler
//************************************************************************************************

struct ASN1ContentHandler
{
	virtual bool integer (long i) { return true; };
	virtual bool octetString (char* str, int length) { return true; };
	virtual bool oid (const OID& oid) { return true; };
	virtual bool asciiString (char* str, int length) { return true; };
	virtual bool utf8String (char* str, int length) { return true; };
	virtual bool sequence (Core::IO::MemoryStream& s) { return true; };
	virtual bool set (Core::IO::MemoryStream& s) { return true; };
	virtual bool context (unsigned char& defaultTag, bool& implicit, unsigned char contextTag) { return true; };
};

//************************************************************************************************
// IntWrapper
//************************************************************************************************

class IntWrapper
{
public:
	IntWrapper ();
	~IntWrapper ();
	
	bool fromString (Core::CStringPtr string, int base);
	bool toString (Core::StringResult& stringResult, int base) const;
	void add (IntWrapper& result, const IntWrapper& value);
	void substract (IntWrapper& result, const IntWrapper& value);
	void multiply (IntWrapper& result, const IntWrapper& factor);
	void divide (IntWrapper& result, const IntWrapper& divisor);
	void modulo (IntWrapper& result, const IntWrapper& value);
	void expMod (IntWrapper& result, const IntWrapper& exp, const IntWrapper& mod);

protected:
	Integer& integer;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Hash Algorithms
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash>
class HashBase
{
public:
	HashBase ();
	virtual ~HashBase ();

	void update (void* data, Core::uint32 size);
	unsigned int getOptimalBlockSize () const;
	void finish (void* data);

protected:
	Hash* hash;
};

typedef HashBase<Weak1::MD5> MD5Hash;
typedef HashBase<SHA1> SHA1Hash;
typedef HashBase<SHA256> SHA256Hash;

//////////////////////////////////////////////////////////////////////////////////////////////////
// XOR
//////////////////////////////////////////////////////////////////////////////////////////////////

void XOR_transform (void* destination, void* source, Core::uint32 length);

//////////////////////////////////////////////////////////////////////////////////////////////////
// RNG
//////////////////////////////////////////////////////////////////////////////////////////////////

void RNG_generate (void* randomData, Core::uint32 randomDataLength);

//////////////////////////////////////////////////////////////////////////////////////////////////
// (Keyed-Hash) Message Authentication Code
//////////////////////////////////////////////////////////////////////////////////////////////////

bool HMAC_SHA1_sign (Core::IO::Stream& signature, void* key, Core::uint32 keyLength, Core::IO::Stream& data);
bool HMAC_SHA256_sign (Core::IO::Stream& signature, void* key, Core::uint32 keyLength, Core::IO::Stream& data);

//////////////////////////////////////////////////////////////////////////////////////////////////
// (HKDF) Key Function Derivation
//////////////////////////////////////////////////////////////////////////////////////////////////

bool HKDF_deriveKey (Core::IO::Stream& derivedKey, Core::uint32 derivedKeyLen, void* secret, Core::uint32 secretLen, void* salt, Core::uint32 saltLen, void* info, Core::uint32 infoLen);

//////////////////////////////////////////////////////////////////////////////////////////////////
// AES
//////////////////////////////////////////////////////////////////////////////////////////////////

bool AES_CTR_encrypt (Core::IO::Stream& cipherData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& plainData);
bool AES_CBC_encrypt (Core::IO::Stream& cipherData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& plainData);
bool AES_ECB_encrypt (Core::IO::Stream& cipherData, void* key, Core::uint32 keyLength, Core::IO::Stream& plainData);
bool AES_CTR_decrypt (Core::IO::Stream& plainData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& cipherData);
bool AES_CBC_decrypt (Core::IO::Stream& plainData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& cipherData);
bool AES_ECB_decrypt (Core::IO::Stream& plainData, void* key, Core::uint32 keyLength, Core::IO::Stream& cipherData);

class AESStreamer
{
public:
	AESStreamer (Core::uint8* key, int keyLength, bool decrypt = false);
	~AESStreamer ();

	bool process (Core::uint8* dst, Core::uint8* src, int length);

private:
	StreamTransformation* transformation;
	ReusableSink* sink;
	ReusableSource* source;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// RSA
//////////////////////////////////////////////////////////////////////////////////////////////////

bool RSA_generateKeyPair (Core::IO::Stream& privateKey, Core::IO::Stream& publicKey, Core::uint32 keyLength, void* randomData, Core::uint32 randomDataLength);
bool RSA_encrypt (Core::IO::Stream& cipherData, Core::IO::Stream& publicKey, void* randomData, Core::uint32 randomDataLength, Core::IO::Stream& plainData);
bool RSA_decrypt (Core::IO::Stream& plainData, Core::IO::Stream& privateKey, Core::IO::Stream& cipherData);
bool RSA_SHA1_sign (Core::IO::Stream& signature, Core::IO::Stream& privateKey, Core::IO::Stream& data);
bool RSA_SHA256_sign (Core::IO::Stream& signature, Core::IO::Stream& privateKey, Core::IO::Stream& data);
bool RSA_SHA1_verify (Core::IO::Stream& data, Core::IO::Stream& publicKey, Core::IO::Stream& signature);
bool RSA_SHA256_verify (Core::IO::Stream& data, Core::IO::Stream& publicKey, Core::IO::Stream& signature);

//////////////////////////////////////////////////////////////////////////////////////////////////
// ASN.1
//////////////////////////////////////////////////////////////////////////////////////////////////

bool BER_decode (ASN1ContentHandler& handler, Core::IO::Stream& encodedData);
bool DER_encode (Core::IO::Stream& encodedData, int tag, Core::IO::Stream& content);

//////////////////////////////////////////////////////////////////////////////////////////////////
// PKCS #7
//////////////////////////////////////////////////////////////////////////////////////////////////

bool PKCS7_getContent (Core::IO::Stream& plainData, Core::IO::Stream& encodedData, int subID);
bool PKCS7_getData (Core::IO::Stream& plainData, Core::IO::Stream& encodedData);
bool PKCS7_getSignedData (Core::IO::Stream& signedData, Core::IO::Stream& encodedData);
bool PKCS7_getCertificates (Core::IO::Stream& certificates, Core::IO::Stream& encodedData);

} // namespace CryptoPP

#endif // _ccl_cryptoppglue_h
