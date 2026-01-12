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
// Filename    : ccl/security/cryptoppglue.cpp
// Description : Crypto++ Glue Code
//
//************************************************************************************************

#include "cryptoppglue.h"

#include "core/public/corebuffer.h"
#include "core/public/corestream.h"
#include "core/public/corememstream.h"
#include "core/public/corestringbuffer.h"
#include "core/system/coredebug.h"

#if _MSC_VER
  #pragma warning (push,0)
#endif

#include "ccl/public/base/ccldefpush.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "cryptopp/aes.h"
#include "cryptopp/argnames.h"
#include "cryptopp/filters.h"
#include "cryptopp/hmac.h"
#include "cryptopp/md5.h"
#include "cryptopp/modes.h"
#include "cryptopp/osrng.h"
#include "cryptopp/rsa.h"
#include "cryptopp/sha.h"
#include "cryptopp/asn.h"
#include "cryptopp/integer.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "cryptopp/hkdf.h"

#include "ccl/public/base/ccldefpop.h"

#if _MSC_VER
  #pragma warning (pop)
#endif

namespace CryptoPP {
	
//************************************************************************************************
// IntWrapper
//************************************************************************************************

IntWrapper::IntWrapper ()
: integer (*new Integer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IntWrapper::~IntWrapper ()
{
	delete &integer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IntWrapper::fromString (Core::CStringPtr string, int base)
{
	Core::ConstString intString (string);
	int len = intString.length ();
	char* intValue = new char[len + 2];

	intString.copyTo (intValue, len + 1);
	
	switch(base)
	{
	case 8 :
		intValue[len] = 'o';
		break;
	case 10 :
		intValue[len] = '.';
		break;
	case 16 :
		intValue[len] = 'h';
		break;
	}
	
	intValue[len + 1] = '\0';
	integer = Integer (intValue, BIG_ENDIAN_ORDER);
	
	delete[] intValue;

	return integer != Integer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IntWrapper::toString (Core::StringResult& stringResult, int base) const
{
	std::string intValue = IntToString (integer, base);
	Core::ConstString (intValue.c_str ()).copyTo (stringResult.charBuffer, stringResult.charBufferSize);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IntWrapper::add (IntWrapper& result, const IntWrapper& value)
{
	result.integer = integer.Plus (value.integer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IntWrapper::substract (IntWrapper& result, const IntWrapper& value)
{
	result.integer = integer.Minus (value.integer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IntWrapper::multiply (IntWrapper& result, const IntWrapper& factor)
{
	result.integer = integer.Times (factor.integer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IntWrapper::divide (IntWrapper& result, const IntWrapper& divisor)
{
	result.integer = integer.DividedBy (divisor.integer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IntWrapper::modulo (IntWrapper& result, const IntWrapper& value)
{
	result.integer = integer.Modulo (value.integer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IntWrapper::expMod (IntWrapper& result, const IntWrapper& exp, const IntWrapper& mod)
{
	result.integer = a_exp_b_mod_c (integer, exp.integer, mod.integer);
}

//************************************************************************************************
// Error
//************************************************************************************************

class Error: public CryptoPP::Exception
{
public:
	Error (const char* text)
	: Exception (IO_ERROR, text) 
	{}
};

//************************************************************************************************
// StreamSink (copied from FileSink)
//************************************************************************************************

class StreamSink: public Sink,
				  public NotCopyable
{
public:
	StreamSink (Core::IO::Stream& stream);
	~StreamSink ();

	void IsolatedInitialize (const NameValuePairs& parameters) override;
	size_t Put2 (const byte* inString, size_t length, int messageEnd, bool blocking) override;
	bool IsolatedFlush (bool hardFlush, bool blocking) override;

protected:
	Core::IO::Stream& m_stream;
};

//************************************************************************************************
// ReusableSink
//************************************************************************************************

class ReusableSink: public Sink,
					public NotCopyable
{
public:
	ReusableSink ();

	void setBuffer (Core::uint8* data, int length);

	// Sink
	void IsolatedInitialize (const NameValuePairs& parameters) override;
	size_t Put2 (const byte* inString, size_t length, int messageEnd, bool blocking) override;
	bool IsolatedFlush (bool hardFlush, bool blocking) override;

private:
	Core::uint8* data;
	int length;
	int bytesWritten;
};

//************************************************************************************************
// ReusableSource
//************************************************************************************************

class ReusableSource: public CryptoPP::ArraySource
{
public:
	ReusableSource (BufferedTransformation* attachment);

	void pumpBuffer (Core::uint8* data, int length);
};

//************************************************************************************************
// StreamStore (copied from FileStore)
//************************************************************************************************

class StreamStore: public Store,
				   private FilterPutSpaceHelper,
				   public NotCopyable
{
public:
	class Error: public Exception
	{
	public:
		Error (const char* text)
		: Exception(IO_ERROR, text) 
		{}
	};

	StreamStore ();

	lword MaxRetrievable () const override;
	size_t TransferTo2 (BufferedTransformation& target, lword& transferBytes, const std::string& channel = DEFAULT_CHANNEL, bool blocking = true) override;
	size_t CopyRangeTo2 (BufferedTransformation& target, lword& begin, lword end = LWORD_MAX, const std::string& channel = DEFAULT_CHANNEL, bool blocking = true) const override;
	lword Skip (lword skipMax = ULONG_MAX) override;

private:
	void StoreInitialize (const NameValuePairs &parameters) override;
	
	Core::IO::Stream* m_stream;
	size_t m_length;
};

//************************************************************************************************
// StreamSource (copied from FileSource)
//************************************************************************************************

class StreamSource: public SourceTemplate<StreamStore>
{
public:
	typedef StreamStore::Error Error;

	StreamSource (Core::IO::Stream& stream, bool pumpAll = true, BufferedTransformation* attachment = nullptr)
	: SourceTemplate<StreamStore> (attachment) { SourceInitialize (pumpAll, MakeParameters ("corestream", &stream)); }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

static void transformStream (Core::IO::Stream& dst, CryptoPP::StreamTransformation& transformation, Core::IO::Stream& src)
{
	StreamSink* sink = NEW CryptoPP::StreamSink (dst);
	StreamTransformationFilter* filter = NEW CryptoPP::StreamTransformationFilter (transformation, sink);
	StreamSource source (src, true, filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static int peekStream (Core::IO::Stream* stream)
{
	char c = 0;
	int result = stream->readBytes (&c, 1);
	if(result <= 0)
		return EOF;
	stream->setPosition (-1, Core::IO::kSeekCur);
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static Core::int64 getStreamLength (Core::IO::Stream* stream)
{
	Core::int64 oldPos = stream->getPosition ();
	Core::int64 length = stream->setPosition (0, Core::IO::kSeekEnd);
	stream->setPosition (oldPos, Core::IO::kSeekSet);
	return length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool getStreamRange (Core::IO::Buffer& buffer, Core::IO::Stream* stream, Core::int64 start, int count)
{
	if(!buffer.resize (count))
		return false;
	if(stream->setPosition (start, Core::IO::kSeekSet) != start)
		return false;
	if(stream->readBytes (buffer, count) != count)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// (Keyed-Hash) Message Authentication Code
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash> bool calculateHMAC (Core::IO::Stream& signature, unsigned char* key, Core::uint32 keyLength, Core::IO::Stream& data)
{
	CryptoPP::MessageAuthenticationCode* mac = NEW HMAC<Hash> (key, keyLength);
	{
		StreamSink* sink = NEW StreamSink (signature);
		HashFilter* filter = NEW HashFilter (*mac, sink);
		StreamSource source (data, true, filter);
	}
	delete mac;

	return true;
}

} // namespace CryptoPP

using namespace CryptoPP;

//////////////////////////////////////////////////////////////////////////////////////////////////
// XOR
//////////////////////////////////////////////////////////////////////////////////////////////////

void CryptoPP::XOR_transform (void* destination, void* source, Core::uint32 length)
{
	xorbuf (static_cast<unsigned char*> (destination), static_cast<unsigned char*> (source), length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// RNG
//////////////////////////////////////////////////////////////////////////////////////////////////

static CryptoPP::RandomNumberGenerator& GlobalRNG ()
{
	static AutoSeededRandomPool randomPool;
	return randomPool;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CryptoPP::RNG_generate (void* randomData, Core::uint32 randomDataLength)
{
	GlobalRNG ().GenerateBlock (static_cast<unsigned char*> (randomData), randomDataLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// (Keyed-Hash) Message Authentication Code
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::HMAC_SHA1_sign (Core::IO::Stream& signature, void* key, Core::uint32 keyLength, Core::IO::Stream& data)
{
	return calculateHMAC<CryptoPP::SHA1> (signature, static_cast<unsigned char*> (key), keyLength, data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::HMAC_SHA256_sign (Core::IO::Stream& signature, void* key, Core::uint32 keyLength, Core::IO::Stream& data)
{
	return calculateHMAC<SHA256> (signature, static_cast<unsigned char*> (key), keyLength, data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// (HKDF) Key Function Derivation
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::HKDF_deriveKey (Core::IO::Stream& derivedKey, Core::uint32 derivedKeyLen, void* secret, Core::uint32 secretLen, void* salt, Core::uint32 saltLen, void* info, Core::uint32 infoLen)
{
	if(secret == nullptr || salt == nullptr || info == nullptr || derivedKeyLen == 0)
			return false;

	byte* byteSecret = static_cast<byte*> (secret);
	byte* byteSalt = static_cast<byte*> (salt);
	byte* byteInfo = static_cast<byte*> (info);

	std::string derivedKeyString (derivedKeyLen, 0);

	HKDF<SHA256> hkdf;
	hkdf.DeriveKey (reinterpret_cast<byte*> (&derivedKeyString[0]), derivedKeyLen, byteSecret, secretLen, byteSalt, saltLen, byteInfo, infoLen);

	derivedKey.writeBytes (reinterpret_cast<const byte*> (derivedKeyString.data ()),  static_cast<int> (derivedKeyString.size ()));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AES
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::AES_CTR_encrypt (Core::IO::Stream& cipherData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& plainData)
{
	typedef CTR_Mode<CryptoPP::AES>::Encryption AES_Encryption_CTR;
	StreamTransformation* t = NEW AES_Encryption_CTR (static_cast<unsigned char*> (key), keyLength, static_cast<unsigned char*> (iv));
	transformStream (cipherData, *t, plainData);
	delete t;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::AES_CBC_encrypt (Core::IO::Stream& cipherData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& plainData)
{
	typedef CBC_Mode<CryptoPP::AES>::Encryption AES_Encryption_CBC;
	StreamTransformation* t = NEW AES_Encryption_CBC (static_cast<unsigned char*> (key), keyLength, static_cast<unsigned char*> (iv));
	transformStream (cipherData, *t, plainData);
	delete t;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::AES_ECB_encrypt (Core::IO::Stream& cipherData, void* key, Core::uint32 keyLength, Core::IO::Stream& plainData)
{
	typedef ECB_Mode<CryptoPP::AES>::Encryption AES_Encryption_ECB;
	StreamTransformation* t = NEW AES_Encryption_ECB (static_cast<unsigned char*> (key), keyLength);
	transformStream (cipherData, *t, plainData);
	delete t;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::AES_CTR_decrypt (Core::IO::Stream& plainData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& cipherData)
{
	typedef CTR_Mode<CryptoPP::AES>::Decryption AES_Decryption_CTR;
	StreamTransformation* t = NEW AES_Decryption_CTR (static_cast<unsigned char*> (key), keyLength, static_cast<unsigned char*> (iv));
	transformStream (plainData, *t, cipherData);
	delete t;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::AES_CBC_decrypt (Core::IO::Stream& plainData, void* key, Core::uint32 keyLength, void* iv, Core::IO::Stream& cipherData)
{
	typedef CBC_Mode<CryptoPP::AES>::Decryption AES_Decryption_CBC;
	StreamTransformation* t = NEW AES_Decryption_CBC (static_cast<unsigned char*> (key), keyLength, static_cast<unsigned char*> (iv));
	transformStream (plainData, *t, cipherData);
	delete t;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::AES_ECB_decrypt (Core::IO::Stream& plainData, void* key, Core::uint32 keyLength, Core::IO::Stream& cipherData)
{
	typedef ECB_Mode<CryptoPP::AES>::Decryption AES_Decryption_ECB;
	StreamTransformation* t = NEW AES_Decryption_ECB (static_cast<unsigned char*> (key), keyLength);
	transformStream (plainData, *t, cipherData);
	delete t;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// RSA
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::RSA_generateKeyPair (Core::IO::Stream& privateKey, Core::IO::Stream& publicKey, Core::uint32 keyLength, void* randomData, Core::uint32 randomDataLength)
{
	RandomPool randomPool;
	randomPool.IncorporateEntropy (static_cast<unsigned char*> (randomData), randomDataLength);

	RSAES_OAEP_SHA_Decryptor priv (randomPool, keyLength);
	StreamSink privFile (privateKey);
	priv.AccessMaterial ().Save (privFile);
	privFile.MessageEnd ();

	RSAES_OAEP_SHA_Encryptor pub (priv);
	StreamSink pubFile (publicKey);
	pub.AccessMaterial ().Save (pubFile);
	pubFile.MessageEnd ();
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::RSA_encrypt (Core::IO::Stream& cipherData, Core::IO::Stream& publicKey, void* randomData, Core::uint32 randomDataLength, Core::IO::Stream& plainData)
{
	StreamSource pubFile (publicKey);
	RSAES_OAEP_SHA_Encryptor pub (pubFile);

	RandomPool randomPool;
	randomPool.IncorporateEntropy (static_cast<unsigned char*> (randomData), randomDataLength);

	StreamSink* cipherSink = NEW StreamSink (cipherData);
	PK_EncryptorFilter* encryptor = NEW PK_EncryptorFilter (randomPool, pub, cipherSink);

	StreamSource plainSource (plainData, true, encryptor);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::RSA_decrypt (Core::IO::Stream& plainData, Core::IO::Stream& privateKey, Core::IO::Stream& cipherData)
{
	CryptoPP::StreamSource privFile (privateKey);
	CryptoPP::RSAES_OAEP_SHA_Decryptor priv (privFile);

	CryptoPP::StreamSink* plainSink = NEW CryptoPP::StreamSink (plainData);
	CryptoPP::PK_DecryptorFilter* decryptor = NEW CryptoPP::PK_DecryptorFilter (GlobalRNG (), priv, plainSink);

	CryptoPP::StreamSource cipherSource (cipherData, true, decryptor);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::RSA_SHA1_sign (Core::IO::Stream& signature, Core::IO::Stream& privateKey, Core::IO::Stream& data)
{
	typedef CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA1>::Signer Signer_SHA1;
	
	CryptoPP::StreamSource keySource (privateKey);
	Signer_SHA1 signer (keySource);

	CryptoPP::StreamSink* signatureSink = NEW CryptoPP::StreamSink (signature);
	CryptoPP::SignerFilter* signerFilter = NEW CryptoPP::SignerFilter (GlobalRNG (), signer, signatureSink);

	CryptoPP::StreamSource source (data, true, signerFilter);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::RSA_SHA256_sign (Core::IO::Stream& signature, Core::IO::Stream& privateKey, Core::IO::Stream& data)
{
	typedef CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Signer Signer_SHA256;
	
	CryptoPP::StreamSource keySource (privateKey);
	Signer_SHA256 signer (keySource);

	CryptoPP::StreamSink* signatureSink = NEW CryptoPP::StreamSink (signature);
	CryptoPP::SignerFilter* signerFilter = NEW CryptoPP::SignerFilter (GlobalRNG (), signer, signatureSink);

	CryptoPP::StreamSource source (data, true, signerFilter);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::RSA_SHA1_verify (Core::IO::Stream& data, Core::IO::Stream& publicKey, Core::IO::Stream& signature)
{
	typedef CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA1>::Verifier Verifier_SHA1;
	
	CryptoPP::StreamSource keySource (publicKey);
	Verifier_SHA1 verifier (keySource);

	CryptoPP::StreamSource signatureFile (signature);
	if(signatureFile.MaxRetrievable () != verifier.SignatureLength ())
		return false;

	CryptoPP::SecByteBlock signatureBlock (verifier.SignatureLength ());
	signatureFile.Get (signatureBlock, signatureBlock.size ());

	CryptoPP::SignatureVerificationFilter* verifierFilter = NEW CryptoPP::SignatureVerificationFilter (verifier);
	verifierFilter->Put (signatureBlock, verifier.SignatureLength ());
	CryptoPP::StreamSource dataSource (data, true, verifierFilter);

	return verifierFilter->GetLastResult ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::RSA_SHA256_verify (Core::IO::Stream& data, Core::IO::Stream& publicKey, Core::IO::Stream& signature)
{
	typedef CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Verifier Verifier_SHA256;
	
	CryptoPP::StreamSource keySource (publicKey);
	Verifier_SHA256 verifier (keySource);

	CryptoPP::StreamSource signatureFile (signature);
	if(signatureFile.MaxRetrievable () != verifier.SignatureLength ())
		return false;

	CryptoPP::SecByteBlock signatureBlock (verifier.SignatureLength ());
	signatureFile.Get (signatureBlock, signatureBlock.size ());

	CryptoPP::SignatureVerificationFilter* verifierFilter = NEW CryptoPP::SignatureVerificationFilter (verifier);
	verifierFilter->Put (signatureBlock, verifier.SignatureLength ());
	CryptoPP::StreamSource dataSource (data, true, verifierFilter);

	return verifierFilter->GetLastResult ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ASN.1
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::BER_decode (CryptoPP::ASN1ContentHandler& handler, Core::IO::Stream& encodedData)
{
	CryptoPP::StreamSource input (encodedData);
	byte tag = 0;
	input.Peek (tag);
	if(tag == 0)
		return true;
	
	auto skipTL = [&input] ()
	{
		byte tag;
		input.Get (tag);
		size_t length;
		BERLengthDecode (input, length);
	};
	
	// loop over TLVs (tag, length, value)
	bool keepOn = true;
	while(input.AnyRetrievable () && keepOn)
	{
		input.Peek (tag);
		byte maskedTag = tag & 0x3f;
		byte mask = tag ^ maskedTag;
		if(mask & CONTEXT_SPECIFIC)
		{
			byte contextTag = maskedTag & 0x1f;
			byte newTag = 0;
			bool implicit = false;
			keepOn = handler.context (newTag, implicit, contextTag);
			if(!keepOn)
				break;
			if(implicit)
			{
				// implicit context tag: add new TL to V
				byte dummy;
				input.Get (dummy);
				size_t length;
				BERLengthDecode (input, length);
				ASSERT (length > 0)
				Core::IO::MemoryStream rewritten;
				CryptoPP::StreamSink sink (rewritten);
				sink.Put (newTag);
				DERLengthEncode (sink, length);
				lword transferedLength = length;
				input.TransferTo2 (sink, transferedLength);
				keepOn = BER_decode (handler, rewritten);
				if(!keepOn)
					break;
				continue;
			}
			else
			{
				// explicit context tag: skip TL and use next tag
				skipTL ();
			}
			continue;
		}
		
		switch(maskedTag)
		{
		case INTEGER :
		{
			CryptoPP::Integer intValue (input);
			if(intValue.IsConvertableToLong ())
				keepOn = handler.integer (intValue.ConvertToLong ());
			break;
		}
		case IA5_STRING :
		case UTF8_STRING :
		{
			CryptoPP::SecByteBlock stringBlock;
			if(BERDecodePeekLength (input) > 0)
				CryptoPP::BERDecodeTextString (input, stringBlock, tag);
			else
				input.Skip ();
			if(maskedTag == IA5_STRING)
				keepOn = handler.asciiString (reinterpret_cast<char*> (stringBlock.data ()), int (stringBlock.size ()));
			else
				keepOn = handler.utf8String (reinterpret_cast<char*> (stringBlock.data ()), int (stringBlock.size ()));
			break;
		}
		case BIT_STRING :
		{
			CryptoPP::SecByteBlock bitBlock;
			unsigned int unusedBits = 0;
			CryptoPP::BERDecodeBitString (input, bitBlock, unusedBits);
			// not yet implemented in handler
			break;
		}
		case OCTET_STRING | CONSTRUCTED :
			skipTL ();
			break;
		case OCTET_STRING :
		{
			CryptoPP::SecByteBlock octetBlock;
			CryptoPP::BERDecodeOctetString (input, octetBlock);
			keepOn = handler.octetString (reinterpret_cast<char*> (octetBlock.data ()), int (octetBlock.size ()));
			break;
		}
		case OBJECT_IDENTIFIER :
		{
			CryptoPP::OID oid (input);
			keepOn = handler.oid (oid);
			break;
		}
		case SEQUENCE | CONSTRUCTED :
		{
			Core::IO::MemoryStream sequence;
			CryptoPP::StreamSink sink (sequence);
			if(BERDecodePeekLength (input) > 0)
			{
				BERSequenceDecoder decoder (input);
				decoder.TransferAllTo (sink);
			}
			else
			{
				// re-encode if indefinite length
				ByteQueue buffer;
				DERReencode (input, buffer);
				BERSequenceDecoder decoder (buffer);
				decoder.TransferAllTo (sink);
			}
	
			keepOn = handler.sequence (sequence);
			break;
		}
		case SET | CONSTRUCTED :
		{
			Core::IO::MemoryStream set;
			CryptoPP::StreamSink sink (set);
			if(BERDecodePeekLength (input) > 0)
			{
				BERSetDecoder decoder (input);
				decoder.TransferAllTo (sink);
			}
			else
			{
				// re-encode if indefinite length
				ByteQueue buffer;
				DERReencode (input, buffer);
				BERSetDecoder decoder (buffer);
				decoder.TransferAllTo (sink);
			}
				
			keepOn = handler.set (set);
			break;
		}
		default :
			input.Skip ();
		}
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::DER_encode (Core::IO::Stream& encodedData, int tag, Core::IO::Stream& content)
{
	CryptoPP::StreamSource input (content);
	CryptoPP::StreamSink sink (encodedData);
		
	DERGeneralEncoder encoder (sink, CryptoPP::byte (tag));
	input.TransferAllTo (encoder);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// PKCS #7
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::PKCS7_getContent (Core::IO::Stream& plainData, Core::IO::Stream& encodedData, int subID)
{
	//	ContentInfo ::= SEQUENCE {
	//	  contentType ContentType,
	//	  content [0] EXPLICIT ANY DEFINED BY contentType OPTIONAL }
	//	ContentType ::= OBJECT IDENTIFIER
	
	class CryptoHandler: public CryptoPP::ASN1ContentHandler
	{
	public:
		CryptoHandler (Core::IO::Stream& _result, int subID)
		: result (_result),
		  oidMatched (false),
		  currentSubID (0)
		{
			// pkcs-7 OBJECT IDENTIFIER ::= { iso(1) member-body(2) US(840) rsadsi(113549) pkcs(1) 7 }
			dataOID += 1; dataOID += 2; dataOID += 840; dataOID += 113549; dataOID +=1 ; dataOID +=7;
			dataOID += subID;
		}
		
		bool oid (const CryptoPP::OID& oid) override
		{
			currentSubID = oid.GetValues ().back ();
			if(oid == dataOID)
				oidMatched = true;
			return true;
		};
		
		bool octetString (char* str, int length) override
		{
			if(!oidMatched)
				return true;
			result.writeBytes (str, length);
				
			// finished: stop reading data
			return false;
		}
		
		bool sequence (Core::IO::MemoryStream& s) override
		{
			if(!oidMatched)
			{
				BER_decode (*this, s);
				if(oidMatched)
					return false;
				else
					return true;
			}
			
			result.writeBytes (s.getBuffer (), s.getBytesWritten ());
			// finished: stop reading data
			return false;
		}
		
		bool context (unsigned char& defaultTag, bool& implicit, unsigned char contextTag) override
		{
			if(contextTag != 0)
				return false; // this should never happen
			
			if(currentSubID == 1)
				defaultTag = OCTET_STRING; // DATA
			else
				defaultTag = SEQUENCE | CONSTRUCTED; // all other content types
				
			implicit = false;
				
			return true;
		}
		
	private:
		CryptoPP::OID dataOID;
		Core::IO::Stream& result;
		bool oidMatched;
		int currentSubID;
	};
	
	CryptoHandler handler (plainData, subID);
	
	return CryptoPP::BER_decode (handler, encodedData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::PKCS7_getData (Core::IO::Stream& plainData, Core::IO::Stream& encodedData)
{
	// data OBJECT IDENTIFIER ::= { pkcs-7 1 }
	return CryptoPP::PKCS7_getContent (plainData, encodedData, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::PKCS7_getSignedData (Core::IO::Stream& signedData, Core::IO::Stream& encodedData)
{
	// signedData OBJECT IDENTIFIER ::= { pkcs-7 2 }
	return CryptoPP::PKCS7_getContent (signedData, encodedData, 2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CryptoPP::PKCS7_getCertificates (Core::IO::Stream& certificates, Core::IO::Stream& encodedData)
{
	Core::IO::MemoryStream signedData;
	if(PKCS7_getSignedData (signedData, encodedData) == false)
		return false;
	
	//  SignedData ::= SEQUENCE {
	//     version Version,
	//     digestAlgorithms DigestAlgorithmIdentifiers,
	//     contentInfo ContentInfo,
	//     certificates [0] IMPLICIT ExtendedCertificatesAndCertificates OPTIONAL,
	//     crls [1] IMPLICIT CertificateRevocationLists OPTIONAL,
	//     signerInfos SignerInfos }

	//	Version ::= INTEGER
	//	DigestAlgorithmIdentifiers ::= SET OF DigestAlgorithmIdentifier
	//	ExtendedCertificatesAndCertificates ::= SET OF ExtendedCertificateOrCertificate
	//  ExtendedCertificateOrCertificate ::= CHOICE { certificate Certificate, -- X.509 extendedCertificate [0] IMPLICIT ExtendedCertificate }
	
	//  Certificate  ::=  SEQUENCE  {
	//	   tbsCertificate       TBSCertificate,
	//     signatureAlgorithm   AlgorithmIdentifier,
	//	   signatureValue       BIT STRING  }
	//
	
	class CryptoHandler : public CryptoPP::ASN1ContentHandler
	{
	public:
		CryptoHandler (Core::IO::Stream& _result)
		: result (_result),
		  setCount (0)
		{}
		
		bool set (Core::IO::MemoryStream& s) override
		{
			if(++setCount == 2)
			{
				// second set in sequence contains the certificate sequences
				result.writeBytes (s.getBuffer (), s.getBytesWritten ());

				// finished: stop reading data
				return false;
			}
			return true;
		}
		
		bool context (unsigned char& defaultTag, bool& implicit, unsigned char contextTag) override
		{
			if(contextTag == 0)
			{
				defaultTag = SET | CONSTRUCTED;
				implicit = true;
				return true;
			}
			return true;
		}
				
	private:
		Core::IO::Stream& result;
		int setCount;
	};

	CryptoHandler handler (certificates);
	
	return CryptoPP::BER_decode (handler, signedData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Hash Algorithms
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash>
HashBase<Hash>::HashBase ()
: hash (NEW Hash)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash>
HashBase<Hash>::~HashBase ()
{
	delete hash;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash>
void HashBase<Hash>::update (void* data, Core::uint32 size)
{
	hash->Update (static_cast<byte*> (data), size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash>
unsigned int HashBase<Hash>::getOptimalBlockSize () const
{
	return hash->OptimalBlockSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Hash>
void HashBase<Hash>::finish (void* data)
{
	hash->Final (static_cast<byte*> (data));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template class CryptoPP::HashBase<Weak1::MD5>;
template class CryptoPP::HashBase<SHA1>;
template class CryptoPP::HashBase<SHA256>;

//************************************************************************************************
// StreamSink
//************************************************************************************************

StreamSink::StreamSink (Core::IO::Stream& stream)
: m_stream (stream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StreamSink::~StreamSink ()
{
	CORE_PRINT ("StreamSink::~StreamSink\n")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StreamSink::IsolatedInitialize (const NameValuePairs& parameters)
{
	CORE_PRINT ("StreamSink::IsolatedInitialize\n")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t StreamSink::Put2 (const byte* inString, size_t length, int messageEnd, bool blocking)
{
	CORE_PRINTF ("StreamSink::Put2 %d\n", length)

	int numWritten = m_stream.writeBytes (inString, (int)length);
	if(numWritten < (int)length)
		throw Error ("IStream write failed!");
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StreamSink::IsolatedFlush (bool hardFlush, bool blocking)
{
	CORE_PRINT ("StreamSink::IsolatedFlush\n")
	return true;
}

//************************************************************************************************
// ReusableSink
//************************************************************************************************

ReusableSink::ReusableSink ()
: data (nullptr),
  length (0),
  bytesWritten (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ReusableSink::setBuffer (Core::uint8* buffer, int bufferSize)
{
	data = buffer;
	length = bufferSize;
	bytesWritten = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ReusableSink::IsolatedInitialize (const NameValuePairs& parameters)
{
	bytesWritten = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t ReusableSink::Put2 (const byte* inString, size_t size, int messageEnd, bool blocking)
{
	int written = 0;
	if(data && inString)
	{
		written = int(size);
		if(written > length - bytesWritten)
			written = length - bytesWritten;
		::memmove (data + bytesWritten, inString, written);
	}
	bytesWritten += written;
	return size - written;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ReusableSink::IsolatedFlush (bool hardFlush, bool blocking)
{
	return true;
}

//************************************************************************************************
// ReusableSource
//************************************************************************************************

ReusableSource::ReusableSource (BufferedTransformation* attachment)
: ArraySource (attachment)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ReusableSource::pumpBuffer (Core::uint8* data, int length)
{
	SourceInitialize (true, MakeParameters ("InputBuffer", ConstByteArrayParameter (static_cast<const byte*> (data), length)));
}

//************************************************************************************************
// StreamStore
//************************************************************************************************

StreamStore::StreamStore ()
: m_stream (nullptr),
  m_length (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StreamStore::StoreInitialize (const NameValuePairs& parameters)
{
	parameters.GetValue ("corestream", m_stream);
	ASSERT (m_stream != nullptr)

	//------------
	m_stream->setPosition (0, Core::IO::kSeekSet);
	m_length = (size_t)getStreamLength (m_stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

lword StreamStore::MaxRetrievable () const
{
	if(!m_stream)
		return 0;

	Core::int64 current = m_stream->getPosition ();
	Core::int64 end = m_stream->setPosition (0, Core::IO::kSeekEnd);
	m_stream->setPosition (current, Core::IO::kSeekSet);
	return end - current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t StreamStore::TransferTo2 (BufferedTransformation& target, lword& transferBytes, const std::string& channel, bool blocking)
{
	if(!m_stream)
	{
		transferBytes = 0;
		return 0;
	}

	Core::int64 current = m_stream->getPosition ();
	size_t i = UnsignedMin (m_length, (size_t)current);
	size_t len = UnsignedMin (m_length-i, transferBytes);

	Core::IO::Buffer temp;
	if(!getStreamRange (temp, m_stream, i, (int)len))
		throw Error ("IStream read failed!");

	size_t blockedBytes = target.ChannelPutModifiable2 (channel, (byte*)temp.getAddress (), len, 0, blocking);
	if(blockedBytes > 0)
		return blockedBytes;

	transferBytes = len;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t StreamStore::CopyRangeTo2 (BufferedTransformation& target, lword& begin, lword end, const std::string& channel, bool blocking) const
{
	if(!m_stream)
		return 0;

	if(begin == 0 && end == 1 && peekStream (m_stream) == EOF)
		return 0;

	Core::int64 current = m_stream->getPosition ();
	m_stream->setPosition (begin, Core::IO::kSeekCur);
	
	lword copyMax = end - begin;
	size_t blockedBytes = const_cast<StreamStore*> (this)->TransferTo2 (target, copyMax, channel, blocking);
	begin += copyMax;
	if(blockedBytes)
		return blockedBytes;
	
	m_stream->setPosition (current, Core::IO::kSeekSet);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

lword StreamStore::Skip (lword skipMax)
{
	Core::int64 oldPos = m_stream->getPosition ();
	Core::int64 offset = skipMax;
	m_stream->setPosition (offset, Core::IO::kSeekCur);
	return (lword)m_stream->getPosition () - oldPos;
}

//************************************************************************************************
// AESStreamer
//************************************************************************************************

AESStreamer::AESStreamer (Core::uint8* key, int keyLength, bool decrypt)
{
	typedef ECB_Mode<AES>::Encryption AES_Encryption_ECB;
	typedef ECB_Mode<AES>::Decryption AES_Decryption_ECB;

	sink = NEW ReusableSink;
	if(decrypt)
		transformation = NEW AES_Decryption_ECB (static_cast<unsigned char*> (key), keyLength);
	else
		transformation = NEW AES_Encryption_ECB (static_cast<unsigned char*> (key), keyLength);

	StreamTransformationFilter* filter = NEW StreamTransformationFilter (*transformation, sink);
	source = NEW ReusableSource (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AESStreamer::~AESStreamer ()
{
	delete source;
	delete transformation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AESStreamer::process (Core::uint8* dst, Core::uint8* src, int length)
{
	sink->setBuffer (dst, length);
	source->pumpBuffer (src, length);
	return true;
}
