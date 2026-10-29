//************************************************************************************************
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
// Filename    : ccl/extras/audio/runtime/audiotransport.cpp
// Description : Audio Transport
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audiotransport.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define CHECK_TRANSPORT_ACTIVE \
ASSERT (isActive ()) \
if(!isActive ()) \
	return kResultUnexpected;

#define CHECK_MAINTHREAD_POLICY \
ASSERT (System::IsInMainThread ()) \
if(!System::IsInMainThread ()) \
	return kResultWrongThread;

//************************************************************************************************
// AudioTransport
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioTransport, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioTransport::~AudioTransport ()
{
	ASSERT (clients.isEmpty ())
	ASSERT (observers.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::setClock (IAudioTransportClock* _clock)
{
	if(clock != _clock)
	{
		clock = _clock;		
		lastReportedTime = kUndefinedTime;
		lastReportedDuration = kUndefinedTime;

		if(active == true)
			syncTime ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds CCL_API AudioTransport::getCurrentTime () const
{
	if(seekOverride == true)
		return seekOverrideTime;

	return clock ? clock->getCurrentTime () : 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds CCL_API AudioTransport::getMaxDuration () const
{		
	return clock ? clock->getMaxDuration () : 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::setActive (bool state)
{
	if(active != state)
	{
		if(active) // deactivating
		{
			stop ();
		}

		active = state;
		lastReportedTime = kUndefinedTime;

		if(!active)
		{
			seekOverride = false;
			seekOverrideTime = 0.;
		}

		if(state != isTimerEnabled ())
		{
			if(state)
				startTimer (kUpdateInterval);
			else
				stopTimer ();
		}

		if(active)
		{
			syncState ();
			syncTime ();
		}

		updateObserverState ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioTransport::isActive () const
{
	return active;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::start ()
{
	CHECK_TRANSPORT_ACTIVE
	CHECK_MAINTHREAD_POLICY

	if(!isPlaying ())
	{
		signalClients ({AudioTransportEvent::kStarted, getCurrentTime ()});
		syncState ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::stop ()
{
	CHECK_TRANSPORT_ACTIVE
	CHECK_MAINTHREAD_POLICY

	setRecordActive (false);

	if(isPlaying ())
	{
		signalClients ({AudioTransportEvent::kStopped, getCurrentTime ()});
		syncState ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioTransport::isPlaying () const
{
	return playing;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::seek (TSeconds _time)
{
	CHECK_TRANSPORT_ACTIVE
	CHECK_MAINTHREAD_POLICY

	if(isRecording ())
		return kResultFalse;

	TSeconds newTime = ccl_bound (_time, 0., getMaxDuration ());
	if(newTime != getCurrentTime ())
	{
		if(seekOverride == true)
			seekOverrideTime = newTime;
		else
			signalClients ({AudioTransportEvent::kSeek, newTime});

		syncState ();
		syncTime ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::setSeekOverride (tbool state)
{
	CHECK_TRANSPORT_ACTIVE
	CHECK_MAINTHREAD_POLICY

	bool newState = state != 0; 
	if(newState != seekOverride)
	{
		if(seekOverride)
			seekOverrideTime = 0.;

		seekOverride = newState;
		
		if(seekOverride)
			seekOverrideTime = getCurrentTime ();

		updateObserverState ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioTransport::isSeekOverride () const
{
	return seekOverride;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::setRecordActive (tbool state)
{
	CHECK_TRANSPORT_ACTIVE
	CHECK_MAINTHREAD_POLICY

	if(state != isRecording ())
	{
		signalClients ({state ? AudioTransportEvent::kRecordStarted : AudioTransportEvent::kRecordStopped,
						getCurrentTime ()});
		syncState ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioTransport::isRecording () const
{
	return recording;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::addClient (IAudioTransportClient* client)
{
	CHECK_MAINTHREAD_POLICY

	ASSERT (client && !clients.contains (client))
	if(!client || clients.contains (client))
		return kResultInvalidArgument;

	clients.add (client);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::removeClient (IAudioTransportClient* client)
{
	CHECK_MAINTHREAD_POLICY

	ASSERT (client && clients.contains (client))
	if(!client || !clients.contains (client))
		return kResultInvalidArgument;

	clients.remove (client);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::addObserver (IAudioTransportObserver* observer)
{
	CHECK_MAINTHREAD_POLICY

	ASSERT (observer && !observers.contains (observer))
	if(!observer || observers.contains (observer))
		return kResultInvalidArgument;

	observers.add (observer);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioTransport::removeObserver (IAudioTransportObserver* observer)
{
	CHECK_MAINTHREAD_POLICY

	ASSERT (observer && observers.contains (observer))
	if(!observer || !observers.contains (observer))
		return kResultInvalidArgument;

	observers.remove (observer);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::signalClients (const AudioTransportEvent& e)
{
	for(IAudioTransportClient* client : clients)
		client->onTransportEvent (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::updateObserverState ()
{
	for(IAudioTransportObserver* observer : observers)
		observer->onTransportStateChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::updateObserverTime (TSeconds time)
{
	for(IAudioTransportObserver* observer : observers)
		observer->onTransportTimeChanged (time);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::updateObserverDuration (TSeconds duration)
{
	for(IAudioTransportObserver* observer : observers)
		observer->onTransportDurationChanged (duration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::syncState ()
{
	bool anyRecording = false, anyPlaying = false;
	for(IAudioTransportClient* client : clients)
	{
		int state = client->getClientState ();
		if(get_flag<int> (state, IAudioTransportClient::kPlaying))
			anyPlaying = true;
		if(get_flag<int> (state, IAudioTransportClient::kRecording))
			anyRecording = true;
	}

	if(anyRecording != recording || anyPlaying != playing)
	{
		playing = anyPlaying;
		recording = anyRecording;
		updateObserverState ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::syncTime ()
{
	TSeconds duration = getMaxDuration ();
	if(duration != lastReportedDuration)
	{
		lastReportedDuration = duration;
		updateObserverDuration (duration);
	}

	TSeconds time = getCurrentTime ();
	if(time != lastReportedTime)
	{
		lastReportedTime = time;
		updateObserverTime (time);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioTransport::onIdleTimer ()
{
	syncState ();
	syncTime ();
}
