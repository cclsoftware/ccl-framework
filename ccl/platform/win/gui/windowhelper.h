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
// Filename    : ccl/platform/win/gui/windowhelper.h
// Description : Win32 window helpers
//
//************************************************************************************************

#ifndef _ccl_win32_windowhelper_h
#define _ccl_win32_windowhelper_h

#include "ccl/public/base/iunknown.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {

class Window;
class SystemEventHandler;
interface IApplication;
	
namespace Win32 {

void* GetPtrFromNativeHandle (HWND hwnd);
Window* GetWindowFromNativeHandle (HWND hwnd);
HWND FindTopLevelWindow (HWND hwnd, bool onlyCCL = false);
HWND CreateMessageWindow (SystemEventHandler* handler);
void SetAlwaysOnTop (HWND hwnd, bool state);
bool ActivateApplication (IApplication* application, bool startupMode, ArgsRef args);
BOOL HandleCopyData (IApplication* application, COPYDATASTRUCT* data);
void EnforceWindowOrder ();

} // namespace Win32
} // namespace CCL

#endif // _ccl_win32_windowhelper_h
