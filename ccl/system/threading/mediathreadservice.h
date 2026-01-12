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
// Filename    : ccl/system/threading/mediathreadservice.h
// Description : Multimedia Threading Services
//
//************************************************************************************************

#ifndef _ccl_mediathreadservice_h
#define _ccl_mediathreadservice_h

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/imediathreading.h"
#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// MediaThreadService
//************************************************************************************************

class MediaThreadService: public Object,
						  public IMediaThreadService
{
public:
	MediaThreadService ();

	// IMediaThreadService
	tresult CCL_API startup () override;
	tresult CCL_API shutdown () override;
	double CCL_API getMediaTime () override;
	IMediaTimer* CCL_API createTimer (StringID name, IMediaTimerTask& task, uint32 period, int timerID, Threading::ThreadPriority priority) override;
	tresult CCL_API getThreadsSnapshot (Threading::ThreadInfo infos[], int& count) override;
	tresult CCL_API setPriorityHandler (IMediaThreadPriorityHandler* priorityHandler) override;
	IMediaThreadWorkgroupHandler* CCL_API getWorkgroupHandler () override;

	CLASS_INTERFACE (IMediaThreadService, Object)
};

//************************************************************************************************
// MediaTimer
//************************************************************************************************

class MediaTimer: public Object,
				  public IMediaTimer
{
public:
	MediaTimer (StringID name, IMediaTimerTask& task,
				uint32 period, 
				int timerID,
				Threading::ThreadPriority priority);
	~MediaTimer ();

	// IMediaTimer
	int CCL_API getTimerID () const override;
	tbool CCL_API isRunning () const override;
	void CCL_API start () override;
	void CCL_API stop () override;

	CLASS_INTERFACE (IMediaTimer, Object)

protected:
	CCL::MutableCString name;
	IMediaTimerTask& task;
	uint32 period;
	int timerID;
	Threading::ThreadPriority priority;
	Threading::IThread* thread;
	Threading::AtomicInt shouldExit;

	int run ();
	static int CCL_API threadEntry (void* arg);
};

} // namespace CCL

#endif // _ccl_mediathreadservice_h
