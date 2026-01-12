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
// Filename    : ccl/platform/win/gui/windowclasses.h
// Description : Window Class Identifiers
//
//************************************************************************************************

#ifndef _ccl_win32_windowclasses_h
#define _ccl_win32_windowclasses_h

#include "ccl/platform/win/cclwindows.h"

#include "ccl/public/cclexports.h"

namespace CCL {
namespace Win32 {

#ifdef CCL_EXPORT_POSTFIX
#define CCL_WINDOW_CLASS_PREFIX LAZYCAT (L, STRINGIFY (LAZYCAT (CCL_EXPORT_POSTFIX, CCL_EXPORT_PREFIX)))
#else
#define CCL_WINDOW_CLASS_PREFIX LAZYCAT (L, STRINGIFY (CCL_EXPORT_PREFIX))
#endif

#define CCL_WINDOW_CLASS(name) LAZYCAT (CCL_WINDOW_CLASS_PREFIX, STRINGIFY (name))

const LPCWSTR kDefaultWindowClass = CCL_WINDOW_CLASS (WindowClass);
const LPCWSTR kShadowWindowClass = CCL_WINDOW_CLASS (ShadowWindowClass);
const LPCWSTR kDialogWindowClass = CCL_WINDOW_CLASS (DialogClass);
const LPCWSTR kShadowDialogClass = CCL_WINDOW_CLASS (ShadowDialogClass);
const LPCWSTR kTransparentWindowClass = CCL_WINDOW_CLASS (TransparentWindowClass);
const LPCWSTR kMessageWindowClass = CCL_WINDOW_CLASS (MessageWindowClass);

} // namespace Win32
} // namespace CCL
	
#endif // _ccl_win32_windowclasses_h
