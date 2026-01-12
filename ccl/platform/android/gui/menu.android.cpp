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
// Filename    : ccl/platform/android/gui/menu.android.cpp
// Description : Platform-specific Menu implementation
//
//************************************************************************************************

#include "ccl/gui/popup/menu.h"

#include "ccl/base/storage/configuration.h"

namespace CCL {

//************************************************************************************************
// AndroidPopupMenu
//************************************************************************************************

class AndroidPopupMenu: public PopupMenu
{
public:
	DECLARE_CLASS (AndroidPopupMenu, PopupMenu)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT (AndroidPopupMenu)
{
	Configuration::Registry::instance ().setValue ("GUI.ContextMenu", "UseExtendedMenu", true);
	return true;
}

//************************************************************************************************
// AndroidPopupMenu
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (AndroidPopupMenu, PopupMenu, "Menu")
DEFINE_CLASS_UID (AndroidPopupMenu, 0x1c1ff2c7, 0xeabe, 0x4b0c, 0xab, 0x94, 0xc2, 0x72, 0x8b, 0xfb, 0xc8, 0x12) // ClassID::Menu
