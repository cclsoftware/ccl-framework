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
// Filename    : ccl/extras/audio/meter/audiometercomponent.cpp
// Description : Audio Meter Component
//
//************************************************************************************************

#include "ccl/extras/audio/meter/audiometercomponent.h"
#include "ccl/extras/audio/meter/audiometerview.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iskinmodel.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum AudioMeterComponentTags
	{
		kPeakBase = 100,
		kRMSBase = 200,
		kClipBase = 300,
		kPeakHoldBase = 400
	};
}

//************************************************************************************************
// AudioMeterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioMeterComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioMeterComponent::AudioMeterComponent (StringRef name, int maxChannelCount)
: Component (name.isEmpty () ? CCLSTR ("AudioMeter") : name),
  peakParams (maxChannelCount),
  rmsParams (maxChannelCount),
  clipParams (maxChannelCount),
  peakHoldParams (maxChannelCount),
  meterBuffer (NEW AudioMeterBuffer)
{
	AudioMeterSettings& settings = getMeterSettings ();
	float minDb = settings.getMinDb ();
	float maxDb = settings.getMaxDb ();

	for(int ch = 0; ch < maxChannelCount; ch++)
	{
		IParameter* peakParam = paramList.addFloat (minDb, maxDb, 
													MutableCString ().appendFormat ("peak%d", ch), 
													Tag::kPeakBase + ch);
		peakParam->setValue (minDb);
		peakParam->setReadOnly ();
		peakParams.add (peakParam);

		IParameter* rmsParam = paramList.addFloat (minDb, maxDb,
												   MutableCString ().appendFormat ("rms%d", ch), 
												   Tag::kRMSBase + ch);
		rmsParam->setValue (minDb);
		rmsParam->setReadOnly ();
		rmsParams.add (rmsParam);

		IParameter* clipParam = paramList.addParam (MutableCString ().appendFormat ("clip%d", ch),
													Tag::kClipBase + ch);
		clipParam->setReadOnly ();
		clipParams.add (clipParam);

		IParameter* peakHoldParam = paramList.addFloat (minDb, maxDb,
														MutableCString ().appendFormat ("peakHold%d", ch),
														Tag::kPeakHoldBase + ch);
		peakHoldParam->setValue (minDb);
		peakHoldParam->setReadOnly ();
		peakHoldParams.add (peakHoldParam);
	}

	meterBuffer->addClient (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioMeterComponent::~AudioMeterComponent ()
{
	meterBuffer->removeClient (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterComponent::setMeterSource (IAudioMeterSource* source)
{
	meterSource = source;

	int channelCount = source ? source->getChannelCount () : 0;
	ASSERT (source == nullptr || channelCount > 0)

	meterBuffer->setChannelCount (channelCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterComponent::setMeterActive (bool state)
{
	if(state == isTimerEnabled ())
		return;

	if(state)
	{
		lastUpdateTime = System::GetProfileTime ();
		startTimer (kMeterUpdateInterval);
	}
	else
	{
		meterBuffer->reset ();
		stopTimer ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioMeterComponent::getMaxChannelCount () const
{
	return peakParams.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioMeterComponent::getChannelCount () const
{
	return meterBuffer->getChannelCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* AudioMeterComponent::getPeakParam (int channel) const
{
	return peakParams.at (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* AudioMeterComponent::getRMSParam (int channel) const
{
	return rmsParams.at (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* AudioMeterComponent::getClipParam (int channel) const
{
	return clipParams.at (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* AudioMeterComponent::getPeakHoldParam (int channel) const
{
	return peakHoldParams.at (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterComponent::updateParamRange ()
{
	AudioMeterSettings& settings = getMeterSettings ();

	int channelCount = getMaxChannelCount ();
	for(int ch = 0; ch < channelCount; ch++)
	{
		peakParams[ch]->setMin (settings.getMinDb ());
		peakParams[ch]->setMax (settings.getMaxDb ());
		rmsParams[ch]->setMin (settings.getMinDb ());
		rmsParams[ch]->setMax (settings.getMaxDb ());
		peakHoldParams[ch]->setMin (settings.getMinDb ());
		peakHoldParams[ch]->setMax (settings.getMaxDb ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterComponent::syncParamValues ()
{
	int channelCount = get_min (getChannelCount (), getMaxChannelCount ());
	for(int ch = 0; ch < channelCount; ch++)
	{
		peakParams[ch]->setValue (meterBuffer->getPeakLevelDb (ch));
		rmsParams[ch]->setValue (meterBuffer->getRMSLevelDb (ch));
		clipParams[ch]->setValue (meterBuffer->isClipped (ch));
		peakHoldParams[ch]->setValue (meterBuffer->getPeakHoldLevelDb (ch));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterComponent::meterDataChanged (const AudioMeterBuffer& buffer)
{
	syncParamValues ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterComponent::meterSettingsChanged (const AudioMeterBuffer& buffer)
{
	updateParamRange ();
	syncParamValues ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API AudioMeterComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "MeterView")
	{
		StyleFlags style (Styles::kVertical);

		if(UnknownPtr<ISkinCreateArgs> args = data.asUnknown ())
			if(ISkinViewElement* element = args->getElement ())
				style = args->getElement ()->getStandardOptions ();

		return *NEW AudioMeterView (meterBuffer, bounds, style);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterComponent::onIdleTimer ()
{
	if(!meterSource.isValid ())
		return;

	AudioMeterDataMax rawData (getChannelCount ());
	meterSource->readMeterData (rawData);

	double now = System::GetProfileTime ();
	TSeconds elapsed = now - lastUpdateTime;
	lastUpdateTime = now;

	meterBuffer->update (rawData, elapsed);
}
