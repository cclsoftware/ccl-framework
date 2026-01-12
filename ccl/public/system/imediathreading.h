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
// Filename    : ccl/public/system/imediathreading.h
// Description : Multimedia Threading Services
//
//************************************************************************************************

#ifndef _ccl_imediathreading_h
#define _ccl_imediathreading_h

#include "ccl/public/system/ithreading.h"

namespace CCL {

interface IMediaTimer;
interface IMediaTimerTask;
interface IMediaThreadWorkgroupHandler;

//************************************************************************************************
// Threading::ThreadInfo
/** Thread information. 
	\ingroup ccl_system */
//************************************************************************************************

namespace Threading {

struct ThreadInfo
{
	ThreadID id;
	char name[128];
	ThreadPriority priority;
	int nativePriority;
	float activity;
	
	ThreadInfo ()
	: id (0),
	  priority (kPriorityNormal),
	  nativePriority (-1),
	  activity (0)
	{ name[0] = 0; }
};

} // namespace Threading

//************************************************************************************************
// IMediaThreadPriorityHandler
/** Handler to set realtime thread priorities for multimedia applications.
	On Windows, this can be MMCSS or a custom driver setting the priority in kernel mode.
	\ingroup ccl_system */
//************************************************************************************************

interface IMediaThreadPriorityHandler: IUnknown
{
	/** Set priority of calling thread to kPriorityRealtimeBase or greater. */
	virtual tresult CCL_API setSelfToRealtimePriority (Threading::ThreadPriority priority) = 0;

	/** Set priority of calling thread to absolute value (0..31 on Windows). */
	virtual tresult CCL_API setSelfToAbsolutePriority (int priority) = 0;

	/** Get priority of calling thread as absolute value (0..31 on Windows). */
	virtual tresult CCL_API getSelfAbsolutePriority (int& priority) = 0;

	DECLARE_IID (IMediaThreadPriorityHandler)
};

DEFINE_IID (IMediaThreadPriorityHandler, 0x87d19d0b, 0xbf84, 0x4b5e, 0x8f, 0x32, 0xe1, 0x11, 0x30, 0x38, 0x9a, 0x65)

//************************************************************************************************
// IMediaThreadService
/** Threading services for multimedia applications.
	\ingroup ccl_system */
//************************************************************************************************

interface IMediaThreadService: IUnknown
{
	/** Call before using services. */
	virtual tresult CCL_API startup () = 0;

	/** Call after using services. */
	virtual tresult CCL_API shutdown () = 0;

	/** Get time-stamp of multimedia timer in seconds. */
	virtual double CCL_API getMediaTime () = 0;
	
	/** Create high-resolution multimedia timer. */
	virtual IMediaTimer* CCL_API createTimer (StringID name, IMediaTimerTask& task, uint32 period, int timerID, 
											  Threading::ThreadPriority priority = Threading::kPriorityAboveNormal) = 0;
									
	/** Get snapshot of all threads known to the framework. */
	virtual tresult CCL_API getThreadsSnapshot (Threading::ThreadInfo infos[], int& count) = 0;

	/** Set thread priority handler to custom implementation. Pass null to reset to default framework handler. */
	virtual tresult CCL_API setPriorityHandler (IMediaThreadPriorityHandler* priorityHandler) = 0;

	/** Get thread workgroup handler. */
	virtual IMediaThreadWorkgroupHandler* CCL_API getWorkgroupHandler () = 0;
	
	DECLARE_IID (IMediaThreadService)
};

DEFINE_IID (IMediaThreadService, 0x69181dce, 0x7700, 0x4aa8, 0xae, 0xa3, 0x3, 0xb7, 0x1b, 0x9c, 0x6b, 0xc3)

//************************************************************************************************
// IMediaTimerTask
/** Task interface for multimedia timer.
	\ingroup ccl_system */
//************************************************************************************************

interface IMediaTimerTask: IUnknown
{
	/** Perform periodic task. */
	virtual void CCL_API task (int timerID, double systemTime) = 0; 

	DECLARE_IID (IMediaTimerTask)
};

DEFINE_IID (IMediaTimerTask, 0x3e7b88d7, 0x76ed, 0x4f41, 0x8a, 0x0, 0x33, 0x7c, 0x85, 0xd3, 0x7e, 0xa3)

//************************************************************************************************
// IMediaTimer
/** High-resolution multimedia timer.
	\ingroup ccl_system */
//************************************************************************************************

interface IMediaTimer: IUnknown
{
	/** Get timer identifier. */
	virtual int CCL_API getTimerID () const = 0;

