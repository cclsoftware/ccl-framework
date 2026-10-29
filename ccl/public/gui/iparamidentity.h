//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/public/gui/iparamidentity.h
// Description : Parameter Identity Interface
//
//************************************************************************************************

#ifndef _ccl_iparamidentity_h
#define _ccl_iparamidentity_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IParamIdentity
/** Basic interface for parameter identity. */
//************************************************************************************************

interface IParamIdentity: IUnknown
{
	/** Get parameter title. */
	virtual tbool CCL_API getTitle (String& title, int flags = 0) const = 0;

	DECLARE_IID (IParamIdentity)
};

DEFINE_IID (IParamIdentity, 0x6d7cede0, 0x8085, 0x4b9c, 0x8b, 0x59, 0x91, 0x41, 0x7a, 0x39, 0x7c, 0x42)

} // namespace CCL

#endif // _ccl_iparamidentity_h