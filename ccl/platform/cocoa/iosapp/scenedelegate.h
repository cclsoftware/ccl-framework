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
// Filename    : ccl/platform/cocoa/iosapp/scenedelegate.h
// Description : iOS Scene Delegate (could be replaced by your own version)
//
//************************************************************************************************

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

//************************************************************************************************
// SceneDelegate
//************************************************************************************************

@interface CCL_ISOLATED (SceneDelegate) : NSObject <UIWindowSceneDelegate>
{}

- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)connectionOptions;
- (void)scene:(UIScene*)scene openURLContexts:(NSSet<UIOpenURLContext*>*)URLContexts;
- (void)sceneDidBecomeActive:(UIScene*)scene;
- (void)sceneWillResignActive:(UIScene*)scene;
- (void)sceneDidEnterBackground:(UIScene*)scene;
- (void)sceneWillEnterForeground:(UIScene*)scene;

@property (nonatomic, retain) UIWindow* window;

@end

#include "ccl/public/base/ccldefpop.h"
