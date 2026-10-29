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
// Filename    : ccl/extras/audio/waveform/audiowaveformview.h
// Description : Audio Waveform View
//
//************************************************************************************************

#ifndef _ccl_audiowaveformview_h
#define _ccl_audiowaveformview_h

#include "ccl/app/controls/usercontrol.h"

#include "ccl/extras/audio/waveform/audiowaveform.h"
#include "ccl/extras/audio/waveform/audiopeakpainter.h"

#include "ccl/public/audio/iaudiotransport.h"

namespace CCL {

//************************************************************************************************
// AudioWaveformView
/** Displays every channel of an AudioWaveform across its complete duration.
	A live waveform is displayed with a fixed resolution, showing the most recent part. */
//************************************************************************************************

class AudioWaveformView: public UserControl,
						 public IAudioTransportObserver
{
public:
	DECLARE_CLASS (AudioWaveformView, UserControl)

	AudioWaveformView (AudioWaveform* waveform = nullptr,
					   IAudioTransport* transport = nullptr,
					   RectRef size = Rect (),
					   StyleRef style = 0);
	~AudioWaveformView ();

	/** Assign the waveform to display (retained). */
	void setWaveform (AudioWaveform* waveform);
	AudioWaveform* getWaveform () const { return waveform; }

	/** Assign transport for position display (retained). */
	void setTransport (IAudioTransport* transport);
	IAudioTransport* getTransport () const { return transport; }

	/** Duration shown for a live waveform, which scrolls once it is longer. */
	void setLivePreviewDuration (TSeconds duration);
	TSeconds getLivePreviewDuration () const { return livePreviewDuration; }
	static constexpr TSeconds kDefaultLivePreviewDuration = 10.;

	// UserControl
	void attached (IView* parent) override;
	void onVisualStyleChanged () override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void draw (const DrawEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IAudioTransportObserver, UserControl)

protected:
	SharedPtr<AudioWaveform> waveform;
	SharedPtr<IAudioTransport> transport;
	TSeconds livePreviewDuration = kDefaultLivePreviewDuration;

	AudioPeakPainter peakPainter;
	Brush backBrush;
	Brush foreBrush;

	void updateStyle ();

	// IAudioTransportObserver
	void CCL_API onTransportStateChanged () override;
	void CCL_API onTransportDurationChanged (TSeconds duration) override;
	void CCL_API onTransportTimeChanged (TSeconds time) override;
};

} // namespace CCL

#endif // _ccl_audiowaveformview_h
