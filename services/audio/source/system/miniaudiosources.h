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
// Filename    : miniaudiosources.h
// Description : Audio Sources based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudiosources_h
#define _miniaudiosources_h

#include "miniaudioringbuffer.h"
#include "miniaudioresampler.h"
#include "miniaudioconverter.h"

#include "../base/audiosourcebase.h"

#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// MiniAudioBufferedSource
/** Buffers an IAudioSource so a real-time consumer can call read() without
	blocking. The caller assigns/resizes the source and drives fill() from
	whichever thread it chooses, while read() drains the buffer independently. */
//************************************************************************************************

class MiniAudioBufferedSource: public AudioSourceBase,
							   public IBufferedAudioSource
{
public:
	DECLARE_CLASS (MiniAudioBufferedSource, AudioSourceBase)

	// IBufferedAudioSource
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API read (AudioData& data, int sampleFrames) override;
	TSamples CCL_API seek (TSamples position) override;
	TSamples CCL_API tell () const override;
	tresult CCL_API setSource (IAudioSource* source) override;
	IAudioSource* CCL_API getSource () const override;
	tresult CCL_API resize (int capacity) override;
	int CCL_API getCapacity () const override;
	tresult CCL_API fill (IProgressNotify* progress = nullptr) override;
	int CCL_API getBufferedAmount () const override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IAudioContent)
		QUERY_INTERFACE (IAudioSource)
		QUERY_INTERFACE (IWrappedAudioSource)
		QUERY_INTERFACE (IBufferedAudioSource)
	END_CLASS_INTERFACES (AudioSourceBase)

protected:
	static constexpr int kDefaultCapacityInFrames = 8192;

	SharedPtr<IAudioSource> source;
	int capacityInFrames = kDefaultCapacityInFrames;
	MiniAudioRingBuffer ringBuffer;

	TSamples readPosition = 0;

	Threading::CriticalSection seekLock;
	bool hasPendingSeek = false;
	TSamples pendingSeekTarget = 0;

	void reinitializeRingBuffer ();
	void applyPendingSeek ();
};

//************************************************************************************************
// MiniAudioBufferedSink
/** Buffers an IAudioSink so a real-time producer can call write() without
	blocking. The caller assigns/resizes the sink and drives flush() from
	whichever thread it chooses, while write() fills the buffer independently. */
//************************************************************************************************

class MiniAudioBufferedSink: public AudioSinkBase,
							 public IBufferedAudioSink
{
public:
	DECLARE_CLASS (MiniAudioBufferedSink, AudioSinkBase)

	// IBufferedAudioSink
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API write (const AudioData& data, int sampleFrames) override;
	tresult CCL_API setSink (IAudioSink* sink) override;
	IAudioSink* CCL_API getSink () const override;
	tresult CCL_API resize (int capacity) override;
	int CCL_API getCapacity () const override;
	tresult CCL_API flush (IProgressNotify* progress = nullptr) override;
	int CCL_API getBufferedAmount () const override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IAudioContent)
		QUERY_INTERFACE (IAudioSink)
		QUERY_INTERFACE (IWrappedAudioSink)
		QUERY_INTERFACE (IBufferedAudioSink)
	END_CLASS_INTERFACES (AudioSinkBase)

protected:
	static constexpr int kDefaultCapacityInFrames = 8192;

	SharedPtr<IAudioSink> sink;
	int capacityInFrames = kDefaultCapacityInFrames;
	MiniAudioRingBuffer ringBuffer;

	void reinitializeRingBuffer ();
};

//************************************************************************************************
// MiniAudioResamplingSource
/** Wraps an IAudioSource, converting its audio to a different sample rate on read(). */
//************************************************************************************************

class MiniAudioResamplingSource: public AudioSourceBase,
								 public IResamplingAudioSource
{
public:
	DECLARE_CLASS (MiniAudioResamplingSource, AudioSourceBase)

	// IResamplingAudioSource
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API read (AudioData& data, int sampleFrames) override;
	TSamples CCL_API seek (TSamples position) override;
	TSamples CCL_API tell () const override;
	tresult CCL_API setSource (IAudioSource* source) override;
	IAudioSource* CCL_API getSource () const override;
	tresult CCL_API setTargetRate (SampleRate sampleRate) override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IAudioContent)
		QUERY_INTERFACE (IAudioSource)
		QUERY_INTERFACE (IWrappedAudioSource)
		QUERY_INTERFACE (IResamplingAudioSource)
	END_CLASS_INTERFACES (AudioSourceBase)

