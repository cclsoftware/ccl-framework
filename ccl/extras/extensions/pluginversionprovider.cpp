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
// Filename    : ccl/extras/extensions/pluginversionprovider.cpp
// Description : Plug-in Version Provider
//
//************************************************************************************************

#include "ccl/extras/extensions/pluginversionprovider.h"
#include "ccl/extras/extensions/extensionmanager.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/app/utilities/pluginclass.h"

using namespace CCL;
using namespace Install;

//************************************************************************************************
// PlugInVersionProvider
//************************************************************************************************

PlugInVersionProvider::PlugInVersionProvider ()
{
	Url modulePath;
	System::GetExecutableLoader ().getMainImage ().getPath (modulePath);
	FileInfo info;
	if(System::GetFileSystem ().getFileInfo (info, modulePath))
		appModifiedTime = info.modifiedTime;
	
	System::GetSystem ().getLocation (plugInsPath, System::kAppPluginsFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInVersionProvider::getVersionString (String& version, const IClassDescription& description) const
{
	Url path;
	if(!PlugIn::getModulePath (path, description)
		|| plugInsPath.contains (path)
		|| path.getProtocol () == ResourceUrl::Protocol)
	{
		version = ExtensionManager::instance ().getAppVersion ();
		return kResultOk;
	}
	
	ExtensionManager& manager = ExtensionManager::instance ();
	if(manager.isInsideExtension (path))
	{
		for(int i = 0; i < manager.getExtensionCount (); i++)
		{
			ExtensionDescription* e = manager.getExtensionDescription (i);
			if(Url (e->getPath ()).contains (path))
			{
				version = e->getVersion ();
				return kResultOk;
			}
		}
	}

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInVersionProvider::getLastModifiedTime (FileTime& lastModified, const IClassDescription& description) const
{
	Url modulePath;
	if(PlugIn::getModulePath (modulePath, description))
	{
		if(System::GetPlugInManager ().getLastModifiedTime (lastModified, modulePath) == kResultOk)
			return kResultOk;
	}
	if(appModifiedTime != DateTime ())
	{
		lastModified = appModifiedTime;
		return kResultOk;
	}
	return kResultFailed;
}
