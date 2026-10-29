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
// Filename    : ccl/platform/cocoa/iosapp/appdelegate.h
// Description : iOS app delegate (could be replaced by your own version)
//
//************************************************************************************************

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

//************************************************************************************************
// ApplicationDelegate
//************************************************************************************************

@interface CCL_ISOLATED (ApplicationDelegate) : NSObject <UIApplicationDelegate>
{}

- (id)init;
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;
- (UISceneConfiguration*)application:(UIApplication*)application configurationForConnectingSceneSession:(UISceneSession*)connectingSceneSession options:(UISceneConnectionOptions*)options;
- (void)application:(UIApplication*)application didDiscardSceneSessions:(NSSet<UISceneSession*>*)sceneSessions;
- (void)applicationWillTerminate:(UIApplication*)application;
- (void)receivedMemoryWarning:(NSNotification*)note;
- (void)application:(UIApplication*)application handleEventsForBackgroundURLSession:(NSString*)identifier completionHandler:(void(^)(void))completionHandler;

- (BOOL)suspendsInBackground;

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, assign) UIBackgroundTaskIdentifier backgroundTask;
@property (nonatomic, retain) NSArray* backgroundModes;

@end

#include "ccl/public/base/ccldefpop.h"
