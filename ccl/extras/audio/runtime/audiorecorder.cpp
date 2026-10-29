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
// Filename    : ccl/extras/audio/runtime/audiorecorder.cpp
// Description : Audio Recorder
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audiorecorder.h"

#include "ccl/extras/audio/cclaudiodataops.h"

#include "ccl/base/message.h"

#include "ccl/public/audio/iaudioservice.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// AudioRecorder::FlushThread
//************************************************************************************************

AudioRecorder::FlushThread::FlushThread (AudioRecorder& recorder)
: UserThread ("AudioRecorder"),
  recorder (recorder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioRecorder::FlushThread::threadEntry ()
{
	while(!shouldTerminate ())
	{
		int waitTime = 20;
		{
			// the final drain of a take takes the same lock, so they never flush concurrently
			Threading::ScopedLock flushGuard (recorder.flushLock);

			SharedPtr<IBufferedAudioSink> flushSink;
			if(recorder.getFlushSink (flushSink))
			{
				tresult result = flushSink->flush (this);
				if(result == kResultFailed)
					recorder.onSinkFailed ();
				else
					waitTime = 5;
			}
		}

		recorder.flushSignal.wait (waitTime);
	}

	return 0;
}

//************************************************************************************************
// AudioRecorder
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioRecorder, AudioProcessorBase)
DEFINE_STRINGID_MEMBER_ (AudioRecorder, kSinkFailed, "SinkFailed")

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioRecorder::AudioRecorder (IAudioRecordHandler& handler)
: handler (handler),
  flushSignal (false),
  flushThread (*this)
{
	streamLayout.add (kAudioInputDirection,
		NEW AudioStream (String::kEmpty, AudioStream::kActive, kChannelCount, kSpeakerFormat));
	streamLayout.add (kAudioOutputDirection,
		NEW AudioStream (String::kEmpty, AudioStream::kActive, kChannelCount, kSpeakerFormat));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioRecorder::~AudioRecorder ()
{
	cancelSignals ();
	unprepareCapture ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioRecorder::prepareCapture (SampleRate inputSampleRate)
{
	ASSERT (inputSampleRate > 0)
	if(inputSampleRate <= 0 || capturePrepared)
		return false;

	captureSampleRate = inputSampleRate;
	capturePrepared = true;

	flushThread.startThread (Threading::kPriorityBelowNormal);
	flushSignal.signal ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioRecorder::unprepareCapture ()
{
	if(!capturePrepared)
		return;

	endTake ();

	capturePrepared = false;
	captureSampleRate = 0;

	flushSignal.signal ();
	if(!flushThread.stopThread (1000))
		CCL_WARN ("AudioRecorder: Thread didn't stop in time, forced termination.\n", 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioRecorder::startRecording ()
{
	if(!capturePrepared)
	{
		CCL_WARN ("AudioRecorder: Cannot start recording (not prepared).\n", 0)
		return false;
	}

	{
		// a take that failed in the background is still pending until the main thread has ended it
		Threading::ScopedLock guard (lock);
		if(bufferedSink)
			return false;
	}

	AutoPtr<IAudioSink> newSink = handler.createRecordSink (captureSampleRate);
	if(!newSink)
		return false;

	// IBufferedAudioSink does not allow setSink/resize concurrent with write/flush,
	// so it is fully built before it becomes visible to the other threads
	int channelCount = newSink->getNumChannels ();
	bool usable = channelCount >= 1 && channelCount <= kChannelCount && newSink->getSampleRate () == captureSampleRate;

	AutoPtr<IBufferedAudioSink> newBufferedSink;
	if(usable)
	{
		newBufferedSink = ccl_new<IBufferedAudioSink> (ClassID::BufferedAudioSink);
		ASSERT (newBufferedSink)

		int capacity = int(captureSampleRate * kBufferDurationMilliseconds / 1000);
		usable = newBufferedSink &&
				 newBufferedSink->setSink (newSink) == kResultOk &&
				 newBufferedSink->resize (capacity) == kResultOk;
	}

	if(!usable)
	{
		handler.onRecordTakeFinished (newSink, kResultFailed);
		return false;
	}

	{
		Threading::ScopedLock guard (lock);
		sink = newSink;
		bufferedSink = newBufferedSink;
		sinkChannelCount = channelCount;

		failed = 0;
		recordedFrames = 0;
		recording = 1;
	}

	flushSignal.signal ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioRecorder::stopRecording ()
{
	endTake ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioRecorder::isRecording () const
{
	return recording != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds AudioRecorder::getRecordedTime () const
{
	return captureSampleRate > 0 ? TSeconds(recordedFrames) / captureSampleRate : 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioRecorder::captureAudio (const AudioData& input)
{
	if(!isRecording () || input.numSamples <= 0)
		return 0;

	Threading::ScopedTryLock guard (lock);
	if(!guard.success)
		return 0;

	if(!bufferedSink)
		return 0;

	if(input.numChannels <= 0)
		return 0;

	// map input channels to sink channels: extra input channels are ignored,
	// missing ones repeat the last input channel (mono input feeds both channels of a stereo sink)
	float32* channelBuffers[kChannelCount] = {};
	for(int channel = 0; channel < sinkChannelCount; channel++)
		channelBuffers[channel] = input.buffers32[ccl_min (channel, input.numChannels - 1)];

	AudioData sinkData (sinkChannelCount, input.numSamples, channelBuffers, input.floatType);

	int numWritten = ccl_max (bufferedSink->write (sinkData, input.numSamples), 0);
	if(numWritten > 0)
	{
		recordedFrames += numWritten;
		flushSignal.signal ();
	}

	return numWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioRecorder::endTake ()
{
	SharedPtr<IAudioSink> takeSink;
	bool takeFailed = false;
	{
		// the flush thread and this drain must not flush concurrently
		Threading::ScopedLock flushGuard (flushLock);

		SharedPtr<IBufferedAudioSink> drainSink;
		{
			Threading::ScopedLock guard (lock);
			recording = 0; // a running captureAudio () holds the lock, so it is quiet after this
			drainSink = bufferedSink;
			takeSink = sink;
		}

		if(!drainSink)
			return;

		takeFailed = failed != 0;
		if(!takeFailed)
		{
			for(int i = 0; i < 8 && drainSink->getBufferedAmount () > 0; i++)
			{
				tresult result = drainSink->flush ();
				if(result == kResultFailed)
					takeFailed = true;
				if(result != kResultOk)
					break;
			}
		}

		Threading::ScopedLock guard (lock);
		bufferedSink.release ();
		sink.release ();
		sinkChannelCount = 0;
	}

	ASSERT (takeSink)
	handler.onRecordTakeFinished (takeSink, takeFailed ? kResultFailed : kResultOk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioRecorder::getFlushSink (SharedPtr<IBufferedAudioSink>& flushSink)
{
	Threading::ScopedLock guard (lock);
	if(!bufferedSink || failed != 0)
		return false;

	flushSink = bufferedSink;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioRecorder::onSinkFailed ()
{
	// called on the flush thread, which cannot end the take itself:
	// stop writing and let the main thread do it
	failed = 1;
	recording = 0;
	(NEW Message (kSinkFailed))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioRecorder::onActiveChanged (bool state)
{
	if(state)
	{
		bool result = prepareCapture (processConfiguration.sampleRate);
		ASSERT (result)
	}
	else
		unprepareCapture ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioComponent/IAudioProcessor
//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioComponent::CapabilityFlags CCL_API AudioRecorder::getCapabilities () const
{
	return kCapabilityKnownStreamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAudioStreamLayout& CCL_API AudioRecorder::getStreamLayout () const
{
	return streamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioRecorder::process (const AudioProcessData& data)
{
	const AudioIOData& ioData = data.ioData;

	if(ioData.numInputs > 0)
	{
		const AudioIOData::StreamData& stream = ioData.inputs[0];
		AudioData inputData (stream.numChannels, ioData.numSamples, stream.buffers32, ioData.floatType);
		captureAudio (inputData);
	}

	// silent output, the recorder is not a monitoring path
	for(int i = 0; i < ioData.numOutputs; i++)
	{
		const AudioIOData::StreamData& stream = ioData.outputs[i];
		AudioData outputData (stream.numChannels, ioData.numSamples, stream.buffers32, ioData.floatType);
		AudioDataOpsGeneric::clear (outputData);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioTransportClock/IAudioTransportClient
//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds CCL_API AudioRecorder::getCurrentTime () const
{
	return getRecordedTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds CCL_API AudioRecorder::getMaxDuration () const
{
	return kMaxDurationUnbound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioTransportClient::StateFlags CCL_API AudioRecorder::getClientState () const
{
	return isRecording () ? kRecording : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioRecorder::onTransportEvent (const AudioTransportEvent& e)
{
	switch(e.type)
	{
	case AudioTransportEvent::kRecordStarted :
		startRecording ();
		break;

	case AudioTransportEvent::kRecordStopped :
		stopRecording ();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioRecorder::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kSinkFailed)
	{
		if(failed != 0) // not stale: a take may have ended in the meantime
			endTake ();
	}
	else
		SuperClass::notify (subject, msg);
}
