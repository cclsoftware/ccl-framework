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
// Filename    : ccl/platform/win/gui/systemshell.win.cpp
// Description : Windows System Shell
//
//************************************************************************************************

#include "ccl/gui/system/systemshell.h"
#include "ccl/gui/gui.h"

#include "ccl/base/storage/url.h"
#include "ccl/main/cclargs.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

#include "ccl/platform/win/gui/shellhelper.h"
#include "ccl/platform/win/system/registry.h"

namespace CCL {

//************************************************************************************************
// WindowsSystemShell
//************************************************************************************************

class WindowsSystemShell: public SystemShell
{
public:
	// SystemShell
	tresult openNativeUrl (UrlRef url, int flags) override;
	tresult showNativeFile (UrlRef url) override;
	tresult CCL_API addRecentFile (UrlRef url) override;
	tresult CCL_API setRunAtStartupEnabled (tbool state) override;
	tbool CCL_API isRunAtStartupEnabled () override;
	tbool CCL_API isRunAtStartupHidden (ArgsRef args) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// WindowsSystemShell
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemShell, WindowsSystemShell)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WindowsSystemShell::openNativeUrl (UrlRef url, int flags)
{
	IWindow* parent = System::GetDesktop ().getDialogParentWindow ();
	HWND hwnd = parent ? (HWND)parent->getSystemWindow () : nullptr;
	
	bool adminNeeded = (flags & System::kRequestAdminPrivileges) != 0;
	LPCTSTR verb = adminNeeded ? L"runas" : L"open";
	
	HINSTANCE result = nullptr;
	if(url.isNativePath ())
	{
		NativePath nativePath (url);
		result = ::ShellExecute (hwnd, verb, nativePath, nullptr, nullptr, SW_SHOW);
	}
	else
	{
		String fullUrl;
		url.getUrl (fullUrl, true);
		result = ::ShellExecute (hwnd, verb, StringChars (fullUrl), nullptr, nullptr, SW_SHOW);
	}
	return (int64)result > 32 ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WindowsSystemShell::showNativeFile (UrlRef _url)
{
	Url url (_url);
	bool found = System::GetFileSystem ().fileExists (url);
	if(found == false && url.isFolder ())
	{
		while(!url.isRootPath ())
		{
			url.ascend ();
			if(System::GetFileSystem ().fileExists (url))
			{
				found = true;
				break;
			}
		}
	}

	if(found == true)
	{
		IWindow* parent = System::GetDesktop ().getDialogParentWindow ();
		HWND hwnd = parent ? (HWND)parent->getSystemWindow () : nullptr;

		// explorer.exe /select,"C:\Folder\file.wav"
		String command;
		if(url.isFile ())
			command = CCLSTR ("/select,"); // select the file

		String nativePath;
		url.toDisplayString (nativePath);
		command << "\"" << nativePath << "\"";

		HINSTANCE result = ShellExecute (hwnd, L"open", L"explorer.exe", StringChars (command), nullptr, SW_SHOW);
		return (int64)result > 32 ? kResultOk : kResultFailed;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsSystemShell::addRecentFile (UrlRef url)
{
	ASSERT (url.isNativePath ())

	String appID = Win32::ShellHelper::getAppUserModelID ();
	if(!appID.isEmpty ())
	{
		String title;
		url.getName (title);
		UrlDisplayString pathString (url);

		Win32::ComPtr<IShellLink> link = Win32::ShellHelper::createLink (pathString, nullptr, title);
		ASSERT (link)
		if(link)
		{
			StringChars appIDChars (appID);

			SHARDAPPIDINFOLINK data = {nullptr};
			data.psl = link;
			data.pszAppID = appIDChars;

			::SHAddToRecentDocs (SHARD_APPIDINFOLINK, &data);
			return kResultOk;
		}
	}

	NativePath path (url);
	::SHAddToRecentDocs (SHARD_PATHW, static_cast<uchar*> (path));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#define AUTO_START_REGISTRY_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
static const String kStartupOption ("/startup"); ///< command line argument for auto-start

struct RunAtStartupHelper
{
	String path;
	String title;

	RunAtStartupHelper ()
	{
		Url exePath;
		System::GetExecutableLoader ().getMainImage ().getPath (exePath);
		path = UrlDisplayString (exePath);

		if(IApplication* app = GUI.getApplication ())
			title = app->getApplicationTitle ();
		ASSERT (!title.isEmpty ())
	}

	bool setEnabled (bool state)
	{
		String startupPath (path);
		startupPath << " " << kStartupOption;

		Registry::Accessor accessor (Registry::kKeyCurrentUser, AUTO_START_REGISTRY_PATH);
		return accessor.writeString (state ? startupPath : String::kEmpty, nullptr, title);
	}

	bool isEnabled () const
	{
		String startupPath;
		Registry::Accessor accessor (Registry::kKeyCurrentUser, AUTO_START_REGISTRY_PATH);
		if(!accessor.readString (startupPath, nullptr, title))
			return false;
		return startupPath.contains (path, false);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsSystemShell::setRunAtStartupEnabled (tbool state)
{
	return RunAtStartupHelper ().setEnabled (state != 0) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsSystemShell::isRunAtStartupEnabled ()
{
	return RunAtStartupHelper ().isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsSystemShell::isRunAtStartupHidden (ArgsRef args)
{
	return args.count () >= 2 && args[1] == kStartupOption;
}
