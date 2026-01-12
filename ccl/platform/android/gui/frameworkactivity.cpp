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
// Filename    : ccl/platform/android/gui/frameworkactivity.cpp
// Description : Framework Activity (native)
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/window.android.h"
#include "ccl/platform/android/gui/keyevent.android.h"

#include "ccl/platform/android/graphics/frameworkgraphics.h"
#include "ccl/platform/android/graphics/androidgraphics.h"

#include "ccl/platform/android/interfaces/iandroidsystem.h"

#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/gui.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/message.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/system/ifilesystem.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include <android/configuration.h>
#include <dlfcn.h>

namespace CCL {
namespace Android {

//************************************************************************************************
// dev.ccl.FrameworkActivity
//************************************************************************************************

DEFINE_JNI_CLASS (FrameworkActivityClass)
	DEFINE_JNI_METHOD (setKeepScreenOn, "(Z)V")
	DEFINE_JNI_METHOD (setAllowedInterfaceOrientations, "(I)V")
	DEFINE_JNI_METHOD (getDisplaySize, "()Landroid/graphics/Point;")
	DEFINE_JNI_METHOD (getRectOnScreen, "()Landroid/graphics/Rect;")
	DEFINE_JNI_METHOD (getDensity, "()I")
	DEFINE_JNI_METHOD (getOrientation, "()I")
	DEFINE_JNI_METHOD (getScreenSize, "()I")
	DEFINE_JNI_METHOD (getStatusBarHeight, "()I")
	DEFINE_JNI_METHOD (getInsets, "()Landroid/graphics/Rect;")
	DEFINE_JNI_METHOD (setLightStatusBar, "(Z)V")
	DEFINE_JNI_METHOD (setSystemUiVisibility, "(ZZ)V")
	DEFINE_JNI_METHOD (isForegroundActivity, "()Z")
	DEFINE_JNI_METHOD (getIntent, "()Landroid/content/Intent;")
	DEFINE_JNI_METHOD (openApplicationSettings, "()Z")
	DEFINE_JNI_METHOD (openUrl, "(Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (getComputerName, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getUserName, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getDeviceID, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getSDKVersion, "()I")
	DEFINE_JNI_METHOD (openContentFile, "(Ljava/lang/String;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;")
	DEFINE_JNI_METHOD (contentFileExists, "(Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (getContentFileInfo, "(Ljava/lang/String;)L" CCLGUI_CLASS_PREFIX "FrameworkActivity$FileInfo;")
	DEFINE_JNI_METHOD (getContentFileDisplayName, "(Ljava/lang/String;)Ljava/lang/String;")
	DEFINE_JNI_METHOD (getAssets, "()Landroid/content/res/AssetManager;")
	DEFINE_JNI_METHOD (getPackageInstallTime, "()J")
	DEFINE_JNI_METHOD (getPackageUpdateTime, "()J")
	DEFINE_JNI_METHOD (isMimeTypeSupported, "(Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (runFileSelector, "(ZLjava/lang/String;Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (runFolderSelector, "(Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (runFileSharing, "(Ljava/lang/String;Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (runTextSharing, "(Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (finish, "()V")
	DEFINE_JNI_METHOD (relaunchActivity, "()V")
	DEFINE_JNI_METHOD (reportLaunchError, "(Ljava/lang/String;)V")
	DEFINE_JNI_STATIC_METHOD (getMainModuleID, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getNativeLibraryDir, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

DEFINE_JNI_CLASS (FrameworkActivityFileInfoClass)
	DEFINE_JNI_FIELD (fileSize, "J")
	DEFINE_JNI_FIELD (modifiedTime, "J")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// FrameworkActivity
//************************************************************************************************

LinkedList<FrameworkActivity*> FrameworkActivity::activityList;
FrameworkActivity* FrameworkActivity::currentActivity = nullptr;
bool FrameworkActivity::quitting = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkActivity::FrameworkActivity (JNIEnv* jni, jobject object, FrameworkView* contentView)
: JniObject (jni, object),
  contentView (contentView)
{
	activityList.append (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkActivity::~FrameworkActivity ()
{
	ASSERT (currentActivity != this)
	activityList.remove (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkActivity* FrameworkActivity::lookupNativeActivity (JNIEnv* jni, jobject jActivity)
{
	for(FrameworkActivity* activity : activityList)
	{
		if(jni->IsSameObject (activity->getJObject (), jActivity))
			return activity;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkActivity::updateCurrentActivity (FrameworkActivity* activity)
{
	UnknownPtr<IAndroidSystem> androidSystem (&System::GetSystem ());
	ASSERT (androidSystem)
	if(androidSystem)
		androidSystem->setNativeActivity (activity);

	currentActivity = activity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkActivity* FrameworkActivity::getCurrentActivity ()
{
	ASSERT (currentActivity)
	return currentActivity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject CCL_API FrameworkActivity::getJObject () const
{
	return object;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject CCL_API FrameworkActivity::getAssetManager () const
{
	return FrameworkActivityClass.getAssets (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API FrameworkActivity::getPackageInstallTime () const
{
	return FrameworkActivityClass.getPackageInstallTime (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API FrameworkActivity::getPackageUpdateTime () const
{
	return FrameworkActivityClass.getPackageUpdateTime (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameworkActivity::getComputerName (String& name) const
{
	JniAccessor jni;
	LocalStringRef string (jni, FrameworkActivityClass.getComputerName (*this));
	fromJavaString (name, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameworkActivity::getUserName (String& name) const
{
	JniAccessor jni;
	LocalStringRef string (jni, FrameworkActivityClass.getUserName (*this));
	fromJavaString (name, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameworkActivity::getDeviceID (String& id) const
{
	JniAccessor jni;
	LocalStringRef string (jni, FrameworkActivityClass.getDeviceID (*this));
	id = fromJavaString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject CCL_API FrameworkActivity::openContentFile (UrlRef url, StringRef modeString) const
{
	String urlString;
	url.getUrl (urlString);

	JniCCLString jniUrlString (urlString);
	JniCCLString jniModeString (modeString);
	return FrameworkActivityClass.openContentFile (*this, jniUrlString, jniModeString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FrameworkActivity::contentFileExists (UrlRef url) const
{
	String urlString;
	url.getUrl (urlString);

	JniCCLString jniUrlString (urlString);
	return FrameworkActivityClass.contentFileExists (*this, jniUrlString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FrameworkActivity::getContentFileInfo (FileInfo& info, UrlRef url) const
{
	String urlString;
	url.getUrl (urlString);

	JniAccessor jni;
	JniCCLString jniUrlString (urlString);
	LocalRef fileInfo (jni, FrameworkActivityClass.getContentFileInfo (*this, jniUrlString));
	if(!fileInfo)
		return false;

	info.fileSize = jni.getField (fileInfo, FrameworkActivityFileInfoClass.fileSize);
	info.modifiedTime = UnixTime::toLocal (jni.getField (fileInfo, FrameworkActivityFileInfoClass.modifiedTime));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FrameworkActivity::relaunchActivity () const
{
	FrameworkActivityClass.relaunchActivity (*this);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameworkActivity::getMainModuleID (String& id) const
{
	JniAccessor jni;
	LocalStringRef string (jni, FrameworkActivityClass.getMainModuleID ());
	id = fromJavaString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameworkActivity::getNativeLibraryDir (String& dir) const
{
	JniAccessor jni;
	LocalStringRef string (jni, FrameworkActivityClass.getNativeLibraryDir (*this));
	dir = fromJavaString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidIntent* FrameworkActivity::getIntent () const
{
	JniAccessor jni;
	return NEW AndroidIntent (jni, FrameworkActivityClass.getIntent (*this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url FrameworkActivity::getIntentContentUrl (AndroidIntent* intent) const
{
	CString action = intent->getAction ();
	if(action != Action::kActionView && action != Action::kActionEdit)
		return Url ();

	Url url = intent->getDataString ();
	String displayName = getContentFileDisplayName (url);
	if(!displayName.isEmpty ())
		url.getParameters ().setEntry (CCLSTR (UrlParameter::kDisplayName), displayName);

	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String FrameworkActivity::getContentFileDisplayName (UrlRef url) const
{
	String urlString;
	url.getUrl (urlString);

	JniAccessor jni;
	JniCCLString jniUrlString (urlString);
	LocalStringRef displayName (jni, FrameworkActivityClass.getContentFileDisplayName (*this, jniUrlString));

	return fromJavaString (displayName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameworkActivity::isForegroundActivity () const
{
	return FrameworkActivityClass.isForegroundActivity (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkActivity::quit () const
{
	quitting = true;

	FrameworkActivityClass.finish (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float FrameworkActivity::getDensityFactor ()
{
#if (0 && DEBUG) // fake content scaling
	return 2.f;
#else
	int density = FrameworkActivityClass.getDensity (*this);
	int minimumDPI = 160;
	int maximumDPI = density;

	CCL::Configuration::Registry::instance ().getValue (minimumDPI, "CCL.Android", "MinimumDPI");
	CCL::Configuration::Registry::instance ().getValue (maximumDPI, "CCL.Android", "MaximumDPI");

	float maximumScaleFactor = ccl_max (1.0, floorf (float(density) / float(minimumDPI) * 4.0) / 4.0);
	float minimumScaleFactor = ccl_max (1.0, ceilf (float(density) / float(maximumDPI) * 4.0) / 4.0);

	// get screen size and subtract status bar height as it's not available to the app
	Point screenSize (getScreenSize ());
	ccl_order (screenSize.x, screenSize.y);

	Point requiredSize;
	CCL::Configuration::Registry::instance ().getValue (requiredSize.x, "CCL.Android", "RequiredScreenWidth");
	CCL::Configuration::Registry::instance ().getValue (requiredSize.y, "CCL.Android", "RequiredScreenHeight");
	if(requiredSize.x < requiredSize.y)
		screenSize.y -= getStatusBarHeight ();
	else
		screenSize.x -= getStatusBarHeight ();
	ccl_order (requiredSize.x, requiredSize.y);

	// only use a scale factor if there will still be enough pixels
	auto canUseFactor = [&] (float factor)
	{
		if(requiredSize.isNull ())
			return true;

		Point screenCoords (screenSize);
		DpiScale::toCoordPoint (screenCoords, factor);
		return screenCoords.x >= requiredSize.x  && screenCoords.y >= requiredSize.y;
	};

	float delta = 0.25f;
	for(float f = 3.f; f > 1.f; f -= delta)
		if(maximumScaleFactor >= f && minimumScaleFactor <= f && canUseFactor (f))
			return f;
	return minimumScaleFactor;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float FrameworkActivity::getBitmapDensityFactor ()
{
	float factor = getDensityFactor ();
	#if 0 // experimental:
	if(factor > 1)
	{
		auto getRequiredMemory = [] (int factor)
		{
			MutableCString keyName ("scaleFactor");
			keyName.appendFormat ("%d", factor);

			int megaBytes = 0;
			CCL::Configuration::Registry::instance ().getValue (megaBytes, "CCL.Android.RequiredMemory", keyName);
			return megaBytes * 1024 * 1024;
		};

		// if factor 2 requires more memory than available, use factor 1 for loading bitmaps
		static int64 requiredMemory2 = getRequiredMemory (2);
		static int64 availableMemory = AndroidSystemInformation::getPhysicalMemoryAvailable ();
		if(requiredMemory2 > availableMemory)
		{
			CCL_PRINT ("Not enough memory for scale factor 2: using bitmaps for factor 1.")
			return 1.;
		}
	}
	#endif
	return factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point FrameworkActivity::getScreenSize ()
{
	JniAccessor jni;
	JniObject jsize (jni, FrameworkActivityClass.getDisplaySize (*this));

	Point size;
	FrameworkGraphics::toCCLPoint (size, jni, jsize);
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect FrameworkActivity::getWorkArea ()
{
	JniAccessor jni;
	JniObject jrect (jni, FrameworkActivityClass.getRectOnScreen (*this));

	Rect rect;
	if(jrect)
		FrameworkGraphics::toCCLRect (rect, jni, jrect);
	return rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OrientationType FrameworkActivity::getOrientation ()
{
	int orientation = FrameworkActivityClass.getOrientation (*this);
	return orientation == ACONFIGURATION_ORIENTATION_PORT ? Styles::kPortrait : Styles::kLandscape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord FrameworkActivity::getStatusBarHeight ()
{
	return FrameworkActivityClass.getStatusBarHeight (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect FrameworkActivity::getInsets ()
{
	JniAccessor jni;
	JniObject jinsets (jni, FrameworkActivityClass.getInsets (*this));

	Rect insets;
	if(jinsets)
		FrameworkGraphics::toCCLRect (insets, jni, jinsets);
	return insets;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkActivity::setLightStatusBar (bool lightStatusBar)
{
	FrameworkActivityClass.setLightStatusBar (*this, lightStatusBar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkActivity::updateSystemUIVisibility ()
{
	static const String kHide = "hide";
	static const String kAuto = "auto";

	// hide system UI elements if requested in cclgui.config
	String statusBarMode;
	String navigationBarMode;
	CCL::Configuration::Registry::instance ().getValue (statusBarMode, "CCL.Android", "StatusBarMode");
	CCL::Configuration::Registry::instance ().getValue (navigationBarMode, "CCL.Android", "NavigationBarMode");

	bool isLandscape = (getOrientation () == Styles::kLandscape);
	bool isSmallScreen = FrameworkActivityClass.getScreenSize (*this) < ACONFIGURATION_SCREENSIZE_LARGE;

	bool hideStatusBar = (statusBarMode == kHide) || (isLandscape && isSmallScreen && statusBarMode == kAuto);
	bool hideNavigationBar = (navigationBarMode == kHide) || (isLandscape && isSmallScreen && statusBarMode == kAuto);
	FrameworkActivityClass.setSystemUiVisibility (*this, hideStatusBar, hideNavigationBar);

	ThemeManager::instance ().onSystemMetricsChanged ();
}

//************************************************************************************************
// FrameworkActivity Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (bool, FrameworkActivity, onCreateNative, jobject savedInstanceState, JniIntPtr nativeViewPtr)
{	
	CCL_PRINT ("FrameworkActivity::onCreateNative")

	// create global graphics factory
	if(gGraphicsFactory == nullptr)
		gGraphicsFactory = NEW FrameworkGraphicsFactory;

	// create native activity
	FrameworkView* contentView = FrameworkView::fromIntPtr (nativeViewPtr);
	ASSERT (contentView != 0)

	FrameworkActivity* activity = NEW FrameworkActivity (env, This, contentView);

	// update current activity pointer
	FrameworkActivity::updateCurrentActivity (activity);

	static bool frameworkInitialized = false;
	if(!frameworkInitialized)
	{
		int sdkVersion = FrameworkActivityClass.getSDKVersion (*activity);
		Configuration::Registry::instance ().setValue ("CCL.Android", "SDKVersion", sdkVersion);

		// init system framework, call main function
		UnknownPtr<IAndroidSystem> androidSystem (&System::GetSystem ());
		ASSERT (androidSystem)
		if(androidSystem)
		{
			ErrorContextGuard errorContext;
			if(androidSystem->callAndroidMain (true) != kExitSuccess)
			{
				int numEvents = errorContext->getEventCount ();
				for(int i = 0; i < numEvents; i++)
				{
					// report first error
					AlertEventRef event = errorContext->getEvent (i);
					if(event.type != Alert::kError)
						continue;

					JniAccessor jni;
					JniString jstring (jni, StringChars (event.message), event.message.length ());
					FrameworkActivityClass.reportLaunchError (*activity, jstring);
				}
				return false;
			}
		}

		frameworkInitialized = true;
	}
	else
	{
		// create window for this activity
		contentView->createApplicationView ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (void, FrameworkActivity, onNewIntentNative)
{
	FrameworkActivity* activity = FrameworkActivity::lookupNativeActivity (env, This);
	ASSERT (activity)
	if(!activity)
		return;

	// Open content URL if provided
	Url contentUrl = activity->getIntentContentUrl (activity->getIntent ());
	if(!contentUrl.isEmpty ())
	{
		IApplication* application = GUI.getApplication ();
		application->openFile (contentUrl);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (void, FrameworkActivity, onResumeNative)
{
	CCL_PRINT ("FrameworkActivity::onResumeNative")

	FrameworkActivity* activity = FrameworkActivity::lookupNativeActivity (env, This);
	ASSERT (activity)
	if(!activity)
		return;

	// update current activity pointer
	FrameworkActivity::updateCurrentActivity (activity);

	// show/hide status and navigation bars
	activity->updateSystemUIVisibility ();

	// notify application of being activated
	GUI.onAppStateChanged (IApplication::kAppActivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (void, FrameworkActivity, onPauseNative)
{
	CCL_PRINT ("FrameworkActivity::onPauseNative")

	// cancel drag session (if any)
	if(Window* window = Desktop.getLastWindow ())
		window->cancelDragSession ();

	// notify application of being deactivated
	GUI.onAppStateChanged (IApplication::kAppDeactivated);

	// save settings
	Settings::autoSaveAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkActivity, onStopNative, bool keepDialogsOpen)
{
	CCL_PRINT ("FrameworkActivity::onStopNative")

	// notify application of being suspended
	GUI.onAppStateChanged (IApplication::kAppSuspended);

	if(!keepDialogsOpen)
		Desktop.closeModalWindows ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (void, FrameworkActivity, onRestartNative)
{
	CCL_PRINT ("FrameworkActivity::onRestartNative")

	// notify application of being resumed
	GUI.onAppStateChanged (IApplication::kAppResumed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (void, FrameworkActivity, onDestroyNative)
{
	CCL_PRINT ("FrameworkActivity::onDestroyNative")

	FrameworkActivity* activity = FrameworkActivity::lookupNativeActivity (env, This);
	ASSERT (activity)

	if(FrameworkActivity::activityCount () == 1 && FrameworkActivity::isQuitting ())
	{
		// notify application about termination
		GUI.onAppStateChanged (IApplication::kAppTerminates);

		// call main function for shutdown (terminator functions, etc.)
		UnknownPtr<IAndroidSystem> androidSystem (&System::GetSystem ());
		ASSERT (androidSystem)
		if(androidSystem)
			androidSystem->callAndroidMain (false);

		// free the global graphics factory
		delete gGraphicsFactory;
		gGraphicsFactory = 0;
	}

	// reset current activity pointer
	if(activity == FrameworkActivity::getCurrentActivity ())
		FrameworkActivity::updateCurrentActivity (nullptr);

	// free the activity
	delete activity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkActivity, onSaveInstanceStateNative, jobject outState, bool keepDialogsOpen)
{
	CCL_PRINT ("FrameworkActivity::onSaveInstanceStateNative")

	if(!keepDialogsOpen)
		Desktop.closeModalWindows ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkActivity, onRestoreInstanceStateNative, jobject savedInstanceState)
{
	CCL_PRINT ("FrameworkActivity::onRestoreInstanceStateNative")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (void, FrameworkActivity, onConfigurationChangedNative)
{
	if(FrameworkActivity* activity = FrameworkActivity::lookupNativeActivity (env, This))
	{
		GUI.setInterfaceOrientation (activity->getOrientation ());

		// show/hide status and navigation bars
		activity->updateSystemUIVisibility ();
	}

	UnknownPtr<IAndroidSystem> androidSystem (&System::GetSystem ());
	ASSERT (androidSystem)
	if(androidSystem)
		androidSystem->onConfigurationChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (bool, FrameworkActivity, onKeyEventNative, bool isDown, int keyCode, int character, int modifiers, bool isRepeat)
{
	// 1.) look for popup window first
	Window* window = Desktop.getTopWindow (kPopupLayer);

	// 2.) look for dialog window
	if(!window)
		window = Desktop.getTopWindow (kDialogLayer);

	// 3.) get active window
	if(!window)
		window = Desktop.getActiveWindow ();

	// 4.) fallback to application window
	if(!window)
	{
		if(FrameworkActivity* activity = FrameworkActivity::lookupNativeActivity (env, This))
			if(FrameworkView* view = activity->getContentView ())
				window = view->getWindow ();
	}

	// exit if no window found
	if(!window)
		return false;

	// process key event
	KeyEvent event (isDown ? KeyEvent::kKeyDown : KeyEvent::kKeyUp);
	VKey::makeKeyEvent (event, keyCode, character, modifiers, isRepeat);

	#if DEBUG_LOG
	if(isDown)
	{
		String s; event.toString (s);
		Debugger::printf ("Key: %s\n", MutableCString (s).str ());
	}
	#endif
	return isDown ? window->onKeyDown (event) : window->onKeyUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (bool, FrameworkActivity, onBackPressedNative)
{
	// 1.) close topmost (modal) dialog
	if(Dialog* dialog = ccl_cast<Dialog> (Desktop.getTopWindow (kDialogLayer)))
	{
		dialog->deferClose ();
		CCL_PRINTLN ("Back: close dialog")
		return true;
	}

	// 2.) close topmost popup (non-modal, otherwise handled as dialog above)
	if(Window* popup = Desktop.getTopWindow (kPopupLayer))
	{
		if(UnknownPtr<IPopupSelectorWindow> popupWindow = popup->asUnknown ())
		{
			CCL_PRINTLN ("Back: close popup")
			popupWindow->closePopup ();
			return true;
		}
	}

	// 3.) close a "sheet style" window
	Window* window = Desktop.getLastWindow ();
	if(window && window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle) && window != Desktop.getApplicationWindow ())
	{
		CCL_PRINTLN ("Back: close sheet window")
		window->deferClose ();
		return true;
	}
	else
	{
		CCL_PRINTLN ("Back: command \"Navigation - Back\"")
		if(window)
			window->cancelDragSession ();

		CommandMsg msg ("Navigation", "Back");
		return CommandTable::instance ().interpretCommand (msg);
	}
}
