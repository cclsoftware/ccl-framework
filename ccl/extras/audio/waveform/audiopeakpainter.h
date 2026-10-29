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
// Filename    : ccl/extras/audio/waveform/audiopeakpainter.h
// Description : Audio Peak Painter
//
//************************************************************************************************

#ifndef _ccl_audiopeakpainter_h
#define _ccl_audiopeakpainter_h

#include "ccl/extras/audio/waveform/audiopeakbuffer.h"

#include "ccl/public/base/enumdef.h"
#include "ccl/public/gui/graphics/igraphics.h"

namespace CCL {

interface IVisualStyle;
class AudioPeakPainter;

//************************************************************************************************
// PeakPoint
/** One drawn peak column, in target coordinates. */
//************************************************************************************************

struct PeakPoint
{
	PeakPoint (Coord x = 0, Coord y1 = 0, Coord y2 = 0, float fPos = 0.f, float fNeg = 0.f)
	: x (x), y1 (y1), y2 (y2), fPos (fPos), fNeg (fNeg)
	{}

	Coord x;
	Coord y1;
	Coord y2;
	float fPos;
	float fNeg;
};

//************************************************************************************************
// AudioPeakRenderer
/** Draws one peak style to an IGraphics. */
//************************************************************************************************

class AudioPeakRenderer
{
public:
	AudioPeakRenderer ()
	: graphics (nullptr)
	{}
	
	PROPERTY_POINTER (IGraphics, graphics, Graphics)
	PROPERTY_VARIABLE (Color, color, Color)
	PROPERTY_VARIABLE (Color, frameColor, FrameColor)

	/** Reset renderer state before a run of renderPeak() calls. */
	virtual void init ();

	/** Draw any geometry buffered since the last flush(). */
	virtual void flush () = 0;

	/** Called once after the last renderPeak() call. */
	virtual void finish () { flush (); }

	/** Called once per contiguous same-resolution pixel batch before peaks are rendered. */
	virtual void update (double framesPerPixel) {}

	/** Render one peak column. */
	virtual void renderPeak (int index, const PeakPoint& peak, const PeakPoint& lastPeak, Coord yMid) = 0;
};

//************************************************************************************************
// AudioPeakScaler
/** Optional amplitude scaler applied per pixel while drawing. */
//************************************************************************************************

class AudioPeakScaler
{
public:
	/**	Get an amplitude multiplier for one pixel
		(pixel * framesPerPixel is the source frame position). */
	virtual float getScaleFactor (int64 pixel, double framesPerPixel) = 0;
};

//************************************************************************************************
// AudioPeakDrawArgs
/**	Parameters for AudioPeakPainter::drawAll()/drawChannel(); also documents the structural contract
	for TArgs. A substitute type must expose the same public members. */
//************************************************************************************************

struct AudioPeakDrawArgs
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Members by contract
	//////////////////////////////////////////////////////////////////////////////////////////////

	const AudioPeakSource& source;	///< source to get peaks from
	IGraphics& graphics;			///< IGraphics target to draw to
	int64 pixelOffset;				///< pixel offset into audio source (0 to draw from beginning)
	AudioPeakScaler* scaler;		///< amplitude scaler (optional, @see AudioPeakScaler)

	PROPERTY_FLAG (stateFlags, kCombineChannels, combineChannels) ///< combine all source channels into one waveform

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Additional members
	//////////////////////////////////////////////////////////////////////////////////////////////

	static constexpr int kCombineChannels = 1<<0;

	int stateFlags;					///< state flags
	double framesPerPixel;			///< audio source frames-per-pixel resolution

	AudioPeakDrawArgs (const AudioPeakSource& source, 
					   IGraphics& graphics, 
					   double framesPerPixel,
					   int64 pixelOffset = 0,
					   int stateFlags = 0)
	: source (source),
	  graphics (graphics),
	  framesPerPixel (framesPerPixel),
	  pixelOffset (pixelOffset),
	  stateFlags (stateFlags),
	  scaler (nullptr)
	{}
};

