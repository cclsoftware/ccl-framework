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
// Filename    : ccl/platform/linux/linuxplatform.h
// Description : Platform-specific data structures and definitions
//
//************************************************************************************************

#ifndef _ccl_linuxplatform_h
#define _ccl_linuxplatform_h

struct xdg_toplevel;
struct xdg_popup;

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// NativeWindowHandle
//************************************************************************************************

struct NativeWindowHandle
{
	xdg_toplevel* topLevelWindow;
	xdg_popup* popupWindow;
	CStringPtr exportedHandle;
	CStringPtr exportedHandleV1;
	
	NativeWindowHandle (xdg_toplevel* topLevelWindow = nullptr, xdg_popup* popupWindow = nullptr, CStringPtr exportedHandle = nullptr)
	: topLevelWindow (topLevelWindow),
	  popupWindow (popupWindow),
	  exportedHandle (nullptr),
	  exportedHandleV1 (nullptr)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Notification Icons
// See https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-0.4.html
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Notification
{
	const CStringPtr kInformationIcon = "dialog-information";
	const CStringPtr kWarningIcon = "dialog-warning";
	const CStringPtr kErrorIcon = "dialog-error";
	const CStringPtr kExceptionIcon = "dialog-error";
}

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_linuxplatform_h
