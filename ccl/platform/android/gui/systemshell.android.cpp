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
// Filename    : ccl/platform/android/gui/systemshell.android.cpp
// Description : Android System Shell
//
//************************************************************************************************

#include "ccl/gui/system/systemshell.h"

#include "ccl/platform/android/gui/frameworkactivity.h"

#include "ccl/public/storage/iurl.h"

namespace CCL {

//************************************************************************************************
// AndroidSystemShell
//************************************************************************************************

class AndroidSystemShell: public SystemShell
{
public:
	// SystemShell
	tresult openApplicationSettings () override;
	tresult openNativeUrl (UrlRef url, int flags) override;
};

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AndroidSystemShell
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemShell, AndroidSystemShell)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidSystemShell::openApplicationSettings ()
{
	JniAccessor jni;
	if(!FrameworkActivityClass.openApplicationSettings (*FrameworkActivity::getCurrentActivity ()))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidSystemShell::openNativeUrl (UrlRef url, int flags)
{
	String urlString;
	if(url.isNativePath ())
		urlString =  NativePath (url);
	else
		url.getUrl (urlString, true);

	JniCCLString javaString (urlString);

	if(!FrameworkActivityClass.openUrl (*FrameworkActivity::getCurrentActivity (), javaString))
		return kResultFailed;

	return kResultOk;
}
