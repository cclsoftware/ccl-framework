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
// Filename    : ccl/security/cryptor.h
// Description : Cryptography implementation using Crypto++
//
//************************************************************************************************

#ifndef _ccl_cryptor_h
#define _ccl_cryptor_h

#include "ccl/base/object.h"

#include "ccl/public/system/icryptor.h"

namespace CryptoPP {
class AESStreamer; }

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// AESCryptor
//************************************************************************************************
	
class AESCryptor: public Object,
				  public ICryptor
{
public:
	AESCryptor (Mode mode, BlockRef key);
	~AESCryptor ();

	// ICryptor
	tresult CCL_API process (BlockRef destination, BlockRef source) override;

	CLASS_INTERFACE2 (ICryptor, IProcessor, Object)

protected:
	CryptoPP::AESStreamer* streamer;

	Mode mode;
};

//************************************************************************************************
// XORProcessorIPP
//************************************************************************************************

class XORProcessor: public Object,
					public IProcessor
{
public:	
	// IProcessor
	tresult CCL_API process (BlockRef destination, BlockRef source) override;

	CLASS_INTERFACE (IProcessor, Object)
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptor_h
