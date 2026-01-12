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
// Filename    : ccl/platform/android/interfaces/iframeworkactivity.h
// Description : interface to framework activity
//
//************************************************************************************************

#ifndef _ccl_android_iframeworkactivity_h
#define _ccl_android_iframeworkactivity_h

#include "ccl/public/base/iunknown.h"

#include <jni.h>

namespace CCL {

struct FileInfo;

namespace Android {

//************************************************************************************************
// IFrameworkActivity
/** Interface to native FrameworkActivity (used by cclsystem). */
//************************************************************************************************

interface IFrameworkActivity: IUnknown
{
	virtual jobject CCL_API getJObject () const = 0;

	virtual jobject CCL_API getAssetManager () const = 0;

	virtual int64 CCL_API getPackageInstallTime () const = 0; // in seconds
	
	virtual int64 CCL_API getPackageUpdateTime () const = 0; // in seconds

	virtual void CCL_API getComputerName (String& name) const = 0;
	
	virtual void CCL_API getUserName (String& name) const = 0;

	virtual void CCL_API getDeviceID (String& id) const = 0;

	virtual jobject CCL_API openContentFile (UrlRef url, StringRef modeString) const = 0;

	virtual tbool CCL_API contentFileExists (UrlRef url) const = 0;
	
	virtual tbool CCL_API getContentFileInfo (FileInfo& info, UrlRef url) const = 0;

	virtual tresult CCL_API relaunchActivity () const = 0;

	virtual void CCL_API getMainModuleID (String& id) const = 0;

	virtual void CCL_API getNativeLibraryDir (String& dir) const = 0;

	DECLARE_IID (IFrameworkActivity)
};

DEFINE_IID (IFrameworkActivity, 0x8f9893d0, 0x47c7, 0x41c0, 0x89, 0xaa, 0xf7, 0x13, 0x48, 0x18, 0x80, 0xa7)

} // namespace Android
} // namespace CCL

#endif // _ccl_android_iframeworkactivity_h
