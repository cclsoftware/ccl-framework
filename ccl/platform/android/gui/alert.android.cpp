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
// Filename    : ccl/platform/android/gui/alert.android.cpp
// Description : platform alert dialog
//
//************************************************************************************************

#include "ccl/gui/dialogs/alert.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/android/gui/frameworkactivity.h"

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// dev.ccl.Alert
//************************************************************************************************

DECLARE_JNI_CLASS (Alert, CCLGUI_CLASS_PREFIX "Alert")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_METHOD (void, run, jobject, jstring, jstring, jstring, jstring, jstring)
	DECLARE_JNI_METHOD (void, dismiss)
END_DECLARE_JNI_CLASS (Alert)

DEFINE_JNI_CLASS (Alert)
	DEFINE_JNI_CONSTRUCTOR (construct, "(J)V")
	DEFINE_JNI_METHOD (run, "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V")
	DEFINE_JNI_METHOD (dismiss, "()V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

namespace CCL {

//************************************************************************************************
// AndroidAlertBox
//************************************************************************************************

class AndroidAlertBox: public AlertBox
{
public:
	DECLARE_CLASS (AndroidAlertBox, AlertBox)

	AndroidAlertBox ();

	PROPERTY_SHARED_AUTO (AsyncOperation, asyncOperation, AsyncOperation)

	// AlertBox
	void closePlatform () override;
	IAsyncOperation* runAsyncPlatform () override;
	int CCL_API run () override;

private:
	Android::JniObject alert;
};

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// Alert::ButtonMapping
//************************************************************************************************

int Alert::ButtonMapping::getResultAtButtonIndex (int buttonIndex) const
{
	switch(buttonIndex)
	{
	case 0 : return defaultResult;
	case 1 : return alternateResult;
	case 2 : return otherResult;
	}
	return kUndefined;
}

//************************************************************************************************
// AndroidAlertBox
//************************************************************************************************

DEFINE_CLASS (AndroidAlertBox, AlertBox)
DEFINE_CLASS_UID (AndroidAlertBox, 0x9bf3ecb5, 0x5bb2, 0x4eb4, 0xaa, 0xac, 0x29, 0xaf, 0xf4, 0x66, 0x45, 0xa5) // ClassID::AlertBox

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidAlertBox::AndroidAlertBox ()
{
	JniAccessor jni;
	alert.assign (jni, jni.newObject (Android::Alert, Android::Alert.construct, (JniIntPtr)this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidAlertBox::closePlatform ()
{
	Android::Alert.dismiss (alert);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidAlertBox::runAsyncPlatform ()
{
	JniCCLString title (getTitle ());
	JniCCLString message (getText ());
	JniCCLString button1 (getFirstButton ());
	JniCCLString button2 (getSecondButton ());
	JniCCLString button3 (getThirdButton ());

	Android::Alert.run (alert, *FrameworkActivity::getCurrentActivity (), title, message, button1, button2, button3);

	asyncOperation.share (NEW AsyncOperation);
	asyncOperation->setState (AsyncOperation::kStarted);	
	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AndroidAlertBox::run ()
{
	CCL_WARN ("synchronous Alert not supported!", 0)
	Promise p (runAsync ());
	ASSERT (0)
	return Alert::kUndefined;
}

//************************************************************************************************
// Alert Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, Alert, onAlertResultNative, JniIntPtr nativeAlertPtr, jint buttonIndex)
{
	// alert dialog finished:
	AndroidAlertBox* alertBox = JniCast<AndroidAlertBox>::fromIntPtr (nativeAlertPtr);
	if(AsyncOperation* asyncOperation = alertBox ? alertBox->getAsyncOperation () : 0)
	{
		int result = Alert::kUndefined;
		if(buttonIndex >= 0)
			result = alertBox->getButtonResult (buttonIndex);
		else
		{
			if(alertBox->isUsingCustomButtonResults ())
 				result = Alert::kEscapePressed;
 			else
 				result = Alert::kCancel;
		}
		CCL_PRINTF ("onAlertResult: buttonIndex %d, result %d", buttonIndex, result)

		SharedPtr<IUnknown> holder (alertBox->asUnknown ()); // keep alive until we're done (will be released in AlertBox::onAlertCompleted)

		asyncOperation->setResult (result);
		asyncOperation->setState (AsyncOperation::kCompleted);
		alertBox->setAsyncOperation (0);
	}
}
