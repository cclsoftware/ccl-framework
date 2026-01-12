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
// Filename    : ccl/gui/theme/renderer/selectboxrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/selectboxrenderer.h"

#include "ccl/gui/graphics/imaging/imagecache.h"
#include "ccl/gui/controls/selectbox.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

extern const char* ButtonStateNames[]; // buttonrenderer.cpp

//************************************************************************************************
// SelectBoxRenderer
/** If option "transparent" is not set, a SelectBox draws a background image. 
When no image is specified, it is filled in "backcolor, and a rectangular frame in color "forecolor" can be drawn with option "border".

An image "states" can be used to show a different frame for (integer) each parameter value.

The "button" image is drawn at the right (can be changed with option "leadingbutton").
The remaining space shows the text representation of the parameter, with an optional "padding". */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (SelectBox, VisualStyle, "SelectBoxStyle")
	ADD_VISUALSTYLE_IMAGE  ("button")			///< image used to draw the button that indicates that somthing can popup
	ADD_VISUALSTYLE_IMAGE  ("states")			///< image with frames for each (integer) parameter state
	ADD_VISUALSTYLE_METRIC ("padding.left")		///< left padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.top")		///< top padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.right")	///< right padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.bottom")	///< bottom padding for the text
	ADD_VISUALSTYLE_METRIC ("padding")			///< padding for title, used if one of the paddings for left, top, right, bottom is not specified
	ADD_VISUALSTYLE_METRIC ("fill.image")		///< image from imageprovider is resized to fill out the selectbox size (aspectratio is kept) value: ]0-1]
	ADD_VISUALSTYLE_METRIC ("fill.button")		///< button image is resized to fill out the selectbox height (aspectratio is kept) value: ]0-1]
	ADD_VISUALSTYLE_COLOR ("contextcolor")		///< if set, used to create a modified version of the image given by the imageprovider
	ADD_VISUALSTYLE_METRIC ("scaletext.maxfont")///< explicit maximal fontsize when scaletext option is set
	ADD_VISUALSTYLE_METRIC ("scaletext.minfont")///< explicit minimal fontsize when scaletext option is set
	ADD_VISUALSTYLE_METRIC ("button.beneath")	///< button image is drawn beneath text or state images and doesn't affect their drawrect
	ADD_VISUALSTYLE_METRIC ("button.leading")	///< button image is drawn before the text - set can also be set with button-option "leadingbutton"
	ADD_VISUALSTYLE_METRIC ("button.trailing")	///< button image is drawn after the text - can also be set with button-option "trailingbutton"
