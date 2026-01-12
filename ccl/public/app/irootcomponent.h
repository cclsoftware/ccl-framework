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
// Filename    : ccl/public/app/irootcomponent.h
// Description : Root Component Interface
//
//************************************************************************************************

#ifndef _ccl_irootcomponent_h
#define _ccl_irootcomponent_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// IRootComponent
/** Root component interface. 
	\ingroup app_inter */
//************************************************************************************************

interface IRootComponent: IUnknown
{
	/** Component description. */
	struct Description
	{
		String appID;			///< application identifier
		String appTitle;		///< application title
		String appVersion;		///< application version
		String appVendor;		///< application vendor
	};

	/** Get component description. */
	virtual void CCL_API getDescription (Description& description) const = 0;

	DECLARE_IID (IRootComponent)
};

DEFINE_IID (IRootComponent, 0x483f5b4b, 0xeb8e, 0x4e50, 0xb5, 0xe1, 0x29, 0x4b, 0xc2, 0x22, 0x78, 0x2c)

} // namespace CCL

#endif // _ccl_irootcomponent_h
