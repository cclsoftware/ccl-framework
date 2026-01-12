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
// Filename    : ccl/gui/windows/nativewindow.h
// Description : Native Window classes
//
//************************************************************************************************

#ifndef _ccl_nativewindow_h
#define _ccl_nativewindow_h

#include "ccl/public/base/platform.h"

//************************************************************************************************
// Windows
//************************************************************************************************

#if CCL_PLATFORM_WINDOWS
#include "ccl/platform/win/gui/window.win.h"
namespace CCL 
{
	typedef Win32Window NativeWindow;
	typedef Win32Dialog NativeDialog;
}
#endif

//************************************************************************************************
// macOS
//************************************************************************************************

#if CCL_PLATFORM_MAC
#include "ccl/platform/cocoa/gui/window.mac.h"
namespace CCL 
{
	typedef OSXWindow NativeWindow;
	typedef OSXDialog NativeDialog;
}
#endif

//************************************************************************************************
// Linux
//************************************************************************************************

#if CCL_PLATFORM_LINUX
#include "ccl/platform/linux/gui/window.linux.h"
namespace CCL 
{
	typedef LinuxWindow NativeWindow;
	typedef LinuxDialog NativeDialog;
}
#endif

//************************************************************************************************
// iOS
//************************************************************************************************

#if CCL_PLATFORM_IOS
#include "ccl/platform/cocoa/gui/window.ios.h"
namespace CCL 
{
	typedef IOSWindow NativeWindow;
	typedef IOSDialog NativeDialog;
}
#endif

//************************************************************************************************
// Android
//************************************************************************************************

#if CCL_PLATFORM_ANDROID
#include "ccl/platform/android/gui/window.android.h"
namespace CCL 
{
	typedef AndroidWindow NativeWindow;
	typedef AndroidDialog NativeDialog;
}
#endif

#endif // _ccl_nativewindow_h
