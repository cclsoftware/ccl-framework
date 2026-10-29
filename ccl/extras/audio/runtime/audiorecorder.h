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
// Filename    : ccl/extras/audio/runtime/audiorecorder.h
// Description : Audio Recorder
//
//************************************************************************************************

#ifndef _ccl_audiorecorder_h
#define _ccl_audiorecorder_h

#include "ccl/extras/audio/audioprocessor.h"

#include "ccl/public/audio/iaudiosource.h"
#include "ccl/public/audio/iaudiotransport.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/system/userthread.h"

namespace CCL {

//************************************************************************************************
// IAudioRecordHandler
/**	Provides the sinks for the takes of an AudioRecorder and is told when a take is done.
	All methods are called on the main thread. */
//************************************************************************************************

interface IAudioRecordHandler
{
	/**	Create the sink for a new take at the given sample rate. */
	virtual IAudioSink* createRecordSink (SampleRate sampleRate) = 0;

	/**	A take has ended (or sink was rejected).
		The sink has received all frames and is not accessed by the recorder anymore. */
	virtual void onRecordTakeFinished (IAudioSink* sink, tresult result) = 0;
};

//************************************************************************************************
// AudioRecorder
/** Records the first input channels to a sink provided by an IAudioRecordHandler, with the
	sink being written to by a background thread.

	Recorder implements IAudioComponent/IAudioProcessor so it can be plugged
	directly into an audio graph via IAudioProcessorNode. 
	
	It also implements IAudioTransportClient and IAudioTransportClock so that it can serve
	as clock and/or client in a shared transport scenario. */
//************************************************************************************************

class AudioRecorder: public AudioProcessorBase,
					 public IAudioTransportClock,
					 public IAudioTransportClient
{
public:
	DECLARE_CLASS_ABSTRACT (AudioRecorder, AudioProcessorBase)

	AudioRecorder (IAudioRecordHandler& handler);
	~AudioRecorder ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Recorder configuration (fixed)
	//////////////////////////////////////////////////////////////////////////////////////////////

	static constexpr int kChannelCount = 2; ///< maximum sink channels, input and (silent) output
	static constexpr SpeakerBitField kSpeakerFormat = SpeakerDef::kStereo;
	static constexpr int kBufferDurationMilliseconds = 2000; ///< buffer 2 seconds of audio

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Transport state
	//////////////////////////////////////////////////////////////////////////////////////////////

	/**	Start a take. */
	bool startRecording ();

	/** End the take. */
	void stopRecording ();

	/** Check if a take is currently recorded. */
	bool isRecording () const;

	/** Get recorded time in seconds. */
	TSeconds getRecordedTime () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Realtime capture
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Prepare capture for the given sample rate and start the background thread. */
	bool prepareCapture (SampleRate inputSampleRate);

	/** End a running take and stop the background thread. */
	void unprepareCapture ();

	/**	Record audio into the current take. Must be called from one audio realtime thread.
		Maps the input to the sink channels, returns the number of frames accepted. */
	int captureAudio (const AudioData& input);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// IAudioComponent/IAudioProcessor
	//////////////////////////////////////////////////////////////////////////////////////////////

	CapabilityFlags CCL_API getCapabilities () const override;
	const IAudioStreamLayout& CCL_API getStreamLayout () const override;
	tresult CCL_API process (const AudioProcessData& data) override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// IAudioTransportClock/IAudioTransportClient
	//////////////////////////////////////////////////////////////////////////////////////////////

	TSeconds CCL_API getCurrentTime () const override;
	TSeconds CCL_API getMaxDuration () const override;
	StateFlags CCL_API getClientState () const override;
	void CCL_API onTransportEvent (const AudioTransportEvent& e) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IAudioTransportClock, IAudioTransportClient, AudioProcessorBase)

protected:
	DECLARE_STRINGID_MEMBER (kSinkFailed)

	class FlushThread: public Unknown,
					   public AbstractProgressNotify,
					   public Threading::UserThread
	{
	public:
		FlushThread (AudioRecorder& recorder);

		// IProgressNotify
		tbool CCL_API isCanceled () override { return shouldTerminate (); }

		CLASS_INTERFACE (IProgressNotify, Unknown)

	protected:
		AudioRecorder& recorder;

		// UserThread
		int threadEntry () override;
	};

	IAudioRecordHandler& handler;

	mutable Threading::CriticalSection lock;
	Threading::CriticalSection flushLock;
	Threading::Signal flushSignal;
	FlushThread flushThread;

	AudioStreamLayout streamLayout;
	SharedPtr<IAudioSink> sink;
	SharedPtr<IBufferedAudioSink> bufferedSink;
	int sinkChannelCount = 0;
	SampleRate captureSampleRate = 0;
	bool capturePrepared = false;

	Threading::AtomicInt recording;
	Threading::AtomicInt failed;
	Threading::AtomicInt recordedFrames;

	void endTake ();
	bool getFlushSink (SharedPtr<IBufferedAudioSink>& flushSink);
	void onSinkFailed ();

	// AudioProcessorBase
	void onActiveChanged (bool state) override;
};

} // namespace CCL

#endif // _ccl_audiorecorder_h
