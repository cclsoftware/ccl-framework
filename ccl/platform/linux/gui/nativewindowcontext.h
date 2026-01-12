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
// Filename    : ccl/platform/linux/gui/nativewindowcontext.h
// Description : Native Window Context
//
//************************************************************************************************

#ifndef _ccl_linux_nativewindowcontext_h
#define _ccl_linux_nativewindowcontext_h

#include "ccl/platform/linux/linuxplatform.h"

namespace CCL {
struct IWindow;

namespace Linux {

//************************************************************************************************
// NativeWindowContext
//************************************************************************************************

struct NativeWindowContext: PlatformIntegration::NativeWindowHandle
{
	IWindow* parent;
	
	NativeWindowContext (xdg_toplevel* topLevelWindow = nullptr, xdg_popup* popupWindow = nullptr, IWindow* parent = nullptr)
	: NativeWindowHandle (topLevelWindow, popupWindow),
	  parent (parent)
	{}
};

} // namespace Linux
} // namespace CCL
	
#endif // _ccl_linux_nativewindowcontext_h
