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
// Filename    : ccl/extras/audio/runtime/audioplayer.h
// Description : Audio Player
//
//************************************************************************************************

#ifndef _ccl_audioplayer_h
#define _ccl_audioplayer_h

#include "ccl/extras/audio/audioprocessor.h"

#include "ccl/public/audio/iaudiosource.h"
#include "ccl/public/audio/iaudiotransport.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/system/userthread.h"

namespace CCL {

//************************************************************************************************
// AudioPlayer
/** Plays audio from an assigned IAudioSource with buffering to a stereo output at a prepared
	render sample rate. Source and transport methods may be called while rendering is active.
	renderAudio () is intended for one real-time audio callback thread only.

	Player implements IAudioComponent/IAudioProcessor/IAudioDisplayLatency so it can be plugged
	directly into an audio graph via IAudioProcessorNode. 
	
	It also implements IAudioTransportClient and IAudioTransportClock so that it can serve
	as clock and/or client in a shared transport scenario. */
//************************************************************************************************

class AudioPlayer: public AudioProcessorBase,
				   public IAudioTransportClock,
				   public IAudioTransportClient
{
public:
	DECLARE_CLASS (AudioPlayer, AudioProcessorBase)

	AudioPlayer ();
	~AudioPlayer ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Player configuration (fixed)
	//////////////////////////////////////////////////////////////////////////////////////////////

	static constexpr int kOutputChannelCount = 2;
	static constexpr SpeakerBitField kOutputSpeakerFormat = SpeakerDef::kStereo;
	static constexpr int kBufferDurationMilliseconds = 2000; ///< buffer 2 seconds of audio

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Audio source
	//////////////////////////////////////////////////////////////////////////////////////////////

	/**	Assign the audio source to play (retained).
		Stops transport but keeps an active render setup. */
	bool setSource (IAudioSource* newSource);

	/**	Clear the current audio source.
		Stops transport but keeps an active render setup. */
	void clearSource ();

	/** Check if an audio source is assigned. */
	bool hasSource () const;

	/**	Get the source length in seconds.
		Returns a negative value if unbounded/unknown. */
	TSeconds getSourceDuration () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Transport state
	//////////////////////////////////////////////////////////////////////////////////////////////

	/**	Start transport from its current position.
		Requires an assigned source and prepared rendering. */
	bool start ();

	/** Stop transport without changing its position. */
	void stop ();

	/** Seek to the beginning of the source.
		Alias for seek (0); playback continues if it was playing. */
	void rewind ();

	/**	Seek to a new position, in seconds.
		Works at any time, playback continues from the new position.
		A short audio interruption is possible while the seek is applied in the background. */
	bool seek (TSeconds time);

	/** Check if transport is playing. */
	bool isPlaying () const;

	/** Get the current playback position in seconds, compensated for render latency. */
	TSeconds getPlaybackTime () const;

	/**	Enable/disable repeat.
		When enabled, playback restarts from the beginning once the end of the source is reached. */
	void setRepeat (bool state = true);

	/** Check if repeat is enabled. */
	bool isRepeat () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Realtime rendering
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Configure the stereo render chain for the given device sample rate. */
	bool prepareRender (SampleRate outputSampleRate);

	/** Stop rendering and release the render-rate-dependent source chain. */
	void unprepareRender ();

	/**	Render audio into output. Must be called from one real-time callback
		thread. Clears output on stopped, unavailable, or underrun playback. */
	int renderAudio (AudioData& output);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// IAudioComponent/IAudioProcessor/IAudioDisplayLatency
	//////////////////////////////////////////////////////////////////////////////////////////////

	CapabilityFlags CCL_API getCapabilities () const override;
	const IAudioStreamLayout& CCL_API getStreamLayout () const override;
	tresult CCL_API process (const AudioProcessData& data) override;
	void CCL_API setDisplayLatency (int latencyInSamples) override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// IAudioTransportClock/IAudioTransportClient
	//////////////////////////////////////////////////////////////////////////////////////////////

	TSeconds CCL_API getCurrentTime () const override;
	TSeconds CCL_API getMaxDuration () const override;
	StateFlags CCL_API getClientState () const override;
	void CCL_API onTransportEvent (const AudioTransportEvent& e) override;

	CLASS_INTERFACE2 (IAudioTransportClock, IAudioTransportClient, AudioProcessorBase)

protected:
	class FillerThread: public Unknown,
						public AbstractProgressNotify,
						public Threading::UserThread

	{
	public:
		FillerThread (AudioPlayer& player);

		// IProgressNotify
		tbool CCL_API isCanceled () override { return shouldTerminate (); }

		CLASS_INTERFACE (IProgressNotify, Unknown)

	protected:
		AudioPlayer& player;

		// UserThread
		int threadEntry () override;
	};

	mutable Threading::CriticalSection lock;
	Threading::Signal fillerSignal;
	FillerThread fillerThread;

	AudioStreamLayout streamLayout;
	SharedPtr<IAudioSource> source;
	SharedPtr<IBufferedAudioSource> bufferedSource;

	SampleRate sourceSampleRate = 0;
	TSamples sourceFrameCount = 0;
	TSamples storedPosition = 0;
	TSamples renderStartPosition = 0;
	TSamples renderedFrameCount = 0;
	SampleRate renderSampleRate = 0;
	int renderLatencyInFrames = 0;
	bool renderPrepared = false;
	bool playing = false;
	bool repeat = false;
	bool sourceExhausted = false;
	int sourceGeneration = 0;

	bool rebuildRenderChainLocked ();
	void releaseRenderChainLocked ();
	TSamples getPositionLocked (bool compensateLatency) const;
	void resetPositionLocked (TSamples position);
	void seekLocked (TSamples position);
	void setSourceExhausted (IBufferedAudioSource* source, int generation);
	bool getFillSource (SharedPtr<IBufferedAudioSource>& source, int& generation);

	// AudioProcessorBase
	void onActiveChanged (bool state) override;
};

} // namespace CCL

#endif // _ccl_audioplayer_h
