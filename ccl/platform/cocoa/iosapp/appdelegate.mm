//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
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
#include "ccl/platform/cocoa/iosapp/scenedelegate.h"

#include "ccl/gui/gui.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/system/ierrorhandler.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"

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

	// hook in to memory warnings...
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector (receivedMemoryWarning:) name:UIApplicationDidReceiveMemoryWarningNotification object:nil];

	// window creation and UI setup happen per-scene, see SceneDelegate's scene:willConnectToSession:options:
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UISceneConfiguration*)application:(UIApplication*)application configurationForConnectingSceneSession:(UISceneSession*)connectingSceneSession options:(UISceneConnectionOptions*)options
{
	// scenes are configured entirely in code so there is no UIApplicationSceneManifest in Info.plist
	UISceneConfiguration* configuration = [[[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role] autorelease];
	configuration.delegateClass = [CCL_ISOLATED (SceneDelegate) class];
	return configuration;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)application:(UIApplication*)application didDiscardSceneSessions:(NSSet<UISceneSession*>*)sceneSessions
{
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

