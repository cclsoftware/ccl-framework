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
// Filename    : ccl/gui/theme/renderer/sliderrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/sliderrenderer.h"

#include "ccl/gui/controls/slider.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/controlscalepainter.h"

#include "ccl/gui/graphics/imaging/coloredbitmap.h"

using namespace CCL;

//************************************************************************************************
// SliderRenderer
/** A Slider draws handle moving over a background. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (Slider, VisualStyle, "SliderStyle")
	ADD_VISUALSTYLE_IMAGE  ("vBack")					///< background image (vertical slider) - can have optional second frame for on-state of style "bargraph" - optional "disabled" on-state frame cannot be the first one
	ADD_VISUALSTYLE_IMAGE  ("vHandle")					///< handle (vertical slider) - can have optional second "pressed" frame - and optional third "mouseover" frame - optional "disabled" frame cannot be the first one
	ADD_VISUALSTYLE_IMAGE  ("vDeepBack")				///< additional background (vertical slider)
	ADD_VISUALSTYLE_IMAGE  ("vSmallBack")				///< background image (vertical slider with option "small") - with optional frames (see above)
	ADD_VISUALSTYLE_IMAGE  ("vSmallHandle")				///< handle (vertical slider with option "small") - with optional frames (see above)
	ADD_VISUALSTYLE_IMAGE  ("hBack")					///< background image (horizontal slider) - with optional frames (see above)
	ADD_VISUALSTYLE_IMAGE  ("hHandle")					///< handle (horizontal slider) - with optional frames (see above)
	ADD_VISUALSTYLE_IMAGE  ("hDeepBack")				///< additional background (horizontal slider)
	ADD_VISUALSTYLE_IMAGE  ("hSmallBack")				///< background image (horizontal slider with option "small") - with optional frames (see above)
	ADD_VISUALSTYLE_IMAGE  ("hSmallHandle")				///< handle (horizontal slider with option "small") - with optional frames (see above)
	ADD_VISUALSTYLE_METRIC ("scaleHandle")				///< when handles are too big or small they are scaled with respect to their aspectratio
	ADD_VISUALSTYLE_METRIC ("margin")					///< margin between handle and view edges in min/max handle positions
	ADD_VISUALSTYLE_METRIC ("touchFineMinScale")		///< min finescale factor (0\.\.\.1)
	ADD_VISUALSTYLE_METRIC ("touchFineMaxScale")		///< max finescale factor (0\.\.\.1)
	ADD_VISUALSTYLE_METRIC ("touchFineMinSpeed")		///< lower bound for finescale factor interpolation
	ADD_VISUALSTYLE_METRIC ("touchFineMaxSpeed")		///< upper bound for finescale factor interpolation
	ADD_VISUALSTYLE_COLOR ("centerlinecolor")			///< color for a centerline, when the parameter is bipolar and style is "bargraph"
	ADD_VISUALSTYLE_METRIC ("centerlinewidth")			///< width for the centerline
	ADD_VISUALSTYLE_COLOR ("handleColor")				///< color to colorize the handle, when the parameter is bipolar and style is "bargraph"
	ADD_VISUALSTYLE_COLOR ("disabledHandleColor")		///< color to colorize the handle, when the parameter is disabled (usually handles are not drawn in this case)
END_VISUALSTYLE_CLASS (Slider)

//////////////////////////////////////////////////////////////////////////////////////////////////

SliderRenderer::SliderRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle),
  initDone (false),
  margin (0),
  scaleOffset (0),
  barGraphMargin (0),
  barGraphOffset (0),
  disabledBackIndex (-1),
  disabledHandleIndex (-1),
  scaleHandle (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SliderRenderer::init (const View* view)
{
	Slider* slider = (Slider*)view;
	StyleRef style = view->getStyle ();
	
	margin = visualStyle->getMetric<int> ("margin", 0);
	scaleHandle = visualStyle->getMetric<bool> ("scaleHandle", false);
	 
	if(style.isVertical ())
	{
		if(slider->getWidth () < kSmallSliderSize)
		{
			backImage = visualStyle->getImage ("vSmallBack");
			handleImage = visualStyle->getImage ("vSmallHandle");
		}
		if(backImage == nullptr)
			backImage = visualStyle->getImage ("vBack");
		if(deepBackImage == nullptr)
			deepBackImage = visualStyle->getImage ("vDeepBack");
		if(handleImage == nullptr)
			handleImage = visualStyle->getImage ("vHandle");
		
		if(handleImage)
		{
			barGraphMargin = handleImage->getHeight ();
			scaleOffset = margin + (barGraphMargin / 2);
		}
	}
	else
	{
		if(slider->getHeight () < kSmallSliderSize)
		{
			backImage = visualStyle->getImage ("hSmallBack");
			handleImage = visualStyle->getImage ("hSmallHandle");
		}
		if(backImage == nullptr)
			backImage = visualStyle->getImage ("hBack");
		if(deepBackImage == nullptr)
			deepBackImage = visualStyle->getImage ("hDeepBack");
		if(handleImage == nullptr)
			handleImage = visualStyle->getImage ("hHandle");
		
		if(handleImage)
		{
			barGraphMargin = handleImage->getWidth ();
			scaleOffset = margin + (barGraphMargin / 2);
		}
	}
	
	if(backImage)
		disabledBackIndex = backImage->getFrameIndex ("disabled");
	if(handleImage)
		disabledHandleIndex = handleImage->getFrameIndex ("disabled");
	
	scaleMargin.left = visualStyle->getMetric ("scale.margin.left", 0);
	scaleMargin.right = visualStyle->getMetric ("scale.margin.right", 0);
	scaleMargin.top = visualStyle->getMetric ("scale.margin.top", 0);
	scaleMargin.bottom = visualStyle->getMetric ("scale.margin.bottom", 0);
	
	centerLinePen.setColor (visualStyle->getColor ("centerlinecolor", visualStyle->getForeColor ().grayScale ()));
	centerLinePen.setWidth (visualStyle->getMetric<float> ("centerlinewidth", 1.f));
	
	handleColor = visualStyle->getColor ("handleColor", visualStyle->getForeColor ());
	disabledHandleColor = visualStyle->getColor ("disabledHandleColor", Color (0,0,0,0));
	
	barGraphOffset = visualStyle->getMetric ("bargraph.offset", 0);
	
	initDone = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SliderRenderer::drawBackImage (GraphicsPort& port, Slider* slider, CachedImage& background)
{
	StyleRef style = slider->getStyle ();
	bool bipolar = slider->getParameter ()->isBipolar () || style.isCustomStyle (Styles::kSliderAppearanceCentered) || style.isCustomStyle (Styles::kSliderAppearanceDefaultCentered);

	Rect rect;
	slider->getClientRect (rect);
	
	if(background)
	{
		if(deepBackImage)
		{
			Rect src (0, 0, deepBackImage->getWidth (), deepBackImage->getHeight ());
			port.drawImage (deepBackImage, src, rect);
		}
		
		if(style.isCustomStyle (Styles::kSliderAppearanceBarGraph))
		{
			// check if background needs to be scaled on the fly (GDI kills alpha channel on Windows when using CachedImage)
			Transform transform;
			bool sizeable = (slider->getSizeMode () & (View::kAttachLeft|View::kAttachRight)) || (slider->getSizeMode () & (View::kAttachTop|View::kAttachBottom));
			if(sizeable && style.isTransparent ())
			{
				float scaleX = (float)background->getWidth () / (float)rect.getWidth ();
				float scaleY = (float)background->getHeight () / (float)rect.getHeight ();
				transform.scale (scaleX, scaleY);
			}
			else
				background.update (slider->getWidth (), slider->getHeight ());
			
			Rect src1, src2;
			calcBarGraphRects (src1, src2, slider, bipolar);
			
			// check if background needs to be scaled on the fly
			Rect _src1 (src1), _src2 (src2);
			if(!transform.isIdentity ())
			{
				transform.transform (_src1);
				transform.transform (_src2);
			}
			
			if(src1.intersect (src2))
			{
				// src1 & src2 overlap, only draw visible parts of src1
				Rect partA (src1); // left / upper part
				Rect partB (src1); // right / lower part
				
				if(style.isHorizontal ())
				{
					ccl_upper_limit (partA.right, src2.left);
					ccl_lower_limit (partB.left, src2.right);
				}
				else
				{
					ccl_upper_limit (partA.bottom, src2.top);
					ccl_lower_limit (partB.top, src2.bottom);
				}
				
				Rect _partA (partA);
				Rect _partB (partB);
				if(!transform.isIdentity ())
				{
					transform.transform (_partA);
					transform.transform (_partB);
				}
				
				background->setCurrentFrame (0);
				if(!partA.isEmpty ())
					port.drawImage (background, _partA, partA);
				
				if(!partB.isEmpty ())
					port.drawImage (background, _partB, partB);
			}
			else
			{
				background->setCurrentFrame (0);
				port.drawImage (background, _src1, src1);
			}
			
			int frameIndex = 1;
			if(!slider->isEnabled () && disabledBackIndex > 0)
				frameIndex = disabledBackIndex;
				
			background->setCurrentFrame (frameIndex);
			port.drawImage (background, _src2, src2);
		}
		else
		{
			int frameIndex = 0;
			if(!slider->isEnabled () && disabledBackIndex > 0)
				frameIndex = disabledBackIndex;
							
			background->setCurrentFrame (frameIndex);
			Rect src (0, 0, background->getWidth (), background->getHeight ());
			port.drawImage (background, src, rect);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SliderRenderer::drawBackcolor (GraphicsPort& port, Slider* slider, Color foreColor)
{
	StyleRef style = slider->getStyle ();
	bool bipolar = slider->getParameter ()->isBipolar () || style.isCustomStyle (Styles::kSliderAppearanceCentered) || style.isCustomStyle (Styles::kSliderAppearanceDefaultCentered);

	Rect rect;
	slider->getClientRect (rect);
	
	if(style.isCustomStyle (Styles::kSliderAppearanceBarGraph))
	{
		Rect src1, src2;
		calcBarGraphRects (src1, src2, slider, bipolar);
		
		if(style.isOpaque ())
			port.fillRect (src1, visualStyle->getBackBrush ());
		
		SolidBrush brush;
		bool hilite = (ThemeElements::kMouseOver == slider->getThemeElementState () || ThemeElements::kPressed == slider->getThemeElementState ());
		
		Color enabledColor (foreColor);
		
		if(UnknownPtr<IColorParam> colorParam = slider->getColorParam ())
			colorParam->getColor (enabledColor);
		
		if(enabledColor.getAlphaF () == 0)
			enabledColor = foreColor;
		
		if(hilite)
			enabledColor = visualStyle->getColor ("hilitecolor", enabledColor);
		
		brush.setColor (slider->isEnabled () ? enabledColor : foreColor.grayScale ());
		port.fillRect (src2, brush);
		
		if(bipolar)
		{
			Point start, end;
			if(style.isHorizontal ())
			{
				Coord xOff = (rect.getWidth () / 2) - (int)(centerLinePen.getWidth () / 2);
				start.offset (xOff, 0);
				end.offset (xOff, rect.getHeight ());
			}
			else
			{
				Coord yOff = (rect.getHeight () / 2) - (int)(centerLinePen.getWidth () / 2);
				start.offset (0, yOff);
				end.offset (rect.getWidth (), yOff);
			}
			port.drawLine (start, end, centerLinePen);
		}
		
	}
	else
	{
		if(style.isOpaque ())
			port.fillRect (rect, visualStyle->getBackBrush ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SliderRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	if(!initDone)
		init (view);

	GraphicsPort port (view);
	Slider* slider = (Slider*)view;
	
	if(slider->getStyle ().isCustomStyle (Styles::kSliderAppearanceTickScale))
		drawTickScale (port, slider);
	
	if(backImage)
		drawBackImage (port, slider, backImage);
	else
		drawBackcolor (port, slider, getVisualStyle ()->getForeColor ());
	
	if(slider->isEnabled () || (disabledHandleColor.getAlphaF () != 0) || disabledHandleIndex > 0)
		drawHandle (port, slider, handleImage, slider->getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SliderRenderer::drawTickScale (GraphicsPort& port, Slider* slider)
{
	if(UnknownPtr<ITickScale> scale = slider->getParameter ()->getCurve ())
	{
		ControlScalePainter scalePainter;
		scalePainter.setScale (scale);
		scalePainter.setTickColor (visualStyle->getForeColor ());
		scalePainter.updateStyle (*visualStyle);

		Rect scaleRect;
		slider->getClientRect (scaleRect);
		// scale ranges from the middle position of the handle at both ends
		
		if(slider->getStyle ().isHorizontal ())
		{
			scaleRect.left += scaleOffset;
			scaleRect.right -= scaleOffset;
			scaleRect.top += scaleMargin.top;
			scaleRect.bottom -= scaleMargin.bottom;
		}
		else
		{
			scaleRect.top += scaleOffset;
			scaleRect.bottom -= scaleOffset;
			scaleRect.left += scaleMargin.left;
			scaleRect.right -= scaleMargin.right;
		}
		
		scalePainter.drawScaleGrid (port, scaleRect, slider->getStyle ().common);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SliderRenderer::drawHandle (GraphicsPort& port, Slider* slider, IImage* handle, float value)
{
	bool colorizeHandle = false;
	Color color (handleColor);
	
	if(slider->isEnabled ())
	{
		if(UnknownPtr<IColorParam> colorParam = slider->getColorParam ())
		{
			colorizeHandle = true;
			colorParam->getColor (color);
		}
	}
	else if(disabledHandleColor.getAlphaF () != 0)
	{
		colorizeHandle = true;
		color = disabledHandleColor;
	}
		
	Rect handleRect;
	getHandleRect (slider, handle, handleRect, value);
	
	if(handle)
	{
		if(ColoredBitmap* coloredImage = unknown_cast<ColoredBitmap> (handle))
		{
			if(colorizeHandle)
			{
				coloredImage->setColor (color);
			}
			else
			{
				Rect originalRect;
				handle = coloredImage->getOriginalImage (originalRect);
			}
		}
		else if(colorizeHandle)
		{
			handle = createColorizedHandle (handle, color);
		}
		
		int frame = (slider->getMouseState () == View::kMouseDown) ? 1 : 0;
		if(handle->getFrameCount () > 2)
			if(slider->getMouseState () == View::kMouseOver)
				frame = 2;
		
		if(disabledHandleIndex > 0)
		{
			if(slider->isEnabled () && (frame == disabledHandleIndex))
				frame = 0; // only use disabled index when control is disabled
			else if(!slider->isEnabled ())
				frame = disabledHandleIndex;				 
		}
		
		handle->setCurrentFrame (frame);
		
		Rect src (0, 0, handle->getWidth (), handle->getHeight ());
		port.drawImage (handle, src, handleRect);
	}
	else if(slider->getStyle ().isOpaque () && !slider->getStyle ().isCustomStyle (Styles::kSliderAppearanceBarGraph))
	{
		port.fillRect (handleRect, SolidBrush (color));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* SliderRenderer::createColorizedHandle (IImage* sourceImage, ColorRef color)
{
	if(Image* image = unknown_cast<Image> (sourceImage))
	{
		if(image->getIsAdaptive ())
			handleImage = AutoPtr<IImage> (NEW LightAdaptedBitmap (sourceImage, color));
		else if(image->getIsTemplate ())
			handleImage = AutoPtr<IImage> (NEW ColoredBitmap (sourceImage, color));
		else
			handleImage = AutoPtr<IImage> (NEW TintedBitmap (sourceImage, color));
	}
	
	return handleImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SliderRenderer::calcBarGraphRects (Rect& src1, Rect& src2, Slider* slider, bool bipolar)
{
	src1 (0, 0, slider->getWidth (), slider->getHeight ());
	src2 = src1;
	float value = slider->getValue ();
	
	if(bipolar)
	{
		float midNormalized = 0.5;
		
		if(slider->style.isCustomStyle (Styles::kSliderAppearanceDefaultCentered))
		{
			float max = slider->getParameter ()->getMax ();
			float min = slider->getParameter ()->getMin ();
			float mid = slider->getParameter ()->getDefaultValue ();
			midNormalized = (mid - min) / (max - min);
		}
		
		if(slider->getStyle ().isHorizontal ())
		{
			float range = (float)(src1.getWidth () - barGraphMargin);
			src2.left = src2.right = ccl_to_int (src1.getWidth () * midNormalized);
			if(value < midNormalized)
				src2.left = src2.right + (Coord)((value - midNormalized) * range);
			else
				src2.right = src2.left + (Coord)((value - midNormalized) * range);
		}
		else
		{
			float range = (float)(src1.getHeight () - barGraphMargin);
			src2.top = src2.bottom = ccl_to_int (src1.getHeight () * midNormalized);
			if(value < midNormalized)
				src2.bottom = src2.top - (Coord)((value - midNormalized) * range);
			else
				src2.top = src2.bottom - (Coord)((value - midNormalized) * range);
		}
	}
	else
	{
		if(slider->getStyle ().isHorizontal ())
		{
			src2.right = barGraphMargin / 2 + (Coord)((float)((src1.getWidth () - barGraphMargin) * value)) - barGraphOffset;
			src1.left = src2.right;
		}
		else
		{
			src1.bottom = barGraphMargin / 2 + (src1.getHeight () - barGraphMargin) - (Coord)((float)((src1.getHeight () - barGraphMargin) * value)) - barGraphOffset;
			src2.top = src1.bottom;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SliderRenderer::hitTest (View* view, const Point& loc, Point* offset)
{
	if(!initDone)
		init (view);

	Rect rect;
	getHandleRect ((Slider*)view, handleImage, rect, ((Slider*)view)->getValue ());
	if(rect.pointInside (loc))
	{
		if(offset)
		{
			offset->x = loc.x - rect.left;
			offset->y = loc.y - rect.top;
		}
		return Slider::kPartHandle;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SliderRenderer::getHandleRect (const Slider* slider, IImage* handleImage, CCL::Rect& rect, float value)
{
	StyleRef style = slider->getStyle ();

	if(style.isVertical ())
	{
		int handleW =	handleImage ? handleImage->getWidth () : slider->getWidth ();
		int handleH =	handleImage ? handleImage->getHeight () : 
						(style.isCustomStyle (Styles::kSliderAppearanceThinHandle) || style.isCustomStyle (Styles::kSliderAppearanceBarGraph)) ? 2 :
						slider->getTheme ().getThemeMetric (ThemeElements::kSliderHandleSize);

		if(handleImage && scaleHandle)
		{
			handleH *= (slider->getWidth () / (float)handleW);
			handleW = slider->getWidth ();
		}

		if(!slider->isEnabled () && disabledHandleColor.getAlphaF () == 0 && disabledHandleIndex == -1)
			handleH = 1;
			
		float max = (float)(slider->getHeight () - handleH - margin * 2);

		rect.left   = ccl_max (0, slider->getWidth ()/2 - handleW/2);
		rect.top    = margin + (int)((1.f - value) * max + 0.5f);
		rect.right  = ccl_min (rect.left + handleW, slider->getWidth ());
		rect.bottom = rect.top  + handleH;
	}
	else
	{
		int handleW =	handleImage ? handleImage->getWidth () : 
						(style.isCustomStyle (Styles::kSliderAppearanceThinHandle) || style.isCustomStyle (Styles::kSliderAppearanceBarGraph)) ? 2 :
						slider->getTheme ().getThemeMetric (ThemeElements::kSliderHandleSize);
		int handleH =	handleImage ? handleImage->getHeight () : slider->getHeight ();
		
		if(handleImage && scaleHandle)
		{
			handleW *= (slider->getHeight () / (float)handleH);
			handleH = slider->getHeight ();
		}

		if(!slider->isEnabled () && disabledHandleColor.getAlphaF () == 0 && disabledHandleIndex == -1)
			handleW = 1;

		float max = (float)(slider->getWidth () - handleW - margin * 2);

		rect.left   = margin + (int)(value * max + 0.5f);
		rect.top    = ccl_max (0, slider->getHeight ()/2 - handleH/2);
		rect.right  = rect.left + handleW;
		rect.bottom = ccl_min (rect.top + handleH, slider->getHeight ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SliderRenderer::getTrackRect (const Slider* slider, CCL::Rect& rect)
{
	rect.left   = margin;
	rect.top    = margin;
	rect.right = slider->getWidth () - margin;
	rect.bottom = slider->getHeight () - margin;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SliderRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	if(!initDone)
		init (view);

	const Slider* slider = static_cast<const Slider*> (view);
	
	if(partCode == Slider::kPartHandle)
		return getHandleRect (slider, handleImage, rect, slider->getValue ());
	else if(partCode == Slider::kPartTrack)
		return getTrackRect (slider, rect);
	
	return false;
}

//************************************************************************************************
// RangeSliderRenderer
/** A Range Slider draws two handle moving over a background. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (RangeSlider, VisualStyle, "RangeSliderStyle")
	ADD_VISUALSTYLE_IMAGE  ("hHandle2")			///< handle (horizontal slider)
	ADD_VISUALSTYLE_IMAGE  ("vHandle2")			///< handle (vertical slider)
	ADD_VISUALSTYLE_IMAGE  ("hSmallHandle2")	///< handle (horizontal slider with option "small")
	ADD_VISUALSTYLE_IMAGE  ("vSmallHandle2")	///< handle (vertical slider with option "small")
END_VISUALSTYLE_CLASS (RangeSlider)

//////////////////////////////////////////////////////////////////////////////////////////////////

RangeSliderRenderer::RangeSliderRenderer (VisualStyle* visualStyle)
: SliderRenderer (visualStyle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSliderRenderer::init (const View* view)
{
	SliderRenderer::init (view);
	
	Slider* slider = (Slider*)view;
	StyleRef style = view->getStyle ();
	
	if(style.isVertical ())
	{
		if(slider->getWidth () < kSmallSliderSize)
		{
			invertedBackImage = visualStyle->getImage ("vSmallInvertedBack");
			handleImage2 = visualStyle->getImage ("vSmallHandle2");
		}
		if(invertedBackImage == nullptr)
			invertedBackImage = visualStyle->getImage ("vInvertedBack");
		if(handleImage2 == nullptr)
			handleImage2 = visualStyle->getImage ("vHandle2");
	}
	else
	{
		if(slider->getHeight () < kSmallSliderSize)
		{
			invertedBackImage = visualStyle->getImage ("hSmallInvertedBack");
			handleImage2 = visualStyle->getImage ("hSmallHandle2");
		}			
		if(invertedBackImage == nullptr)
			invertedBackImage = visualStyle->getImage ("hInvertedBack");
		if(handleImage2 == nullptr)
			handleImage2 = visualStyle->getImage ("hHandle2");
	}
	
	if(invertedBackImage == nullptr)
		invertedBackImage = backImage;
	if(handleImage2 == nullptr)
		handleImage2 = handleImage;
	
	centerColor = visualStyle->getColor ("centerColor", visualStyle->getForeColor ());
	invertedCenterColor = visualStyle->getColor ("invertedCenterColor", centerColor); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSliderRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	if(!initDone)
		init (view);
	
	GraphicsPort port (view);
	RangeSlider* slider = (RangeSlider*)view;
	
	if(slider->getStyle ().isCustomStyle (Styles::kSliderAppearanceTickScale))
		drawTickScale (port, slider);
	

	bool inverted = slider->getValue () > slider->getSecondValue ();
	
	if(backImage)
		drawBackImage (port, slider, inverted ? invertedBackImage : backImage);
	else
		drawBackcolor (port, slider, inverted ? invertedCenterColor : centerColor);
	
	if(slider->isEnabled ())
	{
		drawHandle (port, slider, handleImage, slider->getValue ());
		drawHandle (port, slider, handleImage2, slider->getSecondValue ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeSliderRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	const RangeSlider* slider = static_cast<const RangeSlider*> (view);
	switch(partCode)
	{
		case RangeSlider::kFirstHandle:
			return getHandleRect (slider, handleImage, rect, slider->getValue ());
		case RangeSlider::kTrackBack:
			return getTrackRect (slider, rect);
		case RangeSlider::kSecondHandle:
			return getHandleRect (slider, handleImage2, rect, slider->getSecondValue ());
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeSliderRenderer::calcBarGraphRects (Rect& src1, Rect& src2, Slider* slider, bool)
{
	src1 (0, 0, slider->getWidth (), slider->getHeight ());
	src2 = src1;
	
	if(RangeSlider* rangeSlider = ccl_cast<RangeSlider> (slider))
	{
		if(slider->getStyle ().isHorizontal ())
		{
			Coord width (src1.getWidth () - (2 * margin));
			src2.left = margin + (Coord)(width * rangeSlider->getValue () + 0.5f);
			src2.right = margin + (Coord)(width * rangeSlider->getSecondValue () + 0.5f);
			if(src2.left > src2.right)
			{
				Coord temp = src2.left;
				src2.left = src2.right;
				src2.right = temp;
			}
		}
		else
		{
			Coord height (src1.getHeight () - (2 * margin));
			src2.top = margin + height - (Coord)(height * rangeSlider->getSecondValue () + 0.5f);
			src2.bottom = margin + height - (Coord)(height * rangeSlider->getValue () + 0.5f);
			if(src2.top > src2.bottom)
			{
				Coord temp = src2.top;
				src2.top = src2.bottom;
				src2.bottom = temp;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* RangeSliderRenderer::createColorizedHandle (IImage* sourceImage, ColorRef color)
{
	if(isEqualUnknown (sourceImage, handleImage))
	   return SliderRenderer::createColorizedHandle (sourceImage, color);
	   
	if(Image* image = unknown_cast<Image> (sourceImage))
	{
		if(image->getIsAdaptive ())
			handleImage2 = AutoPtr<IImage> (NEW LightAdaptedBitmap (sourceImage, color));
		else if(image->getIsTemplate ())
			handleImage2 = AutoPtr<IImage> (NEW ColoredBitmap (sourceImage, color));
		else
			handleImage2 = AutoPtr<IImage> (NEW TintedBitmap (sourceImage, color));
	}
	
	return handleImage2;
}
