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
// Filename    : ccl/gui/theme/renderer/textboxrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/textboxrenderer.h"

#include "ccl/gui/controls/textbox.h"

#include "ccl/public/gui/framework/itextmodel.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/iimage.h"

using namespace CCL;

extern const char* ButtonStateNames[]; // buttonrenderer.cpp

//************************************************************************************************
// TextBoxRenderer
/** If option "transparent" is not set, a TextBox draws a background image. 
When no image is specified, it is filled in "backcolor, and a rectangular frame in color "forecolor" can be drawn with option "border".

The text is drawn with an optional "padding". */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (TextBox, VisualStyle, "TextBoxStyle")
	ADD_VISUALSTYLE_COLOR  ("textcolor")			///< textcolor for the displayed string
	ADD_VISUALSTYLE_COLOR  ("textcolor.transparent")///< used instead of "textcolor" if the TextBox's option "transparent" is set or "colorname" color is transparent
	ADD_VISUALSTYLE_COLOR  ("textcolor.bright")		///< used instead of "textcolor" if the current luminance of the "colorname" color is below the "textcolor.threshold"
	ADD_VISUALSTYLE_COLOR  ("textcolor.dark")		///< used when the current luminance of the "colorname" color is above the "textcolor.threshold" (using "textcolor" as fallback, needs definition of textcolor.bright)
	ADD_VISUALSTYLE_COLOR  ("textcolor.alphablend")	///< the non-transparent "colorname" color will be alphablended with this color (using "color.alphablend" as fallback) and used as textcolor
	ADD_VISUALSTYLE_METRIC ("textcolor.threshold")	///< "textcolor.bright" is used instead of "textcolor" if the luminance threshold for the current "colorname" color is below this value - default is 0.35
	ADD_VISUALSTYLE_COLOR  ("textcolor.disabled")	///< text color used (instead of "textcolor") when control is disabled
	ADD_VISUALSTYLE_METRIC ("padding.left")			///< left padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.top")			///< top padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.right")		///< right padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.bottom")		///< bottom padding for the text
	ADD_VISUALSTYLE_METRIC ("padding")				///< padding for the text, used if one of the paddings for left, top, right, bottom is not specified
	ADD_VISUALSTYLE_METRIC ("scaletext.maxfont")	///< explicit maximal fontsize when scaletext option is set	
	ADD_VISUALSTYLE_METRIC ("scaletext.minfont")	///< explicit minimal fontsize when scaletext option is set
