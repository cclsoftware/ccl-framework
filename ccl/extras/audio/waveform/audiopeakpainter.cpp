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
// Filename    : ccl/extras/audio/waveform/audiopeakpainter.cpp
// Description : Audio Peak Painter
//
//************************************************************************************************

#include "ccl/extras/audio/waveform/audiopeakpainter.h"

#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/framework/ivisualstyle.h"

namespace CCL {

//************************************************************************************************
// SolidPeakRenderer
//************************************************************************************************

class SolidPeakRenderer: public AudioPeakRenderer
{
public:
	// AudioPeakRenderer
	void init () override
	{
		AudioPeakRenderer::init ();
		silence = false;
	}

	void flush () override
	{
		if(silence)
		{
			graphics->drawLine (silenceStart, silenceEnd, Pen (getColor ()));
			silence = false;
		}
	}

	void renderPeak (int, const PeakPoint& peak, const PeakPoint&, Coord yMid) override
	{
		Coord y1 = Coord (ccl_min (yMid, peak.y1));
		Coord y2 = Coord (ccl_max (yMid, peak.y2));
		Coord x = Coord (peak.x);

		if(y1 == y2)
		{
			if(!silence)
			{
				silenceStart = Point (x, y1);
				silenceEnd = silenceStart;
				silence = true;
			}
			else
				silenceEnd = Point (x, y1);
		}
		else
		{
			if(silence)
			{
				graphics->drawLine (silenceStart, Point (x, Coord (yMid)), Pen (getColor ()));
				silence = false;
			}

			graphics->drawLine (Point (x, y1), Point (x, y2), Pen (getColor ()));
		}
	}

protected:
	bool silence = false;
	Point silenceStart;
	Point silenceEnd;
};

//************************************************************************************************
// SolidFramedPeakRenderer
//************************************************************************************************

class SolidFramedPeakRenderer: public AudioPeakRenderer
{
public:
	// AudioPeakRenderer
	void flush () override {}

	void renderPeak (int, const PeakPoint& peak, const PeakPoint& lastPeak, Coord yMid) override
	{
		Coord y1 = Coord (ccl_min (yMid, peak.y1));
		Coord y2 = Coord (ccl_max (yMid, peak.y2));
		Coord x = Coord (peak.x);

		Coord y1Last = Coord (ccl_min (yMid, lastPeak.y1));
		Coord y2Last = Coord (ccl_max (yMid, lastPeak.y2));
		Coord xLast = Coord (lastPeak.x);

		// connect to the previous column so the frame traces a continuous outline across the envelope
		graphics->drawLine (Point (xLast, y1Last), Point (x, y1), Pen (getFrameColor ()));
		graphics->drawLine (Point (xLast, y2Last), Point (x, y2), Pen (getFrameColor ()));

		// redraw this column's own frame cap as a strictly vertical stroke
		// a zero-length line still shows a frame.
		Coord frameY2 = Coord (ccl_max (y2, y1 + 1));
		graphics->drawLine (Point (x, y1), Point (x, frameY2), Pen (getFrameColor ()));

		if(y2 - y1 > 2)
			graphics->drawLine (Point (x, y1 + 1), Point (x, y2 - 1), Pen (getColor ()));
	}
};

//************************************************************************************************
// AntialiasedPeakRenderer
//************************************************************************************************

class AntialiasedPeakRenderer: public AudioPeakRenderer
{
public:
	static PointF makePointF (Coord x, Coord y) { return PointF (CoordF (x), CoordF (y) - .5f); }

	// AudioPeakRenderer
	void init () override
	{
		AudioPeakRenderer::init ();
		ASSERT (path1 == nullptr && path2 == nullptr)
		drawFilled = false;
	}

	void update (double framesPerPixel) override
	{
		if(framesPerPixel > 1.0)
			drawFilled = true;
	}

	void flush () override
	{
		if(path1 && graphics)
		{
			if(drawFilled)
			{
				SolidBrush brush (getColor ());

				path1->lineTo (returnPoint);
				path1->closeFigure ();
				path2->lineTo (returnPoint);
				path2->closeFigure ();

				graphics->fillPath (path1, brush);
				graphics->fillPath (path2, brush);
			}
			else
			{
				Pen pen (getColor (), 1.f);
				pen.setLineJoin (Pen::kLineJoinRound);
				graphics->drawPath (path1, pen);
				graphics->drawPath (path2, pen);
			}

			path1.release ();
			path2.release ();
		}
	}