END_VISUALSTYLE_CLASS (SelectBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectBoxRenderer::SelectBoxRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle),
  hideText (false),
  hideButton (false),
  hideImage (false),
  buttonBeneath (false),
  leadingButton (false),
  trailingButton (false),
  hasOffState (false),
  imageFillSize (0.f),
  buttonFillSize (0.f),
  textColorThreshold (0.35f),
  initialized (false)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::initialize (StyleRef style, const View* view)
{
	if(visualStyle == nullptr)
		return;
	
	hideText = style.isCustomStyle (Styles::kSelectBoxAppearanceHideText);
	hideButton = style.isCustomStyle (Styles::kSelectBoxAppearanceHideButton);
	hideImage = style.isCustomStyle (Styles::kSelectBoxAppearanceHideImage);
	hasOffState = style.isCustomStyle (Styles::kSelectBoxAppearanceOffState);
	leadingButton = style.isCustomStyle (Styles::kSelectBoxAppearanceLeadingButton);
	trailingButton = style.isCustomStyle (Styles::kSelectBoxAppearanceTrailingButton);
	
	leadingButton = visualStyle->getMetric<bool> ("button.leading", leadingButton);
	trailingButton = visualStyle->getMetric<bool> ("button.trailing", trailingButton);
	buttonBeneath = visualStyle->getMetric<bool> ("button.beneath", false);
	background = visualStyle->getImage ("background");
	button = visualStyle->getImage ("button");
	states = visualStyle->getImage ("states");
	contextColor = visualStyle->getColor ("contextcolor", Colors::kTransparentBlack);
	
	contextColorOn = visualStyle->getColor ("contextcolor.on", contextColor);
	imageFillSize = visualStyle->getMetric<float> ("fill.image", 0.f);
	buttonFillSize = visualStyle->getMetric<float> ("fill.button", 0.f);
	
	textColorThreshold = visualStyle->getMetric<float> ("textcolor.threshold", textColorThreshold);
	
	textColor = visualStyle->getTextColor ();
	textColorOn = visualStyle->getColor ("textcolor.on", textColor);
	textColorMouseOver = visualStyle->getColor ("textcolor.mouseover", textColor);
	
	textColorBright = visualStyle->getColor ("textcolor.bright", textColor);
	Color disabledFallback (textColor);
	textColorDisabled = visualStyle->getColor ("textcolor.disabled", disabledFallback.alphaBlend (visualStyle->getBackColor (), .5f));
	
	visualStyle->getPadding (padding);
	
	textScaler.setExplicitMaximalFontSize (visualStyle->getMetric<float> ("scaletext.maxfont", 100));
	textScaler.setExplicitMinimalFontSize (visualStyle->getMetric<float> ("scaletext.minfont", 6));

	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	StyleRef style = view->getStyle ();
	
	if(initialized == false)
		initialize (style, view);

	SelectBox* selectBox = (SelectBox*)view;
	GraphicsPort port (selectBox);

	bool paramIsOn = selectBox->getParameter () ? selectBox->getParameter ()->getValue ().asBool () : false;
	bool isOutOfRange = selectBox->getParameter () ? selectBox->getParameter ()->isOutOfRange () : true;
	bool drawAsOnState = (hasOffState && paramIsOn);
	
	Rect r;
	selectBox->getClientRect (r);
	
	auto selectImageFrame = [&](IImage* image)
	{
		int elementState = selectBox->getThemeElementState ();
		if(image->getFrameIndex (ButtonStateNames[elementState]) == -1)
			elementState = 0;
		
		int offStateOffset = 0;
		if(drawAsOnState)
			offStateOffset = ThemeElements::kNumElementStates;
		
		IImage::Selector (image, ButtonStateNames[elementState + offStateOffset]);
	};
	
	if(style.isOpaque ())
	{
		if(background)
		{
			selectImageFrame (background);
			port.drawImage (background, Rect (0, 0, background->getWidth (), background->getHeight ()), r);
		}
		else
		{
			port.fillRect (updateRgn.bounds, visualStyle->getBackBrush ());
			if(style.isBorder ())
				port.drawRect (r, visualStyle->getForePen ());
		}
	}

	if(button && !hideButton)
	{
		selectImageFrame (button);
		Rect imageSrc (0, 0, button->getWidth (), button->getHeight ());
		Point imageSize (button->getWidth (), button->getHeight ());
		Rect buttonRect (r);

		float imageResize = buttonFillSize * ccl_min (r.getHeight (), r.getWidth ());
		float resizeRatio = (imageResize > 0.f) ? (imageResize / ccl_max (imageSize.x, imageSize.y)) : 1;
		imageSize *= resizeRatio;

		if(style.isVertical ())
		{
			if(!hideText)
				buttonRect.top = r.bottom - imageSize.y;

			r.bottom = buttonRect.top;
		}
		else
		{
			if(hideText)
			{
				if(trailingButton)
					buttonRect.left = r.right - imageSize.x;
				else if(leadingButton)
					buttonRect.right = imageSize.x;
			}
			else
			{
				if(leadingButton)
					buttonRect.right = imageSize.x;
				else if(buttonBeneath && !trailingButton)
					; // don't modify "buttonrect"
				else
					buttonRect.left = r.right - imageSize.x;
					
			}
			
			if(buttonBeneath)
				; // don't modify "r"
			else if(leadingButton)
				r.left = buttonRect.right;
			else
				r.right = buttonRect.left;
		}

		Point imagePos = buttonRect.getLeftTop () + ((buttonRect.getSize () - imageSize) * .5f);
		
		Rect dst;
		dst.setSize (imageSize);
		dst.moveTo (imagePos);
		
		port.drawImage (button, imageSrc, dst);
	}

	if(!r.isEmpty ())
	{
		if(!hideImage)
		{

			auto drawCentered = [&](IImage* image)
			{
				Rect imageSrc (0, 0, image->getWidth (), image->getHeight ());
				Point imageSize (image->getWidth (), image->getHeight ());
				Point imagePos;
							
				float imageResize = imageFillSize * ccl_min (r.getHeight (), r.getWidth ());
				float resizeRatio = (imageResize > 0.f) ? (imageResize / ccl_max (imageSize.x, imageSize.y)) : 1;
				imageSize *= resizeRatio;
				imagePos = r.getLeftTop () + ((r.getSize () - imageSize) * .5f);
				
				Rect dst;
				dst.setSize (imageSize);
				dst.moveTo (imagePos);
				
				port.drawImage (image, imageSrc, dst);
			};
			
			if(states)
			{
				// use param value as frame index
				int frameIndex = 0;
				if(IParameter* param = selectBox->getParameter ())
				{
					if(param->isEnabled ())
						frameIndex = param->getValue ().asInt ();
					else
					{
						frameIndex = states->getFrameIndex (ThemeNames::kDisabled);
						if(frameIndex < 0)
							frameIndex = param->getValue ().asInt ();
					}
				}
				states->setCurrentFrame (frameIndex);
				
				if(states->getType () == IImage::kScalable)
					port.drawImage (states, Rect (0, 0, states->getWidth (), states->getHeight ()), r);
				else
				{
					IImage* modifiedState = states;
						
					if((drawAsOnState && contextColorOn.getAlphaF () != 0))
						modifiedState = ModifiedImageCache::instance ().lookup (states, contextColorOn);
					else if(contextColor.getAlphaF () != 0)
						modifiedState = ModifiedImageCache::instance ().lookup (states, contextColor);
				
					drawCentered (modifiedState);
				}
			}
			else if(UnknownPtr<IImageProvider> imageProvider = selectBox->getParameter ())
			{
				if(IImage* image = imageProvider->getImage ())
				{
					IImage* modifiedImage = nullptr;
					
					if((drawAsOnState && contextColorOn.getAlphaF () != 0))
						modifiedImage = ModifiedImageCache::instance ().lookup (image, contextColorOn);
					else if(contextColor.getAlphaF () != 0)
						modifiedImage = ModifiedImageCache::instance ().lookup (image, contextColor);

					drawCentered (modifiedImage ? modifiedImage : image);
					hideText = true;
				}
			}
		}

		r.left   += padding.left;
		r.right  -= padding.right;
		r.top    += padding.top;
		r.bottom -= padding.bottom;

		if(selectBox->isFocused () && !style.isCustomStyle (Styles::kSelectBoxAppearanceHideFocus))
		{
			Rect focusRect;
			selectBox->getClientRect (focusRect);
			
			if(background && background->getFrameIndex (ButtonStateNames[ThemeElements::kFocused]) >= 0) // draw focus overlay if available
			{
				IImage::Selector (background, ButtonStateNames[ThemeElements::kFocused]);
				port.drawImage (background, Rect (0, 0, background->getWidth (), background->getHeight ()), focusRect);
			}
		}

		StringRef text = selectBox->getText ();
		if(!hideText && !text.isEmpty () && !(isOutOfRange && !style.isCustomStyle (Styles::kSelectBoxAppearanceTitleAsText)))
		{
			Font font (visualStyle->getTextFont ().zoom (selectBox->getZoomFactor ()));
						
			if(style.isCustomStyle (Styles::kTextBoxAppearanceScaleText))
				textScaler.scaleTextFont (font, r, text);

			SolidBrush textBrush (textColor);
			
			bool buttonMouseOver = ThemeElements::kMouseOver == selectBox->getThemeElementState ();
			if(buttonMouseOver)
				textBrush.setColor (textColorMouseOver);
			
			if(drawAsOnState)
				textBrush.setColor (textColorOn);
			
			UnknownPtr<IColorParam> colorParam = selectBox->getColorParam ();
			if(needsBrightText (colorParam))
				textBrush.setColor (textColorBright);
				
			int state = selectBox->getVisualState ();
			if(state > 0)
			{
				MutableCString colorName;
				colorName.appendFormat ("state%d", state);
				textBrush.setColor (visualStyle->getColor (colorName, textColor));
			}

			if(!selectBox->isEnabled ())
				textBrush.setColor (textColorDisabled);

			if(style.isVertical ())
			{
				Transform t;
				t.translate ((float)r.left, (float)r.bottom);
				t.rotate (Math::degreesToRad (270.f));
				Rect r2 (0, 0, r.getHeight (), r.getWidth ());
				port.saveState ();
				port.addTransform (t);
				port.drawString (r2, text, font, textBrush, visualStyle->getTextAlignment ());
				port.restoreState ();
			}
			else
			{
				port.drawString (r, text, font, textBrush, visualStyle->getTextAlignment ());
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxRenderer::needsBrightText (IColorParam* colorParam) const
{
	if(colorParam)
	{
		Color colorParamColor;
		colorParam->getColor (colorParamColor);
		return colorParamColor.getLuminance () < textColorThreshold;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SelectBoxRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	Rect r;

	getPartRect (view, SelectBox::kPartContentArea, r);
	if(r.pointInside (loc))
	{
		if(clickOffset)
		{
			clickOffset->x = loc.x - r.left;
			clickOffset->y = loc.y - r.top;
		}
		return SelectBox::kPartContentArea;
	}

	getPartRect (view, SelectBox::kPartDropDownButton, r);
	if(r.pointInside (loc))
	{
		if(clickOffset)
		{
			clickOffset->x = loc.x - r.left;
			clickOffset->y = loc.y - r.top;
		}
		return SelectBox::kPartDropDownButton;
	}
	return SelectBox::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxRenderer::getPartRect (const View* view, int partCode, CCL::Rect& r)
{
	if(initialized == false)
		initialize (view->getStyle (), view);
		
	if(partCode == SelectBox::kPartContentArea)
	{
		if(view->getStyle ().isCustomStyle (Styles::kSelectBoxAppearanceHideText))
			return false;

		view->getClientRect (r);
		if(button && !hideButton)
		{
			if(view->getStyle ().isVertical ())
				r.bottom -= button->getHeight ();
			else if(leadingButton)
				r.left += button->getWidth ();
			else
				r.right -= button->getWidth ();
		}
		r.contract (1);
		return true;
	}
	else if(partCode == SelectBox::kPartDisplayArea)
	{
		if(getPartRect (view, SelectBox::kPartContentArea, r))
		{
			r.left += padding.left;
			r.top += padding.top;
			r.right -= padding.right;
			r.bottom -= padding.bottom;
			return true;
		}
		return false;
	}
	else if(partCode == SelectBox::kPartDropDownButton)
	{
		if(!button || hideButton)
			return false;

		view->getClientRect (r);
		if(!view->getStyle ().isCustomStyle (Styles::kSelectBoxAppearanceHideText))
		{
			if(view->getStyle ().isVertical ())
				r.top = r.bottom - button->getHeight ();
			else if(leadingButton)
				r.right -= r.left + button->getWidth ();
			else
				r.left = r.right - button->getWidth ();
		}
		return true;
	}

	return false;
}
