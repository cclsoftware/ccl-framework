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
// Filename    : ccl/public/gui/framework/idleclient.cpp
// Description : Helper for implementing an idle task
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/base/debug.h"

using namespace CCL;

//************************************************************************************************
// IdleClient
//************************************************************************************************

IdleClient::IdleClient ()
: nextTime (0),
  delay (0),
  periodic (false),
  timerEnabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IdleClient::~IdleClient ()
{
	stopTimer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IdleClient::startTimer (int64 _delay, bool _periodic)
{
	delay = _delay;
	periodic = _periodic;
	nextTime = delay ? System::GetSystemTicks () + delay : 0;

	if(!timerEnabled)
	{
		System::GetGUI ().addIdleTask (this);
		timerEnabled = true;
	}
	CCL_PRINTF ("IdleClient: startTimer (%" FORMAT_INT64"d, %s)\n", delay, periodic ? "periodic" : "once")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IdleClient::stopTimer ()
{
	if(timerEnabled)
	{
		System::GetGUI ().removeIdleTask (this);
		CCL_PRINTLN ("IdleClient: stopTimer")
		timerEnabled = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IdleClient::enableTimer (bool state)
{
	if(state != timerEnabled)
	{
		if(state)
			startTimer ();
		else
			stopTimer ();
	}
	return state == timerEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IdleClient::onTimer (ITimer*)
{
	if(delay)
	{
		int64 now = System::GetSystemTicks ();
		if(now >= nextTime)
		{
			if(periodic)
				nextTime = now + delay;
			else
				stopTimer ();

			CCL_PRINTLN ("IdleClient::onTimer")
			onIdleTimer ();
		}
	}
	else
	{
		if(!periodic)
			stopTimer ();

		CCL_PRINTLN ("IdleClient::onTimer")
		onIdleTimer ();
	}
}
