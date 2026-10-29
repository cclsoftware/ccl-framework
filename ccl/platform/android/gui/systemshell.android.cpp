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
#include "ccl/platform/android/interfaces/jni/androidcontent.h"

#include "ccl/base/asyncoperation.h"

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
	PermissionState CCL_API getPermissionState (StringID permissionId) override;
	IAsyncOperation* CCL_API requestPermission (StringID permissionId) override;
	tresult openNativeUrl (UrlRef url, int flags) override;

private:
	static String getNativePermission (StringID permissionId);
	static PermissionState getNativePermissionState (StringRef permission);
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

String AndroidSystemShell::getNativePermission (StringID permissionId)
{
	if(permissionId == System::kPermissionAudioCapture)
		return CCLSTR ("android.permission.RECORD_AUDIO");

	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PermissionState AndroidSystemShell::getNativePermissionState (StringRef permission)
{
	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();
	if(activity == nullptr)
		return System::kPermissionUnknown;

	// Android cannot tell "never requested" from "denied", so anything not granted is undetermined.
	JniAccessor jni;
	JniCCLString javaPermission (permission);
	if(Context.checkSelfPermission (activity->getJObject (), javaPermission) == Context.PERMISSION_GRANTED)
		return System::kPermissionGranted;

	return System::kPermissionUndetermined;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PermissionState CCL_API AndroidSystemShell::getPermissionState (StringID permissionId)
{
	String nativePermissionId = getNativePermission (permissionId);
	if(nativePermissionId.isEmpty ())
		return SystemShell::getPermissionState (permissionId);
	else
		return getNativePermissionState (nativePermissionId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API AndroidSystemShell::requestPermission (StringID permissionId)
{
	String nativePermissionId = getNativePermission (permissionId);
	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();
	if(nativePermissionId.isEmpty () || activity == nullptr)
		return SystemShell::requestPermission (permissionId);

	// The activity reports the raw Android grant results; map them to the final permission state.
	SharedPtr<AsyncOperation> resultOperation = NEW AsyncOperation;
	resultOperation->setState (AsyncOperation::kStarted);

	Promise (activity->checkAndRequestPermissions (&nativePermissionId, 1)).then ([resultOperation, nativePermissionId] (IAsyncOperation& operation)
	{
		// after a request, anything not granted counts as denied
		bool granted = getNativePermissionState (nativePermissionId) == System::kPermissionGranted;
		resultOperation->setResult (int(granted ? System::kPermissionGranted : System::kPermissionDenied));
		resultOperation->setState (AsyncOperation::kCompleted);
	});

	return return_shared<IAsyncOperation> (resultOperation);
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
