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
// Filename    : ccl/platform/cocoa/iosapp/mainviewcontroller.mm
// Description : iOS main view controller
//
//************************************************************************************************

#define DEBUG_LOG 0

#import "mainviewcontroller.h"

#include "ccl/public/base/debug.h"

#include "ccl/public/gui/framework/controlstyles.h"
#include "ccl/gui/gui.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/iosapp/appview.h"

//************************************************************************************************
// MainViewController
//************************************************************************************************

@implementation CCL_ISOLATED (MainViewController)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)loadView
{
	UIView* contentView = [[[CCL_ISOLATED (AppView) alloc] init] autorelease];
	self.view = contentView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	UIInterfaceOrientationMask mask = 0;
	
	if(CCL::GUI.isAllowedInterfaceOrientation (CCL::Styles::kPortrait))
		mask |= UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
	if(CCL::GUI.isAllowedInterfaceOrientation (CCL::Styles::kLandscape))
		mask |= UIInterfaceOrientationMaskLandscape;
	
	return mask;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	[super willRotateToInterfaceOrientation: toInterfaceOrientation duration:duration];
	CCL_PRINTF ("willRotateToInterfaceOrientation %d\n", toInterfaceOrientation)

	// rebuilding the UI while in background is not possible with Metal, there is another call after activation anyway
	if([[UIApplication sharedApplication] applicationState] != UIApplicationStateBackground)
	{
		CCL::OrientationType orientation = UIInterfaceOrientationIsPortrait (toInterfaceOrientation) ? CCL::Styles::kPortrait : CCL::Styles::kLandscape;
		CCL::GUI.setInterfaceOrientation (orientation);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromOrientation
{
	[super didRotateFromInterfaceOrientation: fromOrientation];
	CCL_PRINTF ("didRotate from %d to %d\n", fromOrientation, self.interfaceOrientation)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewWillLayoutSubviews
{
	UIView* appView = (UIView*)self.view;
	
    CCL::Rect size;
	CCL::MacOS::fromCGRect (size, appView.bounds);
		
	[appView setSize:size];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UIStatusBarStyle)preferredStatusBarStyle
{
	if(applicationView)
	{
		CCL::UnknownPtr<CCL::IObject> object (applicationView->getIWindow ());
		if(object.isValid ())
		{
			CCL::Variant style;
			if(object->getProperty (style, CCL::IWindow::kStatusBarStyle))
			{
				if(style.asInt () == CCL::IWindow::kDarkContent)
					return UIStatusBarStyleDarkContent;
			}
		}
	}

	return UIStatusBarStyleLightContent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)prefersHomeIndicatorAutoHidden
{
	bool hideHomeIndicator = false;
	CCL::Configuration::Registry::instance ().getValue (hideHomeIndicator, "CCL.iOS", "HideHomeIndicator");

	return hideHomeIndicator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewDidAppear:(BOOL)animated
{
	if(!applicationView)
	{
		CCL_ISOLATED (AppView)* appView = (CCL_ISOLATED (AppView)*)self.view;
		applicationView = [appView createApplicationView:CCL::GUI.getApplication ()];
		CCL::GUI.onAppStateChanged (CCL::IApplication::kUIInitialized);
	}
}

@end
