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
// Filename    : ccl/security/cryptofactory.h
// Description : Crypto Factory
//
//************************************************************************************************

#ifndef _ccl_cryptofactory_h
#define _ccl_cryptofactory_h

#include "ccl/base/singleton.h"

#include "ccl/public/system/icryptor.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// CryptoFactory
//************************************************************************************************

class CryptoFactory: public Object,
					 public ICryptoFactory,
					 public StaticSingleton<CryptoFactory>
{
public:
	CryptoFactory ();

	// ICryptoFactory
	ICryptor* CCL_API createCryptor (Mode mode, Algorithm algorithm, BlockRef key, int options = 0) override;
	IProcessor* CCL_API createXORProcessor (int options = 0) override;

	CLASS_INTERFACE (ICryptoFactory, Object)
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptofactory_h
