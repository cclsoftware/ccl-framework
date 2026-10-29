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
// Filename    : miniaudiosources.cpp
// Description : Audio Sources based on miniaudio
//
//************************************************************************************************

#include "miniaudiosources.h"
#include "ccl/extras/audio/audioscratchbuffer.h"

#include "ccl/public/base/iprogress.h"

using namespace CCL;

//************************************************************************************************
// MiniAudioBufferedSource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioBufferedSource, AudioSourceBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioBufferedSource::reinitializeRingBuffer ()
{
	readPosition = 0;
	{
		Threading::ScopedLock guard (seekLock);
		hasPendingSeek = false;
	}

	int numChannels = source ? source->getNumChannels () : 0;
	ringBuffer.resize (numChannels, capacityInFrames);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioBufferedSource::applyPendingSeek ()
{
	for(;;)
	{
		TSamples target = 0;
		{
			Threading::ScopedLock guard (seekLock);
			if(!hasPendingSeek)
				return;
			target = pendingSeekTarget;
		}

		ringBuffer.reset ();

		// don't trust an out-of-range/failed seek - fall back to the source's real position
		readPosition = source->seek (target) >= 0 ? target : source->tell ();

		{
			Threading::ScopedLock guard (seekLock);
			if(pendingSeekTarget == target)
			{
				hasPendingSeek = false;
				return;
			}
			// a newer seek arrived while this one was being applied - loop and apply the latest target
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioBufferedSource::setSource (IAudioSource* newSource)
{
	source = newSource;

	reinitializeRingBuffer ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSource* CCL_API MiniAudioBufferedSource::getSource () const
{
	return source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioBufferedSource::resize (int newCapacityInFrames)
{
	if(newCapacityInFrames <= 0)
		return kResultInvalidArgument;

	capacityInFrames = newCapacityInFrames;
	reinitializeRingBuffer ();

	return !source || ringBuffer.getCapacity () > 0 ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioBufferedSource::fill (IProgressNotify* progress)
{
	if(!source || ringBuffer.getCapacity () == 0)
		return kResultFailed;

	applyPendingSeek ();

	for(;;)
	{
		AudioDataSpan ringSpan;
		int available = ringBuffer.acquireWrite (ringSpan, ringBuffer.getCapacity ());
		if(available == 0)
			return kResultOk; // already full

		int framesRead = source->read (ringSpan, available);
		if(framesRead <= 0)
		{
			ringBuffer.commitWrite (0);
			return framesRead < 0 ? kResultFailed : kResultFalse; // 0 = end of source
		}

		ringBuffer.commitWrite (framesRead);

		if(framesRead < available)
			return kResultOk; // source gave us less than requested - try again next fill()

		if(progress && progress->isCanceled ())
			return kResultAborted;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSource::getBufferedAmount () const
{
	return ringBuffer.getAvailableRead ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSource::getCapacity () const
{
	return capacityInFrames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSource::getNumChannels () const
{
	return source ? source->getNumChannels () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioBufferedSource::getSampleRate () const
{
	return source ? source->getSampleRate () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioBufferedSource::getFrameCount () const
{
	return source ? source->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API MiniAudioBufferedSource::getSpeakerFormat () const
{
	return source ? source->getSpeakerFormat () : SpeakerDef::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSource::read (AudioData& data, int sampleFrames)
{
	if(ringBuffer.getCapacity () == 0)
		return -1;

	ASSERT (data.numChannels == ringBuffer.getNumChannels ())
	if(data.numChannels != ringBuffer.getNumChannels ())
		return -1;

	{
		Threading::ScopedTryLock guard (seekLock);
		if(!guard.success || hasPendingSeek)
			return 0; // seek not yet applied (or can't tell right now) - don't serve stale pre-seek audio
	}

	int framesRead = ringBuffer.read (data, sampleFrames);
	readPosition += framesRead;

	return framesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioBufferedSource::seek (TSamples position)
{
	if(!source)
		return -1;

	ASSERT (position >= 0)
	if(position < 0)
		return -1;

	{
		Threading::ScopedLock guard (seekLock);
		pendingSeekTarget = position;
		hasPendingSeek = true;
	}

	readPosition = position; // optimistic - fill() re-applies once the buffer actually catches up

	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioBufferedSource::tell () const
{
	return readPosition;
}

//************************************************************************************************
// MiniAudioBufferedSink
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioBufferedSink, AudioSinkBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioBufferedSink::reinitializeRingBuffer ()
{
	int numChannels = sink ? sink->getNumChannels () : 0;
	ringBuffer.resize (numChannels, capacityInFrames);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioBufferedSink::setSink (IAudioSink* newSink)
{
	sink = newSink;

	reinitializeRingBuffer ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSink* CCL_API MiniAudioBufferedSink::getSink () const
{
	return sink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioBufferedSink::resize (int newCapacityInFrames)
{
	if(newCapacityInFrames <= 0)
		return kResultInvalidArgument;

	capacityInFrames = newCapacityInFrames;
	reinitializeRingBuffer ();

	return !sink || ringBuffer.getCapacity () > 0 ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioBufferedSink::flush (IProgressNotify* progress)
{
	if(!sink || ringBuffer.getCapacity () == 0)
		return kResultFailed;

	for(;;)
	{
		AudioDataSpan ringSpan;
		int available = ringBuffer.acquireRead (ringSpan, ringBuffer.getCapacity ());
		if(available == 0)
			return kResultOk; // already empty

		int framesWritten = sink->write (ringSpan, available);
		if(framesWritten < 0)
		{
			ringBuffer.commitRead (0);
			return kResultFailed;
		}

		ASSERT (framesWritten <= available)
		if(framesWritten > available)
			framesWritten = available;

		ringBuffer.commitRead (framesWritten);

		if(framesWritten < available)
			return kResultFalse; // sink cannot currently consume the remaining data

		if(progress && progress->isCanceled ())
			return kResultAborted;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSink::getBufferedAmount () const
{
	return ringBuffer.getAvailableRead ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSink::getCapacity () const
{
	return capacityInFrames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSink::getNumChannels () const
{
	return sink ? sink->getNumChannels () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioBufferedSink::getSampleRate () const
{
	return sink ? sink->getSampleRate () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioBufferedSink::getFrameCount () const
{
	return sink ? sink->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API MiniAudioBufferedSink::getSpeakerFormat () const
{
	return sink ? sink->getSpeakerFormat () : SpeakerDef::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioBufferedSink::write (const AudioData& data, int sampleFrames)
{
	if(ringBuffer.getCapacity () == 0)
		return -1;

	ASSERT (data.numChannels == ringBuffer.getNumChannels ())
	if(data.numChannels != ringBuffer.getNumChannels ())
		return -1;

	return ringBuffer.write (data, sampleFrames);
}

//************************************************************************************************
// MiniAudioResamplingSource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioResamplingSource, AudioSourceBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioResamplingSource::reconfigureResampler ()
{
	if(!source || targetRate == 0)
		return;

	resampler.setNumChannels (source->getNumChannels ());
	resampler.setRates (source->getSampleRate (), targetRate);

	carryBuffer.resize (source->getNumChannels (), kCarryCapacityInFrames);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioResamplingSource::setSource (IAudioSource* newSource)
{
	source = newSource;

	reconfigureResampler ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSource* CCL_API MiniAudioResamplingSource::getSource () const
{
	return source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioResamplingSource::setTargetRate (SampleRate sampleRate)
{
	if(sampleRate <= 0)
		return kResultInvalidArgument;

	targetRate = sampleRate;

	reconfigureResampler ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioResamplingSource::getNumChannels () const
{
	return source ? source->getNumChannels () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioResamplingSource::getSampleRate () const
{
	return targetRate;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioResamplingSource::getFrameCount () const
{
	if(!source)
		return 0;

	TSamples sourceFrames = source->getFrameCount ();
	SampleRate sourceRate = source->getSampleRate ();
	if(sourceFrames < 0 || sourceRate == 0 || targetRate == 0)
		return sourceFrames; // unbounded, or rates not yet known - can't scale

	return TSamples(double(sourceFrames) * double(targetRate) / double(sourceRate)); // approximate
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API MiniAudioResamplingSource::getSpeakerFormat () const
{
	return source ? source->getSpeakerFormat () : SpeakerDef::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioResamplingSource::read (AudioData& data, int sampleFrames)
{
	if(!source)
		return -1;

	ASSERT (data.numChannels == source->getNumChannels ())
	if(data.numChannels != source->getNumChannels ())
		return -1;

	int required = resampler.getRequiredSourceFrameCount (sampleFrames);

	// top up the carry buffer with fresh source frames until enough are available, or the source runs dry
	// (frames left over after process() below stay committed in carryBuffer for the next read() call,
	// since the source/target frame ratio is not integral)
	while(carryBuffer.getAvailableRead () < required)
	{
		AudioDataSpan writeSpan;
		int framesToWrite = carryBuffer.acquireWrite (writeSpan, required - carryBuffer.getAvailableRead ());
		if(framesToWrite == 0)
			break; // carry buffer full - convert with what we have

		int framesRead = source->read (writeSpan, framesToWrite);
		if(framesRead <= 0)
		{
			carryBuffer.commitWrite (0);
			break; // source exhausted or error
		}

		carryBuffer.commitWrite (framesRead);

		if(framesRead < framesToWrite)
			break; // source gave less than asked - convert with what we have, try again next read()
	}

	int toConsume = ccl_min (carryBuffer.getAvailableRead (), required);
	if(toConsume == 0)
		return 0; // source exhausted, nothing left to convert

	AudioDataSpan readSpan;
	int srcFrames = carryBuffer.acquireRead (readSpan, toConsume);
	int dstFrames = sampleFrames;

	if(resampler.process (data, dstFrames, readSpan, srcFrames) != kResultOk)
	{
		carryBuffer.commitRead (0);
		return -1;
	}

	carryBuffer.commitRead (srcFrames);

	return dstFrames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioResamplingSource::seek (TSamples position)
{
	if(!source)
		return -1;

	ASSERT (position >= 0)
	if(position < 0)
		return -1;

	SampleRate sourceRate = source->getSampleRate ();
	if(sourceRate == 0 || targetRate == 0)
		return -1;

	TSamples sourcePosition = TSamples(double(position) * double(sourceRate) / double(targetRate));
	if(source->seek (sourcePosition) < 0)
		return -1;

	resampler.reset ();
	carryBuffer.reset ();

	return position; // approximate
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioResamplingSource::tell () const
{
	if(!source)
		return 0;

	SampleRate sourceRate = source->getSampleRate ();
	if(sourceRate == 0 || targetRate == 0)
		return 0;

	return TSamples(double(source->tell ()) * double(targetRate) / double(sourceRate)); // approximate
}

//************************************************************************************************
// MiniAudioResamplingSink
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioResamplingSink, AudioSinkBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioResamplingSink::reconfigureResampler ()
{
	if(!sink || sourceRate == 0)
		return;

	resampler.setNumChannels (sink->getNumChannels ());
	resampler.setRates (sourceRate, sink->getSampleRate ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioResamplingSink::setSink (IAudioSink* newSink)
{
	sink = newSink;

	reconfigureResampler ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSink* CCL_API MiniAudioResamplingSink::getSink () const
{
	return sink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioResamplingSink::setSourceRate (SampleRate sampleRate)
{
	if(sampleRate <= 0)
		return kResultInvalidArgument;

	sourceRate = sampleRate;

	reconfigureResampler ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioResamplingSink::getNumChannels () const
{
	return sink ? sink->getNumChannels () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioResamplingSink::getSampleRate () const
{
	return sourceRate;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioResamplingSink::getFrameCount () const
{
	return sink ? sink->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API MiniAudioResamplingSink::getSpeakerFormat () const
{
	return sink ? sink->getSpeakerFormat () : SpeakerDef::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioResamplingSink::write (const AudioData& data, int sampleFrames)
{
	if(!sink)
		return -1;

	ASSERT (data.numChannels == sink->getNumChannels ())
	if(data.numChannels != sink->getNumChannels ())
		return -1;

	int numChannels = sink->getNumChannels ();
	int capacity = resampler.getExpectedTargetFrameCount (sampleFrames) + 1; // +1 for rounding

	AudioScratchDataBuffer::ScopedBuffer outputBuffer (capacity, numChannels, kAudioFloat32);
	if(!outputBuffer)
		return -1;

	int srcFrames = sampleFrames;
	int dstFrames = capacity;

	if(resampler.process (*outputBuffer, dstFrames, data, srcFrames) != kResultOk)
		return -1;

	// NOTE: assumes sink->write() always fully consumes on success,
	// a partial downstream write would drop the remainder here.
	if(sink->write (*outputBuffer, dstFrames) < 0)
		return -1;

	return srcFrames;
}

//************************************************************************************************
// MiniAudioConvertingSource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioConvertingSource, AudioSourceBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioConvertingSource::reconfigureConverter ()
{
	if(!source || targetChannelCount == 0)
		return;

	converter.setChannelCounts (source->getNumChannels (), targetChannelCount);
	converter.setSpeakerFormats (source->getSpeakerFormat (), targetSpeakerFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioConvertingSource::setSource (IAudioSource* newSource)
{
	source = newSource;

	reconfigureConverter ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSource* CCL_API MiniAudioConvertingSource::getSource () const
{
	return source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioConvertingSource::setTargetChannels (int channelCount, SpeakerBitField speakerFormat)
{
	if(channelCount <= 0)
		return kResultInvalidArgument;

	targetChannelCount = channelCount;
	targetSpeakerFormat = speakerFormat;

	reconfigureConverter ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioConvertingSource::setChannelMixMode (ChannelMixMode mode)
{
	return converter.setChannelMixMode (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ChannelMixMode CCL_API MiniAudioConvertingSource::getChannelMixMode () const
{
	return converter.getChannelMixMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioConvertingSource::getNumChannels () const
{
	return targetChannelCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioConvertingSource::getSampleRate () const
{
	return source ? source->getSampleRate () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioConvertingSource::getFrameCount () const
{
	return source ? source->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API MiniAudioConvertingSource::getSpeakerFormat () const
{
	return targetSpeakerFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioConvertingSource::read (AudioData& data, int sampleFrames)
{
	if(!source)
		return -1;

	ASSERT (data.numChannels == targetChannelCount)
	if(data.numChannels != targetChannelCount)
		return -1;

	int numChannelsIn = source->getNumChannels ();
	AudioScratchDataBuffer::ScopedBuffer scratchBuffer (sampleFrames, numChannelsIn, kAudioFloat32);
	if(!scratchBuffer)
		return -1;

	int framesRead = source->read (*scratchBuffer, sampleFrames);
	if(framesRead <= 0)
		return framesRead;

	if(converter.process (data, *scratchBuffer, framesRead) != kResultOk)
		return -1;

	return framesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioConvertingSource::seek (TSamples position)
{
	return source ? source->seek (position) : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioConvertingSource::tell () const
{
	return source ? source->tell () : 0;
}

//************************************************************************************************
// MiniAudioConvertingSink
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioConvertingSink, AudioSinkBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioConvertingSink::reconfigureConverter ()
{
	if(!sink || sourceChannelCount == 0)
		return;

	converter.setChannelCounts (sourceChannelCount, sink->getNumChannels ());
	converter.setSpeakerFormats (sourceSpeakerFormat, sink->getSpeakerFormat ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioConvertingSink::setSink (IAudioSink* newSink)
{
	sink = newSink;

	reconfigureConverter ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSink* CCL_API MiniAudioConvertingSink::getSink () const
{
	return sink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioConvertingSink::setSourceChannels (int channelCount, SpeakerBitField speakerFormat)
{
	if(channelCount <= 0)
		return kResultInvalidArgument;

	sourceChannelCount = channelCount;
	sourceSpeakerFormat = speakerFormat;

	reconfigureConverter ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioConvertingSink::setChannelMixMode (ChannelMixMode mode)
{
	return converter.setChannelMixMode (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ChannelMixMode CCL_API MiniAudioConvertingSink::getChannelMixMode () const
{
	return converter.getChannelMixMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioConvertingSink::getNumChannels () const
{
	return sourceChannelCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioConvertingSink::getSampleRate () const
{
	return sink ? sink->getSampleRate () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioConvertingSink::getFrameCount () const
{
	return sink ? sink->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API MiniAudioConvertingSink::getSpeakerFormat () const
{
	return sourceSpeakerFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioConvertingSink::write (const AudioData& data, int sampleFrames)
{
	if(!sink)
		return -1;

	ASSERT (data.numChannels == sourceChannelCount)
	if(data.numChannels != sourceChannelCount)
		return -1;

	int numChannelsOut = sink->getNumChannels ();
	AudioScratchDataBuffer::ScopedBuffer scratchBuffer (sampleFrames, numChannelsOut, kAudioFloat32);
	if(!scratchBuffer)
		return -1;

	if(converter.process (*scratchBuffer, data, sampleFrames) != kResultOk)
		return -1;

	// NOTE: assumes sink->write() always fully consumes on success,
	// a partial downstream write would drop the remainder here.
	if(sink->write (*scratchBuffer, sampleFrames) < 0)
		return -1;

	return sampleFrames;
}
