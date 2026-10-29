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
// Filename    : ccl/extras/audio/meter/audiometerview.cpp
// Description : Audio Meter View
//
//************************************************************************************************

#include "ccl/extras/audio/meter/audiometerview.h"

#include "ccl/public/gui/graphics/igraphics.h"

using namespace CCL;

//************************************************************************************************
// AudioMeterView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioMeterView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioMeterView::AudioMeterView (AudioMeterBuffer* buffer, RectRef size, StyleRef style)
: UserControl (size, style),
  backBrush (SolidBrush (Colors::kBlack)),
  foreBrush (SolidBrush (Colors::kGreen)),
  rmsBrush (SolidBrush (Colors::kWhite)),
  clipBrush (SolidBrush (Colors::kRed)),
  peakHoldBrush (SolidBrush (Colors::kRed))
{
	setMeterBuffer (buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioMeterView::~AudioMeterView ()
{
	setMeterBuffer (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::setMeterBuffer (AudioMeterBuffer* newBuffer)
{
	if(meterBuffer != newBuffer)
	{
		if(meterBuffer)
			meterBuffer->removeClient (this);

		meterBuffer = newBuffer;

		if(meterBuffer)
			meterBuffer->addClient (this);

		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::attached (IView* parent)
{
	SuperClass::attached (parent);
	updateStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::onVisualStyleChanged ()
{
	updateStyle ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	updateStyle ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::updateStyle ()
{
	const IVisualStyle& vs = getVisualStyle ();

	backBrush = vs.getBrush (StyleID::kBackColor, backBrush.getColor ());
	foreBrush = vs.getBrush (StyleID::kForeColor, foreBrush.getColor ());
	rmsBrush = vs.getBrush ("rmscolor", rmsBrush.getColor ());
	clipBrush = vs.getBrush ("clipcolor", clipBrush.getColor ());
	peakHoldBrush = vs.getBrush ("peakholdcolor", peakHoldBrush.getColor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AudioMeterView::levelToFraction (float levelDb, const AudioMeterSettings& settings) const
{
	float minDb = settings.getMinDb ();
	float maxDb = settings.getMaxDb ();
	if(maxDb <= minDb)
		return 0.f;

	float fraction = (levelDb - minDb) / (maxDb - minDb);
	return get_max (0.f, get_min (1.f, fraction));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::drawChannel (IGraphics& graphics, const Rect& rect, int channel, bool horizontal)
{
	if(rect.getWidth () <= 0 || rect.getHeight () <= 0)
		return;

	graphics.fillRect (rect, backBrush);

	AudioMeterSettings& settings = meterBuffer->getSettings ();
	float peakFraction = levelToFraction (meterBuffer->getPeakLevelDb (channel), settings);

	// Peak: filled bar from the silent end
	Rect barRect (rect);
	if(horizontal)
		barRect.right = barRect.left + coordFToInt (peakFraction * rect.getWidth ());
	else
		barRect.top = barRect.bottom - coordFToInt (peakFraction * rect.getHeight ());
	graphics.fillRect (barRect, foreBrush);

	// RMS: thin line on top of the peak fill
	if(settings.isRMSEnabled ())
	{
		float rmsFraction = levelToFraction (meterBuffer->getRMSLevelDb (channel), settings);

		Rect rmsRect (rect);
		if(horizontal)
		{
			Coord rmsX = rect.left + coordFToInt (rmsFraction * rect.getWidth ());
			rmsRect.left = get_max (rect.left, rmsX - 1);
			rmsRect.right = rmsX;
		}
		else
		{
			Coord rmsY = rect.bottom - coordFToInt (rmsFraction * rect.getHeight ());
			rmsRect.top = rmsY;
			rmsRect.bottom = get_min (rect.bottom, rmsY + 1);
		}
		graphics.fillRect (rmsRect, rmsBrush);
	}

	// Peak hold: thin line marking the highest recently-held peak
	if(settings.isPeakHoldEnabled ())
	{
		float peakHoldFraction = levelToFraction (meterBuffer->getPeakHoldLevelDb (channel), settings);

		Rect peakHoldRect (rect);
		if(horizontal)
		{
			Coord peakHoldX = rect.left + coordFToInt (peakHoldFraction * rect.getWidth ());
			peakHoldRect.left = get_max (rect.left, peakHoldX - 1);
			peakHoldRect.right = peakHoldX;
		}
		else
		{
			Coord peakHoldY = rect.bottom - coordFToInt (peakHoldFraction * rect.getHeight ());
			peakHoldRect.top = peakHoldY;
			peakHoldRect.bottom = get_min (rect.bottom, peakHoldY + 1);
		}
		graphics.fillRect (peakHoldRect, peakHoldBrush);
	}

	// Clip: fixed-size cell at the loud end, lit only while clipped
	if(settings.isClipEnabled () && meterBuffer->isClipped (channel))
	{
		Rect clipRect (rect);
		if(horizontal)
			clipRect.left = clipRect.right - kClipIndicatorSize;
		else
			clipRect.bottom = clipRect.top + kClipIndicatorSize;
		graphics.fillRect (clipRect, clipBrush);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::draw (const DrawEvent& event)
{
	Rect rect;
	getClientRect (rect);
	if(rect.getWidth () <= 0 || rect.getHeight () <= 0 || !meterBuffer)
		return;

	int channelCount = meterBuffer->getChannelCount ();
	if(channelCount <= 0)
		return;

	if(getStyle ().isHorizontal ())
	{
		Coord channelHeight = rect.getHeight () / channelCount;
		for(int ch = 0; ch < channelCount; ch++)
		{
			Rect channelRect (rect);
			channelRect.top = rect.top + ch * channelHeight;
			channelRect.bottom = channelRect.top + channelHeight;
			drawChannel (event.graphics, channelRect, ch, true);
		}
	}
	else
	{
		Coord channelWidth = rect.getWidth () / channelCount;
		for(int ch = 0; ch < channelCount; ch++)
		{
			Rect channelRect (rect);
			channelRect.left = rect.left + ch * channelWidth;
			channelRect.right = channelRect.left + channelWidth;
			drawChannel (event.graphics, channelRect, ch, false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioMeterView::onMouseDown (const MouseEvent& event)
{
	if(meterBuffer)
		meterBuffer->resetHolds ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::meterDataChanged (const AudioMeterBuffer& buffer)
{
	updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioMeterView::meterSettingsChanged (const AudioMeterBuffer& buffer)
{
	updateClient ();
}
