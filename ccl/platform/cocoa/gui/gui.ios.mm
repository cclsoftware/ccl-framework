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
// Filename    : ccl/platform/cocoa/gui/gui.ios.mm
// Description : platform-specific GUI implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/url.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/views/view.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/systemevent.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/window.h"
#include "ccl/app/application.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/iosapp/appdelegate.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

extern void linkIOSPopupMenu (); // link menu.ios.mm
extern void linkIOSPrintservice (); // link printservice.ios.mm

using namespace CCL;

NSAutoreleasePool* pool = nil;

namespace CCL {

//************************************************************************************************
// IOSSystemTimer
//************************************************************************************************

class IOSSystemTimer: public SystemTimer
{
public:
	IOSSystemTimer (unsigned int period);
	~IOSSystemTimer ();
};

//************************************************************************************************
// IOSUserInterface
//************************************************************************************************

class IOSUserInterface: public UserInterface
{
public:
	IOSUserInterface ();

protected:
	// UserInterface
	bool startupPlatform (ModuleRef module) override;
	void shutdownPlatform () override;

	int CCL_API runEventLoop () override;
	bool detectDoubleClick (View* view, const Point& where) override;
	double CCL_API getDoubleClickDelay () const override;
	void realizeActivityMode (ActivityMode mode) override;
	void onNetworkActivity (bool state) override; 
	ITimer* CCL_API createTimer (unsigned int period) const override;
	
	void allowedOrientationsChanged () override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSUserInterface iosGUI;
UserInterface& GUI = iosGUI;

} // namespace CCL


//************************************************************************************************
// IOSUserInterface
//************************************************************************************************

IOSUserInterface::IOSUserInterface ()
{
	applicationType = kMobileApplication;
	buttonOrder = Styles::kAffirmativeButtonRight;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSUserInterface::startupPlatform (ModuleRef module)
{
	linkIOSPopupMenu ();
	linkIOSPrintservice ();
	
	pool = [[NSAutoreleasePool alloc] init];

	MultiResolutionBitmap::ResolutionNamingMode resolution = Bitmap::kStandardResolution;
	if([UIScreen mainScreen].scale == 2.0)
		resolution = Bitmap::kHighResolution;
	else if([UIScreen mainScreen].scale == 3.0)
		resolution = Bitmap::kExtraHighResolution;
	Bitmap::setResolutionNamingMode (resolution);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSUserInterface::shutdownPlatform ()
{
	[pool release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int IOSUserInterface::runEventLoop ()
{
	if(!finishStartup ())
		return kExitError;

	ScopedVar<bool> scope (eventLoopRunning, true);
	char* argv = nullptr;
	UIApplicationMain (0, &argv, nil, NSStringFromClass ([CCL_ISOLATED (ApplicationDelegate) class]));
	return exitCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSUserInterface::detectDoubleClick (View* view, const Point& where)
{
	if(doubleClicked >= 0)
		return doubleClicked == 1;

	doubleClicked = kDoubleClickFalse;
	return doubleClicked == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API IOSUserInterface::getDoubleClickDelay () const
{
	// UITapGestureRecognizer contains a private property _maximumIntervalBetweenSuccessiveTaps that has this value
	return .35;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSUserInterface::realizeActivityMode (ActivityMode mode)
{
	id delegate = [[UIApplication sharedApplication] delegate];
	ASSERT(delegate);
	switch(mode)
	{
	case ActivityMode::kNormal:
		[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
		break;

	case ActivityMode::kBackground:
		// it is currently not possible to enforce background operation on iOS (exception: apps with UIBackgroundModes=audio _when_ playing audio)
		[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
		break;

	case ActivityMode::kAlwaysOn:
		[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSUserInterface::onNetworkActivity (bool state)
{
	[UIApplication sharedApplication].networkActivityIndicatorVisible = state ? YES : NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer* CCL_API IOSUserInterface::createTimer (unsigned int period) const
{
	return NEW IOSSystemTimer (period);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSUserInterface::allowedOrientationsChanged ()
{
	if(@available (iOS 16, *))
		[[[UIApplication sharedApplication] delegate].window.rootViewController setNeedsUpdateOfSupportedInterfaceOrientations];	
}

//************************************************************************************************
// CASystemTimer
//************************************************************************************************

@interface CCL_ISOLATED (CASystemTimer) : NSObject
{
	@public
	NSTimer* nsTimer;
}

- (void)trigger:(NSTimer*)info;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (CASystemTimer)

- (void)trigger:(NSTimer*)info
{
	if(GUI.isTimerBlocked ())
		return;
	SystemTimer::trigger (self);
}

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSSystemTimer::IOSSystemTimer (unsigned int period)
: SystemTimer (period)
{
	id timer = [[CCL_ISOLATED (CASystemTimer) alloc] init];
	NSTimer* nsTimer = [NSTimer timerWithTimeInterval:period / 1000. target:timer selector:@selector(trigger:) userInfo:nil repeats:YES];
	NSRunLoop* runloop = [NSRunLoop mainRunLoop];
	[runloop addTimer:nsTimer forMode:NSRunLoopCommonModes];
	systemTimer = timer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSSystemTimer::~IOSSystemTimer ()
{
	if(systemTimer)
	{
		CCL_ISOLATED (CASystemTimer)* timer = (CCL_ISOLATED (CASystemTimer)*) systemTimer;
		[timer->nsTimer invalidate];
		systemTimer = nil;
	}
}
