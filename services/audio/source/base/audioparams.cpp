//************************************************************************************************
//
// Audio Service
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : audioparams.cpp
// Description : Audio Parameters
//
//************************************************************************************************

#include "audioparams.h"

using namespace CCL;

//************************************************************************************************
// AudioParamQueue
//************************************************************************************************

bool AudioParamQueue::push (const AudioParamEvent& event)
{
	lock.lock ();

	bool result = true;
	AudioParamEvent* existing = content.findIf ([&] (const AudioParamEvent& e) { return e.tag == event.tag; });
	if(existing)
		*existing = event;
	else
		result = content.add (event);

	lock.unlock ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioParamQueue::popAll (EventList& events)
{
	if(!lock.tryLock ())
		return false;

	events = content;
	content.removeAll ();

	lock.unlock ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioParamQueue::removeAll ()
{
	lock.lock ();
	content.removeAll ();
	lock.unlock ();
}
