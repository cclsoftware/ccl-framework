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
// Filename    : ccl/base/security/cipher.cpp
// Description : Cipher
//
//************************************************************************************************

#include "ccl/base/security/cipher.h"

#include "ccl/base/security/cryptobox.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/securityservices.h"

using namespace CCL;
using namespace Security;
using namespace Crypto;

//************************************************************************************************
// Crypto::Cipher
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Cipher, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

Cipher::Cipher (Algorithm algorithm, BlockCipherMode mode)
: algorithm (algorithm),
  mode (mode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cipher::setFromKeyStore (StringID keyName)
{
	tresult result1 = System::GetCryptoKeyStore ().getMaterial (initialVector, keyName, kInitialVector);
	tresult result2 = System::GetCryptoKeyStore ().getMaterial (secretKey, keyName, kSecretKey);
	return result1 == kResultOk && result2 == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cipher::encrypt (IStream& cipherData, IStream& plainData)
{
	switch(algorithm)
	{
	case kAlgorithmAES : return AES::encrypt (cipherData, secretKey, initialVector, plainData, mode);
	}

	CCL_DEBUGGER ("Unknown cipher algorithm!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cipher::decrypt (IStream& plainData, IStream& cipherData)
{
	switch(algorithm)
	{
	case kAlgorithmAES : return AES::decrypt (plainData, secretKey, initialVector, cipherData, mode);
	}

	CCL_DEBUGGER ("Unknown cipher algorithm!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cipher::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	algorithm = (Algorithm)a.getInt ("algorithm");
	mode = (BlockCipherMode)a.getInt ("mode");
	a.get (initialVector, "initialVector");
	a.get (secretKey, "secretKey");

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Cipher::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	a.set ("algorithm", algorithm);
	a.set ("mode", mode);
	a.set ("initialVector", initialVector);
	a.set ("secretKey", secretKey);

	return true;
}