	void renderPeak (int, const PeakPoint& peak, const PeakPoint&, Coord yMid) override
	{
		if(path1 == nullptr)
		{
			path1 = GraphicsFactory::createPath ();
			path1->startFigure (makePointF (peak.x, yMid));

			path2 = GraphicsFactory::createPath ();
			path2->startFigure (makePointF (peak.x, yMid));
		}

		path1->lineTo (makePointF (peak.x, peak.y1));
		path2->lineTo (makePointF (peak.x, peak.y2));
		returnPoint = makePointF (peak.x, yMid);
	}

protected:
	AutoPtr<IGraphicsPath> path1;
	AutoPtr<IGraphicsPath> path2;
	PointF returnPoint;
	bool drawFilled = false;
};

//************************************************************************************************
// BargraphPeakRenderer
//************************************************************************************************

class BargraphPeakRenderer: public AudioPeakRenderer
{
public:
	// AudioPeakRenderer
	void flush () override
	{}

	void renderPeak (int index, const PeakPoint& peak, const PeakPoint& lastPeak, Coord yMid) override
	{
		if(index % 2 != 0)
			return;

		auto getYRelativeMax = [&] (Coord y1, Coord y2) -> Coord
		{
			Coord y1Min = Coord (ccl_min (yMid, y1));
			Coord y2Max = Coord (ccl_max (yMid, y2));
			return Coord (ccl_max (ccl_abs (yMid - y1Min), ccl_abs (yMid - y2Max)));
		};

		Coord yRelativeMaxPeak = getYRelativeMax (peak.y1, peak.y2);
		Coord yRelativeMaxLastPeak = getYRelativeMax (lastPeak.y1, lastPeak.y2);
		Coord yRelativeMax = ccl_max (yRelativeMaxPeak, yRelativeMaxLastPeak);
		if(yRelativeMax == 0)
			yRelativeMax++;

		Coord ypos = Coord (yMid - yRelativeMax);
		Coord yneg = Coord (yMid + yRelativeMax);
		Coord x = Coord (peak.x);

		graphics->drawLine (Point (x, ypos), Point (x, yneg), Pen (getColor ()));
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static SolidPeakRenderer theSolidPeakRenderer;
static SolidFramedPeakRenderer theSolidFramedPeakRenderer;
static AntialiasedPeakRenderer theAntialiasedPeakRenderer;
static BargraphPeakRenderer theBargraphPeakRenderer;

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AudioPeakRenderer
//************************************************************************************************

void AudioPeakRenderer::init ()
{
	// turn off transparency for faster drawing
	color.alpha = 0xFF;
	frameColor.alpha = 0xFF;
}

//************************************************************************************************
// AudioPeakPainter
//************************************************************************************************

BEGIN_ENUMINFO (AudioPeakPainter::peakStyles)
	{"solid",		kPeakSolid},
	{"solidframed",	kPeakSolidFramed},
	{"antialias",	kPeakAntialias},
	{"bargraph",	kPeakBargraph},
END_ENUMINFO

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AudioPeakPainter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPeakPainter::AudioPeakPainter ()
: zeroLineColor (Colors::kBlack),
  peakColor (Colors::kBlack),
  peakFrameColor (Colors::kWhite),
  peakStyle (kPeakStyleDefault)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakPainter::updateStyle (const IVisualStyle& style)
{
	zeroLineColor = style.getColor ("zerolinecolor", zeroLineColor);
	peakColor = style.getColor ("peakcolor", peakColor);
	peakFrameColor = style.getColor ("peakframecolor", peakFrameColor);
	
	CString styleString = style.getString ("peakstyle");
	if(!styleString.isEmpty ())
		peakStyle = EnumInfo::parseOne<CString> (styleString, peakStyles, kPeakStyleDefault);
	else
		peakStyle = style.getMetric<int> ("peakstyle", kPeakStyleDefault);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPeakRenderer& AudioPeakPainter::getPeakRenderer (int peakStyle) const
{
	switch(peakStyle)
	{
	case kPeakSolid : return theSolidPeakRenderer;
	case kPeakSolidFramed : return theSolidFramedPeakRenderer;
	case kPeakAntialias : return theAntialiasedPeakRenderer;
	case kPeakBargraph : return theBargraphPeakRenderer;
	default : return theSolidPeakRenderer;
	}
}