	/** Check if timer is running. */
	virtual tbool CCL_API isRunning () const = 0;

	/** Start timer. */
	virtual void CCL_API start () = 0;

	/** Stop timer. */
	virtual void CCL_API stop () = 0;

	DECLARE_IID (IMediaTimer)
};

DEFINE_IID (IMediaTimer, 0x7c2c1bbf, 0x2b9a, 0x4b85, 0xa4, 0xb4, 0x1b, 0x6e, 0x69, 0x5e, 0xe8, 0x10)

//************************************************************************************************
// IMediaThreadWorkgroupHandler
/** Handler to add / remove threads to workgroups in multimedia applications.
	\ingroup ccl_system */
//************************************************************************************************

interface IMediaThreadWorkgroupHandler: IUnknown
{	
	/** Create workgroup. */
	virtual tresult CCL_API createWorkgroup (Threading::WorkgroupID& workgroup, StringID name) = 0;

	/** Release workgroup. */
	virtual tresult CCL_API releaseWorkgroup (Threading::WorkgroupID workgroup) = 0;
	
	/** Start workgroup interval. */
	virtual tresult CCL_API startWorkgroupInterval (Threading::WorkgroupID workgroup, double intervalSeconds) = 0;

	/** Finish workgroup interval. */
	virtual tresult CCL_API finishWorkgroupInterval (Threading::WorkgroupID workgroup) = 0;

	/** Join this thread to the workgroup. */
	virtual tresult CCL_API addSelfToWorkgroup (Threading::WorkgroupToken& token, Threading::WorkgroupID workgroup) = 0;

	/** Before exiting the thread, leave the workgroup. */
	virtual tresult CCL_API removeSelfFromWorkgroup (Threading::WorkgroupToken token, Threading::WorkgroupID workgroup) = 0;

	/** Optimal number of threads per workgroup. */
	virtual tresult CCL_API getMaxWorkgroupThreads (int& nThreads, Threading::WorkgroupID workgroup) = 0;
	
	DECLARE_IID (IMediaThreadWorkgroupHandler)
};

DEFINE_IID (IMediaThreadWorkgroupHandler, 0xbb93714c, 0xf81e, 0x437f, 0xa3, 0xcb, 0x5, 0x1, 0xfe, 0x2f, 0x47, 0xb0)

namespace Threading {

//************************************************************************************************
// WorkgroupJoinScope
//************************************************************************************************

struct WorkgroupJoinScope
{
	IMediaThreadWorkgroupHandler* workgroupHandler;
	WorkgroupID workgroup = nullptr;
	WorkgroupToken token = nullptr;

	WorkgroupJoinScope (IMediaThreadWorkgroupHandler* workgroupHandler, WorkgroupID _workgroup)
	: workgroupHandler (workgroupHandler),
	  workgroup (_workgroup)
	{
		if(workgroup && workgroupHandler)
			workgroupHandler->addSelfToWorkgroup (token, workgroup);
	}

	~WorkgroupJoinScope ()
	{
		if(workgroup && token && workgroupHandler)
			workgroupHandler->removeSelfFromWorkgroup (token, workgroup);
	}

	bool isError () const { return workgroup != nullptr && workgroupHandler != nullptr && token == nullptr; }
};

//************************************************************************************************
// WorkgroupIntervalScope
//************************************************************************************************

struct WorkgroupIntervalScope
{
	IMediaThreadWorkgroupHandler* workgroupHandler;
	WorkgroupID workgroup = nullptr;

	WorkgroupIntervalScope (IMediaThreadWorkgroupHandler* workgroupHandler, WorkgroupID _workgroup, double intervalSeconds)
	: workgroupHandler (workgroupHandler),
	  workgroup (_workgroup)
	{
		if(workgroup && workgroupHandler)
			workgroupHandler->startWorkgroupInterval (workgroup, intervalSeconds);
	}

	~WorkgroupIntervalScope ()
	{
		if(workgroup && workgroupHandler)
			workgroupHandler->finishWorkgroupInterval (workgroup);
	}
};

} // namespace Threading

} // namespace CCL

#endif // _ccl_imediathreading_h
