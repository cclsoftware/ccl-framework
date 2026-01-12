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
// Filename    : ccl/extras/extensions/extensionmanagement.h
// Description : Extension Management
//
//************************************************************************************************

#ifndef _ccl_extensionmanagement_h
#define _ccl_extensionmanagement_h

#include "ccl/extras/extensions/extensiondescription.h"

namespace CCL {
class StringList;

namespace Install {

//************************************************************************************************
// ExtensionManagement
//************************************************************************************************

namespace ExtensionManagement
{
	static const String kExtensionFolderName ("Extensions");

	void setSharedLocation (UrlRef path);

	void getLocation (Url& path, ExtensionTypeID type);
	void makePath (Url& path, const ExtensionDescription& e);
	void makePath (Url& path, StringRef id, ExtensionTypeID type);
	
	bool lockDirectory (ExtensionTypeID type, StringRef applicationName = nullptr);
	bool unlockDirectory (ExtensionTypeID type);
	bool isDirectoryLocked (ExtensionTypeID type);
	void getLockingApplicationNames (StringList& applicationNames, ExtensionTypeID type);

	bool isInsideExtension (UrlRef path); ///< check if file is inside an extension
	bool isUserInstalled (const ExtensionDescription& e);

	bool checkSignature (UrlRef srcPath, IUrlFilter* signatureFilter = nullptr, IProgressNotify* progress = nullptr);

	bool installFile (UrlRef srcPath, ExtensionDescription& e, IProgressNotify* progress = nullptr);
	bool uninstall (ExtensionDescription& e);
}

} // namespace Install
} // namespace CCL

#endif // _ccl_extensionmanagement_h
