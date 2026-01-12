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
}

//************************************************************************************************
// ISystemShell
/**	Interface to interact with operating system graphical shell.
	Access singleton via System::GetSystemShell().
	[guiservices.h]	- #include "ccl/public/guiservices.h" 
	\ingroup gui */
//************************************************************************************************

interface ISystemShell: IUnknown
{
	/**	Open URL in this or external application.
		Can be a local document or a website. */
	virtual tresult CCL_API openUrl (UrlRef url, int flags = 0) = 0;

	/**	Show file or folder in system file management application
		(File Explorer on Windows, Finder on macOS, etc.). */
	virtual tresult CCL_API showFile (UrlRef url) = 0;
	
	/** Notifies system that a file has been accessed. */
	virtual tresult CCL_API addRecentFile (UrlRef url) = 0;

	/** Enable auto-start for current application. */
	virtual tresult CCL_API setRunAtStartupEnabled (tbool state) = 0;

	/** Check if auto-start is enabled. */
	virtual tbool CCL_API isRunAtStartupEnabled () = 0;

	/** Check if application should hide window when auto-starting. */
	virtual tbool CCL_API isRunAtStartupHidden (ArgsRef args) = 0;

	/**	Open the mobile platform settings page for current application.
		Not implemented for desktop platforms. */
	virtual tresult CCL_API openApplicationSettings () = 0;

	/**	Start authentication session via default web browser.
		Callback scheme must be registered with current application.
		Operation result is IUrl with callback data. */
	virtual IAsyncOperation* CCL_API startBrowserAuthentication (UrlRef url, StringRef scheme, 
																 IWindow* window = nullptr) = 0;

	DECLARE_IID (ISystemShell)
};

DEFINE_IID (ISystemShell, 0xcdf7dd1d, 0x2184, 0x4ab4, 0xac, 0x39, 0x62, 0x4f, 0x21, 0x6a, 0xb2, 0xc1 )

} // namespace CCL

#endif // _ccl_isystemshell_h
