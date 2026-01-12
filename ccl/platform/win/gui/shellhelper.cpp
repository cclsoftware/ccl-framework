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
// Filename    : ccl/platform/win/gui/shellhelper.cpp
// Description : Shell Helper
//
//************************************************************************************************

#include "ccl/platform/win/gui/shellhelper.h"

#include "ccl/base/storage/configuration.h"

#include <propkey.h>

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (ShellHelper, kFrameworkLevelSecond)
{
	ShellHelper::initialize ();
	return true;
}

//************************************************************************************************
// ShellHelper
//************************************************************************************************

void ShellHelper::initialize ()
{
	String appID = getAppUserModelID ();
	if(!appID.isEmpty ())
	{
		HRESULT hr = ::SetCurrentProcessExplicitAppUserModelID (StringChars (appID)); // requires Windows 7 or later!
		ASSERT (SUCCEEDED (hr))
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ShellHelper::getAppUserModelID ()
{
	// Application User Model IDs (AppUserModelIDs)
	// See https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
	// CompanyName.ProductName.SubProduct.VersionInformation

	String appID;
	Configuration::Registry::instance ().getValue (appID, "CCL.Win32", "AppUserModelID");
	return appID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShellLink* ShellHelper::createLink (StringRef path, StringRef arguments, StringRef title)
{
	ASSERT (!path.isEmpty ())

	ComPtr<IShellLink> link;
	HRESULT hr = ::CoCreateInstance (CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, link);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return nullptr;

	hr = link->SetPath (StringChars (path));
	ASSERT (SUCCEEDED (hr))

	if(!arguments.isEmpty ())
	{
		hr = link->SetArguments (StringChars (arguments));
		ASSERT (SUCCEEDED (hr))
	}

	ComPtr<IPropertyStore> ps;
	link->QueryInterface (IID_IPropertyStore, ps);
	ASSERT (ps)

	if(!title.isEmpty () && ps)
	{
		PropVariant var;
		var.fromString (title);
		hr = ps->SetValue (PKEY_Title, var);
		ASSERT (SUCCEEDED (hr))

		hr = ps->Commit ();
		ASSERT (SUCCEEDED (hr))
	}

	return link.detach ();
}