//************************************************************************************************
// AudioPeakDrawIterator
/**	Maps output pixels to source sample positions for AudioPeakPainter::drawChannel();
	also documents the structural contract for TIterator.
	
	It must be constructible from (const AudioPeakPainter&, const TArgs&, const Rect&),
	and must expose the same public members. 
	
	The base covers one linear segment across the whole waveform. */
//************************************************************************************************

struct AudioPeakDrawIterator
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Members by contract
	//////////////////////////////////////////////////////////////////////////////////////////////

	int64 startCoord;				///< start coordinate of waveform segment on IGraphics target
	int64 endCoord;					///< end coordinate of waveform segment on IGraphics target
	int64 sourcePixelOffset;		///< pixel offset into audio source (0 to draw from beginning)
	double sourceResolution;		///< audio source frames-per-pixel resolution

	/** Advance to pixel x; return false once there is nothing left to draw. */
	bool nextPixel (int x) { return x < endCoord; }

	/** Return true and set color to override current peak color. */
	bool getColorOverride (Color& color) const { return false; }

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Additional members
	//////////////////////////////////////////////////////////////////////////////////////////////

	const AudioPeakPainter& painter;

	AudioPeakDrawIterator (const AudioPeakPainter& painter, 
						   const AudioPeakDrawArgs& args, 
						   const Rect& rect)
	: painter (painter),
	  startCoord (rect.left),
	  endCoord (rect.right),
	  sourcePixelOffset (args.pixelOffset),
	  sourceResolution (args.framesPerPixel)
	{}
};

//************************************************************************************************
// AudioPeakPainter
/** Draws an AudioPeakSource as a waveform in one of several peak styles. */
//************************************************************************************************

class AudioPeakPainter: public Object
{
public:
	DECLARE_CLASS (AudioPeakPainter, Object)

	AudioPeakPainter ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Peak Colors and Style
	//////////////////////////////////////////////////////////////////////////////////////////////

	PROPERTY_OBJECT (Color, zeroLineColor, ZeroLineColor)
	PROPERTY_OBJECT (Color, peakColor, PeakColor)
	PROPERTY_OBJECT (Color, peakFrameColor, PeakFrameColor)

	enum PeakStyle
	{
		kPeakSolid,
		kPeakSolidFramed,
		kPeakAntialias,
		kPeakBargraph,
		kNumPeakStyles,

		kPeakStyleDefault = kPeakSolid
	};

	DECLARE_ENUMINFO (peakStyles)

	PROPERTY_VARIABLE (int, peakStyle, PeakStyle)

	/** Update colors and style from a skin style. */
	virtual void updateStyle (const IVisualStyle& style);
	
	/** Override zero line color (optional); height is half the draw rect's height. */
	virtual void getOverrideZeroLineColor (Color& color, Coord height) const {}

	/** Get the renderer for a peak style. */
	virtual AudioPeakRenderer& getPeakRenderer (int peakStyle) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Drawing
	//////////////////////////////////////////////////////////////////////////////////////////////

	using DrawArgs = AudioPeakDrawArgs;
	using DrawIterator = AudioPeakDrawIterator;

	/** Draw all channels of args.source into rect, splitting it into per-channel rows. */
	template <class TIterator = DrawIterator, class TArgs = DrawArgs>
	void drawAll (const TArgs& args, const Rect& rect);

