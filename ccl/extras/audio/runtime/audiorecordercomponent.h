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
// Filename    : ccl/extras/audio/runtime/audiorecordercomponent.h
// Description : Audio Recorder Component
//
//************************************************************************************************

#ifndef _ccl_audiorecordercomponent_h
#define _ccl_audiorecordercomponent_h

#include "ccl/app/component.h"

#include "ccl/extras/audio/runtime/audiorecorder.h"
#include "ccl/extras/audio/waveform/audiowaveform.h"

namespace CCL {

//************************************************************************************************
// AudioRecorderComponent
/**	Wraps an AudioRecorder with parameters for channels (mono/stereo) and file name and
	creates an AudioWaveformView of the current take.

	Capturing is left to the host application. Use getRecorder () to call
	(un)prepareCapture ()/captureAudio () from the host's own audio device callback,
	or to add it to an audio graph. */
//************************************************************************************************

class AudioRecorderComponent: public Component,
							  public IAudioRecordHandler,
							  public IAudioTransportObserver
{
public:
	DECLARE_CLASS (AudioRecorderComponent, Component)

	AudioRecorderComponent (StringRef name = nullptr);
	~AudioRecorderComponent ();

	/** Assign shared transport. */
	void setTransport (IAudioTransport* transport);

	/** Access the underlying audio recorder. */
	AudioRecorder& getRecorder () const { return *recorder; }

	// IAudioRecordHandler
	IAudioSink* createRecordSink (SampleRate sampleRate) override;
	void onRecordTakeFinished (IAudioSink* sink, tresult result) override;

	PROPERTY_SHARED_AUTO (Url, lastRecordedFile, LastRecordedFile)

	// IAudioTransportObserver
	void CCL_API onTransportStateChanged () override;
	void CCL_API onTransportDurationChanged (TSeconds duration) override;
	void CCL_API onTransportTimeChanged (TSeconds time) override;

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tresult CCL_API terminate () override;

	CLASS_INTERFACE (IAudioTransportObserver, Component)

protected:
	static const String kDefaultFileName;

	AutoPtr<AudioRecorder> recorder;
	AutoPtr<AudioWaveform> waveform;
	SharedPtr<IAudioTransport> transport;
	bool busy = false;
};

} // namespace CCL

#endif // _ccl_audiorecordercomponent_h
