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
// Filename    : ccl/platform/mac/backgroundservice.mac.cpp
// Description : Mac Background Service
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/main/backgroundservice.h"

#include "ccl/main/cclargs.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include <mach/mach_time.h>

#undef interface

using namespace CCL;

//************************************************************************************************
// TimerCallback
//************************************************************************************************

@interface TimerCallback: NSObject
{
	BackgroundService* backgroundService;
	uint64_t lastOnIdle;
	double millisecondsFactor;
}
- (void)timerCallback:(NSObject*)info;
- (void)service;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation TimerCallback

- (instancetype)initWithService:(BackgroundService*) service
{
    self = [super init];
    if(self) {
		backgroundService = service;

		// precalculate factor to convert absolute time to milliseconds
		mach_timebase_info_data_t machClockInfo;
		mach_timebase_info (&machClockInfo);
		millisecondsFactor = (double)machClockInfo.numer / (double)machClockInfo.denom;
		millisecondsFactor *= 1e-6;

		lastOnIdle = 0;
    }
    return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)timerCallback:(NSObject*)info
{
	[self performSelectorOnMainThread:@selector(service) withObject:nil waitUntilDone:NO];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)service
{
	if(backgroundService && backgroundService->isHighPerformanceMode ())
	{
		// get called more often then BackgroundService::kIdlePeriod
		// throttle down
		uint64_t now = (uint64_t)(millisecondsFactor * mach_absolute_time ());
		bool idleTimeOut = now - lastOnIdle > BackgroundService::kIdlePeriod;

		if(idleTimeOut)
		{
			backgroundService->onIdle ();
			lastOnIdle = now;
		}
	}
	else
		backgroundService->onIdle ();
}

@end

//************************************************************************************************
// BackgroundService
//************************************************************************************************

bool BackgroundService::startPlatformService ()
{
	static bool quitRequested = false;
	
	struct Handler
	{
		static void onSignal (int sig)
		{
			quitRequested = true;
		}
	};
	
	::signal (SIGINT, Handler::onSignal);
	::signal (SIGTERM, Handler::onSignal);
	::signal (SIGKILL, Handler::onSignal);
	if(!startup (*g_ArgumentList))
		return false;
	
	TimerCallback* callBack = [[TimerCallback alloc] initWithService:this];

	// when executed from a daemon process NSTimer fires quite randomly
	// request to fire more often than kIdlePeriod and limit in TimerCallback
	static const int kPerformaceFactor = 4;
	bool timerIsHighPerformance = false;

	auto createTimer = [&] ()
	{
		int performanceFactor = isHighPerformanceMode () ? kPerformaceFactor : 1;
		float timerTime = (float)kIdlePeriod / (performanceFactor * 1000);
		CCL_PRINTF ("New BackgroundService Timer: %f\n", timerTime);
		NSTimer* timer = [[NSTimer scheduledTimerWithTimeInterval:timerTime target:callBack selector:@selector(timerCallback:) userInfo:nil repeats:true] retain];
		[timer setTolerance:0.005];
		return timer;
	};
	
	NSTimer* timer = createTimer ();
	NSRunLoop* runLoop = [NSRunLoop currentRunLoop];
	[runLoop addTimer:timer forMode:NSDefaultRunLoopMode];
	while(!quitRequested)
		@autoreleasepool
		{
			if(![runLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:1]]) // time out after one second
				break;
			
			if(timerIsHighPerformance != highPerformanceMode)
			{
				[timer invalidate];
				[timer release];
				timer = createTimer ();
				[runLoop addTimer:timer forMode:NSDefaultRunLoopMode];
				timerIsHighPerformance = highPerformanceMode;
			}
		}

	[timer invalidate];
	[timer release];
	[callBack release];
	
	shutdown ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundService::startDevelopmentService ()
{
	return startPlatformService (); // same same
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundService::flushPlatformUpdates ()
{
	[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundService::install (bool)
{
	return false;
}
