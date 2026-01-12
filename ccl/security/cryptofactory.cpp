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
// Filename    : ccl/security/cryptofactory.cpp
// Description : Crypto Factory
//
//************************************************************************************************

#include "ccl/security/cryptofactory.h"
#include "ccl/public/securityservices.h"

#if CCL_PLATFORM_IOS || CCL_PLATFORM_MAC
	#define USE_COMMON_CRYPTOR 1
#endif

#if USE_COMMON_CRYPTOR
	#include "ccl/platform/cocoa/security/cryptorcommon.h"
#else
	#include "ccl/security/cryptor.h"	
#endif

using namespace CCL;
using namespace Security;
using namespace Crypto;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ICryptoFactory& CCL_API System::CCL_ISOLATED (GetCryptoFactory) ()
{
	return CryptoFactory::instance ();
}

//************************************************************************************************
// CryptoFactory
//************************************************************************************************

CryptoFactory::CryptoFactory ()
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

ICryptor* CCL_API CryptoFactory::createCryptor (Mode mode, Algorithm algorithm, BlockRef key, int options)
{
	if(algorithm == kAlgorithmAES)
	{
		ASSERT(key.length == kAES_DefaultKeySize)
		#if USE_COMMON_CRYPTOR
		return NEW AESCryptorCommon (mode, key);
		#else
		return NEW AESCryptor (mode, key);
		#endif
	}
	return nullptr;
}
	
////////////////////////////////////////////////////////////////////////////////////////////////////

IProcessor* CCL_API CryptoFactory::createXORProcessor (int options)
{
	#if USE_COMMON_CRYPTOR
	return NEW XORProcessorCommon ();
	#else
	return NEW XORProcessor ();
	#endif
}
