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
// Filename    : ccl/public/extras/gadgets.h
// Description : Gadget Definitions
//
//************************************************************************************************

#ifndef _ccl_gadgets_h
#define _ccl_gadgets_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IParameter;

//************************************************************************************************
// Gadget Definitions
//************************************************************************************************

#define PLUG_CATEGORY_GADGET CCLSTR ("Gadget")

namespace Meta 
{
	/** Gadget resource identifier. */
	DEFINE_STRINGID (kClassGadgetResource, "Class:GadgetResource")
}

/*
	Gadget Resource Example:

	<Gadget
		themeName="..."
		formName="..."
		iconName="..."
		menuIconName="..."
		/>
*/

//************************************************************************************************
// IGadgetSite
/** Host site passed to IComponent::initialize(). */
//************************************************************************************************

interface IGadgetSite: IUnknown
{
	/** Open gadget window. */
	virtual tresult CCL_API openGadget (IUnknown* gadget) = 0;

	/** Get parameter controlling the gadget window. */
	virtual IParameter* CCL_API getGadgetWindowParam (IUnknown* gadget) = 0;

	DECLARE_IID (IGadgetSite)
};

DEFINE_IID (IGadgetSite, 0x39c9750b, 0x28c4, 0x459e, 0xa9, 0x4b, 0xd3, 0xf3, 0x47, 0xb0, 0xd6, 0xc2)

} // namespace CCL

#endif // _ccl_gadgets_h
