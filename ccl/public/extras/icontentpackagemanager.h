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
// Filename    : ccl/public/extras/icontentpackagemanager.h
// Description : Content Package Manager Interface
//
//************************************************************************************************

#ifndef _ccl_icontentpackagemanager_h
#define _ccl_icontentpackagemanager_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ContentPackageManager, 0xc077f684, 0xdc73, 0x4c44, 0x91, 0xaa, 0x10, 0xbc, 0x8, 0x5d, 0xf6, 0xcc)
}

//************************************************************************************************
// IUpdateCheckObserver
//************************************************************************************************

interface IUpdateCheckObserver: IUnknown
{
	virtual void CCL_API onVersionCheckCompleted (StringRef packageId, StringRef installedVersion, StringRef currentVersion, tresult result) = 0;

	DECLARE_IID (IUpdateCheckObserver)
};

DEFINE_IID (IUpdateCheckObserver, 0x88cda563, 0x6f85, 0x4a83, 0x93, 0xbe, 0x6, 0x4b, 0xbb, 0x89, 0x53, 0xbd)

//************************************************************************************************
// IContentPackageManager
//************************************************************************************************

interface IContentPackageManager: IUnknown
{
	virtual String CCL_API getAppVersion () const = 0;
	
	virtual tresult CCL_API checkPackageVersion (StringRef packageId, IUpdateCheckObserver* observer) = 0;
	
	virtual tresult CCL_API cancelVersionCheck (IUpdateCheckObserver* observer) = 0;
	
	virtual tresult CCL_API triggerPackageInstallation (StringRef packageId) = 0;
	
	DECLARE_STRINGID_MEMBER (kPackageInstalled)	///< args[0]: package id (string), args[1]: success (tbool). */

	DECLARE_IID (IContentPackageManager)
};

DEFINE_IID (IContentPackageManager, 0x33c56dc1, 0x48f0, 0x4939, 0xba, 0x47, 0xf7, 0x20, 0xdd, 0x10, 0x1d, 0x2b)

DEFINE_STRINGID_MEMBER (IContentPackageManager, kPackageInstalled, "PackageInstalled")

} // namespace CCL

#endif // _ccl_icontentpackagemanager_h
