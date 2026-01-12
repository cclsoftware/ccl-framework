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
// Filename    : ccl/gui/system/systemtimer.h
// Description : System Timer
//
//************************************************************************************************

#ifndef _ccl_systemtimer_h
#define _ccl_systemtimer_h

#include "ccl/base/collections/objectlist.h"
#include "ccl/public/collections/vector.h"

#include "ccl/public/gui/framework/itimer.h"

namespace CCL {

//************************************************************************************************
// SystemTimer
/** Do not use this class directly (timers should be created via GUI.createTimer)! */
//************************************************************************************************

class SystemTimer:	public Object,
					public ITimer
{
public:
	SystemTimer (unsigned int period);
	~SystemTimer ();

	static void trigger (void* systemTimer);
	static void trigger (SystemTimer* timer);
	static void serviceTimers ();

	bool isKilled () const { return killed; }

	// ITimer
	void CCL_API task () override;
	void CCL_API kill () override;
	void CCL_API addTask (ITimerTask* task) override;
	void CCL_API removeTask (ITimerTask* task) override;

	CLASS_INTERFACE (ITimer, Object)

protected:
	LinkedList<ITimerTask*> tasks;
	typedef ListIterator<ITimerTask*> TasksIterator;
	Vector<TasksIterator*> tasksIterators; ///< stack of local iterators in nested task () calls
	void* systemTimer;
	bool killed;

	double period;
	double lastTriggerTime;

	static ObjectList timers;
};

} // namespace CCL

#endif // _ccl_systemtimer_h
