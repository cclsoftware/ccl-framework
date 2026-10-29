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
// Filename    : ccl/extras/audio/waveform/audiowaveform.cpp
// Description : Audio Waveform
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/audio/waveform/audiowaveform.h"

#include "ccl/base/message.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/systemservices.h"

namespace CCL {

using namespace Threading;

//************************************************************************************************
// AudioWaveform::WorkItem
//************************************************************************************************

class AudioWaveform::WorkItem: public Object,
							   public AbstractWorkItem,
							   public AbstractProgressNotify
{
public:
	WorkItem (AudioWaveform& waveform, IAudioSource& source)
	: AbstractWorkItem (&waveform),
	  waveform (waveform),
	  source (source)
	{}

	// IWorkItem
	void CCL_API cancel () override { canceled = 1; }
	void CCL_API work () override;

	// IProgressNotify
	void CCL_API beginProgress () override;
	void CCL_API updateProgress (const State& state) override;
	tbool CCL_API isCanceled () override { return canceled != 0; }

	CLASS_INTERFACE2 (IWorkItem, IProgressNotify, Object)

protected:
	static constexpr int kUpdateInterval = 500;

	AudioWaveform& waveform;
	IAudioSource& source;
	AtomicInt canceled;
	int64 lastUpdatTime = 0;
};

//************************************************************************************************
// AudioWaveformWorker
//************************************************************************************************

namespace AudioWaveformWorker
{
	static IThreadPool* getInstance (bool create)
	{
		static AutoPtr<IThreadPool> theInstance;
		if(theInstance == nullptr && create)
			theInstance = System::CreateThreadPool ({1, kPriorityLow, "AudioWaveformWorker"});
		return theInstance;
	}
}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM (AudioWaveform)
{
	if(auto* worker = AudioWaveformWorker::getInstance (false))
		worker->terminate ();
}

//************************************************************************************************
// AudioWaveform
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioWaveform, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioWaveform::AudioWaveform ()
: sourceSampleRate (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioWaveform::~AudioWaveform ()
{
	if(isLive ())
		liveBuilder->detach ();
	else
		stopBuilding ();

	cancelSignals ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveform::setState (State newState, bool deferred)
{
	if(state != newState)
	{
		state = newState;
		if(deferred == true)
			deferChanged ();
		else
			signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveform::setAudioSource (IAudioSource* _audioSource)
{
	ASSERT (state != kStarted)
	if(state == kStarted)
		stopBuilding ();

	ASSERT (isLive () == false)
	if(liveBuilder)
	{
		liveBuilder->detach ();
		liveBuilder.release ();
	}

	audioSource = _audioSource;
	sourceSampleRate = audioSource ? audioSource->getSampleRate () : 0.;

	peakBuffer.disposeBuffers ();
	peakBufferReady = 0;
	
	setState (kNone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveform::startBuilding ()
{
	if(state == kNone)
	{
		ASSERT (audioSource.isValid ())
		if(audioSource.isValid () == false)
		{
			setState (kFailed);
			return;
		}

		setState (kStarted);

		AudioWaveformWorker::getInstance (true)->scheduleWork (NEW WorkItem (*this, *audioSource));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveform::stopBuilding ()
{
	if(state == kStarted)
	{
		ASSERT (isLive () == false)
		
		if(auto* worker = AudioWaveformWorker::getInstance (false))
			worker->cancelWork (this, true);

		setState (kCanceled);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioWaveform::startLive (const AudioContentFormat& format, uint32 framesPerPeak)
{
	ASSERT (format.isValid () && framesPerPeak > 0)
	if(!format.isValid () || framesPerPeak == 0)
		return false;

	if(state == kStarted)
		stopBuilding ();

	audioSource.release ();
	sourceSampleRate = format.sampleRate;

	peakBuffer.disposeBuffers ();
	peakBuffer.resizeBuffers (uint32(format.channelCount), AudioPeakBuffer::kPeakAllocationSize);
	peakBuffer.setFramesPerPeak (framesPerPeak);
	peakBuffer.setNumSourceFrames (0);
	peakBufferReady = 1;

	liveBuilder = NEW AudioLivePeakBuilder;
	liveBuilder->setup (peakBuffer);

	state = kNone; // restart signals even if the previous state was the same
	setState (kStarted);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveform::updateLive ()
{
	ASSERT (isLive () == true)
	if(liveBuilder)
	{
		if(liveBuilder->flush () > 0)
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveform::finishLive (TSamples totalFrames)
{
	ASSERT (isLive () == true)
	if(!isLive ())
		return;

	liveBuilder->flush ();
	liveBuilder->detach ();
	liveBuilder.release ();

	if(totalFrames > peakBuffer.getNumSourceFrames ())
		peakBuffer.setNumSourceFrames (totalFrames);

	peakBuffer.updateTimestamp (true);
	setState (kCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples AudioWaveform::getFrameCount () const
{
	return peakBuffer.getNumSourceFrames ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds AudioWaveform::getDuration () const
{
	return sourceSampleRate > 0 ? double(getFrameCount ()) / sourceSampleRate : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncInfo::State CCL_API AudioWaveform::getState () const
{
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 AudioWaveform::getNumPeakChannels () const
{
	return peakBuffer.getNumPeakChannels ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioWaveform::getPeaks (const PeakRequest& request) const
{
	if(state == kNone)
		return 0;
	if(state == kStarted && !peakBufferReady)
		return 0;
	if(state == kFailed || state == kCanceled)
		return -1;

	int result = 0;

	// read from source directly if zoomed in further
	if(state == kCompleted && 
	   request.framesPerPixel < peakBuffer.getFramesPerPeak () &&
	   audioSource.isValid ())
	{
		AudioSourcePeakReader reader (*audioSource);
		result = reader.getPeaks (request);
	}
	else
	{
		bool allowScaledDown = state == kCompleted;
		AudioPeakBufferReader reader (const_cast<AudioPeakBuffer&> (peakBuffer), allowScaledDown);
		result = reader.getPeaks (request);
	}

	return result;
}

//************************************************************************************************
// AudioWaveform::WorkItem
//************************************************************************************************

void CCL_API AudioWaveform::WorkItem::work ()
{
	CCL_PRINTF ("Building waveform %p in background...\n", &waveform)

	AudioPeakBuilder builder (waveform.peakBuffer);
	tresult result = builder.build (source, AudioPeakBuffer::kPeakFormatDefault, 
									AudioPeakBuffer::kPeakRatioDefault, this);
	if(result == kResultOk)
		waveform.setState (kCompleted, true);
	else if(result != kResultAborted)
		waveform.setState (kFailed, true);

	CCL_PRINTF ("...waveform done.\n", &waveform)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioWaveform::WorkItem::beginProgress ()
{
	CCL_PRINTF ("... buffer ready\n", &waveform)

	// peak buffer has been fully allocated by builder
	waveform.peakBufferReady = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioWaveform::WorkItem::updateProgress (const State& state)
{
	int64 now = System::GetSystemTicks ();
	if(now - lastUpdatTime >= kUpdateInterval)
	{
		CCL_PRINTF ("... progress %.2lf\n", state.value * 100.)

		lastUpdatTime = now;
		waveform.deferChanged ();
	}
}
