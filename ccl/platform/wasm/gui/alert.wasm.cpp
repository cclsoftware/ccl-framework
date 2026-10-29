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
// Filename    : ccl/platform/wasm/gui/alert.wasm.cpp
// Description : platform alert dialog
//
//************************************************************************************************

#include "ccl/platform/shared/host/frameworkalert.h"

namespace CCL {

//************************************************************************************************
// WasmAlertBox
//************************************************************************************************

class WasmAlertBox: public FrameworkAlertBox
{
public:
	DECLARE_CLASS (WasmAlertBox, WasmAlertBox)
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Alert::ButtonMapping
//************************************************************************************************

int Alert::ButtonMapping::getResultAtButtonIndex (int buttonIndex) const
{
	switch(buttonIndex)
	{
	case 0 : return defaultResult;
	case 1 : return alternateResult;
	case 2 : return otherResult;
	}
	return kUndefined;
}

//************************************************************************************************
// WasmAlertBox
//************************************************************************************************

DEFINE_CLASS (WasmAlertBox, FrameworkAlertBox)
DEFINE_CLASS_UID (WasmAlertBox, 0x9bf3ecb5, 0x5bb2, 0x4eb4, 0xaa, 0xac, 0x29, 0xaf, 0xf4, 0x66, 0x45, 0xa5) // ClassID::AlertBox
