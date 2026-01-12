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
// Filename    : ccl/app/utilities/shellcommand.h
// Description : Shell Command Helper
//
//************************************************************************************************

#include "ccl/app/utilities/shellcommand.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Shell")
	XSTRING (ShowInExplorer, "Show in Explorer")
	XSTRING (ShowInFinder, "Show in Finder")
	XSTRING (ShowInFileManager, "Show in File Manager")
END_XSTRINGS

//************************************************************************************************
// ShellCommand
//************************************************************************************************

StringRef ShellCommand::getShowFileInSystemTitle ()
{
	#if CCL_PLATFORM_MAC
	StringRef title = XSTR (ShowInFinder);
	#elif CCL_PLATFORM_WINDOWS
	StringRef title = XSTR (ShowInExplorer);
	#else
	StringRef title = XSTR (ShowInFileManager);
	#endif

	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShellCommand::showFileInSystem (UrlRef path, bool checkOnly)
{
	Url resolvedPath;

	if(path.isNativePath ())
		resolvedPath = path;
	else
	{
		if(path.getProtocol () == PackageUrl::Protocol)
		{
			auto resolvePackagePath = [] (UrlRef path)
			{
				Url packagePath;
				AutoPtr<IPackageVolume> volume = System::GetPackageHandler ().openPackageVolume (path.getHostName ());
				if(volume)
					packagePath = volume->getPackage ()->getPath ();
				return packagePath;
			};

			resolvedPath = resolvePackagePath (path);
			if(resolvedPath.getProtocol () == PackageUrl::Protocol) // package in a package?
				resolvedPath = resolvePackagePath (resolvedPath);
		}
		else if(path.getProtocol () == CCLSTR ("local"))
			System::GetSystem ().resolveLocation (resolvedPath, path);
	}

	if(resolvedPath.getPath ().isEmpty ())
		return false;

	if(!checkOnly)
		System::GetSystemShell ().showFile (resolvedPath);
	return true;
}
