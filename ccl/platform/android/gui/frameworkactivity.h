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
// Filename    : ccl/platform/android/gui/frameworkactivity.h
// Description : Framework Activity (native)
//
//************************************************************************************************

#ifndef _ccl_android_frameworkactivity_h
#define _ccl_android_frameworkactivity_h

#include "ccl/platform/android/gui/androidintent.h"

#include "ccl/platform/android/interfaces/iframeworkactivity.h"
#include "ccl/platform/android/interfaces/jni/androidcontent.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/base/unknown.h"
#include "ccl/public/base/datetime.h"

namespace CCL {

namespace Android {

//************************************************************************************************
// dev.ccl.FrameworkActivity
//************************************************************************************************

DECLARE_JNI_CLASS (FrameworkActivityClass, CCLGUI_CLASS_PREFIX "FrameworkActivity")
	DECLARE_JNI_METHOD (void, setKeepScreenOn, bool)
	DECLARE_JNI_METHOD (void, setAllowedInterfaceOrientations, int)
	DECLARE_JNI_METHOD (jobject, getDisplaySize)
	DECLARE_JNI_METHOD (jobject, getRectOnScreen)
	DECLARE_JNI_METHOD (int, getDensity)
	DECLARE_JNI_METHOD (int, getOrientation)
	DECLARE_JNI_METHOD (int, getScreenSize)
	DECLARE_JNI_METHOD (int, getStatusBarHeight)
	DECLARE_JNI_METHOD (jobject, getInsets)
	DECLARE_JNI_METHOD (void, setLightStatusBar, bool)
	DECLARE_JNI_METHOD (void, setSystemUiVisibility, bool, bool)
	DECLARE_JNI_METHOD (bool, isForegroundActivity)
	DECLARE_JNI_METHOD (jobject, getIntent)
	DECLARE_JNI_METHOD (bool, openApplicationSettings)
	DECLARE_JNI_METHOD (bool, openUrl, jstring)
	DECLARE_JNI_METHOD (jstring, getComputerName)
	DECLARE_JNI_METHOD (jstring, getUserName)
	DECLARE_JNI_METHOD (jstring, getDeviceID)
	DECLARE_JNI_METHOD (int, getSDKVersion)
	DECLARE_JNI_METHOD (jobject, openContentFile, jstring, jstring)
	DECLARE_JNI_METHOD (bool, contentFileExists, jstring)
	DECLARE_JNI_METHOD (jobject, getContentFileInfo, jstring)
	DECLARE_JNI_METHOD (jstring, getContentFileDisplayName, jstring)
	DECLARE_JNI_METHOD (jobject, getAssets)
	DECLARE_JNI_METHOD (int64, getPackageInstallTime)
	DECLARE_JNI_METHOD (int64, getPackageUpdateTime)
	DECLARE_JNI_METHOD (bool, isMimeTypeSupported, jstring)
	DECLARE_JNI_METHOD (bool, runFileSelector, bool, jstring, jstring)
	DECLARE_JNI_METHOD (bool, runFolderSelector, jstring)
	DECLARE_JNI_METHOD (bool, runFileSharing, jstring, jstring)
	DECLARE_JNI_METHOD (bool, runTextSharing, jstring)
	DECLARE_JNI_METHOD (void, finish)
	DECLARE_JNI_METHOD (void, relaunchActivity)
	DECLARE_JNI_METHOD (void, reportLaunchError, jstring)
	DECLARE_JNI_STATIC_METHOD (jstring, getMainModuleID)
	DECLARE_JNI_METHOD (jstring, getNativeLibraryDir)
END_DECLARE_JNI_CLASS (FrameworkActivityClass)

DECLARE_JNI_CLASS (FrameworkActivityFileInfoClass, CCLGUI_CLASS_PREFIX "FrameworkActivity$FileInfo")
	DECLARE_JNI_FIELD (int64, fileSize)
	DECLARE_JNI_FIELD (int64, modifiedTime)
END_DECLARE_JNI_CLASS (FrameworkActivityFileInfoClass)

class FrameworkView;

//************************************************************************************************
// FrameworkActivity
//************************************************************************************************

class FrameworkActivity: public JniObject,
						 public Unknown,
						 public IFrameworkActivity
{
public:
	FrameworkActivity (JNIEnv* jni, jobject object, FrameworkView* contentView);
	~FrameworkActivity ();

	static int activityCount () { return activityList.count (); }
	static FrameworkActivity* lookupNativeActivity (JNIEnv* jni, jobject jActivity);

	static void updateCurrentActivity (FrameworkActivity* activity);
	static FrameworkActivity* getCurrentActivity ();

	FrameworkView* getContentView () { return contentView; }
	AndroidIntent* getIntent () const;
	Url getIntentContentUrl (AndroidIntent* intent) const;
	String getContentFileDisplayName (UrlRef url) const;

	bool isForegroundActivity () const;

	void updateSystemUIVisibility ();

	void quit () const;

	static bool isQuitting () { return quitting; }

	// IFrameworkActivity
	jobject CCL_API getJObject () const override;
	jobject CCL_API getAssetManager () const override;
	int64 CCL_API getPackageInstallTime () const override;
	int64 CCL_API getPackageUpdateTime () const override;
	void CCL_API getComputerName (String& name) const override;
	void CCL_API getUserName (String& name) const override;
	void CCL_API getDeviceID (String& id) const override;
	jobject CCL_API openContentFile (UrlRef url, StringRef modeString) const override;
	tbool CCL_API contentFileExists (UrlRef url) const override;
	tbool CCL_API getContentFileInfo (FileInfo& info, UrlRef url) const override;
	tresult CCL_API relaunchActivity () const override;
	void CCL_API getMainModuleID (String& id) const override;
	void CCL_API getNativeLibraryDir (String& dir) const override;

	// UI configuration
	float getDensityFactor ();
	float getBitmapDensityFactor ();
	OrientationType getOrientation ();
	Point getScreenSize ();							// in pixels
	Rect getWorkArea ();							// in pixels
	Coord getStatusBarHeight ();					// in pixels
	Rect getInsets ();								// in pixels

	void setLightStatusBar (bool lightStatusBar);

	CLASS_INTERFACE (IFrameworkActivity, Unknown)

protected:
	static LinkedList<FrameworkActivity*> activityList;
	static FrameworkActivity* currentActivity;
	static bool quitting;

	FrameworkView* contentView;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_android_frameworkactivity_h
