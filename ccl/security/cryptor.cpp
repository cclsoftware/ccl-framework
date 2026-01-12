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
// Filename    : ccl/security/cryptor.cpp
// Description : Cryptography implementation using Crypto++
//
//************************************************************************************************

#include "ccl/security/cryptor.h"
#include "ccl/security/cryptoppglue.h"

using namespace CCL;
using namespace Security;
using namespace Crypto;

//************************************************************************************************
// AESCryptor
//************************************************************************************************

AESCryptor::AESCryptor (Mode _mode, BlockRef key) 
: mode (_mode)
{
	streamer = NEW CryptoPP::AESStreamer (key.data, key.length, mode == kDecryptMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AESCryptor::~AESCryptor ()
{
	delete streamer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AESCryptor::process (BlockRef destination, BlockRef source)
{
	ASSERT (source.length == destination.length)
	bool succeeded = streamer->process (destination.data, source.data, source.length);
	return succeeded ? kResultOk : kResultFailed;
}

//************************************************************************************************
// XORProcessor
//************************************************************************************************

tresult XORProcessor::process (BlockRef destination, BlockRef source)
{
	ASSERT (source.length == destination.length)
	CryptoPP::XOR_transform (destination.data, source.data, destination.length);
	return kResultOk;
}
