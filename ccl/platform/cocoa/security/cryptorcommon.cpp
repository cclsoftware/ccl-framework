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
// Filename    : ccl/platform/cocoa/security/cryptorcommon.cpp
// Description : Cryptography implementation using Apple CommonCryptor library
//
//************************************************************************************************

#include "ccl/platform/cocoa/security/cryptorcommon.h"

using namespace CCL;
using namespace Security;
using namespace Crypto;

//************************************************************************************************
// AESCryptorCommon
//************************************************************************************************

typedef uint8 vuint16 __attribute__((ext_vector_type(16), aligned(4)));

AESCryptorCommon::AESCryptorCommon (Mode mode, BlockRef key) :
 context (nullptr)
 {
 	CCOperation operation = mode == kDecryptMode ? kCCDecrypt : kCCEncrypt;
	CCCryptorStatus status = CCCryptorCreateWithMode (operation, kCCModeECB, kCCAlgorithmAES, ccNoPadding, NULL, key.data, key.length, NULL, 0, 0, 0, &context);
	 
	if(status != kCCSuccess && context)
	{
		CCCryptorRelease (context);
		context = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AESCryptorCommon::~AESCryptorCommon ()
{
	if(context)
		CCCryptorRelease (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AESCryptorCommon::process (BlockRef destination, BlockRef source)
{
	if(!context)
		return kResultFailed;
	
	uint8* sourcePtr = source.data;
	uint8* destinationPtr = destination.data;
	
	ASSERT(source.length == destination.length)
	
	size_t bytesProcessed = 0;
	CCCryptorStatus status = CCCryptorUpdate (context, sourcePtr, source.length, destinationPtr, destination.length, &bytesProcessed);
	if(status != kCCSuccess)
		return kResultFailed;
	
	return kResultOk;
}

//************************************************************************************************
// XORProcessorCommon
//************************************************************************************************

tresult XORProcessorCommon::process (BlockRef destination, BlockRef source)
{
	ASSERT(source.length == destination.length)
	
	uint8* sourcePtr = source.data;
	uint8* destinationPtr = destination.data;

	//check alignment
	if((size_t)sourcePtr % 4 || (size_t)destinationPtr % 4)
	{
		for(int i = 0; i < source.length ; i++)
			destinationPtr[i] ^= sourcePtr[i];
		return kResultOk;
	}

	uint32 blocks = source.length >> 4;
	for(int i = 0; i < blocks; i++)
	{
		*(vuint16*)destinationPtr ^= *(vuint16*)sourcePtr;
		sourcePtr += 16;
		destinationPtr += 16;
	}
	
	for(int i = 0; i < source.length % 16; i++)
		destinationPtr[i] ^= sourcePtr[i];
	
	return kResultOk;
}
