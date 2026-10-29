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
// Filename    : ccl/extras/audio/meter/audiometerbuffer.cpp
// Description : Audio Meter Buffer
//
//************************************************************************************************

#include "ccl/extras/audio/meter/audiometerbuffer.h"

#include "ccl/base/message.h"

using namespace CCL;

//************************************************************************************************
// AudioMeterSettings
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioMeterSettings, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setPeakDecayRate (float dbPerSecond)
{
	if(peakDecayRate != dbPerSecond)
	{
		peakDecayRate = dbPerSecond;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setRMSSmoothingTime (TSeconds seconds)
{
	if(rmsSmoothingTime != seconds)
	{
		rmsSmoothingTime = seconds;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setClipHoldTime (TSeconds seconds)
{
	if(clipHoldTime != seconds)
	{
		clipHoldTime = seconds;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setPeakHoldTime (TSeconds seconds)
{
	if(peakHoldTime != seconds)
	{
		peakHoldTime = seconds;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setRMSEnabled (bool state)
{
	if(rmsEnabled != state)
	{
		rmsEnabled = state;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setClipEnabled (bool state)
{
	if(clipEnabled != state)
	{
		clipEnabled = state;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setPeakHoldEnabled (bool state)
{
	if(peakHoldEnabled != state)
	{
		peakHoldEnabled = state;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterSettings::setRange (float newMinDb, float newMaxDb)
{
	if(minDb != newMinDb || maxDb != newMaxDb)
	{
		minDb = newMinDb;
		maxDb = newMaxDb;
		signal (Message (kChanged));
	}
}

//************************************************************************************************
// AudioMeterBuffer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioMeterBuffer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioMeterBuffer::~AudioMeterBuffer ()
{
	ASSERT (clients.isEmpty ())
	setSettings (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioMeterBuffer::addClient (IAudioMeterClient* client)
{
	ASSERT (client && !clients.contains (client))
	if(!client || clients.contains (client))
		return false;

	clients.add (client);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioMeterBuffer::removeClient (IAudioMeterClient* client)
{
	ASSERT (client && clients.contains (client))
	if(!client || !clients.contains (client))
		return false;

	clients.remove (client);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterBuffer::signalDataChanged ()
{
	for(IAudioMeterClient* client : clients)
		client->meterDataChanged (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterBuffer::signalSettingsChanged ()
{
	for(IAudioMeterClient* client : clients)
		client->meterSettingsChanged (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterBuffer::setChannelCount (int channelCount)
{
	if(ballistics.setChannelCount (channelCount))
		signalDataChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioMeterBuffer::update (const AudioMeterData& rawData, TSeconds elapsedSeconds)
{
	bool changed = ballistics.update (rawData, elapsedSeconds);
	if(changed)
		signalDataChanged ();
	return changed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioMeterBuffer::reset ()
{
	bool changed = ballistics.reset ();
	if(changed)
		signalDataChanged ();
	return changed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioMeterBuffer::resetHolds ()
{
	bool changed = ballistics.resetHolds ();
	if(changed)
		signalDataChanged ();
	return changed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AudioMeterBuffer::getPeakLevelDb (int channel) const
{
	ASSERT (channel >= 0 && channel < getChannelCount ())
	return ballistics.getPeakLevelDb (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AudioMeterBuffer::getRMSLevelDb (int channel) const
{
	ASSERT (channel >= 0 && channel < getChannelCount ())
	return ballistics.getRMSLevelDb (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AudioMeterBuffer::getPeakHoldLevelDb (int channel) const
{
	ASSERT (channel >= 0 && channel < getChannelCount ())
	return ballistics.getPeakHoldLevelDb (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioMeterBuffer::isClipped (int channel) const
{
	ASSERT (channel >= 0 && channel < getChannelCount ())
	return ballistics.isClipped (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioMeterSettings& AudioMeterBuffer::getSettings ()
{
	if(!settings)
		setSettings (AutoPtr<AudioMeterSettings> (NEW AudioMeterSettings));
	return *settings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterBuffer::setSettings (AudioMeterSettings* newSettings)
{
	if(settings != newSettings)
	{
		if(settings)
			settings->removeObserver (this);

		settings = newSettings;

		if(settings)
			settings->addObserver (this);

		applySettings ();
		signalSettingsChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterBuffer::applySettings ()
{
	if(settings)
	{
		ballistics.setPeakDecayRate (settings->getPeakDecayRate ());
		ballistics.setRMSSmoothingTime (settings->getRMSSmoothingTime ());
		ballistics.setClipHoldTime (settings->getClipHoldTime ());
		ballistics.setPeakHoldTime (settings->getPeakHoldTime ());
		ballistics.setRMSEnabled (settings->isRMSEnabled ());
		ballistics.setClipEnabled (settings->isClipEnabled ());
		ballistics.setPeakHoldEnabled (settings->isPeakHoldEnabled ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioMeterBuffer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && settings && subject == settings)
	{
		applySettings ();
		signalSettingsChanged ();
	}
}
