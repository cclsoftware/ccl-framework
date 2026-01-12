//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/theme/renderer/labelrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/knobrenderer.h"

#include "ccl/gui/controls/knob.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/graphics/graphicspath.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/base/math/mathcurve.h"

using namespace CCL;

//************************************************************************************************
// KnobRenderer
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (Knob, VisualStyle, "KnobStyle")
	ADD_VISUALSTYLE_IMAGE  ("background")				///< background image
	ADD_VISUALSTYLE_IMAGE  ("foreground")				///< overlay image above the circle (alternative name "overlay" can be used)
	ADD_VISUALSTYLE_IMAGE  ("topoverlay")				///< topoverlay image (above the indicator)
	ADD_VISUALSTYLE_METRIC ("filmstrip")				///< specifies that the "background" image should be used as a filmstrip.  Same as Knob option "filmstrip".
	ADD_VISUALSTYLE_METRIC ("overlay.filmstrip")		///< specifies that the "foreground" image should be used as a filmstrip
	ADD_VISUALSTYLE_METRIC ("topoverlay.filmstrip")		///< specifies that the "topoverlay" image should be used as a filmstrip
	ADD_VISUALSTYLE_METRIC ("framecount")				///< an external framecount can be used to repeat the existing filmstrip frames and reduce the overall memory footprint
	ADD_VISUALSTYLE_METRIC ("circle")					///< a partial circle around the center is drawn. Same as Knob option "circle". the circle color is the "hilitecolor" or set from the "colorname" parameter
	ADD_VISUALSTYLE_METRIC ("circle.indicatorwidth")	///< draw a partial circle indicator with the given width in degrees around the center. The circle indicator uses "indicatorcolor".
	ADD_VISUALSTYLE_COLOR  ("circle.reversecolor")		///< color used to draw a circle from the current position to range end
	ADD_VISUALSTYLE_METRIC ("forecolor")				///< color used to draw a circle from the range start to range end (when no overlay is used)
	ADD_VISUALSTYLE_METRIC ("strokewidth")				///< strokewidth of the optional circle
	ADD_VISUALSTYLE_COLOR  ("hilitecolor")				///< color used to draw a circle from range start to the current position
	ADD_VISUALSTYLE_COLOR  ("althilitecolor")			///< color used to draw a circle from the current position to the current position of an optional offset parameter. Set via Knob attribute "referencename".
	ADD_VISUALSTYLE_METRIC ("indicator")				///< a line from the center to the end of the circle is drawn. Same as Knob option "indicator". Color "indicatorcolor" is used.
	ADD_VISUALSTYLE_IMAGE  ("disabledoverlay")			///< optional disabled overlay image to show an individual disabled state
	ADD_VISUALSTYLE_METRIC ("margin")					///< margin between view boudaries and circle
	ADD_VISUALSTYLE_METRIC ("indicatormargin")			///< margin between view boudaries and indicator
	ADD_VISUALSTYLE_COLOR  ("indicatorcolor")			///< color used for drawing the "indicator"
	ADD_VISUALSTYLE_METRIC ("indicatorwidth")			///< used when it should be different from the strokewidth (which is the default)
	ADD_VISUALSTYLE_METRIC ("range")					///< the range in degrees for circle and indicator
	ADD_VISUALSTYLE_COLOR  ("state1")					///< color used for drawing a special marker when the knob's visual state is 1
	ADD_VISUALSTYLE_COLOR  ("state2")					///< color used for drawing a special marker when the knob's visual state is 2
	ADD_VISUALSTYLE_COLOR  ("state3")					///< color used for drawing a special marker when the knob's visual state is 3
	ADD_VISUALSTYLE_METRIC ("allowstretch")				///< allowstretch of background image (default is TRUE)
	ADD_VISUALSTYLE_METRIC ("linearScaler")				///< used to remap filmstrip frames. linearScaler is the slope used to transform the normalized value
	ADD_VISUALSTYLE_METRIC ("linearOffset")				///< used to remap filmstrip frames. linearOffset is the optional offset used to transform the normalized value
	ADD_VISUALSTYLE_METRIC ("logscaler")				///< used to remap filmstrip frames. 1/logscaler * log(normalizedValue) is used to map frames onto a logarithmic curve
	ADD_VISUALSTYLE_METRIC ("padding.left")				///< left padding for background image
	ADD_VISUALSTYLE_METRIC ("padding.top")				///< top padding for background image
	ADD_VISUALSTYLE_METRIC ("padding.right")			///< right padding for background image
	ADD_VISUALSTYLE_METRIC ("padding.bottom")			///< bottom padding for background image
	ADD_VISUALSTYLE_METRIC ("padding")					///< padding for background image, used if one of the paddings for left, top, right, bottom is not specified
