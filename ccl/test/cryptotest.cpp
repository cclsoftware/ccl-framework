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
// Filename    : cryptotest.cpp
// Description : Crypto Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/security/cryptobox.h"
#include "ccl/base/security/signature.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/system/logging.h"

using namespace CCL;
using namespace Security;

//////////////////////////////////////////////////////////////////////////////////////////////////

static const CString kTestString ("This is the crypto test string.");

//////////////////////////////////////////////////////////////////////////////////////////////////

static void printHex (const char* name, const uint8* data, int size)
{
	String string;
	string << name;

	for(int i = 0; i < size; i++)
		string.appendHexValue (data[i], 2);

	Logging::debug (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CryptoSuite, TestRNG)
{
	Crypto::Material randomData (1024);
	CCL_TEST_ASSERT (Crypto::RandomPool::generate (randomData) == true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CryptoSuite, TestMD5)
{
	MemoryStream data;
	data.write (kTestString.str (), kTestString.length ());
	data.rewind ();

	Crypto::Material digest (Crypto::MD5::kDigestSize);
	CCL_TEST_ASSERT (Crypto::MD5::calculate (digest, data) == true);

	Logging::debug ("Digest = %(1)", digest.toHex ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CryptoSuite, TestAES)
{
	// 1) Generate random key and initalization vector
	Crypto::Material key (Crypto::AES::kDefaultKeySize);
	Crypto::Material iv (Crypto::AES::kBlockSize); // initial vector
	CCL_TEST_ASSERT (Crypto::RandomPool::generate (key) == true);
	CCL_TEST_ASSERT (Crypto::RandomPool::generate (iv) == true);

	Logging::debug ("Key = %(1)", key.toHex ());
	Logging::debug ("IV = %(1)", iv.toHex ());

	// 2) Encrypt
	MemoryStream plainData;
	plainData.write (kTestString.str (), kTestString.length ());
	plainData.rewind ();
	MemoryStream cipherData;
	CCL_TEST_ASSERT (Crypto::AES::encrypt (cipherData, key, iv, plainData) == true);

	cipherData.rewind ();

	// 3) Decrypt
	MemoryStream recoveredData;
	CCL_TEST_ASSERT (Crypto::AES::decrypt (recoveredData, key, iv, cipherData) == true);

	// 5) Compare results
	CCL_TEST_ASSERT (recoveredData.getBytesWritten () == plainData.getBytesWritten ());
	int byteCount = ccl_min (recoveredData.getBytesWritten (), plainData.getBytesWritten ());
	CCL_TEST_ASSERT (::memcmp (recoveredData.getMemoryAddress (), plainData.getMemoryAddress (), byteCount) == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CryptoSuite, TestRSA)
{
	// 1) Generate key pair
	Crypto::Material privateKey;
	Crypto::Material publicKey;
	CCL_TEST_ASSERT (Crypto::RSA::generateKeyPair (privateKey, publicKey) == true);

	Logging::debug ("Private Key = %(1)", privateKey.toBase64 ());
	Logging::debug ("Public Key = %(1)", publicKey.toBase64 ());

	// 2) Encrypt
	MemoryStream plainData;
	plainData.write (kTestString.str (), kTestString.length ());
	plainData.rewind ();
	MemoryStream cipherData;
	CCL_TEST_ASSERT (Crypto::RSA::encrypt (cipherData, publicKey, plainData) == true);

	// 3) Decrypt
	MemoryStream recoveredData;
	cipherData.rewind ();
	CCL_TEST_ASSERT (Crypto::RSA::decrypt (recoveredData, privateKey, cipherData) == true);

	// 5) Compare results
	CCL_TEST_ASSERT (recoveredData.getBytesWritten () == plainData.getBytesWritten ());
	int byteCount = ccl_min (recoveredData.getBytesWritten (), plainData.getBytesWritten ());
	CCL_TEST_ASSERT (::memcmp (recoveredData.getMemoryAddress (), plainData.getMemoryAddress (), byteCount) == 0);

	// 6) Create Signature
	plainData.rewind ();
	Crypto::Material signature;
	CCL_TEST_ASSERT (Crypto::RSA::sign (signature, privateKey, plainData) == true);

	Logging::debug ("Signature = %(1)", signature.toBase64 ());

	// 7) Verify Signature
	CCL_TEST_ASSERT (Crypto::RSA::verify (plainData, publicKey, signature) == true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (CryptoSuite, TestSignature)
{
	// 1) Generate key pair
	Crypto::Material privateKey;
	Crypto::Material publicKey;
	CCL_TEST_ASSERT (Crypto::RSA::generateKeyPair (privateKey, publicKey) == true);

	Logging::debug ("Private Key = %(1)", privateKey.toBase64 ());
	Logging::debug ("Public Key = %(1)", publicKey.toBase64 ());

	MemoryStream messageStream;

	{
		// 2) Prepare Message
		Crypto::SignedXmlMessage message;
		IStream& dataStream = message.getData ().asStream ();
		dataStream.write (kTestString.str (), kTestString.length ());
		dataStream.rewind ();

		// 3) Sign Message
		Crypto::Signer signer;
		signer.setPrivateKey (privateKey);
		CCL_TEST_ASSERT (signer.sign (message) == true);

		// 4) Save Message
		CCL_TEST_ASSERT (message.saveToStream (messageStream) == true);
		messageStream.rewind ();
	}

	{
		// 5) Load Message
		Crypto::SignedXmlMessage message2;
		CCL_TEST_ASSERT (message2.loadFromStream (messageStream) == true);

		// 6) Verify Message
		Crypto::Verifier verifier;
		verifier.setPublicKey (publicKey);
		CCL_TEST_ASSERT (verifier.verify (message2) == true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

class BasicEncrypter
{
public:
	BasicEncrypter (uint8 _key[16])
	: byteCounter (0)
	{ ::memcpy (key, _key, 16); }

	void rewind () { byteCounter = 0; }

	void encrypt (uint8* dst, const uint8* src, int size)
	{
		for(int i = 0; i < size; i++, byteCounter++)
		{
			uint8 ctr = (uint8)((byteCounter + 0x1234) % 0xFF);
			uint8 k = key[byteCounter % 16];
			uint8 v = *src++;
			
			v ^= k;
			v ^= ctr;
			
			*dst++ = v;
		}
	}

	void decrypt (uint8* dst, const uint8* src, int size)
	{
		for(int i = 0; i < size; i++, byteCounter++)
		{
			uint8 ctr = (uint8)((byteCounter + 0x1234) % 0xFF);
			uint8 k = key[byteCounter % 16];
			uint8 v = *src++;

			v ^= ctr;
			v ^= k;
			
			*dst++ = v;
		}
	}

protected:
	int64 byteCounter;
	uint8 key[16];
};

CCL_TEST (CryptoSuite, TestBasic)
{
	uint8 randomData[16];
	for(int i = 0; i < 16; i++)
	{
		randomData[i] = 1 + (rand () % 0xFE);
		CCL_TEST_ASSERT (randomData[i] != 0);
	}

	uint8 data[] = 
	{
		0x00, 0xFF, 0xEF, 0xAB, 0xCD, 0x12, 0x01, 0x02, 0x67, 0x00, 0x00, 0x13, 0x65, 0x00, 0x00, 0x00
	};

	int size = sizeof(data);
	
	// test with null
	::memset (data, 0x00, size);

	//randomData[1] = data[1]; // test with key = data

	printHex ("Key = ", randomData, 16);
	printHex ("Data = ", data, size);

	BasicEncrypter encrypter (randomData);

	Buffer buffer;
	buffer.resize (size);
	uint8* output = (uint8*)buffer.getAddress ();
	::memset (output, 0, size);

	encrypter.encrypt (output, data, size);
	printHex ("Encrypted = ", output, size);

	encrypter.rewind ();

	encrypter.decrypt (output, output, size);
	printHex ("Decrypted = ", output, size);

	CCL_TEST_ASSERT (::memcmp (output, data, size) == 0);
}

static uint8 crc8 (uint8 data, uint8 crc)
{
	static const uint8 kCrc8Poly = 0x18;

	for(int i = 0; i < 8; i++)
	{
		if((crc & 0x80) ^ (data & 0x80))
			crc = (uint8)(crc << 1) ^ kCrc8Poly;
		else
			crc = (uint8)(crc << 1);
		data <<= 1;
	}
	return crc;
}

static uint8 crc8 (const uint8 data[], int length)
{
	uint8 crc = 0;
	for(int i = 0; i < length; i++)
		crc = crc8 (data[i], crc);
	return crc;
}

CCL_TEST (CryptoSuite, TestCRC)
{
	uint8 data[] = {23, (uint8)-46, (uint8)-106, (uint8)-2, (uint8)-92, 107, 49, (uint8)-115, 107, 0};
	uint8 check = crc8 (data, 10);
	ASSERT (check == 40)
}
