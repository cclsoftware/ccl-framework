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
// Filename    : ccl/platform/cocoa/iosapp/scenedelegate.mm
// Description : iOS Scene Delegate
//
//************************************************************************************************

#include "ccl/platform/cocoa/iosapp/scenedelegate.h"
#include "ccl/platform/cocoa/iosapp/appdelegate.h"
#include "ccl/platform/cocoa/iosapp/mainviewcontroller.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/gui/gui.h"

#include "ccl/public/gui/iapplication.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/url.h"

using namespace CCL;

//************************************************************************************************
// SceneDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (SceneDelegate)

@synthesize window = _window;

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)connectionOptions
{
	if(![scene isKindOfClass:[UIWindowScene class]])
		return;

	UIWindowScene* windowScene = (UIWindowScene*)scene;

	// Create the window, sized and positioned by the scene
	self.window = [[[UIWindow alloc] initWithWindowScene:windowScene] autorelease];
	self.window.autoresizesSubviews = YES;
	self.window.backgroundColor = [UIColor blackColor];

	// Add the main view controller's view to the window and display.
	// The UIWindow will resize it to fill the screen except the status bar area
	CCL_ISOLATED (MainViewController)* mainViewController = [[[CCL_ISOLATED (MainViewController) alloc] init] autorelease];

	// a cold launch via URL needs to call uiInitializedHandler
	if(connectionOptions.URLContexts.count > 0)
	{
		NSSet<UIOpenURLContext*>* urlContexts = connectionOptions.URLContexts;
		mainViewController->uiInitializedHandler = [^{
			[self scene:scene openURLContexts:urlContexts];
		} copy];
	}

	self.window.rootViewController = mainViewController;
	[self.window makeKeyAndVisible];

	// mirror the window on the app delegate: it outlives this scene delegate and some platform code still reaches the window through it
	CCL_ISOLATED (ApplicationDelegate)* appDelegate = (CCL_ISOLATED (ApplicationDelegate)*)[UIApplication sharedApplication].delegate;
	appDelegate.window = self.window;

	// init orientation
	CCL::OrientationType orientation = UIInterfaceOrientationIsPortrait (windowScene.interfaceOrientation) ? CCL::Styles::kPortrait : CCL::Styles::kLandscape;
	GUI.setInterfaceOrientation (orientation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)scene:(UIScene*)scene openURLContexts:(NSSet<UIOpenURLContext*>*)URLContexts
{
	for(UIOpenURLContext* context in URLContexts)
	{
		Url cclUrl;
		MacUtils::urlFromNSUrl (cclUrl, context.URL, IUrl::kFile, true);
		if(auto* application = GUI.getApplication ())
			application->openFile (cclUrl);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)sceneDidBecomeActive:(UIScene*)scene
{
	GUI.onAppStateChanged (IApplication::kAppActivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)sceneWillResignActive:(UIScene*)scene
{
	Settings::autoSaveAll ();
	GUI.onAppStateChanged (IApplication::kAppDeactivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)sceneDidEnterBackground:(UIScene*)scene
{
	CCL_ISOLATED (ApplicationDelegate)* appDelegate = (CCL_ISOLATED (ApplicationDelegate)*)[UIApplication sharedApplication].delegate;
	if([appDelegate suspendsInBackground])
		GUI.onAppStateChanged (IApplication::kAppSuspended);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)sceneWillEnterForeground:(UIScene*)scene
{
	CCL_ISOLATED (ApplicationDelegate)* appDelegate = (CCL_ISOLATED (ApplicationDelegate)*)[UIApplication sharedApplication].delegate;
	if([appDelegate suspendsInBackground])
		GUI.onAppStateChanged (IApplication::kAppResumed);
}

@end
