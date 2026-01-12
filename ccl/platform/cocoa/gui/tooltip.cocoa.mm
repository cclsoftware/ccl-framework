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
// Filename    : ccl/platform/cocoa/gui/tooltip.cocoa.mm
// Description : OSX Tooltips
//
//************************************************************************************************

#include "ccl/gui/windows/tooltip.h"

#if CCL_PLATFORM_IOS
#error This file is OSX only. No stub needed for iOS.
#endif

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipFactory::linkTooltipFactory () {} // force linkage of this file

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (TooltipWindow, kFrameworkLevelFirst)
{
	// no native implementation available, use framework window instead
	TooltipPopup::setFactory (&TooltipWindow::getFactory ());
	return true;
}

//************************************************************************************************
// MacTooltip
//************************************************************************************************

class MacTooltip: public TooltipWindow
{
public:
	DECLARE_CLASS (MacTooltip, TooltipWindow)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (MacTooltip, TooltipPopup)

// ClassID::TooltipPopup
DEFINE_CLASS_UID (MacTooltip, 0xA077C193, 0x3A76, 0x4834, 0xB2, 0x34, 0x05, 0x78, 0xF1, 0x13, 0xAA, 0x32)
