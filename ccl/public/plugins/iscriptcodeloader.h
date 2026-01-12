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
// Filename    : ccl/public/plugins/iscriptcodeloader.h
// Description : Script Loader Interface
//
//************************************************************************************************

#ifndef _ccl_iscriptcodeloader_h
#define _ccl_iscriptcodeloader_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IEncryptionKeyProvider;

//************************************************************************************************
// IScriptCodeLoader
/** \ingroup base_plug */
//************************************************************************************************

interface IScriptCodeLoader: IUnknown
{
	/** Set key provider for encrypted scripts. */
	virtual tresult CCL_API setKeyProvider (IEncryptionKeyProvider* keyProvider) = 0;

	DECLARE_IID (IScriptCodeLoader)
	DECLARE_STRINGID_MEMBER (kExtensionID);
};

DEFINE_IID (IScriptCodeLoader, 0x711814c8, 0x3d25, 0x4dd4, 0x97, 0x24, 0xdc, 0x3, 0x5c, 0x33, 0x66, 0xd)
DEFINE_STRINGID_MEMBER (IScriptCodeLoader, kExtensionID, "ScriptCodeLoader");

} // namespace CCL

#endif // _ccl_iscriptcodeloader_h
