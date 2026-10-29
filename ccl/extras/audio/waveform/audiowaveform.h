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
// Filename    : ccl/extras/audio/waveform/audiowaveform.h
// Description : Audio Waveform
//
//************************************************************************************************

#ifndef _ccl_audiowaveform_h
#define _ccl_audiowaveform_h

#include "ccl/extras/audio/waveform/audiolivepeakbuilder.h"

#include "ccl/public/base/iasyncoperation.h"

namespace CCL {

//************************************************************************************************
// AudioWaveform
/**	Audio waveform stored in a peak buffer and optionally read from an audio source
	when zoomed in. Updates its state and sends kChanged notifications while being built
	in background or live. */
//************************************************************************************************

class AudioWaveform: public Object,
					 public IAsyncInfo,
					 public AudioPeakSource
{
public:
	DECLARE_CLASS (AudioWaveform, Object)

	AudioWaveform ();
	~AudioWaveform ();

	/** Assign source to get audio from (retained). */
	void setAudioSource (IAudioSource* audioSource);
	
	/** Sample rate of audio source/live content. */
	PROPERTY_VARIABLE (SampleRate, sourceSampleRate, SourceSampleRate)
		
	/**	Start building waveform in background,
		will signal kChanged while it's updating. */
	void startBuilding ();

	/** Stop building waveform in background. */
	void stopBuilding ();

	/**	Start a live waveform that grows while audio is recorded.
		Returns false if the format is not valid. */
	bool startLive (const AudioContentFormat& format, uint32 framesPerPeak = AudioPeakBuffer::kPeakRatioDefault);

	/**	Append the peaks produced so far to the live waveform.
		Call regularly from the main thread. */
	void updateLive ();

	/**	Finish a live waveform after a final update.
		Optionally sets the exact number of frames recorded. */
	void finishLive (TSamples totalFrames = 0);

	/** Check if a live waveform is being recorded. */
	bool isLive () const { return liveBuilder != nullptr; }
	AudioLivePeakBuilder* getLiveBuilder () const { return liveBuilder; }

	/** Get length of waveform in audio samples. */
	TSamples getFrameCount () const;

	/** Get length of waveform in seconds. */
	TSeconds getDuration () const;

	// IAsyncInfo
	State CCL_API getState () const override;

	// AudioPeakSource
	uint32 getNumPeakChannels () const override;
	int getPeaks (const PeakRequest& request) const override;

	CLASS_INTERFACE (IAsyncInfo, Object)

protected:
	class WorkItem;

	AudioPeakBuffer peakBuffer;
	SharedPtr<IAudioSource> audioSource;
	Threading::AtomicInt state;
	Threading::AtomicInt peakBufferReady;
	AutoPtr<AudioLivePeakBuilder> liveBuilder;

	void setState (State newState, bool deferred = false);
};

} // namespace CCL

#endif // _ccl_audiowaveform_h
