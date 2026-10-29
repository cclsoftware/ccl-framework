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
// Description : Text Box Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/textboxrenderer.h"

#include "ccl/gui/controls/textbox.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/itextmodel.h"

using namespace CCL;

extern CStringPtr ButtonStateNames[]; // buttonrenderer.cpp

//************************************************************************************************
// TextBoxRenderer
/** If option "transparent" is not set, a TextBox draws a background image.
When no image is specified, it is filled with the background color.
A frame around it is defined by "background.bordercolor" and "background.borderwidth", rounded by the
four "background.border[left,right,top,bottom]radius" metrics - the radii also clip the background
The text is drawn with an optional "padding". */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (TextBox, VisualStyle, "TextBoxStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (TextBox)
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kBackground)	///< the background image and the frame around it
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kText)			///< the displayed string
	ADD_TYPELIB_MEMBER_STRING (StyleID::kPadding)			///< combined padding (left, right, top, bottom) 
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingLeft)		///< left padding
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingTop)		///< top padding
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingRight)		///< right padding
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingBottom)		///< bottom padding
END_TYPELIB_MEMBERS (TextBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (TextBoxRenderer, ControlRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBoxRenderer::TextBoxRenderer (VisualStyle* visualStyle, View* owner)
: ControlRenderer (visualStyle, owner),
  observedTextModel (nullptr),
  backgroundAtom (AtomName::kBackground, TextBox::kPartContentArea),
  textAtom (AtomName::kText, TextBox::kPartTextExtent)
{
	addAtom (backgroundAtom);
	addAtom (textAtom);
	backgroundAtom.isContentFill (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBoxRenderer::~TextBoxRenderer ()
{
	if(observedTextModel)
		share_and_observe_unknown<ITextModel> (this, observedTextModel, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBoxRenderer::updateAtoms ()
{
	SuperClass::updateAtoms ();

	if(owner == nullptr)
		return;

	TextBox* textBox = static_cast<TextBox*> (owner);

	Rect r;
	textBox->getClientRect (r);

	ITextModel* textModel = textBox->getTextModel ();
	if(observedTextModel != textModel)
		share_and_observe_unknown<ITextModel> (this, observedTextModel, textModel);

	backgroundAtom.setSize (rectIntToF (r));
	if(IImage* background = backgroundAtom.getImage ())
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

		backgroundAtom.setImageFrame (frameIndex);
	}

	r = textBox->getTextRect (r);

	textAtom.setSize (rectIntToF (r));
	textAtom.setTextModel (textModel);
	textAtom.updateTextLayout (); // create text layout if necessary
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBoxRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	if(visualStyle == nullptr || owner == nullptr)
		return;

	SuperClass::applyVisualStyle (transaction);

	textAtom.isTextMultiline (owner->getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine));
	textAtom.isContentFill (owner->getStyle ().isCustomStyle (Styles::kTextBoxAppearanceScaleText));
	// TextFit is handled manually by text box. Don't let the atom also collapse the string.

	// The border is defined by the style, as "background.bordercolor" and "background.borderwidth"
	// SuperClass::applyVisualStyle () has already read those into the atom, so start from them.
	Pen borderPen (backgroundAtom.getBorderPen ());
	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// With the "border" option, "bordercolor" frames the text box. A border color of the atom takes its place,
	// a transparent one switches the border off. A style that defines neither frames it in "forecolor".
	if(owner->getStyle ().isBorder ())
	{
		Color borderColor;
		if(visualStyle->getColor (borderColor, LegacyStyleID::kBorderColor))
			borderPen.setColor (borderColor);
		else if(!backgroundAtom.getStyleColor (borderColor, *visualStyle, AtomPropertyID::kBorderColor, &conditionContext))
			borderPen.setColor (visualStyle->getForeColor ());
	}

	if(!owner->getStyle ().isCommonStyle (Styles::kTransparent))
	{
		Color legacyBackColor;
		bool hasLegacyBackColor = visualStyle->getGradient (StyleID::kBackColor) || visualStyle->getColor (legacyBackColor, StyleID::kBackColor);
		if(hasLegacyBackColor || !backgroundAtom.getBrush ().isVisible ())
			backgroundAtom.setBrush (visualStyle->getBackBrush ());
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif
	
	// a transparent text box has no background, otherwise "background" is an alias of "background.image"
	if(!isOpaque ())
	{
		backgroundAtom.setImage (nullptr);
		backgroundAtom.setBrush (SolidBrush (Colors::kTransparentBlack));
	}
	else if(Image* styleImage = unknown_cast<Image> (visualStyle->getImage (StyleID::kBackground)))
		backgroundAtom.setImage (styleImage);

	if(backgroundAtom.getImage () || borderPen.getColor ().isVisible ())
		backgroundAtom.setBorderPen (borderPen);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	RenderAtom::ColorContrastHelper& helper = textAtom.getColorContrastHelper ();

	Color textColor (Colors::kBlack);
	bool useLegacyTextColor = visualStyle->getColor (textColor, StyleID::kTextColor) || !textAtom.getBrush ().isVisible ();
	if(useLegacyTextColor)
		helper.setDefaultColor (textColor);

	static const LegacyStyleID::ContrastColorIDs kTextContrastColors = {LegacyStyleID::kTextColorBright, LegacyStyleID::kTextColorDark,
		LegacyStyleID::kTextColorTransparent, LegacyStyleID::kTransparentColor, LegacyStyleID::kTextColorAlphaBlend};
	applyLegacyContrastColors (textAtom, kTextContrastColors);

	if(!isOpaque ())
		helper.setDefaultColor (helper.getContrastColorTransparent ());

	helper.setBrightColorThreshold (visualStyle->getMetric<float> (LegacyStyleID::kTextColorThreshold, 0.f));

	if(useLegacyTextColor)
	{
		Brush textBrush (textAtom.getBrush ());
		textBrush.setColor (helper.getDefaultColor ());
		textAtom.setBrush (textBrush);
	}

	if(!owner->isEnabled ())
	{
		Color disabledTextColor (textColor);
		disabledTextColor.alphaBlend (visualStyle->getBackColor (), 0.5f);
		if(visualStyle->getColor (disabledTextColor, LegacyStyleID::kTextColorDisabled) || useLegacyTextColor)
			textAtom.setBrush (SolidBrush (disabledTextColor));
	}

	textAtom.setMinFontSize (visualStyle->getMetric<float> (LegacyStyleID::kScaleTextMinFont, 6));
	textAtom.setMaxFontSize (visualStyle->getMetric<float> (LegacyStyleID::kScaleTextMaxFont, 100));
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	BEGIN_LEGACY_STYLES_DISABLED_BLOCK
	// fall back to "textcolor"
	if(!textAtom.getBrush ().isVisible ())
	{
		Color textColor;
		if(visualStyle->getColor (textColor, StyleID::kTextColor, &conditionContext))
		{
			textAtom.setBrush (SolidBrush (textColor));
			textAtom.getColorContrastHelper ().setDefaultColor (textColor);
		}
	}
	END_LEGACY_STYLES_DISABLED_BLOCK

	textAtom.setTextFont (visualStyle->getTextFont ());

	// setup text color
	TextBox* textBox = static_cast<TextBox*> (owner);

	if(owner->isEnabled ())
	{
		Color foregroundColor = textAtom.getColorContrastHelper ().getDefaultColor ();
		if(UnknownPtr<IColorParam> colorParam = textBox->getColorParam ())
		{
			colorParam->getColor (foregroundColor);
			foregroundColor = textAtom.getColorContrastHelper ().getForegroundColor (foregroundColor);
		}
		textAtom.setBrush (SolidBrush (foregroundColor));
	}

	textAtom.setTextFormat (visualStyle->getTextFormat ());

	textAtom.needsTextLayoutRebuild (true);

	visualStyle->getPadding (padding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxRenderer::onTextInteraction (const IMutableTextModel::InteractionInfo& info)
{
	UnknownPtr<IMutableTextModel> model (textAtom.getTextModel ());
	if(model && textAtom.getTextLayout ())
		return model->onTextInteraction (*textAtom.getTextLayout (), info);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITextLayout* TextBoxRenderer::getTextLayout () const
{
	return textAtom.getTextLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxRenderer::getPartRect (Rect& rect, int partCode)
{
	if(partCode == TextBox::kPartTextExtent)
	{
		// the text atom spans the whole text rect, report the bounds of the actual text instead
		if(const ITextLayout* textLayout = textAtom.getTextLayout ())
		{
			RectF bounds;
			textLayout->getBounds (bounds);
			bounds.offset (textAtom.getSize ().getLeftTop ());
			rect = rectFToEnclosingInt (bounds);
			return true;
		}
	}
	return SuperClass::getPartRect (rect, partCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBoxRenderer::setSize (RectFRef size)
{
	textAtom.setSize (size);
	backgroundAtom.setSize (size);

	// textAtom doesn't have textFit set because textbox handles text collapsing, so we need to trigger the rebuild manually.
	StyleRef style = owner->getStyle ();
	if(!style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine) && style.isCustomStyle (Styles::kTextBoxAppearanceFitText))
		textAtom.needsTextLayoutRebuild (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBoxRenderer::isOpaque () const
{
	if(owner == nullptr)
		return false;

	StyleRef style = owner->getStyle ();
	return style.isOpaque ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextBoxRenderer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && isEqualUnknown (subject, observedTextModel))
		textAtom.needsTextLayoutRebuild (true);
	else if(msg == ITextModel::kRequestLayoutUpdate && isEqualUnknown (subject, observedTextModel))
		textAtom.needsTextLayoutUpdate (true);
}
