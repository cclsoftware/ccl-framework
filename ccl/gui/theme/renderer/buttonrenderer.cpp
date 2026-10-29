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
// Filename    : ccl/gui/theme/renderer/buttonrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/buttonrenderer.h"

#include "ccl/gui/graphics/imaging/imagecache.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/gui/controls/button.h"
#include "ccl/gui/controls/textmodel.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ButtonStateNames[] = // reused in selectboxrenderer.cpp
{
	"normal",    "pressed",    "mouseover",    "disabled",    "focus",
	"normalOn",  "pressedOn",  "mouseoverOn",  "disabledOn",  "focusOn"
};

//************************************************************************************************
// ButtonRenderer
/** A button draws a background and some content. The background is not drawn when the option "transparent" is set.
The background can be an image "background" (or "left" or "right" when the correspending option is set).

A frame of the image is chosen depending on the mouse state and parameter value.
Supported frame names are "normal[On]", "pressed[On]", "mouseover[On]", "disabled[On]", "focus[On]".

Unlike other atoms, the "background" atom fills its "color" behind the image, but only if the image has neither
the "template" nor the "adaptive" attribute. Otherwise the color colorizes the image.
When no image is specified, a rectangular frame in color "forecolor" can be drawn with option "border".
For the content, a padding can be specified. The content can be an "icon", "title" or both.*/
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (Button, BIV, "ButtonStyle", VISUALSTYLES_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (ButtonRenderer, BIVRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

ButtonRenderer::ButtonRenderer (VisualStyle* visualStyle, View* owner)
: BIVRenderer (visualStyle, owner),
  initialized (false),
  backgroundImage (nullptr)
  #if LEGACY_STYLES_SUPPORTED
  , animationFilmstrip (nullptr),
  lastButtonValue (-1),
  phaseIconFrameIndex (-1),
  phaseForPendingAnimation (0),
  animationState (kAnimationStopped)
  #endif
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	if(!initialized)
		initialize ();

	SharedPtr<IImage> oldIcon = iconAtom.getImage ();

	// This applies the general condition based style removing
	// any properties that are set in this renderer. We have to
	// set them again below.
	SuperClass::applyVisualStyle (transaction);

	Button* button = static_cast<Button*> (owner);
	StyleRef style = owner->getStyle ();
	if(style.isCustomStyle (Styles::kButtonAppearanceLeadingIcon))
		order = kLeadingIcon;
	else if(style.isCustomStyle (Styles::kButtonAppearanceTrailingIcon))
		order = kTrailingIcon;

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	animationFilmstrip = unknown_cast<Image> (visualStyle->getImage (LegacyStyleID::kAnimationFilmstrip));
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	backgroundImage = backgroundAtom.getImage ();
	backgroundAtom.isBrushBackdrop (!backgroundAtom.canRecolorImage ());

	int frame = button->getCurrentFrame ();
	if(backgroundImage)
	{
		initFrames (frameIndex, *backgroundImage);
		if(frameIndex.count () > frame)
			backgroundAtom.setImageFrame (frameIndex.at (frame));
	}

	if(Image* buttonIcon = unknown_cast<Image> (button->getIcon ()))
		iconAtom.setImage (buttonIcon);

	Image* icon = iconAtom.getImage ();
	if(icon && icon->getFrameCount () > 0)
	{
		// the icon can change with the style conditions - keep the frame index in sync
		if(oldIcon != icon)
			updateIconFrames (*icon);

		#if LEGACY_STYLES_SUPPORTED
		BEGIN_LEGACY_STYLES_ENABLED_BLOCK
		if(visualStyle->getMetric<bool> (LegacyStyleID::kColorizeIcon, false))
			icon->setIsTemplate (true);

		if(iconAtom.canRecolorImage ())
			iconAtom.setBrush (getIconBrush ());
		END_LEGACY_STYLES_ENABLED_BLOCK
		#endif

		// icon size can depend on current frame...
		// ...select frame beforehand
		if(iconFrameIndex.count () > frame)
			iconAtom.setImageFrame (iconFrameIndex.at (frame));

		#if LEGACY_STYLES_SUPPORTED
		BEGIN_LEGACY_STYLES_ENABLED_BLOCK
		if(!button->isEnabled ())
		{
			if(iconFrameIndex[ThemeElements::kDisabled] == iconFrameIndex[ThemeElements::kNormal]) // when the icon has no own 'disabled' frame
			{
				Color iconColorDisabled;
				if(owner->getVisualStyle ().getColor (iconColorDisabled, LegacyStyleID::kIconColorDisabled)) // when the iconColorDisabled is set
					iconAtom.setBrush (SolidBrush (iconColorDisabled));
			}
		}
		END_LEGACY_STYLES_ENABLED_BLOCK
		#endif
	}

	valueAtom.isTextMultiline (style.isCustomStyle (Styles::kButtonAppearanceMultiLine));
	if(valueAtom.isTextMultiline ())
		valueAtom.setWordBreak (true);

	valueAtom.isContentFill (style.isCustomStyle (Styles::kButtonAppearanceScaleText));

	applyColorParam ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::draw (const UpdateRgn& updateRgn)
{
	SuperClass::draw (updateRgn);

	StyleRef style = owner->getStyle ();
	if(owner->isFocused () && !style.isCustomStyle (Styles::kButtonAppearanceHideFocus))
	{
		auto drawFocus = [&] (Image* image, RectRef rect, int frame)
		{
			if(image && image->getFrameIndex (ThemeNames::kFocused) >= 0) // draw focus overlay if available
			{
				image->setCurrentFrame (frame);

				GraphicsPort port (owner);
				Rect src (0, 0, image->getWidth (), image->getHeight ());
				port.drawImage (image, src, rect);
			}
		};

		if(!backgroundAtom.isHidden () && frameIndex.count () > ThemeElements::kFocused)
			drawFocus (backgroundAtom.getImage (), rectFToInt (backgroundAtom.getSize ()), frameIndex [ThemeElements::kFocused]);

		if(!iconAtom.isHidden () && iconFrameIndex.count () > ThemeElements::kFocused)
			drawFocus (iconAtom.getImage (), rectFToInt (iconAtom.getSize ()), iconFrameIndex [ThemeElements::kFocused]);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::updateAtoms ()
{
	if(!initialized)
		initialize ();

	Button* button = static_cast<Button*> (owner);
	updateTextModel (button->getTextModel ());

	SuperClass::updateAtoms ();

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// draw animated phase overlay
	if(animationFilmstrip)
	{
		updateAnimationState ();
		if(isAnimationPending () || isAnimationRunning ())
		{
			float phase = isAnimationPending () ? phaseForPendingAnimation : button->getPhase ();
			int phaseFrame = ccl_to_int (animationFilmstrip->getFrameCount () * phase);
			backgroundAtom.setImage (animationFilmstrip);
			backgroundAtom.setImageFrame (phaseFrame);
		}
		else if(backgroundImage) // animation stopped - draw button image
		{
			backgroundAtom.setImage (backgroundImage);
			backgroundAtom.setImageFrame (frameIndex [button->getCurrentFrame ()]);
		}
	}

	if(phaseIconFrameIndex != -1 && button->getPhase () > 0.f)
	{
		iconAtom.setImageFrame (phaseIconFrameIndex);
		iconAtom.setOpacity (button->getPhase ());
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::initialize ()
{
	Button* button = static_cast<Button*> (owner);

	int numFrames = ccl_max (10, button->getNumFrames ());
	frameIndex.resize (numFrames);
	frameIndex.setCount (numFrames);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	Image* icon = unknown_cast<Image> (button->getIcon ());
	if(icon == nullptr)
	{
		if(icon = unknown_cast<Image> (visualStyle->getImage (LegacyStyleID::kIcon)))
		{
			button->setIcon (icon);
			button->hasIconFromVisualStyle (true);
		}
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::updateIconFrames (Image& icon)
{
	Button* button = static_cast<Button*> (owner);

	int numFrames = ccl_max (10, button->getNumFrames ());
	iconFrameIndex.resize (numFrames);
	iconFrameIndex.setCount (numFrames);
	initFrames (iconFrameIndex, icon);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	phaseIconFrameIndex = icon.getFrameIndex (LegacyStyleID::kPhaseOn);
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::initFrames (IntVector& index, IImage& image)
{
	for(int i = 0; i < ARRAY_COUNT (ButtonStateNames); i++)
		index[i] = image.getFrameIndex (ButtonStateNames[i]);

	// fallbacks for missing frames:
	// disabledOn -> disabled
	if(index[8] == -1)
		index[8] = index[3];

	// element states -> "normal"
	for(int i = 1; i < 5; i++)
	{
		if(index[i] == -1)
			index[i] = index[0];
	}

	// "normalOn" -> "pressed"
	if(index[5] == -1)
		index[5] = index[1];

	// "..On" element states -> "normalOn"
	for(int i = 6; i < 10; i++)
	{
		if(index[i] == -1)
			index[i] = index[5];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::updateAnimationState ()
{
	Button* button = static_cast<Button*> (owner);
	if(lastButtonValue == -1)
	{
		lastButtonValue = (button->isOn () ? 1 : 0);
		animationState = kAnimationStopped; // no animation pending or running when initial value was not set
	}
	else
	{
		if(lastButtonValue != (button->isOn () ? 1 : 0))
		{
			animationState = kAnimationPending;
			phaseForPendingAnimation = lastButtonValue;
			lastButtonValue = (button->isOn () ? 1 : 0);
		}
		
		if(isAnimationPending () || isAnimationRunning ())
		{
			if(button->getPhase () > 0.f && button->getPhase () < 1.f)
				animationState = kAnimationRunning;
			else if(animationState == kAnimationRunning)
				animationState = kAnimationStopped;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif // LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonRenderer::getColorParamColor (Color& color) const
{
	Button* button = static_cast<Button*> (owner);
	if(!button->isEnabled ())
		return false;

	UnknownPtr<IColorParam> colorParam (button->getColorParam ());
	if(!colorParam.isValid ())
		return false;

	colorParam->getColor (color);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::applyColorParam ()
{
	Color colorParamColor (Colors::kTransparentBlack);
	if(!getColorParamColor (colorParamColor))
		return;

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// an opaque color from the color parameter replaces the background color from the style
	bool colorizeBackground = colorParamColor.isVisible ();

	// "backcolor.off" hides the color of the color parameter while the button is off
	Color backcolorOff (Colors::kTransparentBlack);
	if(!conditionContext.matches (ThemeNames::kOn) && visualStyle->getColor (backcolorOff, LegacyStyleID::kBackcolorOff))
		colorizeBackground = false;

	if(colorizeBackground)
		backgroundAtom.setBrush (SolidBrush (colorParamColor));
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	BEGIN_LEGACY_STYLES_DISABLED_BLOCK
	// a background color from the style wins over the color parameter
	if(!backgroundAtom.getBrush ().isVisible ())
	{
		if(colorParamColor.isVisible ())
			backgroundAtom.setBrush (SolidBrush (colorParamColor));
		else
			backgroundAtom.applyContrastColor (colorParamColor); // "contrastcolortransparent" is used when the color parameter has no color
	}
	END_LEGACY_STYLES_DISABLED_BLOCK

	// value and icon follow the color parameter only when the style defines contrast colors for them
	valueAtom.applyContrastColor (colorParamColor);

	// a visible icon brush means that the icon is colorized, so it follows the color parameter as well
	if(iconAtom.getBrush ().isVisible ())
		iconAtom.applyContrastColor (colorParamColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectF ButtonRenderer::getSize () const
{
	RectF r (SuperClass::getSize ());
	Button* button = static_cast<Button*> (owner);
	Theme& theme = button->getTheme ();

	if(IVisualStyle* vs = owner->getVisualStyleDirect ())
	{
		// A style that defines no minimum size falls back to the size of the background image.
		float styleMinWidth = 0.f;
		float styleMinHeight = 0.f;
		bool hasMinWidth = visualStyle->getMetric (styleMinWidth, StyleID::kMinWidth);
		bool hasMinHeight = visualStyle->getMetric (styleMinHeight, StyleID::kMinHeight);

		ccl_lower_limit (r.right,  styleMinWidth);
		ccl_lower_limit (r.bottom, styleMinHeight);

		IImage* image = (hasMinWidth || hasMinHeight) ? nullptr : vs->getBackgroundImage ();
		if(image && !owner->getStyle ().isCustomStyle (Styles::kButtonLayoutIgnoreImageSize))
		{
			r.setWidth (ccl_max<float> (r.getWidth (), image->getWidth ()));
			r.setHeight (ccl_max<float> (r.getHeight (), image->getHeight ()));
		}
	}	
	else if(ccl_strict_cast<CheckBox> (owner) == nullptr)
	{
		if(ccl_strict_cast<RadioButton> (owner) == nullptr)
		{
			// default size for Buttons and ToolButtons without a given style
			ccl_lower_limit<float> (r.right,  theme.getThemeMetric (ThemeElements::kButtonWidth));
			ccl_lower_limit<float> (r.bottom, theme.getThemeMetric (ThemeElements::kButtonHeight));
		}
	}

	return r;
}

//************************************************************************************************
// MultiToggleRenderer
/** A MultiToggle is drawn like a Button, but with different frame names for the parameter values.
For each value of the button's (integer) parameter, the 5 frame names
"normal", "pressed", "mouseover", "disabled", and "focus", appended with the value, are used if available in the background bitmap.

	"normal0", "pressed0", "mouseover0", "disabled0", "focus0",
	"normal1", "pressed1", "mouseover1", "disabled1",  "focus1",
	"normal2", "pressed2", ...

Frame names that are not available, are replaced with reasonable fallbacks. */
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (MultiToggleRenderer, ButtonRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiToggleRenderer::MultiToggleRenderer (VisualStyle* visualStyle, View* owner)
: ButtonRenderer (visualStyle, owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiToggleRenderer::initFrames (IntVector& index, IImage& image)
{
	// "normal0", "pressed0", ..., "focus0"
	// "normal1", "pressed1", ..., "focus1"
	// "normal2", "pressed2", ..., "focus2"
	// ...

	int numFrames = index.count ();
	int numValues = numFrames / 5;

	char name[64] = {0};
	for(int i = 0; i < numFrames; i++)
	{
		::snprintf (name, sizeof(name), "%s%d", ButtonStateNames[i % 5], i / 5);
		index[i] = image.getFrameIndex (name);
	}

	// allow "normal", "pressed", ... instead of "normal0", "pressed0", ...
	for(int i = 0; i < 5; i++)
		if(index[i] == -1)
			index[i] = image.getFrameIndex (ButtonStateNames[i]);

	// fallbacks for missing frames:
	// "normalX" -> "normal0", "pressedX" -> "pressed0", ...
	for(int value = 1; value < numValues; value++)
	{
		for(int i = 0; i < 5; i++)
		{
			int normalIndex = 5 * value + i;
			if(index[normalIndex] == -1)
				index[normalIndex] = index[i];
		}
	}

	// element states -> "normalX"
	for(int value = 0; value < numValues; value++)
	{
		int normalIndex = 5 * value;
		for(int i = normalIndex + 1; i < normalIndex + 5; i++)
			if(index[i] == -1)
				index[i] = index[normalIndex];
	}
}

//************************************************************************************************
// CheckBoxRenderer
/** Draws an image that shows the parameter state (typically a checkmark) and an additional title besides the image. */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (CheckBox, Button, "CheckBoxStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (CheckBox)
	ADD_TYPELIB_MEMBER_METRIC ("checkboxrightside") ///< if this is set ("1"), the checkbox is drawn algined to the right side of the view rect.
	ADD_TYPELIB_MEMBER_IMAGE ("mixedicon") ///< fallback when no mixedicon at the checkbox is defined
END_TYPELIB_MEMBERS (CheckBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (CheckBoxRenderer, ButtonRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

CheckBoxRenderer::CheckBoxRenderer (VisualStyle* visualStyle, View* owner)
: ButtonRenderer (visualStyle, owner),
  mixedIcon (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBoxRenderer::initialize ()
{
	CheckBox* checkBox = static_cast<CheckBox*> (owner);
	mixedIcon = unknown_cast<Image> (checkBox->getMixedIcon ());

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	if(mixedIcon == nullptr)
	{
		if(mixedIcon = unknown_cast<Image> (visualStyle->getImage (LegacyStyleID::kIconMixed)))
			checkBox->setMixedIcon (mixedIcon);
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	if(mixedIcon)
	{
		ASSERT (mixedIcon->getFrameCount ()) // resources are missing!

		int numFrames = ccl_max (10, checkBox->getNumFrames ());
		mixedIconFrameIndex.resize (numFrames);
		mixedIconFrameIndex.setCount (numFrames);

		initFrames (mixedIconFrameIndex, *mixedIcon);
	}

	SuperClass::initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBoxRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	SuperClass::applyVisualStyle (transaction);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	float rightSide = 0.f;
	if(visualStyle->getMetric (rightSide, LegacyStyleID::kCheckBoxRightSide))
	{
		if(rightSide != 0.f)
			order = kTrailingIcon;
		else
			order = kLeadingIcon;
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	CheckBox* checkBox = static_cast<CheckBox*> (owner);
	if(mixedIcon && checkBox->isMixed ())
	{
		iconAtom.setImage (mixedIcon);
		iconAtom.setImageFrame (mixedIconFrameIndex.at (checkBox->getCurrentFrame ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeElementID CheckBoxRenderer::getThemeId () const
{
	CheckBox* checkBox = static_cast<CheckBox*> (owner);
	bool checked = checkBox->getParameter ()->getValue () == checkBox->getParameter ()->getMax ();
	return checked ? ThemeElements::kCheckBoxChecked : ThemeElements::kCheckBoxNormal;
}

//************************************************************************************************
// RadioButtonRenderer
/** A Radio button is drawn like a CheckBox. */
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (RadioButtonRenderer, CheckBoxRenderer)
