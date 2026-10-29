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
// Filename    : ccl/extras/audio/waveform/audiolivepeakbuilder.h
// Description : Audio Live Peak Builder
//
//************************************************************************************************

#ifndef _ccl_audiolivepeakbuilder_h
#define _ccl_audiolivepeakbuilder_h

#include "ccl/extras/audio/waveform/audiopeakbuffer.h"

#include "ccl/public/audio/iaudiosource.h"
#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// AudioLivePeakBuilder
/** Builds peaks from live audio and appends them to an AudioPeakBuffer.

	process() is called by one producer thread. Completed peak groups are staged in a lock-free
	FIFO, flush() appends them to the attached peak buffer. */
//************************************************************************************************

class AudioLivePeakBuilder: public Object
{
public:
	DECLARE_CLASS (AudioLivePeakBuilder, Object)

	static constexpr int kDefaultFifoCapacity = 8192; ///< peak groups

	/** Bind to a prepared and empty peak buffer, not owned. Not realtime. */
	void setup (AudioPeakBuffer& peakBuffer, int fifoCapacity = kDefaultFifoCapacity);

	/** Unbind from the peak buffer, later flush() calls do nothing. Not realtime. */
	void detach ();

	/** Producer (realtime): feed audio. */
	void process (const AudioData& data, int numFrames);

	/**	Consumer: append all completed peak groups to the peak buffer,
		returns the number of groups. */
	int flush ();

	/** Number of peak groups dropped because flush() was not called often enough. */
	int getOverrunCount () const;

protected:
	AudioPeakBuffer* peakBuffer = nullptr;
	uint32 numChannels = 0;
	uint32 framesPerPeak = 0;
	uint32 fifoCapacity = 0;
	Buffer fifo[AudioPeakBuffer::kMaxChannels];

	// producer state
	uint32 framesInGroup = 0;
	float32 groupPositive[AudioPeakBuffer::kMaxChannels] = {};
	float32 groupNegative[AudioPeakBuffer::kMaxChannels] = {};

	// monotonic counters
	Threading::AtomicInt writeCount;
	Threading::AtomicInt readCount;
	Threading::AtomicInt overrunCount;

	void pushGroup ();
};

//************************************************************************************************
// AudioPeakBuilderSink
/** Passes audio to a wrapped sink and feeds to an AudioLivePeakBuilder. */
//************************************************************************************************

class AudioPeakBuilderSink: public Object,
							public IWrappedAudioSink
{
public:
	DECLARE_CLASS (AudioPeakBuilderSink, Object)

	/** Set the builder to feed (retained). */
	void setBuilder (AudioLivePeakBuilder* builder);

	// IWrappedAudioSink
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API write (const AudioData& data, int sampleFrames) override;
	tresult CCL_API setSink (IAudioSink* sink) override;
	IAudioSink* CCL_API getSink () const override;

	CLASS_INTERFACE3 (IAudioContent, IAudioSink, IWrappedAudioSink, Object)

protected:
	SharedPtr<IAudioSink> sink;
	SharedPtr<AudioLivePeakBuilder> builder;
};

} // namespace CCL

#endif // _ccl_audiolivepeakbuilder_h
