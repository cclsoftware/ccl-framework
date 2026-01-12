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
// Filename    : ccl/platform/cocoa/iosapp/appdelegate.mm
// Description : iOS app delegate
//
//************************************************************************************************

#include "ccl/platform/cocoa/iosapp/appdelegate.h"
#include "ccl/platform/cocoa/iosapp/mainviewcontroller.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/gui/gui.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/system/ierrorhandler.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/url.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

//************************************************************************************************
// ApplicationDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (ApplicationDelegate)

@synthesize window = _window;
@synthesize backgroundTask = _backgroundTask;
@synthesize backgroundModes = _backgroundModes;

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)init
{
    if(self = [super init])
	{
		_backgroundTask = UIBackgroundTaskInvalid;
		_backgroundModes = nil;
	}

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
	if(IApplicationProvider* appStartup = GUI.getApplicationProvider ())
		appStartup->onInit ();
		
	// Create the window (full screen size, including status bar area)
	CGRect screenSize = [[UIScreen mainScreen] bounds];
	self.window = [[[UIWindow alloc] initWithFrame:screenSize] autorelease];
	self.window.autoresizesSubviews = YES;
	self.window.backgroundColor = [UIColor blackColor];
	
	// hook in to memory warnings...
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector (receivedMemoryWarning:) name:UIApplicationDidReceiveMemoryWarningNotification object:nil];
			 	
	// Add the main view controller's view to the window and display.
	// The UIWindow will resize it to fill the screen except the status bar area 
    self.window.rootViewController = [[[CCL_ISOLATED (MainViewController) alloc] init] autorelease];
    [self.window makeKeyAndVisible];
	
	// init orientation
	UIInterfaceOrientation platformOrientation = [[UIApplication sharedApplication] statusBarOrientation];
	CCL::OrientationType orientation = UIInterfaceOrientationIsPortrait (platformOrientation) ? CCL::Styles::kPortrait : CCL::Styles::kLandscape;
	GUI.setInterfaceOrientation (orientation);

	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)application:(UIApplication*)app openURL:(NSURL*)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id>*)options;
{
	if(url)
	{
		Url cclUrl;
		MacUtils::urlFromNSUrl (cclUrl, url, IUrl::kFile, true);
		if(auto application = GUI.getApplication ())
			if(application->openFile (cclUrl))
			   return YES;
	}
	
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidBecomeActive:(UIApplication*)application
{
	GUI.onAppStateChanged (IApplication::kAppActivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationWillResignActive:(UIApplication*)application
{
	Settings::autoSaveAll ();
	GUI.onAppStateChanged (IApplication::kAppDeactivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidEnterBackground:(UIApplication*)application
{
	if([self suspendsInBackground])
		GUI.onAppStateChanged (IApplication::kAppSuspended);

}
//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationWillEnterForeground:(UIApplication*)application
{
	if([self suspendsInBackground])
		GUI.onAppStateChanged (IApplication::kAppResumed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationWillTerminate:(UIApplication*)application
{
	GUI.onAppStateChanged (IApplication::kAppTerminates);

	IApplication* cclApplication = GUI.getApplication ();
	ASSERT (cclApplication)	
	if(cclApplication)
		cclApplication->requestQuit ();

	GUI.onExit ();
	
    self.window = nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)receivedMemoryWarning:(NSNotification*)note
{
	SignalSource (Signals::kErrorHandler).signal (Message (Signals::kLowMemoryWarning));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)application:(UIApplication*)application handleEventsForBackgroundURLSession:(NSString*)identifier completionHandler:(void(^)(void))handler
{
	handler ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)suspendsInBackground
{
	if(self.backgroundModes == nil)
		self.backgroundModes = [[NSBundle mainBundle].infoDictionary objectForKey:@"UIBackgroundModes"];

	// FIXME: backgroundMode "audio" is not sufficient for NO, there also has to be an active audio device
	if([self.backgroundModes containsObject:@"audio"])
		return NO;
		
	if(self.backgroundTask != UIBackgroundTaskInvalid)
		return NO;

	return YES;
}

@end

