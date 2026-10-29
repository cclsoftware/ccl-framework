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
// Filename    : ccl/public/gui/framework/isystemshell.h
// Description : System Shell Interface
//
//************************************************************************************************

#ifndef _ccl_isystemshell_h
#define _ccl_isystemshell_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IWindow;
interface IAsyncOperation;
interface ISystemPermissionProvider;

//************************************************************************************************
// ISystemShell constants
//************************************************************************************************

namespace System
{
	/** Flags used with ISystemShell::openUrl(). */
	enum ShellOpenFlags
	{
		kRequestAdminPrivileges = 1<<0,	///< request for administrator privileges 
		kDoNotOpenExternally	= 1<<1,	///< do not try to open document in external application
		kDeferOpenURL			= 1<<2	///< document should not be opened from current call stack
	};

	/** Permission state used with ISystemPermissionProvider. */
    DEFINE_ENUM (PermissionState)
    {
		kPermissionUnknown,			///< not determinable or not implemented on this platform
		kPermissionGranted,			///< granted, or no runtime permission needed on this platform
		kPermissionDenied,			///< permission denied by user
		kPermissionUndetermined		///< permission not requested yet
	};

	// Common System Permissions
	DEFINE_STRINGID (kPermissionAudioCapture, "PermissionAudioCapture")
}

using System::PermissionState;

//************************************************************************************************
// ISystemShell
/**	Interface to interact with operating system graphical shell.
	Access singleton via System::GetSystemShell().
	[guiservices.h]	- #include "ccl/public/guiservices.h" 
	\ingroup gui */
//************************************************************************************************

interface ISystemShell: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Files and URLs
	//////////////////////////////////////////////////////////////////////////////////////////////

	/**	Open URL in this or external application.
		Can be a local document or a website. */
	virtual tresult CCL_API openUrl (UrlRef url, int flags = 0) = 0;

	/**	Show file or folder in system file management application
		(File Explorer on Windows, Finder on macOS, etc.). */
	virtual tresult CCL_API showFile (UrlRef url) = 0;
	
	/** Notifies system that a file has been accessed. */
	virtual tresult CCL_API addRecentFile (UrlRef url) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Application Startup and Settings
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Enable auto-start for current application. */
	virtual tresult CCL_API setRunAtStartupEnabled (tbool state) = 0;

	/** Check if auto-start is enabled. */
	virtual tbool CCL_API isRunAtStartupEnabled () = 0;

	/** Check if application should hide window when auto-starting. */
	virtual tbool CCL_API isRunAtStartupHidden (ArgsRef args) = 0;

	/**	Open the mobile platform settings page for current application.
		Not implemented for desktop platforms. */
	virtual tresult CCL_API openApplicationSettings () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Browser Authentication
	//////////////////////////////////////////////////////////////////////////////////////////////

	/**	Start authentication session via default web browser.
		Callback scheme must be registered with current application.
		Operation result is IUrl with callback data. */
	virtual IAsyncOperation* CCL_API startBrowserAuthentication (UrlRef url, StringRef scheme, 
																 IWindow* window = nullptr) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Permissions
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Register permission provider (not retained, must unregister later). */
	virtual tresult CCL_API registerPermissionProvider (ISystemPermissionProvider* provider) = 0;

	/** Unregister permission provider. */
	virtual tresult CCL_API unregisterPermissionProvider (ISystemPermissionProvider* provider) = 0;

	/** Get interface to combined permission providers. */
	virtual ISystemPermissionProvider& CCL_API getPermissions () = 0;

	DECLARE_IID (ISystemShell)
};

DEFINE_IID (ISystemShell, 0xcdf7dd1d, 0x2184, 0x4ab4, 0xac, 0x39, 0x62, 0x4f, 0x21, 0x6a, 0xb2, 0xc1)

//************************************************************************************************
// ISystemPermissionProvider
/**	Interface to handle certain system permissions.
	\ingroup gui */
//************************************************************************************************

interface ISystemPermissionProvider: IUnknown
{
	/** Get current state of a permission (does not show any UI). */
	virtual PermissionState CCL_API getPermissionState (StringID permissionId) = 0;

	/**	Request a permission, result is the final state (kPermissionGranted/kPermissionDenied).
		Completes immediately if no user consent is needed. 
		
		When implementing, providers must return nullptr for ids they don't handle. */
	virtual IAsyncOperation* CCL_API requestPermission (StringID permissionId) = 0;

	DECLARE_IID (ISystemPermissionProvider)
};

DEFINE_IID (ISystemPermissionProvider, 0x45196fa4, 0xa5a, 0x4162, 0x82, 0xcd, 0x5b, 0x34, 0x7, 0x2, 0xb1, 0xd0)

} // namespace CCL

#endif // _ccl_isystemshell_h
