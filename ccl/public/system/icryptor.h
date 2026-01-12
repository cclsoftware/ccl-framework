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
// Filename    : ccl/public/system/icryptor.h
// Description : Cryptor interface
//
//************************************************************************************************

#ifndef _ccl_icryptor_h
#define _ccl_icryptor_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/system/cryptotypes.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::Mode
//************************************************************************************************

DEFINE_ENUM (Mode)
{
	kDecryptMode,
	kEncryptMode
};

//************************************************************************************************
// IProcessor
//************************************************************************************************

interface IProcessor: IUnknown
{
	virtual tresult CCL_API process (BlockRef destination, BlockRef source) = 0;

	DECLARE_IID (IProcessor)
};

DEFINE_IID (IProcessor, 0x6a617245, 0xc413, 0x4736, 0xa3, 0xc8, 0x28, 0xad, 0xbb, 0x48, 0x83, 0xda)

//************************************************************************************************
// ICryptor - to be extended if needed
//************************************************************************************************

interface ICryptor: IProcessor
{
	DECLARE_IID (ICryptor)
};

DEFINE_IID (ICryptor, 0x3fd866c5, 0x1482, 0x4b04, 0x82, 0x47, 0x37, 0xfc, 0x23, 0x97, 0x8a, 0xf0)

//************************************************************************************************
// ICryptoFactory
//************************************************************************************************

interface ICryptoFactory: IUnknown
{
	virtual ICryptor* CCL_API createCryptor (Mode mode, Algorithm algorithm, BlockRef key, int options = 0) = 0;
	
	virtual IProcessor* CCL_API createXORProcessor (int options = 0) = 0;

	DECLARE_IID (ICryptoFactory)
};

DEFINE_IID (ICryptoFactory, 0x1e4f1630, 0xb23c, 0x4025, 0xb2, 0x50, 0xf6, 0x33, 0x87, 0x90, 0xd2, 0xe2)

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_icryptor_h