END_VISUALSTYLE_CLASS (TextBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBoxRenderer::TextBoxRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle),
  brightColorThreshold (0.35f),
  initialized (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBoxRenderer::initialize (StyleRef style)
{
	if(visualStyle == nullptr)
		return;
	
	textColorOpaque = visualStyle->getColor ("textcolor");
	textContrastDarkColor = visualStyle->getColor ("textcolor.dark", textColorOpaque);
	textContrastTransparentColor = visualStyle->getColor ("textcolor.transparent", visualStyle->getColor ("transparentcolor", textColorOpaque));
	textContrastBrightColor = visualStyle->getColor ("textcolor.bright", Colors::kTransparentBlack);
	alphaBlendColor = visualStyle->getColor ("textcolor.alphablend", visualStyle->getColor ("color.alphablend", Colors::kTransparentBlack));
	
	Color disabledFallbackColor = textColorOpaque;
	disabledFallbackColor.alphaBlend (visualStyle->getBackColor (), 0.5f);
	disabledTextColor = visualStyle->getColor ("textcolor.disabled", disabledFallbackColor);
	
	borderPenColor = visualStyle->getColor ("bordercolor", visualStyle->getForeColor ());
	background = visualStyle->getImage ("background");
	
	brightColorThreshold = visualStyle->getMetric<float> ("textcolor.threshold", brightColorThreshold);
	
	// set default textColor
	textBrush.setColor (style.isOpaque () ? textColorOpaque : textContrastTransparentColor);
	
	visualStyle->getPadding (padding);
	
	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBoxRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	StyleRef style = view->getStyle ();
	if(initialized == false)
		initialize (style);

	TextBox* textBox = static_cast<TextBox*> (view);
	GraphicsPort port (textBox);
	Rect r;
	textBox->getClientRect (r);

	if(isOpaque (view))
	{
		if(background)
		{
			int frameIndex = 0;
			if(textBox->isFocused ())
			{
				frameIndex = background->getFrameIndex (ButtonStateNames[ThemeElements::kFocused]);
				if(frameIndex < 0)
					frameIndex = background->getFrameIndex (ButtonStateNames[ThemeElements::kNormal]); // fallback
			}
			else
				frameIndex = background->getFrameIndex (ButtonStateNames[ThemeElements::kNormal]);
			
			if(frameIndex < 0)
				frameIndex = 0;

			background->setCurrentFrame (frameIndex);
			port.drawImage (background, Rect (0, 0, background->getWidth (), background->getHeight ()), r);
		}
		else 
		{
			port.fillRect (updateRgn.bounds, visualStyle->getBackBrush ());
			if(style.isBorder ())
				port.drawRect (r, borderPenColor);
		}
	}

	// setup text color
	UnknownPtr<IColorParam> colorParam = textBox->getColorParam ();
	SolidBrush currentBrush (view->isEnabled () ? getTextColor (colorParam) : disabledTextColor);
	
	ITextLayout* textLayout = textBox->getTextLayout ();
	if(textLayout != nullptr)
		drawLayout (view, port, textLayout, currentBrush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef TextBoxRenderer::getTextColor (IColorParam* colorParam) const
{
	// textColor is dependent on colorParam when this parameter is used
	if(colorParam != nullptr)
	{
		colorParamColor = Colors::kTransparentBlack;
		colorParam->getColor (colorParamColor);
		
		if(colorParamColor.getAlphaF () == 0.f)
		{
			colorParamColor = textContrastTransparentColor;
		}
		else if(textContrastBrightColor != Colors::kTransparentBlack)
		{
			if(colorParamColor.getLuminance () < brightColorThreshold)
				colorParamColor = textContrastBrightColor;
			else
				colorParamColor = textContrastDarkColor;
		}
		else if(alphaBlendColor.getAlphaF () != 0)
		{
			colorParamColor.alphaBlend (alphaBlendColor, alphaBlendColor.getAlphaF ());
		}
	}
	else
	{
		colorParamColor = textBrush.getColor ();
	}
	
	return colorParamColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxRenderer::drawLayout (View* view, GraphicsPort& port, ITextLayout* layout, BrushRef textBrush)
{
	auto textBox = static_cast<TextBox*> (view);
	Rect r (textBox->getSize ());
	r.moveTo (Point (0, 0));

	port.saveState ();
	port.addClip (r);
	Transform t;
	t.translate (-textBox->getDisplayOffset (), 0);
	port.addTransform (t);

	if(ITextModel* textModel = textBox->getTextModel ())
	{
		ITextModel::DrawInfo drawInfo = { view, port, r };
		textModel->drawBackground (*layout, drawInfo);
	}

	bool succeeded = port.drawTextLayout (textBox->getTextRect ().getLeftTop (), layout, textBrush) == kResultOk;
	port.restoreState ();
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TextBoxRenderer::hitTest (View* view, const CCL::Point& loc, CCL::Point* clickOffset)
{
	CCL::Rect r;

	getPartRect (view, TextBox::kPartContentArea, r);
	if(r.pointInside (loc))
	{
		if(clickOffset)
		{
			clickOffset->x = loc.x - r.left;
			clickOffset->y = loc.y - r.top;
		}
		return TextBox::kPartContentArea;
	}
	return TextBox::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxRenderer::getPartRect (const View* view, int partCode, CCL::Rect& r)
{
	StyleRef style = view->getStyle ();
	if(initialized == false)
		initialize (style);
		
	view->getClientRect (r);

	if(partCode == TextBox::kPartContentArea)
		return true;
	else if(partCode == TextBox::kPartTextExtent)
	{
		TextBox* textBox = (TextBox*)view;
		Rect rect = textBox->getTextRect ();
		ITextLayout* textLayout = textBox->getTextLayout ();
		if(textLayout == nullptr)
		{
			if(view->getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
				return true;

			rect.setSize (Point ());
			r = rect;
		}
		else
			textLayout->getBounds (rect);

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxRenderer::isOpaque (const View* view) const
{
	StyleRef style = view->getStyle ();
	return style.isOpaque ();
}
