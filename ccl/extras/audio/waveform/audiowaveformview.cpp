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
// Filename    : ccl/extras/audio/waveform/audiowaveformview.cpp
// Description : Audio Waveform View
//
//************************************************************************************************

#include "ccl/extras/audio/waveform/audiowaveformview.h"

using namespace CCL;

//************************************************************************************************
// AudioWaveformView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioWaveformView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioWaveformView::AudioWaveformView (AudioWaveform* waveform, IAudioTransport* transport, 
									  RectRef size, StyleRef style)
: UserControl (size, style),
  backBrush (SolidBrush (Colors::kWhite)),
  foreBrush (SolidBrush (Colors::kLtGray))
{
	setWaveform (waveform);
	setTransport (transport);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioWaveformView::~AudioWaveformView ()
{
	setWaveform (nullptr);
	setTransport (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::setWaveform (AudioWaveform* newWaveform)
{
	if(waveform != newWaveform)
	{
		if(waveform)
			waveform->removeObserver (this);

		waveform = newWaveform;
		
		if(waveform)
			waveform->addObserver (this);

		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::setTransport (IAudioTransport* newTransport)
{
	if(transport != newTransport)
	{
		if(transport)
			transport->removeObserver (this);

		transport = newTransport;
		
		if(transport)
			transport->addObserver (this);

		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::setLivePreviewDuration (TSeconds duration)
{
	if(livePreviewDuration != duration)
	{
		livePreviewDuration = duration;
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::attached (IView* parent)
{
	SuperClass::attached (parent);
	updateStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::onVisualStyleChanged ()
{
	updateStyle ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	updateStyle ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::updateStyle ()
{
	const IVisualStyle& vs = getVisualStyle ();
	peakPainter.updateStyle (vs);
	backBrush = vs.getBackBrush ();
	foreBrush = vs.getForeBrush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioWaveformView::draw (const DrawEvent& event)
{
	Rect rect;
	getClientRect (rect);
	Coord width = rect.getWidth ();
	if(width <= 0)
		return;

	IGraphics& g = event.graphics;
	if(backBrush.isVisible ())
	{
		g.fillRect (event.updateRgn.bounds, backBrush);
	}

	if(waveform)
	{
		TSamples frameCount = waveform->getFrameCount ();
		if(frameCount > 0)
		{
			double framesPerPixel = double(frameCount) / double(width);
			int64 pixelOffset = 0;

			// live waveform: fixed resolution showing the most recent part, scrolling once it is longer
			SampleRate sampleRate = waveform->getSourceSampleRate ();
			if(waveform->isLive () && sampleRate > 0 && livePreviewDuration > 0)
			{
				framesPerPixel = livePreviewDuration * sampleRate / double(width);
				pixelOffset = get_max<int64> (0, int64(double(frameCount) / framesPerPixel) - width);
			}

			AudioPeakPainter::DrawArgs args (*waveform, g, framesPerPixel, pixelOffset);
			peakPainter.drawAll (args, rect);
		}
	}

	if(transport && transport->isActive () && !transport->isRecording ())
	{
		TSeconds duration = waveform ? waveform->getDuration () : 0;
		TSeconds time = transport->getCurrentTime ();
		if(duration > 0 && time >= 0 && time <= duration)
		{
			float position = float(time / duration);

			constexpr Coord kCursorW = 1;

			Rect cursorRect (0, 0, kCursorW, rect.getHeight ());
			Coord x = get_min<Coord> (coordFToInt (position * width), width - kCursorW);
			cursorRect.offset (x);

			g.fillRect (cursorRect, foreBrush);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioWaveformView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(waveform && subject == waveform)
		{
			updateClient ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioWaveformView::onTransportStateChanged ()
{
	updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioWaveformView::onTransportDurationChanged (TSeconds duration)
{
	updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioWaveformView::onTransportTimeChanged (TSeconds time)
{
	updateClient ();
}
