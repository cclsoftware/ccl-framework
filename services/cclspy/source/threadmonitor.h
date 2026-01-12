//************************************************************************************************
//
// CCL Spy
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
// Filename    : threadmonitor.h
// Description : Thread Monitor
//
//************************************************************************************************

#ifndef _threadmonitor_h
#define _threadmonitor_h

#include "ccl/app/component.h"

#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {

class ThreadItemModel;

//************************************************************************************************
// ThreadMonitor
//************************************************************************************************

class ThreadMonitor: public Component,
					 public IdleClient
{
public:
	DECLARE_CLASS (ThreadMonitor, Component)

	ThreadMonitor ();
	~ThreadMonitor ();

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (ITimerTask, Component)

protected:
	ThreadItemModel* threadModel;
	IParameter* idleValue;

	// IdleClient
	void onIdleTimer () override;
};

} // namespace CCL

#endif // _threadmonitor_h
