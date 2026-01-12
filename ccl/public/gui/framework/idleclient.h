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
// Filename    : ccl/public/gui/framework/idleclient.h
// Description : Helper for implementing an idle task
//
//************************************************************************************************

#ifndef _ccl_idleclient_h
#define _ccl_idleclient_h

#include "ccl/public/gui/framework/itimer.h"

namespace CCL {

//************************************************************************************************
// IdleClient
/** Helper mixin class for implementing an idle task. Override onIdleTimer () to do the work. 
	\ingroup gui */
//************************************************************************************************

class IdleClient: public ITimerTask
{
public:
	IdleClient ();
	~IdleClient ();

	void startTimer (int64 delay = 0, bool periodic = true);
	void stopTimer ();

	bool isTimerEnabled () const { return timerEnabled; }
	bool enableTimer (bool state);

	// ITimerTask
	void CCL_API onTimer (ITimer*) override;

protected:
	virtual void onIdleTimer () {} ///< override to perform the periodic action
	int64 delay;

private:
	int64 nextTime;
	bool periodic;
	bool timerEnabled;
};

} // namespace CCL

#endif // _ccl_idleclient_h
