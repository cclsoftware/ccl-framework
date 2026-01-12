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
// Filename    : ccl/platform/android/gui/gui.android.cpp
// Description : Platform-specific GUI implementation
//
//************************************************************************************************

#include "ccl/gui/gui.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"

#include "ccl/public/gui/iapplication.h"

#include <android/configuration.h>

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// SystemTimerClass
//************************************************************************************************

DECLARE_JNI_CLASS (SystemTimerClass, CCLGUI_CLASS_PREFIX "SystemTimer")
	DECLARE_JNI_CONSTRUCTOR (construct, JniIntPtr, int64)
	DECLARE_JNI_METHOD (void, stop)
END_DECLARE_JNI_CLASS (SystemTimerClass)

DEFINE_JNI_CLASS (SystemTimerClass)
	DEFINE_JNI_CONSTRUCTOR (construct, "(JJ)V")
	DEFINE_JNI_METHOD (stop, "()V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

namespace CCL {

//************************************************************************************************
// AndroidUserInterface
//************************************************************************************************

class AndroidUserInterface: public UserInterface
{
public:
	AndroidUserInterface ();

protected:
	// UserInterface
	bool startupPlatform (ModuleRef module) override;
	void shutdownPlatform () override;
	void quitPlatform () override;
	int CCL_API runEventLoop () override;
	void CCL_API quit (int exitCode) override;
	void CCL_API setAllowedInterfaceOrientations (OrientationTypeFlags orientations) override;
	void realizeActivityMode (ActivityMode mode) override;
	ITimer* CCL_API createTimer (unsigned int period) const override;
};

//************************************************************************************************
// AndroidTimer
//************************************************************************************************

class AndroidTimer: public SystemTimer
{
public:
	AndroidTimer (unsigned int period);
	~AndroidTimer ();

private:
	Android::JniObject timer;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidUserInterface androidGUI;
UserInterface& GUI = androidGUI;

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AndroidUserInterface
//************************************************************************************************

AndroidUserInterface::AndroidUserInterface ()
{
	applicationType = kMobileApplication;
	buttonOrder = Styles::kAffirmativeButtonRight;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidUserInterface::startupPlatform (ModuleRef module)
{
	float densityFactor = FrameworkActivity::getCurrentActivity ()->getBitmapDensityFactor ();
	Bitmap::setResolutionNamingMode (Bitmap::chooseResolutionMode (densityFactor));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidUserInterface::shutdownPlatform ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidUserInterface::quitPlatform ()
{
	FrameworkActivity::getCurrentActivity ()->quit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AndroidUserInterface::runEventLoop ()
{
	if(!finishStartup ())
		return kExitError;

	if(appProvider)
		appProvider->onInit ();

	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();
	if(activity)
	{
		if(FrameworkView* contentView = activity->getContentView ())
			contentView->createApplicationView ();

		setInterfaceOrientation (activity->getOrientation ());
		
		onAppStateChanged (IApplication::kUIInitialized);
	}

	// (no loop here)
	return exitCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidUserInterface::quit (int exitCode)
{
	if(quitDone)
		return;

	::UserInterface::quit ();

	if(quitDone)
		quitPlatform ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidUserInterface::setAllowedInterfaceOrientations (OrientationTypeFlags orientations)
{
	UserInterface::setAllowedInterfaceOrientations (orientations);

	if(FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ())
		FrameworkActivityClass.setAllowedInterfaceOrientations (*activity, orientations);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidUserInterface::realizeActivityMode (ActivityMode mode)
{
	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();
	if(!activity)
		return;

	switch(mode)
	{
	case ActivityMode::kNormal:
		FrameworkActivityClass.setKeepScreenOn (*activity, false);
		break;

	case ActivityMode::kAlwaysOn:
		FrameworkActivityClass.setKeepScreenOn (*activity, true);
		break;

	case ActivityMode::kBackground:
		
		break;
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer* CCL_API AndroidUserInterface::createTimer (unsigned int period) const
{
	return NEW AndroidTimer (period);
}

//************************************************************************************************
// AndroidTimer
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, SystemTimer, onTimerNative, JniIntPtr nativeTimerPtr)
{
	if(AndroidTimer* timer = JniCast<AndroidTimer>::fromIntPtr (nativeTimerPtr))
		SystemTimer::trigger (timer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidTimer::AndroidTimer (unsigned int period)
: SystemTimer (period)
{
	JniAccessor jni;
	timer.assign (jni, jni.newObject (SystemTimerClass, SystemTimerClass.construct, (JniIntPtr)this, (jlong)period));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidTimer::~AndroidTimer ()
{
	if(timer.isValid ())
		SystemTimerClass.stop (timer);
}