	/** Draw one channel of args.source into rect (or all channels combined, if args.combineChannels()). */
	template <class TIterator = DrawIterator, class TArgs = DrawArgs>
	void drawChannel (const TArgs& args, const Rect& rect, uint32 channel);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioPeakPainter inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class TIterator, class TArgs>
void AudioPeakPainter::drawAll (const TArgs& args, const Rect& rect)
{
	int numChannels = int(args.source.getNumPeakChannels ());
	if(numChannels <= 0 || rect.getWidth () <= 0 || rect.getHeight () <= 0)
		return;

	if(numChannels == 1 || args.combineChannels ())
		drawChannel<TIterator, TArgs> (args, rect, 0);
	else
	{
		int channelHeight = rect.getHeight () / numChannels;
		for(int channel = 0; channel < numChannels; channel++)
		{
			Rect channelRect (rect);
			channelRect.top = rect.top + channel * channelHeight;
			channelRect.bottom = channelRect.top + channelHeight;
			drawChannel<TIterator, TArgs> (args, channelRect, uint32 (channel));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class TIterator, class TArgs>
void AudioPeakPainter::drawChannel (const TArgs& args, const Rect& rect, uint32 channel)
{
	if(rect.getWidth () <= 0 || rect.getHeight () <= 0)
		return;

	int h = rect.getHeight () / 2;
	int cy = rect.top + h;
	PeakPoint lastPeak (rect.left, cy, cy, 0.f, 0.f);

	AudioPeakRenderer& peakRender = getPeakRenderer (peakStyle);
	peakRender.setGraphics (&args.graphics);
	peakRender.setColor (peakColor);
	peakRender.setFrameColor (peakFrameColor);
	
	if(peakStyle != kPeakBargraph)
	{
		Color currentZeroLineColor (zeroLineColor);
		getOverrideZeroLineColor (currentZeroLineColor, h);
		args.graphics.drawLine (Point (rect.left, cy - 1), Point (rect.right - 1, cy - 1), Pen (currentZeroLineColor));
	}

	peakRender.init ();

	TIterator iter (*this, args, rect);
	bool combine = args.combineChannels ();

	int drawIndex = 0;
	for(int x = rect.left; x < rect.right && iter.nextPixel (x); )
	{
		float pixelData[1024];
		int numPixels = 512;

		int64 startPixel = iter.sourcePixelOffset + (x - iter.startCoord);
		double samplePosition = double (startPixel) * iter.sourceResolution;
		if(samplePosition < 0 || iter.sourceResolution == 0.0)
		{
			x++;
			continue;
		}

		ccl_upper_limit (numPixels, int (iter.endCoord) - x);
		if(numPixels == 0)
			break;

		AudioPeakSource::PeakRequest request {channel, startPixel, iter.sourceResolution, pixelData, numPixels, 0};
		numPixels = combine ? args.source.getCombinedPeaks (request) : args.source.getPeaks (request);
		if(numPixels <= 0)
			break;

		Color segmentColor;
		Color wantedColor = iter.getColorOverride (segmentColor) ? segmentColor : peakColor;
		if(wantedColor != peakRender.getColor ())
		{
			peakRender.flush ();
			peakRender.setColor (wantedColor);
			peakRender.init ();
		}

		peakRender.update (iter.sourceResolution);

		for(int i = 0, count = numPixels * 2; i < count; i += 2)
		{
			float fPosRaw = pixelData[i];
			float fNegRaw = pixelData[i + 1];
			float fPos = fPosRaw;
			float fNeg = fNegRaw;
			int64 pixelIndex = drawIndex + args.pixelOffset;

			if(args.scaler)
			{
				float scale = args.scaler->getScaleFactor (pixelIndex, iter.sourceResolution);
				fPos = ccl_min (fPos * scale, 1.f);
				fNeg = ccl_min (fNeg * scale, 1.f);
			}

			int yPos = int (fPos * float (h));
			int yNeg = int (fNeg * float (h));

			PeakPoint peak (x, cy - yPos, cy + yNeg, fPosRaw, fNegRaw);
			peakRender.renderPeak (int(pixelIndex), peak, lastPeak, cy);
			lastPeak = peak;
			x++;
			drawIndex++;
		}
	}

	peakRender.finish ();
	peakRender.setGraphics (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_audiopeakpainter_h