protected:
	static constexpr int kCarryCapacityInFrames = 8192; // in source-rate frames

	SharedPtr<IAudioSource> source;
	SampleRate targetRate = 0;
	MiniAudioResampler resampler;
	MiniAudioRingBuffer carryBuffer;

	void reconfigureResampler ();
};

//************************************************************************************************
// MiniAudioResamplingSink
/** Wraps an IAudioSink, converting incoming audio from a different sample rate on write(). */
//************************************************************************************************

class MiniAudioResamplingSink: public AudioSinkBase,
							   public IResamplingAudioSink
{
public:
	DECLARE_CLASS (MiniAudioResamplingSink, AudioSinkBase)

	// IAudioSink
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API write (const AudioData& data, int sampleFrames) override;
	tresult CCL_API setSink (IAudioSink* sink) override;
	IAudioSink* CCL_API getSink () const override;
	tresult CCL_API setSourceRate (SampleRate sampleRate) override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IAudioContent)
		QUERY_INTERFACE (IAudioSink)
		QUERY_INTERFACE (IWrappedAudioSink)
		QUERY_INTERFACE (IResamplingAudioSink)
	END_CLASS_INTERFACES (AudioSinkBase)

protected:
	SharedPtr<IAudioSink> sink;
	SampleRate sourceRate = 0;
	MiniAudioResampler resampler;

	void reconfigureResampler ();
};

//************************************************************************************************
// MiniAudioConvertingSource
/** Wraps an IAudioSource, converting its channel count/layout on read(). */
//************************************************************************************************

class MiniAudioConvertingSource: public AudioSourceBase,
								 public IConvertingAudioSource
{
public:
	DECLARE_CLASS (MiniAudioConvertingSource, AudioSourceBase)

	// IConvertingAudioSource
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API read (AudioData& data, int sampleFrames) override;
	TSamples CCL_API seek (TSamples position) override;
	TSamples CCL_API tell () const override;
	tresult CCL_API setSource (IAudioSource* source) override;
	IAudioSource* CCL_API getSource () const override;
	tresult CCL_API setTargetChannels (int channelCount, SpeakerBitField speakerFormat) override;
	tresult CCL_API setChannelMixMode (ChannelMixMode mode) override;
	ChannelMixMode CCL_API getChannelMixMode () const override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IAudioContent)
		QUERY_INTERFACE (IAudioSource)
		QUERY_INTERFACE (IWrappedAudioSource)
		QUERY_INTERFACE (IConvertingAudioSource)
	END_CLASS_INTERFACES (AudioSourceBase)

protected:
	SharedPtr<IAudioSource> source;
	int targetChannelCount = 0;
	SpeakerBitField targetSpeakerFormat = SpeakerDef::kEmpty;
	MiniAudioChannelConverter converter;

	void reconfigureConverter ();
};

//************************************************************************************************
// MiniAudioConvertingSink
/** Wraps an IAudioSink, converting incoming audio from a different channel count/layout on write(). */
//************************************************************************************************

class MiniAudioConvertingSink: public AudioSinkBase,
							   public IConvertingAudioSink
{
public:
	DECLARE_CLASS (MiniAudioConvertingSink, AudioSinkBase)

	// IAudioSink
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API write (const AudioData& data, int sampleFrames) override;
	tresult CCL_API setSink (IAudioSink* sink) override;
	IAudioSink* CCL_API getSink () const override;
	tresult CCL_API setSourceChannels (int channelCount, SpeakerBitField speakerFormat) override;
	tresult CCL_API setChannelMixMode (ChannelMixMode mode) override;
	ChannelMixMode CCL_API getChannelMixMode () const override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IAudioContent)
		QUERY_INTERFACE (IAudioSink)
		QUERY_INTERFACE (IWrappedAudioSink)
		QUERY_INTERFACE (IConvertingAudioSink)
	END_CLASS_INTERFACES (AudioSinkBase)

protected:
	SharedPtr<IAudioSink> sink;
	int sourceChannelCount = 0;
	SpeakerBitField sourceSpeakerFormat = SpeakerDef::kEmpty;
	MiniAudioChannelConverter converter;

	void reconfigureConverter ();
};

} // namespace CCL

#endif // _miniaudiosources_h
