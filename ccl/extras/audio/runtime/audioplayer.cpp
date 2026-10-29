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
// Filename    : ccl/extras/audio/runtime/audioplayer.cpp
// Description : Audio Player
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audioplayer.h"

#include "ccl/extras/audio/cclaudiodataops.h"

#include "ccl/public/audio/iaudioservice.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// AudioPlayer::FillerThread
//************************************************************************************************

AudioPlayer::FillerThread::FillerThread (AudioPlayer& player)
: UserThread ("AudioPlayer"),
  player (player)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioPlayer::FillerThread::threadEntry ()
{
	while(!shouldTerminate ())
	{
		SharedPtr<IBufferedAudioSource> source;
		int sourceGeneration = 0;
		if(!player.getFillSource (source, sourceGeneration))
		{
			player.fillerSignal.wait (20);
			continue;
		}

		tresult result = source->fill (this);
		if(result == kResultFalse)
			player.setSourceExhausted (source, sourceGeneration);

		// fill() returns immediately when the buffer is full. Also wait after an end-of-file or
		// error result, both of which otherwise would cause a busy loop.
		player.fillerSignal.wait (result == kResultOk ? 5 : 20);
	}

	return 0;
}

//************************************************************************************************
// AudioPlayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioPlayer, AudioProcessorBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPlayer::AudioPlayer ()
: fillerSignal (false),
  fillerThread (*this)
{
	streamLayout.add (kAudioOutputDirection,
		NEW AudioStream (String::kEmpty, AudioStream::kActive, kOutputChannelCount, kOutputSpeakerFormat));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPlayer::~AudioPlayer ()
{
	unprepareRender ();
	clearSource ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::setSource (IAudioSource* newSource)
{
	if(!newSource || newSource->getNumChannels () <= 0 || newSource->getSampleRate () <= 0)
		return false;

	{
		Threading::ScopedLock guard (lock);

		bool wasPlaying = playing;

		playing = false;
		resetPositionLocked (0);
		sourceGeneration++;

		source = newSource;
		sourceSampleRate = newSource->getSampleRate ();
		sourceFrameCount = newSource->getFrameCount ();

		if(renderPrepared && !rebuildRenderChainLocked ())
			return false;

		playing = wasPlaying;
	}

	fillerSignal.signal ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::clearSource ()
{
	{
		Threading::ScopedLock guard (lock);

		playing = false;
		resetPositionLocked (0);
		sourceGeneration++;

		source.release ();
		sourceSampleRate = 0;
		sourceFrameCount = 0;

		if(renderPrepared)
			releaseRenderChainLocked ();
	}

	fillerSignal.signal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::hasSource () const
{
	Threading::ScopedLock guard (lock);
	return source != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds AudioPlayer::getSourceDuration () const
{
	Threading::ScopedLock guard (lock);
	if(sourceFrameCount < 0)
		return -1.;
	return sourceSampleRate > 0 ? TSeconds(sourceFrameCount) / sourceSampleRate : 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::prepareRender (SampleRate outputSampleRate)
{
	ASSERT (outputSampleRate > 0)
	if(outputSampleRate <= 0)
		return false;

	{
		Threading::ScopedLock guard (lock);
		if(renderPrepared)
			return false;

		renderSampleRate = outputSampleRate;
		renderPrepared = true;
		playing = false;
		resetPositionLocked (storedPosition);

		if(!rebuildRenderChainLocked ())
		{
			releaseRenderChainLocked ();
			renderPrepared = false;
			renderSampleRate = 0;
			return false;
		}
	}

	fillerThread.startThread (Threading::kPriorityBelowNormal);
	fillerSignal.signal ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::unprepareRender ()
{
	{
		Threading::ScopedLock guard (lock);
		if(!renderPrepared)
			return;

		TSamples finalPosition = getPositionLocked (false);
		playing = false;
		releaseRenderChainLocked ();
		renderPrepared = false;
		renderSampleRate = 0;
		resetPositionLocked (finalPosition);
	}

	fillerSignal.signal ();
	if(!fillerThread.stopThread (1000))
		CCL_WARN ("AudioPlayer: Thread didn't stop in time, forced termination.\n", 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::start ()
{
	Threading::ScopedLock guard (lock);
	if(!renderPrepared || !bufferedSource)
		return false;

	if(sourceExhausted && bufferedSource->getBufferedAmount () == 0)
		seekLocked (0);

	playing = true;
	fillerSignal.signal ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::stop ()
{
	Threading::ScopedLock guard (lock);
	playing = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::rewind ()
{
	seek (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::seek (TSeconds time)
{
	if(time < 0)
		return false;

	{
		Threading::ScopedLock guard (lock);

		TSamples position = sourceSampleRate > 0 ? TSamples(time * sourceSampleRate) : 0;
		if(sourceFrameCount >= 0)
			position = ccl_min (position, sourceFrameCount);

		seekLocked (position);
	}

	fillerSignal.signal ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::isPlaying () const
{
	Threading::ScopedLock guard (lock);
	return playing;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds AudioPlayer::getPlaybackTime () const
{
	Threading::ScopedLock guard (lock);
	return sourceSampleRate > 0 ? TSeconds(getPositionLocked (true)) / sourceSampleRate : 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::setRepeat (bool state)
{
	Threading::ScopedLock guard (lock);
	repeat = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::isRepeat () const
{
	Threading::ScopedLock guard (lock);
	return repeat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioPlayer::renderAudio (AudioData& output)
{
	ASSERT (output.numChannels == kOutputChannelCount)
	if(output.numChannels != kOutputChannelCount)
		return -1;

	if(output.numSamples <= 0)
		return 0;

	AudioDataOpsGeneric::clear (output);

	int framesRead = 0;
	bool wasRestarted = false;
	{
		Threading::ScopedTryLock guard (lock);
		if(!guard.success || !renderPrepared || !playing || !bufferedSource)
			return 0;

		framesRead = bufferedSource->read (output, output.numSamples);
		if(framesRead > 0)
		{
			renderedFrameCount += framesRead;
			return framesRead;
		}

		if(framesRead == 0 && sourceExhausted && bufferedSource->getBufferedAmount () == 0)
		{
			if(repeat)
			{
				seekLocked (0);
				wasRestarted = true;
			}
			else
			{
				playing = false;
				storedPosition = 0;
				renderStartPosition = 0;
				renderedFrameCount = 0;
			}
		}
	}

	if(wasRestarted)
		fillerSignal.signal ();

	return framesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioComponent/IAudioProcessor/IAudioDisplayLatency
//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioComponent::CapabilityFlags CCL_API AudioPlayer::getCapabilities () const
{
	return kCapabilityKnownStreamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAudioStreamLayout& CCL_API AudioPlayer::getStreamLayout () const
{
	return streamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::onActiveChanged (bool state)
{
	if(state)
	{
		bool result = prepareRender (processConfiguration.sampleRate);
		ASSERT (result)
	}
	else
	{
		unprepareRender ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioPlayer::process (const AudioProcessData& data)
{
	if(data.ioData.numOutputs > 0)
	{
		const AudioIOData::StreamData& stream = data.ioData.outputs[0];
		if(stream.numChannels >= kOutputChannelCount)
		{
			int channelCount = get_min (stream.numChannels, kOutputChannelCount);
			AudioData outputData (channelCount, data.ioData.numSamples, stream.buffers32, data.ioData.floatType);
			renderAudio (outputData);
		}
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioPlayer::setDisplayLatency (int latencyInSamples)
{
	Threading::ScopedLock guard (lock);
	renderLatencyInFrames = latencyInSamples;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioTransportClock/IAudioTransportClient
//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds CCL_API AudioPlayer::getCurrentTime () const
{
	return getPlaybackTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds CCL_API AudioPlayer::getMaxDuration () const
{
	return getSourceDuration ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioTransportClient::StateFlags CCL_API AudioPlayer::getClientState () const
{
	return isPlaying () ? kPlaying : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioPlayer::onTransportEvent (const AudioTransportEvent& e)
{
	switch(e.type)
	{
	case AudioTransportEvent::kStarted :
		start ();
		break;

	case AudioTransportEvent::kStopped :
		stop ();
		break;

	case AudioTransportEvent::kSeek :
		seek (e.time);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::rebuildRenderChainLocked ()
{
	releaseRenderChainLocked ();

	if(!source)
		return true;

	if(storedPosition > 0 && source->seek (storedPosition) < 0)
		return false;

	AutoPtr<IConvertingAudioSource> convertingSource = ccl_new<IConvertingAudioSource> (ClassID::ConvertingAudioSource);
	AutoPtr<IResamplingAudioSource> resamplingSource = ccl_new<IResamplingAudioSource> (ClassID::ResamplingAudioSource);
	AutoPtr<IBufferedAudioSource> newBufferedSource = ccl_new<IBufferedAudioSource> (ClassID::BufferedAudioSource);
	ASSERT (convertingSource && resamplingSource && newBufferedSource)
	if(!convertingSource || !resamplingSource || !newBufferedSource)
		return false;

	if(convertingSource->setSource (source) != kResultOk ||
	   convertingSource->setTargetChannels (kOutputChannelCount, kOutputSpeakerFormat) != kResultOk ||
	   resamplingSource->setSource (convertingSource) != kResultOk ||
	   resamplingSource->setTargetRate (renderSampleRate) != kResultOk ||
	   newBufferedSource->setSource (resamplingSource) != kResultOk)
		return false;

	int capacity = int(renderSampleRate * kBufferDurationMilliseconds / 1000);
	if(newBufferedSource->resize (capacity) != kResultOk)
		return false;

	bufferedSource = newBufferedSource;
	sourceGeneration++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::releaseRenderChainLocked ()
{
	bufferedSource.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples AudioPlayer::getPositionLocked (bool compensateLatency) const
{
	if(!renderPrepared || renderSampleRate <= 0 || sourceSampleRate <= 0)
		return storedPosition;

	TSamples outputFrames = renderedFrameCount;
	if(compensateLatency)
		outputFrames = get_max<TSamples> (outputFrames - renderLatencyInFrames, 0);

	TSamples position = renderStartPosition + TSamples(double(outputFrames) * double(sourceSampleRate) / double(renderSampleRate));
	if(sourceFrameCount >= 0)
		position = ccl_min (position, sourceFrameCount);
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::resetPositionLocked (TSamples position)
{
	storedPosition = position;
	renderStartPosition = position;
	renderedFrameCount = 0;
	sourceExhausted = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::seekLocked (TSamples position)
{
	resetPositionLocked (position);
	sourceGeneration++;

	if(bufferedSource && sourceSampleRate > 0 && renderSampleRate > 0)
	{
		// bufferedSource reads at renderSampleRate (it wraps the resampler), not sourceSampleRate
		TSamples renderPosition = TSamples(double(position) * double(renderSampleRate) / double(sourceSampleRate));
		bufferedSource->seek (renderPosition);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayer::setSourceExhausted (IBufferedAudioSource* source, int generation)
{
	Threading::ScopedLock guard (lock);
	if(generation == sourceGeneration && source == bufferedSource)
		sourceExhausted = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayer::getFillSource (SharedPtr<IBufferedAudioSource>& source, int& generation)
{
	Threading::ScopedLock guard (lock);
	if(!renderPrepared || !bufferedSource)
		return false;

	source = bufferedSource;
	generation = sourceGeneration;
	return true;
}
