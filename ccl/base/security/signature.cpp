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
// Filename    : ccl/base/security/signature.cpp
// Description : Cryptographical Signature
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/security/signature.h"
#include "ccl/base/security/cryptobox.h"

#include "ccl/base/storage/xmltree.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/securityservices.h"

using namespace CCL;
using namespace Security;
using namespace Crypto;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Note: CCL implementation is similar to W3Cs XML Digital Signature (XMLDSig), but not compatible!
// For further details see: http://en.wikipedia.org/wiki/XML_Signature
//////////////////////////////////////////////////////////////////////////////////////////////////

#define TAG_ROOT		"Signature"
#define TAG_DATA		"SignedData"
#define TAG_SIGNATURE	"SignatureValue"
#define TAG_KEYINFO		"KeyInfo"

//************************************************************************************************
// Crypto::SignedMessage
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SignedMessage, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& SignedMessage::getData ()
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& SignedMessage::getSignature ()
{
	return signature;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& SignedMessage::getKeyInfo ()
{
	return keyInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignedMessage::setDataWithObject (const StorableObject& object)
{
	object.saveToStream (data.asStream ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SignedMessage::getObjectFromData (StorableObject& object)
{
	return object.loadFromStream (data.asStream ());
}

//************************************************************************************************
// Crypto::SignedXmlMessage
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SignedXmlMessage, SignedMessage)

//////////////////////////////////////////////////////////////////////////////////////////////////

SignedXmlMessage::SignedXmlMessage (StringID _rootName)
: rootName (_rootName.isEmpty () ? CSTR (TAG_ROOT) : _rootName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SignedXmlMessage::save (IStream& outStream) const
{
	// 1) Prepare XML
	String rootName (getRootName ());
	XmlNode rootNode (rootName);

	XmlNode* dataNode = NEW XmlNode (TAG_DATA);
	String dataText = data.toBase64 ();
	CCL_PRINT ("SignedXmlMessage data = ")
	CCL_PRINTLN (dataText)
	dataNode->setText (dataText);
	rootNode.addChild (dataNode);

	XmlNode* signatureNode = NEW XmlNode (TAG_SIGNATURE);
	String signatureText = signature.toBase64 ();
	CCL_PRINT ("SignedXmlMessage signature = ")
	CCL_PRINTLN (signatureText)
	signatureNode->setText (signatureText);
	rootNode.addChild (signatureNode);

	if(!keyInfo.isEmpty ())
	{
		XmlNode* keyInfoNode = NEW XmlNode (TAG_KEYINFO);
		String keyInfoText = keyInfo.toBase64 ();
		keyInfoNode->setText (keyInfoText);
		rootNode.addChild (keyInfoNode);
	}

	XmlTreeWriter writer;
	writer.setLineFormat (Text::kLFLineFormat); // use same line endings on all platforms
	writer.setTextEnabled (true);

	// 2) Encrypt (optional)
	if(cipher != nullptr)
	{
		AutoPtr<IStream> xmlStream = NEW MemoryStream;
		if(!writer.writeDocument (*xmlStream, rootNode))
			return false;

		xmlStream->rewind ();
		return cipher->encrypt (outStream, *xmlStream);
	}
	else
		return writer.writeDocument (outStream, rootNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SignedXmlMessage::load (IStream& inStream)
{
	// 1) Decrypt (optional)
	AutoPtr<IStream> xmlStream;
	if(cipher != nullptr)
	{
		MemoryStream* ms = NEW MemoryStream;
		ms->allocateMemory (4096, true);

		xmlStream = ms;
		if(!cipher->decrypt (*xmlStream, inStream))
			return false;
		xmlStream->rewind ();
	}
	else
		xmlStream.share (&inStream);

	// 2) Parse XML
	XmlTreeParser parser;
	parser.setTextEnabled (true);
	parser.setIgnoreWhitespace (true);
	if(!parser.parse (*xmlStream))
		return false;

	XmlNode* rootNode = parser.getRoot ();
	if(rootNode == nullptr)
		return false;
	if(rootNode->getNameCString () != getRootName ())
		return false;

	XmlNode* dataNode = rootNode->findNodeCString (TAG_DATA);
	XmlNode* signatureNode = rootNode->findNodeCString (TAG_SIGNATURE);
	if(dataNode == nullptr || signatureNode == nullptr)
		return false;

	data.fromBase64 (dataNode->getText ());
	signature.fromBase64 (signatureNode->getText ());

	if(XmlNode* keyInfoNode = rootNode->findNodeCString (TAG_KEYINFO))
		keyInfo.fromBase64 (keyInfoNode->getText ());
	return true;
}

//************************************************************************************************
// Crypto::Signer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Signer, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

Signer::Signer (Hash hash)
: hash (hash)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Signer::sign (IStream& signature, IStream& data)
{
	ASSERT (privateKey.isEmpty () == false)
	return RSA::sign (signature, privateKey, data, hash);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Signer::sign (SignedMessage& message)
{
	if(!keyInfo.isEmpty ())
		message.getKeyInfo ().copyFrom (keyInfo);
	return sign (message.getSignature (), message.getData ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Signer::save (IStream& stream) const
{
	return privateKey.copyTo (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Signer::load (IStream& stream)
{
	privateKey.copyFrom (stream);
	return !privateKey.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Signer::load (const Storage& storage)
{
	return storage.getAttributes ().get (privateKey, "privateKey");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Signer::save (const Storage& storage) const
{
	return storage.getAttributes ().set ("privateKey", privateKey, true);
}

//************************************************************************************************
// Crypto::Verifier
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Verifier, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

Verifier::Verifier (Hash hash)
: hash (hash)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Verifier::setFromKeyStore (StringID keyName, bool detectKeyEncryption)
{
	if(detectKeyEncryption) // check if public key is stored encrypted
	{
		Cipher cipher;
		if(cipher.setFromKeyStore (keyName))
		{
			Material encryptedPublicKey;
			if(System::GetCryptoKeyStore ().getMaterial (encryptedPublicKey, keyName, kPublicKey) != kResultOk)
				return false;

			return cipher.decrypt (publicKey, encryptedPublicKey);
		}
	}

	return System::GetCryptoKeyStore ().getMaterial (publicKey, keyName, kPublicKey) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Verifier::verify (SignedMessage& message)
{
	return verify (message.getData (), message.getSignature ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Verifier::verify (IStream& data, IStream& signature)
{
	ASSERT (publicKey.isEmpty () == false)
	return RSA::verify (data, publicKey, signature, hash);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Verifier::save (IStream& stream) const
{
	return publicKey.copyTo (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Verifier::load (IStream& stream)
{
	publicKey.copyFrom (stream);
	return !publicKey.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Verifier::load (const Storage& storage)
{
	return storage.getAttributes ().get (publicKey, "publicKey");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Verifier::save (const Storage& storage) const
{
	return storage.getAttributes ().set ("publicKey", publicKey, true);
}
