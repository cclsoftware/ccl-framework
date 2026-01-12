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
// Filename    : ccl/extras/extensions/extensionmanagement.cpp
// Description : Extension Management 
//
//************************************************************************************************

#include "ccl/extras/extensions/extensionmanagement.h"

#include "ccl/base/collections/stringlist.h"
#include "ccl/base/development.h"
#include "ccl/base/security/packagesignature.h"
#include "ccl/base/storage/file.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/securityservices.h"

namespace CCL {
namespace Install {

namespace ExtensionManagement 
{
	Url sharedExtensionLocation;
}

} // namespace Install
} // namespace CCL

using namespace CCL;
using namespace Install;

//************************************************************************************************
// ExtensionManagement
//************************************************************************************************

void ExtensionManagement::setSharedLocation (UrlRef path)
{
	sharedExtensionLocation = path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManagement::getLocation (Url& path, ExtensionTypeID type)
{
	path = Url::kEmpty;
	if(type == ExtensionType::kUser)
	{
		System::GetSystem ().getLocation (path, System::kAppSettingsFolder);
		path.descend (kExtensionFolderName, Url::kFolder);
	}
	else if(type == ExtensionType::kDeveloper)
	{
		// TODO: configure on user system for 3rd party developers!
		GET_DEVELOPMENT_FOLDER_LOCATION (path, "extensions", "deployment")
	}
	else if(type == ExtensionType::kProgram)
	{
		System::GetSystem ().getLocation (path, System::kAppSupportFolder);
		path.descend (kExtensionFolderName, Url::kFolder);
	}
	else if(type == ExtensionType::kShared)
	{
		if(!sharedExtensionLocation.isEmpty ())
			path = sharedExtensionLocation;
		else
		{
			System::GetSystem ().getLocation (path, System::kCompanySettingsFolder);
			path.descend (kExtensionFolderName, Url::kFolder);
		}
	}
	else
	{
		ASSERT (false)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManagement::makePath (Url& path, const ExtensionDescription& e)
{
	ExtensionTypeID type = e.isUsingSharedLocation () ? ExtensionType::kShared : ExtensionType::kUser;
	makePath (path, e.getID (), type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManagement::makePath (Url& path, StringRef id, ExtensionTypeID type)
{
	getLocation (path, type);
	LegalFileName folderName (id);
	path.descend (folderName, Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::lockDirectory (ExtensionTypeID type, StringRef applicationName)
{
	Url path;
	getLocation (path, type);
	return LockFile::lockDirectory (path, applicationName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::unlockDirectory (ExtensionTypeID type)
{
	Url path;
	getLocation (path, type);
	return LockFile::unlockDirectory (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::isDirectoryLocked (ExtensionTypeID type)
{
	StringList nameList;
	getLockingApplicationNames (nameList, type);
	return nameList.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManagement::getLockingApplicationNames (StringList& nameList, ExtensionTypeID type)
{
	Url folder;
	getLocation (folder, type);
	LockFile::getLockingApplicationNames (nameList, folder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::isInsideExtension (UrlRef path)
{
	static const ExtensionTypeID types[] = 
	{
		ExtensionType::kUser, ExtensionType::kProgram, ExtensionType::kDeveloper, ExtensionType::kShared 
	};

	for(int i = 0; i < ARRAY_COUNT (types); i++)
	{
		Url folder;
		getLocation (folder, types[i]);
		if(folder.contains (path))
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::isUserInstalled (const ExtensionDescription& e)
{
	return e.getType () == ExtensionType::kUser || e.getType () == ExtensionType::kShared;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::checkSignature (UrlRef srcPath, IUrlFilter* signatureFilter, IProgressNotify* progress)
{
	Security::Crypto::PackageVerifier verifier;
	verifier.setLoggingEnabled (true);
	verifier.setFromKeyStore (Security::KeyID::kExtensions);
	return verifier.verify (srcPath, signatureFilter, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::installFile (UrlRef srcPath, ExtensionDescription& e, IProgressNotify* progress)
{
	ExtensionTypeID type = e.isUsingSharedLocation () ? ExtensionType::kShared : ExtensionType::kUser;
	if(isDirectoryLocked (type))
		return false;

	// open package
	AutoPtr<IPackageFile> p = System::GetPackageHandler ().openPackage (srcPath);
	if(p == nullptr)
		return false;

	// extract package
	Url dstPath;
	makePath (dstPath, e);
	System::GetFileSystem ().removeFolder (dstPath, INativeFileSystem::kDeleteRecursively);
	int count = p->extractAll (dstPath, true, nullptr, progress);
	p->close ();

	e.setPath (dstPath);

	return count > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagement::uninstall (ExtensionDescription& e)
{
	ExtensionTypeID type = e.isUsingSharedLocation () ? ExtensionType::kShared : ExtensionType::kUser;
	if(isDirectoryLocked (type))
		return false;
	return System::GetFileSystem ().removeFolder (e.getPath (), INativeFileSystem::kDeleteRecursively);
}
