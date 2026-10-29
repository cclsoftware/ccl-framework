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
#include "ccl/gui/theme/renderer/textboxrenderer.h"

#include "ccl/gui/controls/selectbox.h"
#include "ccl/gui/views/graphicsport.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

extern CStringPtr ButtonStateNames[]; // buttonrenderer.cpp

//************************************************************************************************
// SelectBoxRenderer
/** If option "transparent" is not set, a SelectBox draws a background image. 
When no image is specified, it is filled in "backcolor", and a rectangular frame in color "forecolor" can be drawn with option "border".

A SelectBox uses the same render atoms as a Button: the "background" atom draws the background,
the "icon" atom draws the popup button, and the "value" atom draws the text.

The background image shows the current parameter value when it has no element state frames: each of
its frames stands for one (integer) value then, like the frames of a MultiToggle button. Such a state
image covers the whole control, so it is usually opaque.

The popup button is pinned to an edge of the area left by "padding", like the icon of a Button,
and the value keeps a distance of "iconvaluespacing" to it. The edge is the horizontal part of
"iconalign": left and right pin the button, centered leaves it unpinned - which only shows when
the text is hidden or "iconoverlay" is set, because otherwise the button is pinned to the trailing
edge to keep the space for the text. With hidden text, an unpinned button is centered within the
whole control, ignoring "padding".
When the parameter provides an image itself, the "value" atom draws this image instead of the text.

A color parameter ("colorname") only affects the "value" and the "icon" atom when the atom has a
"contrastcolorbright" or a "contrastcoloralphablend". Unlike a Button or a TextBox, a SelectBox never
draws them in the color of the parameter itself. */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (SelectBox, TextBox, "SelectBoxStyle", VISUALSTYLES_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (SelectBoxRenderer, BIVRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectBoxRenderer::SelectBoxRenderer (VisualStyle* visualStyle, View* owner)
: BIVRenderer (visualStyle, owner),
  buttonFill (0.f),
  hideText (false),
  hideButton (false),
  hideImage (false),
  showValueText (true),
  fitTextSpace (-1)
  #if LEGACY_STYLES_SUPPORTED
  , valueImageFill (0.f)
  #endif
{
	order = kTrailingIcon; // the popup button is drawn after the value by default
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	if(visualStyle == nullptr || owner == nullptr)
		return;

	StyleRef style = owner->getStyle ();

	hideText = style.isCustomStyle (Styles::kTextBoxAppearanceHideText);
	hideButton = style.isCustomStyle (Styles::kSelectBoxAppearanceHideButton);
	hideImage = style.isCustomStyle (Styles::kSelectBoxAppearanceHideImage);

	// the edge the popup button is pinned to is the horizontal icon alignment - centered means unpinned
	Alignment buttonAlignment (iconAlignment);
	if(style.isCustomStyle (Styles::kSelectBoxAppearanceLeadingIcon))
		buttonAlignment.setAlignH (Alignment::kLeft);
	else if(style.isCustomStyle (Styles::kSelectBoxAppearanceTrailingIcon))
		buttonAlignment.setAlignH (Alignment::kRight);
	else
		buttonAlignment.setAlignH (Alignment::kHCenter);

	// "iconleading" is the style's say on the order and therefore on the edge, so it wins over the
	// button options - the base class reads it again to apply it to the element order
	if(visualStyle->getMetric<bool> (StyleID::BIV::kIconLeading, false))
		buttonAlignment.setAlignH (Alignment::kLeft);

	setButtonAlignment (buttonAlignment);

	// the "on" condition selects the "...On" frames and colors of the visual style
	conditionContext.setCondition (ThemeNames::kOn, isOnState ());

	// This resets all atom properties that are set below.
	SuperClass::applyVisualStyle (transaction);

	buttonFill = iconFillFactor;

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	Alignment legacyButtonAlignment (iconAlignment);
	if(visualStyle->getMetric<bool> (LegacyStyleID::kSelectBoxButtonLeading, false))
	{
		legacyButtonAlignment.setAlignH (Alignment::kLeft);
		setButtonAlignment (legacyButtonAlignment);
	}
	else if(visualStyle->getMetric<bool> (LegacyStyleID::kSelectBoxButtonTrailing, false))
	{
		legacyButtonAlignment.setAlignH (Alignment::kRight);
		setButtonAlignment (legacyButtonAlignment);
	}

	// the state image used to be drawn by a separate "states" atom on top of the background
	if(Image* legacyStates = unknown_cast<Image> (visualStyle->getImage (LegacyStyleID::kSelectBoxStates)))
		backgroundAtom.setImage (legacyStates);

	if(Image* legacyButton = unknown_cast<Image> (visualStyle->getImage (LegacyStyleID::kSelectBoxButton)))
		iconAtom.setImage (legacyButton);

	if(iconAtom.canRecolorImage ())
		iconAtom.setBrush (getIconBrush ());

	buttonFill = visualStyle->getMetric<float> (LegacyStyleID::kSelectBoxButtonFillFactor, buttonFill);
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	if(Image* background = backgroundAtom.getImage ())
		backgroundAtom.setImageFrame (getBackgroundFrame (*background));
	else if(style.isBorder ())
		backgroundAtom.setBorderPen (visualStyle->getForePen ());

	iconAtom.isContentFit (buttonFill > 0.f); // the popup button is scaled to fit into the rect

	// the button that indicates the popup is drawn as icon
	Image* button = iconAtom.getImage ();
	iconAtom.isHidden (button == nullptr || hideButton);
	if(button)
		iconAtom.setImageFrame (getElementStateFrame (*button));

	valueAtom.isTextMultiline (style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine));
	if(valueAtom.isTextMultiline ())
		valueAtom.setWordBreak (true);

	valueAtom.isContentFill (style.isCustomStyle (Styles::kTextBoxAppearanceScaleText));

	updateValueBrush ();

	bool valueImageReplacesText = updateValueImage (); // the image color overrides the text color

	SelectBox* selectBox = static_cast<SelectBox*> (owner);
	IParameter* param = selectBox->getParameter ();
	bool isOutOfRange = param == nullptr || param->isOutOfRange ();
	bool showTitleAsText = style.isCustomStyle (Styles::kSelectBoxAppearanceTitleAsText);

	showValueText = !hideText && !valueImageReplacesText && !(isOutOfRange && !showTitleAsText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::setButtonAlignment (AlignmentRef alignment)
{
	setIconAlignment (alignment);
	if(alignment.getAlignH () == Alignment::kLeft)
		order = kLeadingIcon;
	else
		order = kTrailingIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxRenderer::updateValueImage ()
{
	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	valueImageFill = 0.f;
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	// a parameter that provides an image is shown by that image instead of its value text
	if(hideImage)
		valueAtom.setImage (nullptr);
	else if(UnknownPtr<IImageProvider> imageProvider = static_cast<SelectBox*> (owner)->getParameter ())
	{
		if(Image* image = unknown_cast<Image> (imageProvider->getImage ()))
			valueAtom.setImage (image);
	}

	bool hasValueImage = valueAtom.getImage () != nullptr;
	if(!hasValueImage)
	{
		valueAtom.isContentFit (false);
		return false;
	}

	// the image is fitted into the value area, keeping its aspect ratio - "scaletext" only scales text
	valueAtom.isContentFit (true);
	valueAtom.isContentFill (false);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// the atom brush is given by "value.color" now, "contextcolor" still overrides it
	Color contextColor = visualStyle->getColor (LegacyStyleID::kContextColor, Colors::kTransparentBlack);
	if(isOnState ())
		contextColor = visualStyle->getColor (LegacyStyleID::kSelectBoxContextColorOn, contextColor);

	if(contextColor.isVisible ())
		valueAtom.setBrush (SolidBrush (contextColor)); // a visible color creates a modified version of the image

	valueImageFill = visualStyle->getMetric<float> (LegacyStyleID::kImageFillFactor, 0.f);
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SelectBoxRenderer::getBackgroundFrame (const Image& image) const
{
	// A background image with element state frames is drawn like the background of a Button. One
	// without them is a state image: each of its frames stands for one (integer) parameter value.
	if(image.getFrameIndex (ButtonStateNames[ThemeElements::kNormal]) >= 0)
		return getElementStateFrame (image);

	IParameter* param = static_cast<SelectBox*> (owner)->getParameter ();
	if(param == nullptr)
		return 0;

	if(!param->isEnabled ())
	{
		int disabledFrame = image.getFrameIndex (ThemeNames::kDisabled);
		if(disabledFrame >= 0)
			return disabledFrame;
	}

	return param->getValue ().asInt ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::updateValueBrush ()
{
	if(!owner->isEnabled ())
		return; // the disabled text color has already been applied

	SelectBox* selectBox = static_cast<SelectBox*> (owner);

	if(UnknownPtr<IColorParam> colorParam = selectBox->getColorParam ())
	{
		Color colorParamColor (Colors::kTransparentBlack);
		colorParam->getColor (colorParamColor);

		// a SelectBox never takes over the color of the parameter itself
		if(valueAtom.hasDefinedContrast ())
			valueAtom.applyContrastColor (colorParamColor);

		if(iconAtom.canRecolorImage () && iconAtom.hasDefinedContrast ())
			iconAtom.applyContrastColor (colorParamColor);
	}

	Brush valueBrush (valueAtom.getBrush ());

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// an explicit color for the current visual state overrides the text color
	int visualState = selectBox->getVisualState ();
	if(visualState > 0)
	{
		MutableCString colorName;
		colorName.appendFormat (LegacyStyleID::kSelectBoxStateColorFormat, visualState);
		valueBrush.setColor (visualStyle->getColor (colorName, valueBrush.getColor ()));
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	valueAtom.setBrush (valueBrush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::checkFitTextSpace ()
{
	// With "fittext" the collapsed value text is a cache over the display area (see TextBox::modifyDisplayString ()),
	// so it must be reevaluated whenever that area changes.
	StyleRef style = owner->getStyle ();
	if(valueAtom.isTextMultiline () || !style.isCustomStyle (Styles::kTextBoxAppearanceFitText))
		return;

	SelectBox* selectBox = static_cast<SelectBox*> (owner);
	Coord displaySpace = style.isVertical () ? selectBox->getDisplayHeight () : selectBox->getDisplayWidth ();
	if(displaySpace != fitTextSpace)
	{
		fitTextSpace = displaySpace;
		valueAtom.needsTextLayoutRebuild (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::updateAtoms ()
{
	if(owner == nullptr)
		return;

	SelectBox* selectBox = static_cast<SelectBox*> (owner);

	if(updateTextModel (selectBox->getTextModel ()))
		valueAtom.updateTextLayout ();

	checkFitTextSpace (); // must happen before the text layout is built below

	SuperClass::updateAtoms ();

	// The popup button is pinned to an edge of the padded area and the value takes the remaining
	// space, so icon and value are placed here instead of being aligned as one group.
	StyleRef style = owner->getStyle ();
	RectF totalRect (backgroundAtom.getSize ());
	RectF paddedRect (totalRect);
	paddedRect.left   += padding.left;
	paddedRect.top    += padding.top;
	paddedRect.right  -= padding.right;
	paddedRect.bottom -= padding.bottom;

	RectF valueRect (paddedRect);	// area left for the value text
	RectF buttonRect (paddedRect);

	if(!iconAtom.isHidden ())
	{
		RectF imageSize;
		getImageSize (imageSize, iconAtom.getImage (), totalRect, buttonFill);

		if(style.isVertical ())
		{
			if(hideText)
				buttonRect = totalRect; // the button is centered within the whole control
			else
				buttonRect.top = buttonRect.bottom - imageSize.getHeight ();

			valueRect.bottom = buttonRect.top - iconValueSpacing;
		}
		else
		{
			int buttonAlignH = iconAlignment.getAlignH ();
			if(buttonAlignH == Alignment::kLeft) // pin to left
				buttonRect.right = buttonRect.left + imageSize.getWidth ();
			else if(buttonAlignH == Alignment::kRight || !(hideText || iconOverlay)) // pin to right
				buttonRect.left = buttonRect.right - imageSize.getWidth ();
			else if(hideText)
				buttonRect = totalRect; // the button is centered within the whole control
			// otherwise the popup button is centered within the padded area

			if(!iconOverlay) // an overlay button doesn't take away space from the value
			{
				if(buttonAlignH == Alignment::kLeft)
					valueRect.left = buttonRect.right + iconValueSpacing;
				else
					valueRect.right = buttonRect.left - iconValueSpacing;
			}
		}

		RectF iconRect (imageSize);
		iconRect.center (buttonRect);
		snapToPixelGrid (iconRect, buttonRect);
		iconAtom.setSize (iconRect);
	}

	// an image provided by the parameter takes the place of the value text
	bool hasValueImage = valueAtom.getImage () != nullptr;
	valueAtom.isHidden ((!showValueText && !hasValueImage) || valueRect.isEmpty ());

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// "fill.image" sizes the value image relative to the control and may reach beyond it, which the
	// atom's proportional fitting cannot express
	if(hasValueImage && valueImageFill > 0.f && !valueAtom.isHidden ())
	{
		RectF imageRect;
		getImageSize (imageRect, valueAtom.getImage (), totalRect, valueImageFill);
		imageRect.center (valueRect);
		valueRect = imageRect;
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	valueAtom.setSize (valueRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectBoxRenderer::draw (const UpdateRgn& updateRgn)
{
	SuperClass::draw (updateRgn);

	StyleRef style = owner->getStyle ();
	if(!owner->isFocused () || style.isCustomStyle (Styles::kSelectBoxAppearanceHideFocus))
		return;

	Image* background = backgroundAtom.getImage ();
	if(background == nullptr || backgroundAtom.isHidden ())
		return;

	int focusFrame = background->getFrameIndex (ThemeNames::kFocused);
	if(focusFrame < 0) // draw focus overlay if available
		return;

	background->setCurrentFrame (focusFrame);

	GraphicsPort port (owner);
	Rect src (0, 0, background->getWidth (), background->getHeight ());
	port.drawImage (background, src, rectFToInt (backgroundAtom.getSize ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxRenderer::isOnState () const
{
	if(owner == nullptr || !owner->getStyle ().isCustomStyle (Styles::kSelectBoxAppearanceOffState))
		return false;

	IParameter* param = static_cast<SelectBox*> (owner)->getParameter ();
	return param ? param->getValue ().asBool () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SelectBoxRenderer::getElementStateFrame (const Image& image) const
{
	int elementState = owner->getThemeElementState ();
	if(image.getFrameIndex (ButtonStateNames[elementState]) < 0)
		elementState = ThemeElements::kNormal;

	if(isOnState ())
	{
		int onStateFrame = image.getFrameIndex (ButtonStateNames[elementState + ThemeElements::kNumElementStates]);
		if(onStateFrame >= 0)
			return onStateFrame;
	}

	int frame = image.getFrameIndex (ButtonStateNames[elementState]);
	return frame < 0 ? 0 : frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SelectBoxRenderer::hitTest (PointRef where, Point* clickOffset)
{
	Rect r;

	getPartRect (r, SelectBox::kPartContentArea);
	if(r.pointInside (where))
	{
		if(clickOffset)
		{
			clickOffset->x = where.x - r.left;
			clickOffset->y = where.y - r.top;
		}
		return SelectBox::kPartContentArea;
	}

	getPartRect (r, SelectBox::kPartDropDownButton);
	if(r.pointInside (where))
	{
		if(clickOffset)
		{
			clickOffset->x = where.x - r.left;
			clickOffset->y = where.y - r.top;
		}
		return SelectBox::kPartDropDownButton;
	}
	return SelectBox::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectBoxRenderer::getPartRect (Rect& r, int partCode)
{
	if(owner == nullptr)
		return false;

	if(styleConditionsDirty)
		AtomRenderer::updateAtoms (); // apply the visual style only - measuring the text here could recurse via the fit text handling

	Image* button = hideButton ? nullptr : iconAtom.getImage ();

	if(partCode == SelectBox::kPartContentArea)
	{
		if(hideText)
			return false;

		owner->getClientRect (r);
		if(button)
		{
			// "iconvaluespacing" is kept between the popup button and the value, so it is not part of
			// the content area either - an overlay button takes no space, hence no spacing (see updateAtoms ())
			Coord spacing = iconOverlay ? 0 : iconValueSpacing;
			if(owner->getStyle ().isVertical ())
				r.bottom -= button->getHeight () + iconValueSpacing;
			else if(iconAlignment.getAlignH () == Alignment::kLeft)
				r.left += button->getWidth () + spacing;
			else
				r.right -= button->getWidth () + spacing;
		}
		r.contract (1);
		return true;
	}
	else if(partCode == SelectBox::kPartDisplayArea)
	{
		if(getPartRect (r, SelectBox::kPartContentArea))
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
		if(button == nullptr)
			return false;

		owner->getClientRect (r);
		if(!hideText)
		{
			if(owner->getStyle ().isVertical ())
				r.top = r.bottom - button->getHeight ();
			else if(iconAlignment.getAlignH () == Alignment::kLeft)
				r.right -= r.left + button->getWidth ();
			else
				r.left = r.right - button->getWidth ();
		}
		return true;
	}

	return false;
}
