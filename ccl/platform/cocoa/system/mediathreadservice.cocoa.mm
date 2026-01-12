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
// Filename    : ccl/platform/cocoa/system/mediathreadservice.cocoa.mm
// Description : OSX/iOS Multimedia Threading Services
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/threading/mediathreadservice.h"

#include "ccl/base/singleton.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/public/system/ithreading.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/base/ccldefpush.h"
#include <mach/mach_time.h>
#include <MacTypes.h>
#import <AudioToolbox/AudioToolbox.h>
#include <os/workgroup.h>
#include "ccl/public/base/ccldefpop.h"

namespace CCL {

//************************************************************************************************
// CocoaThreadWorkgroupHandler
//************************************************************************************************

class CocoaThreadWorkgroupHandler :	public Object,
									public IMediaThreadWorkgroupHandler,
									public StaticSingleton<CocoaThreadWorkgroupHandler>
{
public:
	// IMediaThreadWorkgroupHandler
	tresult CCL_API createWorkgroup (Threading::WorkgroupID& workgroup, StringID name) override;
	tresult CCL_API releaseWorkgroup (Threading::WorkgroupID workgroup) override;
	tresult CCL_API startWorkgroupInterval (Threading::WorkgroupID workgroup, double intervalSeconds) override;
	tresult CCL_API finishWorkgroupInterval (Threading::WorkgroupID workgroup) override;
	tresult CCL_API addSelfToWorkgroup (Threading::WorkgroupToken& token, Threading::WorkgroupID workgroup) override;
	tresult CCL_API removeSelfFromWorkgroup (Threading::WorkgroupToken token, Threading::WorkgroupID workgroup) override;
	tresult CCL_API getMaxWorkgroupThreads (int& nThreads, Threading::WorkgroupID workgroup) override;
	
	CLASS_INTERFACE (IMediaThreadWorkgroupHandler, Object)
	
protected:
	static const double kIntervalFactor;
};

//************************************************************************************************
// CocoaMediaThreadService
//************************************************************************************************

class CocoaMediaThreadService: public MediaThreadService
{
public:
	static double machTimeToSeconds (uint64_t time);
	static uint64_t secondsToMachTime (double time);
	
	// IMediaThreadService
	double CCL_API getMediaTime ();
	IMediaThreadWorkgroupHandler* CCL_API getWorkgroupHandler ();
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IMediaThreadService& CCL_API System::CCL_ISOLATED (GetMediaThreadService) ()
{
	static CocoaMediaThreadService theMediaThreadService;
	return theMediaThreadService;
}

//************************************************************************************************
// CocoaThreadWorkgroupHandler
//************************************************************************************************

DEFINE_SINGLETON (CocoaThreadWorkgroupHandler)

const double CocoaThreadWorkgroupHandler::kIntervalFactor = .5;

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaThreadWorkgroupHandler::createWorkgroup (Threading::WorkgroupID& workgroup, StringID name)
{
	os_clockid_t clockId = OS_CLOCK_MACH_ABSOLUTE_TIME;
	workgroup = AudioWorkIntervalCreate (name, clockId, nullptr);
	return workgroup ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaThreadWorkgroupHandler::releaseWorkgroup (Threading::WorkgroupID workgroup)
{
	if(workgroup)
		CFRelease (static_cast<os_workgroup_t> (workgroup));
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaThreadWorkgroupHandler::startWorkgroupInterval (Threading::WorkgroupID workgroup, double intervalSeconds)
{
	uint64 start = mach_absolute_time ();
	uint64 deadline = start + CocoaMediaThreadService::secondsToMachTime (intervalSeconds * kIntervalFactor);
	
	tresult result = os_workgroup_interval_start (static_cast<os_workgroup_interval_t> (workgroup), start, deadline, nullptr) == noErr ? kResultOk : kResultFalse;
	ASSERT (result == kResultOk)
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaThreadWorkgroupHandler::finishWorkgroupInterval (Threading::WorkgroupID workgroup)
{
	tresult	result = os_workgroup_interval_finish (static_cast<os_workgroup_interval_t> (workgroup), nullptr) == noErr ? kResultOk : kResultFalse;
	ASSERT (result == kResultOk)
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaThreadWorkgroupHandler::addSelfToWorkgroup (Threading::WorkgroupToken& token, Threading::WorkgroupID workgroup)
{
	// prevent crash in os_workgroup_leave when joining a non-real time thread
	if(AutoPtr<Threading::IThread> thread = System::CreateThreadSelf ())
		if(thread->getPriority () <= Threading::kPriorityTimeCritical)
			return kResultFalse;
	
	token = NEW os_workgroup_join_token_s;
	CCL_PRINTF ("%d : addSelfToWorkgroup : %p token: %p\n", System::GetThreadSelfID (), workgroup, token)
	OSStatus status = -1;
	static const int kMaxRetries = 5;
	for(int i = 0; i < kMaxRetries; i++)
	{
		status = os_workgroup_join (static_cast<os_workgroup_t> (workgroup), static_cast<os_workgroup_join_token_t> (token));
		if(status == noErr)
			break;
		CCL_PRINTF ("os_workgroup_join failed, retry #%d\n", i + 1)
		usleep (1000);
	}
	
	tresult result = status == noErr ? kResultOk : kResultFalse;
	
	ASSERT (result == kResultOk)
	if(result != kResultOk)
	{
		delete static_cast<os_workgroup_join_token_s*> (token);
		token = nullptr;
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaThreadWorkgroupHandler::removeSelfFromWorkgroup (Threading::WorkgroupToken token, Threading::WorkgroupID workgroup)
{
	CCL_PRINTF ("%d : removeSelfFromWorkgroup : %p token: %p\n", System::GetThreadSelfID (), workgroup, token)
	os_workgroup_leave (static_cast<os_workgroup_t> (workgroup), static_cast<os_workgroup_join_token_t> (token));
	delete static_cast<os_workgroup_join_token_s*> (token);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaThreadWorkgroupHandler::getMaxWorkgroupThreads (int& nThreads, Threading::WorkgroupID workgroup)
{
	nThreads = os_workgroup_max_parallel_threads (static_cast<os_workgroup_t> (workgroup), nullptr);
	return kResultOk;
}

//************************************************************************************************
// CocoaMediaThreadService
//************************************************************************************************

double CocoaMediaThreadService::machTimeToSeconds (uint64_t time)
{
	mach_timebase_info_data_t theTimeBaseInfo;
	mach_timebase_info (&theTimeBaseInfo);
	return (double)time * theTimeBaseInfo.numer / theTimeBaseInfo.denom / 1000000000.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint64_t CocoaMediaThreadService::secondsToMachTime (double time)
{
	mach_timebase_info_data_t theTimeBaseInfo;
	mach_timebase_info (&theTimeBaseInfo);
	return (uint64_t)(time * 1000000000. * theTimeBaseInfo.denom / theTimeBaseInfo.numer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API CocoaMediaThreadService::getMediaTime ()
{
	uint64_t theTime = mach_absolute_time ();
	return machTimeToSeconds (theTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool isProcessTranslated ()
{
	Attributes computerInfo;
	System::GetSystem ().getComputerInfo (computerInfo, System::kQueryExtendedComputerInfo);
	return computerInfo.getBool (System::kProcessIsTranslated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMediaThreadWorkgroupHandler* CCL_API CocoaMediaThreadService::getWorkgroupHandler ()
{
	return &CocoaThreadWorkgroupHandler::instance ();
}
