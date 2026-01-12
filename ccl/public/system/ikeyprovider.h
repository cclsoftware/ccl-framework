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
// Filename    : ccl/public/system/ikeyprovider.h
// Description : Encryption Key Provider Interface
//
//************************************************************************************************

#ifndef _ccl_ikeyprovider_h
#define _ccl_ikeyprovider_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IEncryptionKeyProvider
/**	\ingroup ccl_system */
//************************************************************************************************

interface IEncryptionKeyProvider: IUnknown
{
	virtual tresult CCL_API getEncryptionKey (String& key, StringRef keyId) = 0;

	DECLARE_IID (IEncryptionKeyProvider)
};

DEFINE_IID (IEncryptionKeyProvider, 0x25be5095, 0x6d42, 0x408a, 0xa5, 0xfd, 0xbd, 0xc6, 0xdc, 0x5c, 0xa5, 0xe4)

} // namespace CCL

#endif // _ccl_ikeyprovider_h
