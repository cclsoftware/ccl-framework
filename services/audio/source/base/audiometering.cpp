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
// Filename    : audiometering.cpp
// Description : Audio Level Detection
//
//************************************************************************************************

#include "audiometering.h"

#include "ccl/extras/audio/cclaudiodataops.h"

using namespace CCL;

//************************************************************************************************
// AudioLevelDetector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioLevelDetector, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioLevelDetector::resize (int newChannelCount, SampleRate newSampleRate, int newLatencyFrames)
{
	ASSERT (newChannelCount <= kMaxAudioMeterChannels)
	channelCount = get_min (newChannelCount, kMaxAudioMeterChannels);
	sampleRate = newSampleRate;

	// zero (or unknown) latency still meters, only the newest slot is read then
	bool enabled = channelCount > 0;
	latencyFrames = enabled ? get_max (newLatencyFrames, 0) : -1;

	// the slot with the audible position is up to latency frames behind the newest one
	int capacity = enabled ? (latencyFrames + kMinSlotFrames - 1) / kMinSlotFrames + 1 + kRingSafetyMargin : 0;
	ring.setCount (capacity);

	framesProcessed = 0;
	writeIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioLevelDetector::process (const AudioData& data)
{
	if(latencyFrames < 0 || ring.isEmpty () || channelCount <= 0 || data.numSamples <= 0)
		return;

	int slotIndex = writeIndex.getValue () % ring.count ();
	Slot& entry = ring[slotIndex];
	entry.startFrame = framesProcessed;
	framesProcessed += data.numSamples;
	entry.endFrame = framesProcessed;

	AudioMeterDataMax& slot = entry.data;
	int numChannels = get_min (data.numChannels, channelCount);
	slot.numChannels = numChannels;
	slot.clipState = 0;

	for(int ch = 0; ch < numChannels; ch++)
	{
		float peak = 0.f, rms = 0.f;
		AudioVectorOpsGeneric::vectorGetPeakRms (peak, rms, data.buffers32[ch], data.numSamples);
		
		slot.peakLevels[ch] = peak;
		slot.rmsLevels[ch] = rms;

		if(peak >= 1.f)
			set_bit (slot.clipState, ch);
	}

	writeIndex.increment ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioLevelDetector::getChannelCount () const
{
	return channelCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioLevelDetector::readMeterData (AudioMeterData& meterData)
{	
	if(!processing || latencyFrames < 0 || ring.isEmpty ())
		return kResultFalse;

	int writePos = writeIndex.getValue ();
	if(writePos <= 0)
		return kResultFalse; // nothing processed yet

	// the audible position is the newest processed frame minus the latency
	int64 audibleFrame = ring[(writePos - 1) % ring.count ()].endFrame - latencyFrames;
	if(audibleFrame < 0)
		return kResultFalse; // not enough audio processed yet

	// walk back to the slot containing it, but not into the slots the producer may be overwriting
	int oldestPos = get_max (0, writePos - (ring.count () - kRingSafetyMargin));
	int readPos = writePos - 1;
	while(readPos > oldestPos && ring[readPos % ring.count ()].startFrame > audibleFrame)
		readPos--;

	const AudioMeterDataMax& slot = ring[readPos % ring.count ()].data;

	ASSERT (meterData.numChannels >= slot.numChannels)
	int numChannels = get_min (meterData.numChannels, slot.numChannels);
	for(int ch = 0; ch < numChannels; ch++)
	{
		meterData.peakLevels[ch] = slot.peakLevels[ch];
		meterData.rmsLevels[ch] = slot.rmsLevels[ch];
	}
	meterData.clipState = slot.clipState;

	return kResultOk;
}
