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
// Filename    : ccl/extras/audio/waveform/audiolivepeakbuilder.cpp
// Description : Audio Live Peak Builder
//
//************************************************************************************************

#include "ccl/extras/audio/waveform/audiolivepeakbuilder.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// AudioLivePeakBuilder
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioLivePeakBuilder, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioLivePeakBuilder::setup (AudioPeakBuffer& _peakBuffer, int capacity)
{
	ASSERT (_peakBuffer.getNumPeakChannels () > 0 && _peakBuffer.getFramesPerPeak () > 0)
	ASSERT (_peakBuffer.getNumPeaksValid () == 0)

	detach ();

	peakBuffer = &_peakBuffer;
	numChannels = ccl_min<uint32> (peakBuffer->getNumPeakChannels (), AudioPeakBuffer::kMaxChannels);
	framesPerPeak = ccl_max<uint32> (peakBuffer->getFramesPerPeak (), 1);

	uint32 powerOfTwo = 1;
	while(powerOfTwo < uint32(ccl_max (capacity, 1)))
		powerOfTwo <<= 1;

	for(uint32 channel = 0; channel < numChannels; channel++)
		fifo[channel].resize (powerOfTwo * sizeof(AudioPeakBuffer::PeakF));

	framesInGroup = 0;
	writeCount = 0;
	readCount = 0;
	overrunCount = 0;
	fifoCapacity = powerOfTwo; // marks the builder as bound, set last
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioLivePeakBuilder::detach ()
{
	fifoCapacity = 0;
	peakBuffer = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioLivePeakBuilder::process (const AudioData& data, int numFrames)
{
	if(fifoCapacity == 0 || numFrames <= 0)
		return;

	uint32 dataChannels = ccl_min<uint32> (ccl_max (data.numChannels, 0), numChannels);

	int offset = 0;
	while(offset < numFrames)
	{
		int count = ccl_min<int> (numFrames - offset, int(framesPerPeak - framesInGroup));

		for(uint32 channel = 0; channel < numChannels; channel++)
		{
			float32 positive = 0.f, negative = 0.f;
			if(channel < dataChannels)
				AudioPeakDetection::detect (positive, negative, data.buffers32[channel] + offset, count);

			if(framesInGroup == 0)
			{
				groupPositive[channel] = positive;
				groupNegative[channel] = negative;
			}
			else
			{
				groupPositive[channel] = ccl_max (groupPositive[channel], positive);
				groupNegative[channel] = ccl_min (groupNegative[channel], negative);
			}
		}

		framesInGroup += uint32(count);
		offset += count;

		if(framesInGroup >= framesPerPeak)
		{
			pushGroup ();
			framesInGroup = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioLivePeakBuilder::pushGroup ()
{
	uint32 write = uint32(writeCount.getValue ());
	uint32 read = uint32(readCount.getValue ());
	if(write - read >= fifoCapacity)
	{
		overrunCount.increment ();
		return;
	}

	uint32 index = write & (fifoCapacity - 1);
	for(uint32 channel = 0; channel < numChannels; channel++)
	{
		AudioPeakBuffer::PeakF& peak = fifo[channel].as<AudioPeakBuffer::PeakF> ()[index];
		peak.p = ccl_bound<float32> (fabsf (groupPositive[channel]), 0.f, AudioPeakBuffer::Internal::kMaxPeakF);
		peak.n = ccl_bound<float32> (fabsf (groupNegative[channel]), 0.f, AudioPeakBuffer::Internal::kMaxPeakF);
	}

	writeCount.add (1); // publish
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioLivePeakBuilder::flush ()
{
	if(peakBuffer == nullptr || fifoCapacity == 0)
		return 0;

	uint32 write = uint32(writeCount.getValue ());
	uint32 read = uint32(readCount.getValue ());
	uint32 available = write - read;
	if(available == 0)
		return 0;

	// the FIFO wraps at most once, so there are two contiguous segments
	uint32 start = read & (fifoCapacity - 1);
	uint32 firstCount = ccl_min (available, fifoCapacity - start);

	const AudioPeakBuffer::PeakF* source[AudioPeakBuffer::kMaxChannels] = {};
	for(uint32 channel = 0; channel < numChannels; channel++)
		source[channel] = fifo[channel].as<AudioPeakBuffer::PeakF> () + start;
	peakBuffer->appendPeaks (source, firstCount);

	if(available > firstCount)
	{
		for(uint32 channel = 0; channel < numChannels; channel++)
			source[channel] = fifo[channel].as<AudioPeakBuffer::PeakF> ();
		peakBuffer->appendPeaks (source, available - firstCount);
	}

	readCount.add (int(available)); // release the FIFO space

	// a partial trailing group is not emitted yet
	peakBuffer->setNumSourceFrames (TSamples(peakBuffer->getNumPeaksValid ()) * framesPerPeak);
	return int(available);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioLivePeakBuilder::getOverrunCount () const
{
	return overrunCount.getValue ();
}

//************************************************************************************************
// AudioPeakBuilderSink
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioPeakBuilderSink, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakBuilderSink::setBuilder (AudioLivePeakBuilder* newBuilder)
{
	builder = newBuilder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioPeakBuilderSink::getNumChannels () const
{
	return sink ? sink->getNumChannels () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API AudioPeakBuilderSink::getSampleRate () const
{
	return sink ? sink->getSampleRate () : 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API AudioPeakBuilderSink::getFrameCount () const
{
	return sink ? sink->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API AudioPeakBuilderSink::getSpeakerFormat () const
{
	return sink ? sink->getSpeakerFormat () : SpeakerDef::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioPeakBuilderSink::write (const AudioData& data, int sampleFrames)
{
	if(sink == nullptr)
		return -1;

	int numWritten = sink->write (data, sampleFrames);
	if(numWritten > 0 && builder)
		builder->process (data, numWritten);
	return numWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioPeakBuilderSink::setSink (IAudioSink* _sink)
{
	sink = _sink;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSink* CCL_API AudioPeakBuilderSink::getSink () const
{
	return sink;
}
