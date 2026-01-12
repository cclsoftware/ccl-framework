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
// Filename    : ccl/gui/theme/renderer/valuebarrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/theme/renderer/valuebarrenderer.h"

#include "ccl/gui/controls/valuebar.h"
#include "ccl/gui/windows/window.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// ValueBarRenderer
/** Displays a value as a horizontal or verical bar.
If no image is specified, "forecolor" and "backcolor" are used to draw the bar and the background as a filled rectangle. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (ValueBar, VisualStyle, "ValueBarStyle")
	ADD_VISUALSTYLE_IMAGE  ("background")	///< used to draw the background (frame "normal") and bar (frame "normalOn")
	ADD_VISUALSTYLE_COLOR ("centerlinecolor")	///< color for a centerline, when the parameter is bipolar
	ADD_VISUALSTYLE_METRIC ("centerlinewidth")	///< width for the centerline
	ADD_VISUALSTYLE_METRIC ("colorize.hilite")	///< use hilitecolor or colorparam color if applicable to colorize the hiliteimage
	ADD_VISUALSTYLE_COLOR ("backcolor")		///< the background valuebar color when no valueBarImage is available
	ADD_VISUALSTYLE_COLOR ("hilitecolor")		///< the active valuebar color (using "forecolor" as fallback) - also used to colorize the valueBarImage when "colorize.hilite" is true
	ADD_VISUALSTYLE_COLOR ("hilitecolor.transparent")	///< hilitecolor.transparent is used when colorize.hilite is set and the current color from colorparam is transparent
	ADD_VISUALSTYLE_COLOR ("hilitecolor.alphablend")	///< the non-transparent "colorname" color will be alphablended with this color (using "color.alphablend" as fallback) and used to colorize the hiliteimage
END_VISUALSTYLE_CLASS (ValueBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueBarRenderer::ValueBarRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle),
  activeFrameIndex (1),
  backFrameIndex (0),
  useColorize (false),
  isScalableImage (false),
  initialized (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBarRenderer::initialize ()
{
	if(visualStyle == nullptr)
		return;
	
	valueBarImage = visualStyle->getImage ("background");
	if(!valueBarImage)
		valueBarImage = visualStyle->getImage ("image"); // look for legacy backgrounds
	
	if(valueBarImage)
	{
		backFrameIndex = valueBarImage->getFrameIndex (IImage::kNormal);
		if(backFrameIndex < 0)
			backFrameIndex = 0;
		activeFrameIndex = !backFrameIndex;
		
		if(valueBarImage->getType () == IImage::kScalable)
			isScalableImage = true;
	}
		
	hiliteColor = visualStyle->getColor (StyleID::kHiliteColor, visualStyle->getForeColor ());
	hiliteTransparentColor = visualStyle->getColor ("hilitecolor.transparent", hiliteColor);
	hiliteColorAlphaBlend = visualStyle->getColor ("hilitecolor.alphablend", visualStyle->getColor ("color.alphablend", Colors::kTransparentBlack));
	useColorize = visualStyle->getMetric<bool> ("colorize.hilite", false);
	centerLinePen.setColor (visualStyle->getColor ("centerlinecolor", visualStyle->getForeColor ().grayScale ()));
	centerLinePen.setWidth (visualStyle->getMetric<float> ("centerlinewidth", 1.f));
	
	colorParamColor = Colors::kTransparentBlack;
	
	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBarRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	if(initialized == false)
		initialize ();
	
	ValueBar* valueBar = (ValueBar*)view;
	float value = valueBar->getValue ();

	Rect back, hilite;
	valueBar->getRects (value, back, hilite);

	//CCL_PRINTF ("ValueBar %4.4f %3d %3d %3d %3d + %3d %3d %3d %3d\n", value, rect.left, rect.top, rect.getWidth (), rect.getHeight (), rect2.left, rect2.top, rect2.getWidth (), rect2.getHeight ())

	GraphicsPort port (view);
	UnknownPtr<IColorParam> colorParam = valueBar->getColorParam ();
	
	const VisualStyle& vs = *getVisualStyle ();
	
	if(valueBarImage)
	{
		if(isScalableImage == false)
			valueBarImage.update (view->getWidth (), view->getHeight ());

		valueBarImage->setCurrentFrame (backFrameIndex);
		
		if(!back.isEmpty ())
		{
			if(valueBar->getStyle ().isOpaque ())
				port.drawImage (valueBarImage, Rect (0, 0, valueBarImage->getWidth (), valueBarImage->getHeight ()), Rect (0, 0, valueBar->getWidth (), valueBar->getHeight ()));
			else
				port.drawImage (valueBarImage, getSourceRect (valueBar, back), back);
		}

		valueBarImage->setCurrentFrame (activeFrameIndex);
	
		if(!hilite.isEmpty ())
		{
			IImage* hiliteImage = valueBarImage;
			if(useColorize)
				hiliteImage = ModifiedImageCache::instance ().lookup (valueBarImage, getHiliteColor (colorParam), true);
		
			port.drawImage (hiliteImage, getSourceRect (valueBar, hilite), hilite);
		}
	}
	else
	{
		if(!back.isEmpty ())
			port.fillRect (back, vs.getBackBrush ());

		if(!hilite.isEmpty ())
			port.fillRect (hilite, SolidBrush (getHiliteColor (colorParam)));
	}
	
	if(valueBar->getParameter ()->isBipolar ())
	{
		Point start, end;
		if(valueBar->getStyle ().isHorizontal ())
		{
			Coord xOff =  (view->getWidth () / 2) - (int)(centerLinePen.getWidth () / 2);
			start.offset (xOff, 0);
			end.offset (xOff, view->getHeight ());
		}
		else
		{
			Coord yOff = (view->getHeight () / 2) - (int)(centerLinePen.getWidth () / 2);
			start.offset (0, yOff);
			end.offset (view->getWidth (), yOff);
		}
		port.drawLine (start, end, centerLinePen);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ValueBarRenderer::getSourceRect (View* valueBar, RectRef dstRect) const
{
	if(isScalableImage == false) // cached image with updated size available
		return dstRect;
	
	Rect fullDstRect (0, 0, valueBar->getWidth (), valueBar->getHeight ());
	Rect sourceRect (0, 0, valueBarImage->getWidth (), valueBarImage->getHeight ());
	RectF normalizedDst (0,0,1,1);
	
	if(valueBar->getStyle ().isHorizontal ())
	{
		normalizedDst.left = dstRect.left / (float)valueBar->getWidth ();
		normalizedDst.right = dstRect.right / (float)valueBar->getWidth ();
		
		sourceRect.left = ccl_to_int (sourceRect.getWidth () * normalizedDst.left);
		sourceRect.right = ccl_to_int (sourceRect.right * normalizedDst.right);
	}
	else
	{
		normalizedDst.top = dstRect.top / (float)valueBar->getHeight ();
		normalizedDst.bottom = dstRect.bottom / (float)valueBar->getHeight ();
		
		sourceRect.top = ccl_to_int (sourceRect.getHeight () * normalizedDst.top);
		sourceRect.bottom = ccl_to_int (sourceRect.bottom * normalizedDst.bottom);
	}
	
	return sourceRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef ValueBarRenderer::getHiliteColor (IColorParam* colorParam) const
{
	// hiliteColor is dependent on colorParam when this parameter is used
	if(colorParam != nullptr)
	{
		colorParamColor = Colors::kTransparentBlack;
		colorParam->getColor (colorParamColor);
		
		if(colorParamColor.getAlphaF () == 0.f)
		{
			colorParamColor = hiliteTransparentColor;
		}
		else if(hiliteColorAlphaBlend.getAlphaF () != 0)
		{
			colorParamColor.alphaBlend (hiliteColorAlphaBlend, hiliteColorAlphaBlend.getAlphaF ());
		}
		
		return colorParamColor;
	}
	
	return hiliteColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ValueBarRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBarRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
}


//************************************************************************************************
// ProgressBarRenderer
/** A Progres bar draws a bar that reflects the parameter value over a background. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (ProgressBar, VisualStyle, "ProgressBarStyle")
	ADD_VISUALSTYLE_IMAGE  ("foreground")	///< used to draw a bar over the background that reflects the parameter value
	ADD_VISUALSTYLE_IMAGE  ("indicator")	///< drawn at the boundary between the background and the bar
END_VISUALSTYLE_CLASS (ProgressBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressBarRenderer::ProgressBarRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle)
{
	background = visualStyle->getImage ("background");
	foreground = visualStyle->getImage ("foreground");
	indicator = visualStyle->getImage ("indicator");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressBarRenderer::draw (View* view, const UpdateRgn& updateRgn)
{	
	ProgressBar* progressBar = (ProgressBar*)view;
	float value = progressBar->getValue ();
	StyleRef style = progressBar->getStyle ();
	
	GraphicsPort port (progressBar);

	Rect hiliteRect, backgroundRect;
	progressBar->getRects (value, backgroundRect, hiliteRect);

	// Background
	if(background)
	{
		background.update (progressBar->getWidth (), progressBar->getHeight ());
		port.drawImage (background, updateRgn.bounds, updateRgn.bounds);
	}
	else if(style.isOpaque ())
		port.fillRect (updateRgn.bounds, visualStyle->getBackBrush ());

	if(foreground)
	{
		foreground.update (progressBar->getWidth (), progressBar->getHeight ());

		// select frame by progress phase
		int frames = foreground->getFrameCount ();
		if(frames > 1)
		{
			int index = (int)(progressBar->getPhase () * frames);
			foreground->setCurrentFrame (index);
			CCL_PRINTF ("ProgressBar phase %4.4f frame = %3d\n", progressBar->getPhase (), index)
		}	

		port.drawImage (foreground, hiliteRect, hiliteRect);
	}
	else
		port.fillRect (hiliteRect, visualStyle->getForeBrush ());	

	// indicator is shifted with progress position
	if(indicator)
	{
		Coord indicatorWidth = indicator->getWidth ();
		indicator.update (indicatorWidth, progressBar->getHeight ());

		Coord minIndicatorLeft = 2 * indicatorWidth;
		Coord maxIndicatorLeft = progressBar->getWidth () - minIndicatorLeft;
		
		Coord rest = maxIndicatorLeft - backgroundRect.left;
		if(rest > 0)
			rest = 0;

		if(backgroundRect.left >= minIndicatorLeft && (backgroundRect.left <= maxIndicatorLeft || (rest < 0 && rest > -indicatorWidth)))
		{		
			Rect src (-rest, 0, indicatorWidth, progressBar->getHeight ());
			Rect dst (src);
			dst.moveTo (Point (backgroundRect.left, 0));

			port.drawImage (indicator, src, dst);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ProgressBarRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProgressBarRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
}