END_VISUALSTYLE_CLASS (Knob)

//////////////////////////////////////////////////////////////////////////////////////////////////

KnobRenderer::KnobRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle)
{
	image = visualStyle->getBackgroundImage ();
	overlay = visualStyle->getImage ("overlay");
	if(!overlay)
		overlay = visualStyle->getImage ("foreground");
	topOverlay = visualStyle->getImage ("topoverlay");
	disabledOverlay = visualStyle->getImage ("disabledoverlay");

	useFilmStrip = image ? visualStyle->getMetric<bool> ("filmstrip", false) : false;
	useOverlayFilmstrip = overlay ? visualStyle->getMetric<bool> ("overlay.filmstrip", false) : false;
	useTopOverlayFilmstrip = topOverlay ? visualStyle->getMetric<bool> ("topoverlay.filmstrip", false) : false;

	drawCircle = (image || overlay) ? visualStyle->getMetric<bool> ("circle", false) : true;
	drawIndicator = (image || overlay) ? visualStyle->getMetric<bool> ("indicator", false) : true;
	margin = (int)visualStyle->getMetric ("margin", 0);
	
	circleIndicatorWidth = drawCircle ? visualStyle->getMetric<float> ("circle.indicatorwidth", 0.f) : 0.f;
	
	circleReverseColor = visualStyle->getColor ("circle.reversecolor", Color (0,0,0,0));
	drawReverseRangeCircle = (circleReverseColor.getAlphaF () != 0.f) ? true : false;
	
	// calculate indicator margin fallback
	float strokeWidth = visualStyle->getStrokeWidth ();
	int indicatorMarginFallback = margin + ccl_to_int (strokeWidth / 2.f);
	if(image != nullptr)
		indicatorMarginFallback += ccl_to_int (strokeWidth + 1);
	
	indicatorMargin = (int)visualStyle->getMetric ("indicatormargin", indicatorMarginFallback);
	indicatorColor = visualStyle->getColor ("indicatorcolor", visualStyle->getHiliteColor ());
    indicatorWidth = (float)visualStyle->getMetric ("indicatorwidth", 0.f);
	range = (float)visualStyle->getMetric ("range", 300.f);
	
	linearScaler = visualStyle->getMetric<float> ("linearScaler", 0.f);
	linearOffset = visualStyle->getMetric<float> ("linearOffset", 0.f);
	logScaler = visualStyle->getMetric<float> ("logscaler", 0.f);
	if(logScaler == 0.f)
		logScaler = visualStyle->getMetric<float> ("filmstrip.logscaler", 0.f);	// look for legacy name
	
	
	allowstretch = visualStyle->getMetric<bool> ("allowstretch", true);
	visualStyle->getPadding (padding);
	
	externalFrameCount = getVisualStyle ()->getMetric<int> ("framecount", 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double KnobRenderer::transformNormalized (double value)
{
	if(logScaler != 0)
	{
		Math::LogarithmicCurve transformCurve;
		transformCurve.setYScaler (1/(double)logScaler);
		
		if(logScaler > 0)
		{
			transformCurve.setPoints (0, 0, 1, 1);
			value = transformCurve.getY (value);
		}
		else
		{
			transformCurve.setPoints (0, 1, 1, 0);
			value = transformCurve.getY (1 - value);
		}
	}
	else if(linearScaler != 0)
	{
		Math::LinearCurve transformCurve;
		transformCurve.setK (linearScaler);
		transformCurve.setD (linearOffset);
		value = transformCurve.getY (value);
	}
	
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void KnobRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	Knob* knob = (Knob*)view;
	StyleRef style = view->getStyle ();
	Variant value = knob->getValue ();
	Variant offsetReference = knob->getOffsetReferenceValue ();
	bool drawOffsetCircle = knob->hasOffsetReference ();
	
	GraphicsPort port (knob);
	Rect clientRect;
	knob->getClientRect (clientRect);

	float v = (float)transformNormalized (value.asDouble ());
	const float transformedV = v;
	
	float offsetReferenceV = offsetReference.asFloat ();
	
	int state = knob->getVisualState ();
	
	auto drawImage = [&](IImage* toDraw, bool isFilmStrip)
	{
		if(toDraw)
		{
			Rect imageSrc (0, 0, toDraw->getWidth (), toDraw->getHeight ());
			Rect imageDst (clientRect);
			
			imageDst.left += padding.left;
			imageDst.top += padding.top;
			imageDst.right -= padding.right;
			imageDst.bottom -= padding.bottom;
			
			int frameCount = int(transformedV * toDraw->getFrameCount ());
			
			if(externalFrameCount > 0)
				frameCount = (int(transformedV * externalFrameCount) % toDraw->getFrameCount ());
			
			if(isFilmStrip)
				toDraw->setCurrentFrame (frameCount);
			
			if(!allowstretch)
			{
				Point leftTop (imageDst.getLeftTop ());
				imageDst.moveTo (Point ());
				imageSrc.bound (imageDst);
				imageDst.moveTo (leftTop);
				imageDst.setWidth (imageSrc.getWidth ());
				imageDst.setHeight (imageSrc.getHeight ());
			}
			
			port.drawImage (toDraw, imageSrc, imageDst);
		}
	};
	
	auto drawStateCircle = [&]
	{
		if(state > 0)
		{
			MutableCString colorStateName;
			colorStateName.appendFormat ("state%d", state);

			SolidBrush brush (visualStyle->getForeColor ());
			if(knob->getParameter ()->isEnabled ())
                brush.setColor (indicatorColor);
			
			brush.setColor (visualStyle->getColor (colorStateName, brush.getColor ()));
			Rect stateRect (0, 0, 6, 6);
			if(clientRect.getWidth ()%2)
			{
				stateRect.bottom += 1;
				stateRect.right += 1;
			}
			stateRect.center (clientRect);
			port.fillEllipse (stateRect, brush);

			Color stateBorderColor (visualStyle->getColor ("stateborder", Color (0,0,0,0)));
			if(stateBorderColor != 0)
				port.drawEllipse (stateRect, Pen (stateBorderColor));
		}
	};
	
	if(disabledOverlay && (knob->isEnabled () == false))
	{
		drawImage (disabledOverlay, false);
	}
	else if(useFilmStrip || style.isCustomStyle (Styles::kKnobAppearanceFilmstrip))	// drawBackgroundOnly - no circle or indicator rendering
	{
		drawImage (image, true);
		drawStateCircle ();
	}
	else
	{
		drawImage (image, false);

		// drawCircleRanges
		Pen pen (visualStyle->getForePen ());
		Rect innerRect (clientRect);
		int strokeMargin = ccl_to_int (pen.getWidth () / 2.f);
		innerRect.contract (strokeMargin);
		innerRect.contract (margin);
		if(image == nullptr && overlay == nullptr)
		{
			Brush brush (visualStyle->getBackBrush ());
			if(brush.getType () == Brush::kSolid)
			{
				Color color (brush.getColor ());
				brush.setColor (color.setAlphaF (v));
				if(color.alpha != 0)
					port.fillEllipse (innerRect, brush);
			}
			else
				port.fillEllipse (innerRect, brush);
		}

		float onStart, onRange, offStart, offRange, start, deltaRange, deltaStart;
		start = 270.f - (range / 2);
		offStart = start;
		offRange = range;
		if(knob->getParameter ()->isBipolar () || style.isCustomStyle (Styles::kSliderAppearanceCentered))
		{
			float max = knob->getParameter ()->getMax ();
			float min = knob->getParameter ()->getMin ();
			float mid = knob->getParameter ()->getDefaultValue ();
			if(style.isCustomStyle (Styles::kSliderAppearanceCentered) && ccl_abs (max) == ccl_abs (min)) // when knob display should be centered, do not rely on default value
				mid = (max + min) / 2.f;
			else if(knob->getParameter ()->isReverse ())
			{
				if(mid == min)
					mid = max;
				else if(mid == max)
					mid = min;
			}

			mid = (mid - min) / (max - min);
			if(knob->getParameter ()->getCurve ())
				mid = (float)transformNormalized (knob->getParameter ()->getCurve ()->normalizedToDisplay (mid));
			v -= mid;
			
			start = start + range * mid;

			onStart = v < 0 ? start + v * range : start;
			onRange = v < 0 ? start - onStart : range * v;
		}
		else
		{
			onStart = start;
			onRange = v * range;
			
			if(v < offsetReferenceV)
			{
				deltaStart = onStart + onRange;
				deltaRange = (offsetReferenceV * range) - onRange;
			}
			else
			{
				deltaStart = onStart + (offsetReferenceV * range);
				deltaRange = onRange - (offsetReferenceV * range);
			}
		}
		
		if((drawCircle || style.isCustomStyle (Styles::kKnobAppearanceCircle)) && innerRect.isEmpty () == false)
		{
			if(overlay == nullptr)
			{
				GraphicsPath path;
				path.addArc (innerRect, offStart, offRange);
				port.drawPath (path, pen);
			}

			if(knob->getParameter ()->isEnabled ())
			{
				Color circleColor (getVisualStyle ()->getHiliteColor ());
				
				if(UnknownPtr<IColorParam> colorParam = knob->getColorParam ())
					if(colorParam->getColor (circleColor).getAlphaF () == 0)
						circleColor = getVisualStyle ()->getHiliteColor ();
				
				pen.setColor (circleColor);
			}
				
			if(onRange > 0)
			{
				GraphicsPath path;
				path.addArc (innerRect, onStart, onRange);
				port.drawPath (path, pen);
			}

			if(drawReverseRangeCircle)
			{
				GraphicsPath path;
				path.addArc (innerRect, onStart + onRange, offRange - onRange);
				pen.setColor (circleReverseColor);
				port.drawPath (path, pen);
			}
			
			if(drawOffsetCircle)
			{
				if(knob->getParameter ()->isEnabled ())
					pen.setColor (getVisualStyle ()->getColor ("althilitecolor", Colors::kWhite));
				
				GraphicsPath path;
				path.addArc (innerRect, deltaStart, deltaRange);
				port.drawPath (path, pen);
			}
			
			if(circleIndicatorWidth > 0)
			{
				GraphicsPath path;
				float indicatorOffset = (v > 0) ? onRange : 0.f;
				path.addArc (innerRect, onStart + indicatorOffset - (circleIndicatorWidth / 2.f), circleIndicatorWidth);
				pen.setColor (indicatorColor);
				port.drawPath (path, pen);
			}
		}
		
		drawImage (overlay, useOverlayFilmstrip);

		// draw indicator
		if(drawIndicator || style.isCustomStyle (Styles::kKnobAppearanceIndicator))
		{
            v *= Math::degreesToRad (range);
            v += Math::degreesToRad (start);
            
			Rect indicatorRect (clientRect);
			indicatorRect.contract (indicatorMargin);
			
            float r = indicatorRect.getWidth () / 2.f;
            Point p, m = indicatorRect.getCenter ();
            p.x = (int)(m.x + r * cos (v) + .5f);
            p.y = (int)(m.y + r * sin (v) + .5f);
            if(knob->getParameter ()->isEnabled ())
                pen.setColor (indicatorColor);
            
            if(indicatorWidth > 0)
                pen.setWidth (indicatorWidth);
            
            AntiAliasSetter smoother (port);
            port.drawLine (m, p, pen);

		}
        
		drawImage (topOverlay, useTopOverlayFilmstrip);
		drawStateCircle ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int KnobRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KnobRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
}
