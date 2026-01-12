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
// Filename    : ccl/platform/cocoa/security/cryptorcommon.h
// Description : Cryptography implementation using Apple CommonCryptor library
//
//************************************************************************************************

#ifndef _ccl_cryptorcommon_h
#define _ccl_cryptorcommon_h

#include "ccl/base/object.h"

#include "ccl/public/system/icryptor.h"

#include <CommonCrypto/CommonCrypto.h>

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// AESCryptorCommon
//************************************************************************************************
	
class AESCryptorCommon: public Object,
						public ICryptor
{
public:	
	AESCryptorCommon (Mode mode, BlockRef key);
	~AESCryptorCommon ();
	
    // ICryptor
	tresult process (BlockRef destination, BlockRef source) override;
		
	CLASS_INTERFACE2 (ICryptor, IProcessor, Object)

protected:
	CCCryptorRef context;
};

//************************************************************************************************
// XORProcessorCommon
//************************************************************************************************

class XORProcessorCommon: public Object,
						  public IProcessor
{
public:
    
    // IProcessor
	tresult process (BlockRef destination, BlockRef source) override;
	CLASS_INTERFACE (IProcessor, Object)
};
	
} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptorcommon_h
