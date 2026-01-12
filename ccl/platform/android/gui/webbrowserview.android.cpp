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
// Filename    : ccl/platform/android/gui/webbrowserview.android.cpp
// Description : Android Web Browser View
//
//************************************************************************************************

#include "ccl/gui/system/webbrowserview.h"

#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/window.android.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/graphics/dpiscale.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// dev.ccl.WebControl
//************************************************************************************************

DECLARE_JNI_CLASS (WebControl, CCLGUI_CLASS_PREFIX "WebControl")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_METHOD (void, show)
	DECLARE_JNI_METHOD (void, remove)
	DECLARE_JNI_METHOD (void, setSize, int, int, int, int)
	DECLARE_JNI_METHOD (void, loadUrl, jstring)
	DECLARE_JNI_METHOD (void, reload)
	DECLARE_JNI_METHOD (bool, canGoBackOrForward, int)
	DECLARE_JNI_METHOD (void, goBackOrForward, int)
	DECLARE_JNI_METHOD (jstring, getTitle)
	DECLARE_JNI_METHOD (jstring, getUrl)
END_DECLARE_JNI_CLASS (WebControl)

DEFINE_JNI_CLASS (WebControl)
	DEFINE_JNI_CONSTRUCTOR (construct, "(Landroid/content/Context;JL" CCLGUI_CLASS_PREFIX "FrameworkView;Z)V")
	DEFINE_JNI_METHOD (show, "()V")
	DEFINE_JNI_METHOD (remove, "()V")
	DEFINE_JNI_METHOD (setSize, "(IIII)V")
	// the following are inherited from android.webkit.WebView
	DEFINE_JNI_METHOD (loadUrl, "(Ljava/lang/String;)V")
	DEFINE_JNI_METHOD (reload, "()V")
	DEFINE_JNI_METHOD (canGoBackOrForward, "(I)Z")
	DEFINE_JNI_METHOD (goBackOrForward, "(I)V")
	DEFINE_JNI_METHOD (getTitle, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getUrl, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// AndroidWebControl
//************************************************************************************************

class AndroidWebControl: public NativeWebControl
{
public:
	AndroidWebControl (WebBrowserView& owner);

	// NativeWebControl
	void attachView () override;
	void detachView () override;
	void updateSize () override;
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API refresh () override;
	tresult CCL_API goBack () override;
	tresult CCL_API goForward () override;

	void updatePageInfo ();

protected:
	JniObject webControl;
	bool attached;
};

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// NativeWebControl
//************************************************************************************************

bool NativeWebControl::isAvailable ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWebControl* NativeWebControl::createInstance (WebBrowserView& owner)
{
	return NEW AndroidWebControl (owner);
}

//************************************************************************************************
// AndroidWebControl
//************************************************************************************************

AndroidWebControl::AndroidWebControl (WebBrowserView& owner)
: NativeWebControl (owner),
  attached (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWebControl::attachView ()
{
	ASSERT (!attached)
	ASSERT (!webControl)
	attached = true;

	if(!webControl)
	{
		// owning control must be attached
		AndroidWindow* w = AndroidWindow::cast (owner.getWindow ());
		FrameworkView* frameworkView = w ? w->getFrameworkView () : nullptr;
		ASSERT (frameworkView)
		if(!frameworkView)
			return;

		bool restrictToLocal = getOptions ().isCustomStyle (Styles::kWebBrowserViewBehaviorRestrictToLocal);

		JniAccessor jni;
		webControl.assign (jni, jni.newObject (WebControl, WebControl.construct, FrameworkActivity::getCurrentActivity ()->getJObject (), (JniIntPtr)this, frameworkView->getJObject (), restrictToLocal));

		if(webControl)
			WebControl.show (webControl);
	}
	updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWebControl::detachView ()
{
	attached = false;

	if(webControl)
	{
		JniAccessor jni;
		WebControl.remove (webControl);

		webControl.assign (jni, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWebControl::updateSize ()
{
	if(webControl && attached)
	{
		Rect rect (getSizeInWindow ());
		float scaleFactor = owner.getWindow ()->getContentScaleFactor ();
		DpiScale::toPixelRect (rect, scaleFactor);

		WebControl.setSize (webControl, rect.left, rect.top, rect.getWidth (), rect.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidWebControl::navigate (UrlRef url)
{
	if(webControl)
	{
		UrlFullString urlString (url, true);
		JniCCLString string (urlString);

		WebControl.loadUrl (webControl, string);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWebControl::updatePageInfo ()
{
	flagCanBack (WebControl.canGoBackOrForward (webControl, -1));
	flagCanForward (WebControl.canGoBackOrForward (webControl, 1));

	JniAccessor jni;
	LocalStringRef string (jni, WebControl.getTitle (webControl));
	fromJavaString (currentTitle, string);

	LocalStringRef urlString (jni, WebControl.getUrl (webControl));
	currentUrl.setUrl (fromJavaString (urlString));

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidWebControl::refresh ()
{
	if(webControl)
	{
		WebControl.reload (webControl);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidWebControl::goBack ()
{
	if(webControl)
	{
		WebControl.goBackOrForward (webControl, -1);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidWebControl::goForward ()
{
	if(webControl)
	{
		WebControl.goBackOrForward (webControl, 1);
		return kResultOk;
	}
	return kResultFailed;
}

//************************************************************************************************
// dev.ccl.WebControl Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, WebControl, onPageFinishedNative, JniIntPtr nativeWebControlPtr)
{
	auto* webControl = JniCast<AndroidWebControl>::fromIntPtr (nativeWebControlPtr);
	if(webControl)
		webControl->updatePageInfo ();
}
