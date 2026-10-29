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
// Filename    : ccl/extras/audio/meter/audiometerview.h
// Description : Audio Meter View
//
//************************************************************************************************

#ifndef _ccl_audiometerview_h
#define _ccl_audiometerview_h

#include "ccl/app/controls/usercontrol.h"

#include "ccl/extras/audio/meter/audiometerbuffer.h"

namespace CCL {

//************************************************************************************************
// AudioMeterView
/** Displays peak/RMS/clip of every channel of an AudioMeterBuffer. */
//************************************************************************************************

class AudioMeterView: public UserControl,
					  public IAudioMeterClient
{
public:
	DECLARE_CLASS (AudioMeterView, UserControl)

	AudioMeterView (AudioMeterBuffer* buffer = nullptr, RectRef size = Rect (), StyleRef style = Styles::kVertical);
	~AudioMeterView ();

	/** Assign the meter buffer to display (retained). */
	void setMeterBuffer (AudioMeterBuffer* buffer);
	AudioMeterBuffer* getMeterBuffer () const { return meterBuffer; }

	// UserControl
	void attached (IView* parent) override;
	void onVisualStyleChanged () override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void draw (const DrawEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;

protected:
	static constexpr Coord kClipIndicatorSize = 4;

	SharedPtr<AudioMeterBuffer> meterBuffer;

	Brush backBrush;
	Brush foreBrush;
	Brush rmsBrush;
	Brush clipBrush;
	Brush peakHoldBrush;

	void updateStyle ();
	float levelToFraction (float levelDb, const AudioMeterSettings& settings) const;
	void drawChannel (IGraphics& graphics, const Rect& rect, int channel, bool horizontal);

	// IAudioMeterClient
	void meterDataChanged (const AudioMeterBuffer& buffer) override;
	void meterSettingsChanged (const AudioMeterBuffer& buffer) override;
};

} // namespace CCL

#endif // _ccl_audiometerview_h
