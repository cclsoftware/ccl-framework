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
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/math/mathprimitives.h"


using namespace CCL;

const char* ButtonStateNames[] = // reused in selectboxrenderer.cpp
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

When an image is used, an additonal background color "backcolor.on" or "backcolor.off" can be drawn underneath the button image.
When no image is specified, a rectangular frame in color "forecolor" can be drawn with option "border".
For the content, a padding can be specified. The content can be an "icon", "title" or both.*/
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (Button, VisualStyle, "ButtonStyle")
	ADD_VISUALSTYLE_IMAGE  ("left")						///< used instead of "background" image when button has option "left"
	ADD_VISUALSTYLE_IMAGE  ("right")					///< used instead of "background" image when button has option "right"
	ADD_VISUALSTYLE_IMAGE  ("single")					///< used instead of "background" image when button has option "left" and "right"
	ADD_VISUALSTYLE_IMAGE  ("middle")					///< used instead of "background" image when button has option "middle"
	ADD_VISUALSTYLE_IMAGE  ("icon")						///< fallback when no icon at the button is defined
	ADD_VISUALSTYLE_COLOR  ("textcolor")				///< textcolor for the button title
	ADD_VISUALSTYLE_COLOR  ("textcolor.on")				///< used instead of "textcolor" when button is switched on
	ADD_VISUALSTYLE_COLOR  ("textcolor.pressed")		///< used instead of "textcolor.on" when button is pressed and off
	ADD_VISUALSTYLE_COLOR  ("textcolor.pressedOn")		///< used instead of "textcolor.pressed" when button is pressed and on
	ADD_VISUALSTYLE_COLOR  ("textcolor.mouseover")		///< used instead of "textcolor" when mouse is over button
	ADD_VISUALSTYLE_COLOR  ("textcolor.mouseoverOn")	///< used instead of "textcolor.on" when mouse is over button
	ADD_VISUALSTYLE_COLOR  ("textcolor.disabled")		///< used instead of "textcolor" when button is disabled
	ADD_VISUALSTYLE_COLOR  ("textcolor.disabledOn")		///< used instead of "textcolor.disabled" when button is disabled and on
	ADD_VISUALSTYLE_COLOR  ("textcolor.transparent")	///< used when the current color from colorparam is transparent
	ADD_VISUALSTYLE_COLOR  ("textcolor.transparentOn")	///< used when the parameter is on and the current color from colorparam is transparent
	ADD_VISUALSTYLE_COLOR  ("textcolor.bright")			///< used when the current luminance of the "colorname" color is below the "textcolor.threshold"
	ADD_VISUALSTYLE_COLOR  ("textcolor.brightOn")		///< used when the parameter is on and the current colorparam luminance is below the "textcolor.threshold"
	ADD_VISUALSTYLE_COLOR  ("textcolor.dark")			///< used when the current luminance of the "colorname" color is above the "textcolor.threshold" (using "textcolor" as fallback, needs definition of textcolor.bright)
	ADD_VISUALSTYLE_COLOR  ("textcolor.darkOn")			///< used when the current luminance of the "colorname" color is above the "textcolor.threshold" (using "textcolor" as fallback, needs definition of textcolor.bright)
	ADD_VISUALSTYLE_COLOR  ("textcolor.alphablend")		///< the non-transparent "colorname" color will be alphablended with this color (using "color.alphablend" as fallback) and used as textcolor
	ADD_VISUALSTYLE_COLOR  ("textcolor.phaseOn")		///< used instead of "textcolor" when phase is on and button is disabled
	ADD_VISUALSTYLE_METRIC ("textcolor.threshold")		///< used instead of "textcolor" if the luminance threshold for the current "colorname" color is below this value - default is 0.35
	ADD_VISUALSTYLE_METRIC ("text.width")				///< truncate title to fit into this width
	ADD_VISUALSTYLE_COLOR  ("backcolor.on")				///< background color drawn underneath button image when button is switched on (overwritten by opaque colorParam color)
	ADD_VISUALSTYLE_COLOR  ("backcolor.off")			///< background color drawn underneath button image when button is switched off (even when a colorParam is used)
	ADD_VISUALSTYLE_COLOR  ("backcolor.transparent")	///< background color drawn underneath button image when button is switched off (when colorParam color is transparent)
	ADD_VISUALSTYLE_METRIC ("padding.left")				///< left padding for title and/or icon
	ADD_VISUALSTYLE_METRIC ("padding.top")				///< top padding for title and/or icon
	ADD_VISUALSTYLE_METRIC ("padding.right")			///< right padding for title and/or icon
	ADD_VISUALSTYLE_METRIC ("padding.bottom")			///< bottom padding for title and/or icon
	ADD_VISUALSTYLE_METRIC ("padding")					///< padding for icon & title, used if one of the paddings for left, top, right, bottom is not specified
	ADD_VISUALSTYLE_METRIC ("spacing.icon")				///< customizable space between icon and the text (default is 3 points)
	ADD_VISUALSTYLE_METRIC ("fill.icon")				///< scalable icons (shapes/svgs) are resized to fill out the button size (aspect ratio is kept) value: ]0-1]
	ADD_VISUALSTYLE_METRIC ("colorize.icon")			///< set to use "iconcolor" / "iconcolor.on" to colorize the icon (checkmark of CheckBox is always colorized!)
	ADD_VISUALSTYLE_METRIC ("lightadapt.icon")			///< set to use "iconcolor" / "iconcolor.on" to modify the icon and adapt the luminance of dark/light pixels to the iconcolor
	ADD_VISUALSTYLE_COLOR  ("iconcolor")				///< used when "colorize.icon" is set ("iconcolor" is CheckBox's "checkmark" color)
	ADD_VISUALSTYLE_COLOR  ("iconcolor.pressed")		///< used when "colorize.icon" is set
	ADD_VISUALSTYLE_COLOR  ("iconcolor.pressedOn")		///< used when "colorize.icon" is set
	ADD_VISUALSTYLE_COLOR  ("iconcolor.mouseover")		///< used when "colorize.icon" is set
	ADD_VISUALSTYLE_COLOR  ("iconcolor.mouseoverOn")	///< used when "colorize.icon" is set
	ADD_VISUALSTYLE_COLOR  ("iconcolor.on")				///< used when "colorize.icon" is set and button is switched on
	ADD_VISUALSTYLE_COLOR  ("iconcolor.disabled")		///< used when button is disabled
	ADD_VISUALSTYLE_COLOR  ("iconcolor.transparent")	///< used when "colorize.icon" is set and the current color from colorparam is transparent
	ADD_VISUALSTYLE_COLOR  ("iconcolor.transparentOn")	///< used when "colorize.icon" is set, the parameter is on and the current color from colorparam is transparent
	ADD_VISUALSTYLE_COLOR  ("iconcolor.bright")			///< used when "colorize.icon" is set and the current colorparam luminance is below the "textcolor.threshold"
	ADD_VISUALSTYLE_COLOR  ("iconcolor.brightOn")		///< used when "colorize.icon" is set, the parameter is on and the current colorparam luminance is below the "textcolor.threshold"
	ADD_VISUALSTYLE_COLOR  ("iconcolor.dark")			///< used when "colorize.icon" is set and the current colorparam luminance is above the "textcolor.threshold" (using "textcolor" as fallback, needs definition of textcolor.bright)
	ADD_VISUALSTYLE_COLOR  ("iconcolor.darkOn")			///< used when "colorize.icon" is set, the parameter is on and the current colorparam luminance is above the "textcolor.threshold" (using "textcolor" as fallback, needs definition of textcolor.bright)
	ADD_VISUALSTYLE_COLOR  ("iconcolor.alphablend")		///< the non-transparent "colorname" color will be alphablended with this color (using "color.alphablend" as fallback) and used as iconcolor
	ADD_VISUALSTYLE_METRIC ("useButtonMinSize")			///< use minimal button size (uses theme size as default)
	ADD_VISUALSTYLE_METRIC ("buttonMinWidth")			///< use minimal button width
	ADD_VISUALSTYLE_METRIC ("buttonMinHeight")			///< use minimal button height
	ADD_VISUALSTYLE_METRIC ("backcolor.radius")			///< radius for backcolor / forcolor
	ADD_VISUALSTYLE_IMAGE  ("animation.filmstrip")		///< an animation filmstrip could be used to show intermediate button states - the visualstyle is responisble to define the appropriate animiation triggers for the phase property
	ADD_VISUALSTYLE_METRIC ("scaletext.maxfont")		///< explicit maximal fontsize when scaletext option is set
	ADD_VISUALSTYLE_METRIC ("scaletext.minfont")		///< explicit minimal fontsize when scaletext option is set
	ADD_VISUALSTYLE_METRIC ("textshiftdown")			///< the text baseline will be offsetted by one point if the button is on
END_VISUALSTYLE_CLASS (Button)

//////////////////////////////////////////////////////////////////////////////////////////////////

ButtonRenderer::ButtonRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle),
  frameIndex (10),
  iconFrameIndex (10),
  phaseFrameIndex (-1),
  phaseIconFrameIndex (-1),
  backcolorRadius (0),
  iconSpacing (3),
  iconFillSize (0.f),
  leadingIcon (false),
  trailingIcon (false),
  useModifiedIcon (false),
  drawAsTemplate (true),
  animationState (kAnimationStopped),
  phaseForPendingAnimation (0),
  lastButtonValue (-1),
  textShiftDownMode (false),
  brightColorThreshold (0.35f),
  initialized (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::setImage (IImage* _image)
{
	image = _image;

	if(image)
		initFrames (frameIndex, *image);

	phaseFrameIndex = image ? image->getFrameIndex ("phaseOn") : -1;
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
		if(index[i] == -1)
			index[i] = index[0];

	// "normalOn" -> "pressed"
	if(index[5] == -1)
		index[5] = index[1];

	// "..On" element states -> "normalOn"
	for(int i = 6; i < 10; i++)
		if(index[i] == -1)
			index[i] = index[5];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::initialize (StyleRef style, View* view)
{
	if(visualStyle == nullptr)
		return;
	
	Button* button = static_cast<Button*> (view);
	int numFrames = ccl_max (10, button->getNumFrames ());
	frameIndex.resize (numFrames);
	frameIndex.setCount (numFrames);
	
	IImage* image = nullptr;
	if(style.isCommonStyle (Styles::kMiddle))
		image = visualStyle->getImage ("middle");
	if(style.isCommonStyle (Styles::kLeft) && style.isCommonStyle (Styles::kRight))
		image = visualStyle->getImage ("single");
	else if(style.isCommonStyle (Styles::kLeft))
		image = visualStyle->getImage ("left");
	else if(style.isCommonStyle (Styles::kRight))
		image = visualStyle->getImage ("right");
	if(image == nullptr)
		image = visualStyle->getBackgroundImage ();
	setImage (image);

	IImage* icon = button->getIcon ();
	if(icon == nullptr)
	{
		if(icon = visualStyle->getImage ("icon"))
		{
			button->setIcon (icon);
			button->hasIconFromVisualStyle (true);
		}
	}
	if(icon)
	{
		ASSERT (icon->getFrameCount ()) // resources are missing!
		iconFrameIndex.resize (numFrames);
		iconFrameIndex.setCount (numFrames);
		initFrames (iconFrameIndex, *icon);
		phaseIconFrameIndex = icon->getFrameIndex ("phaseOn");
	}
	
	leadingIcon = style.isCustomStyle (Styles::kButtonAppearanceLeadingIcon);
	trailingIcon = style.isCustomStyle (Styles::kButtonAppearanceTrailingIcon);
	
	textColor = visualStyle->getTextColor ();
	textColorOn = visualStyle->getColor ("textcolor.on", textColor);
	textColorPressed = visualStyle->getColor ("textcolor.pressed", textColor);
	textColorMouseOver = visualStyle->getColor ("textcolor.mouseover", textColor);
	textColorMouseOverOn = visualStyle->getColor ("textcolor.mouseoverOn", textColorOn);
	textColorPressedOn = visualStyle->getColor ("textcolor.pressedOn", textColorOn);
	textColorDisabled = visualStyle->getColor ("textcolor.disabled", textColor);
	textColorDisabledOn = visualStyle->getColor ("textcolor.disabledOn", textColorDisabled);
	textColorPhaseOn = visualStyle->getColor ("textcolor.phaseOn", textColor);
	iconColor = visualStyle->getColor ("iconcolor", textColor);
	iconColorOn = visualStyle->getColor ("iconcolor.on", textColorOn);
	iconColorDisabled = visualStyle->getColor ("iconcolor.disabled", Colors::kTransparentBlack);
	iconMouseoverColor = visualStyle->getColor ("iconcolor.mouseover", iconColor);
	iconMouseoverColorOn = visualStyle->getColor ("iconcolor.mouseoverOn", iconColorOn);
	iconPressedColor = visualStyle->getColor ("iconcolor.pressed", iconColor);
	iconPressedColorOn = visualStyle->getColor ("iconcolor.pressedOn", iconColorOn);
	bool colorizeIcon = visualStyle->getMetric<bool> ("colorize.icon", false);
	bool lightAdaptIcon = visualStyle->getMetric<bool> ("lightadapt.icon", false);
	useModifiedIcon = lightAdaptIcon || colorizeIcon; 
	drawAsTemplate = !lightAdaptIcon;

	textContrastBrightColor = visualStyle->getColor ("textcolor.bright", Colors::kTransparentBlack);
	textContrastBrightColorOn = visualStyle->getColor ("textcolor.brightOn", textContrastBrightColor);
	textContrastDarkColor = visualStyle->getColor ("textcolor.dark", textColor);
	textContrastDarkColorOn = visualStyle->getColor ("textcolor.darkOn", textContrastDarkColor);
	textContrastTransparentColor = visualStyle->getColor ("textcolor.transparent", textColor);
	textContrastTransparentColorOn = visualStyle->getColor ("textcolor.transparentOn", textColorOn);
	textColorAlphaBlend = visualStyle->getColor ("textcolor.alphablend", visualStyle->getColor ("color.alphablend", Colors::kTransparentBlack));
	
	iconContrastBrightColor = visualStyle->getColor ("iconcolor.bright", Colors::kTransparentBlack);
	iconContrastBrightColorOn = visualStyle->getColor ("iconcolor.brightOn", iconContrastBrightColor);
	iconContrastDarkColor = visualStyle->getColor ("iconcolor.dark", iconColor);
	iconContrastDarkColorOn = visualStyle->getColor ("iconcolor.darkOn", iconContrastDarkColor);
	iconContrastTransparentColor = visualStyle->getColor ("iconcolor.transparent", iconColor);
	iconContrastTransparentColorOn = visualStyle->getColor ("iconcolor.transparentOn", iconColorOn);
	iconColorAlphaBlend = visualStyle->getColor ("iconcolor.alphablend", textColorAlphaBlend);
	
	colorParamColor = Colors::kTransparentBlack;
	
	visualStyle->getPadding (padding);
	
	backcolorRadius = visualStyle->getMetric<Coord> ("backcolor.radius", 0);
	iconSpacing = visualStyle->getMetric<Coord> ("spacing.icon", iconSpacing);
	iconFillSize = visualStyle->getMetric<float> ("fill.icon", 0.f);
	brightColorThreshold = visualStyle->getMetric<float> ("textcolor.threshold", brightColorThreshold);
	
	animationFilmstrip = visualStyle->getImage ("animation.filmstrip");	
	
	textScaler.setExplicitMaximalFontSize (visualStyle->getMetric<float> ("scaletext.maxfont", 100));
	textScaler.setExplicitMinimalFontSize (visualStyle->getMetric<float> ("scaletext.minfont", 6));
	textShiftDownMode = visualStyle->getMetric<bool> ("textshiftdown", false);
	
	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::draw (View* view, const UpdateRgn& updateRgn)
{	
	StyleRef style = view->getStyle ();
	if(initialized == false)
		initialize (style, view);

	Button* button = static_cast<Button*> (view);
	GraphicsPort port (button);
	Rect rect;
	button->getClientRect (rect);

	UnknownPtr<IColorParam> colorParam = button->getColorParam ();
	
	if(style.isOpaque ())
	{
		Color backColor (Colors::kTransparentBlack);
		
		if(colorParam)
			colorParam->getColor (backColor);

		if(button->isOn () == false)
			backColor = visualStyle->getColor ("backcolor.off", backColor); // backcolor.off can be used to hide the colorParam color
		
		if(backColor.getAlphaF () == 0)
			backColor = visualStyle->getColor (button->isOn () ? (image ? "backcolor.on" : StyleID::kForeColor.str ()) : 
											  (image ? "backcolor.transparent" : StyleID::kBackColor.str ()),
											  Colors::kTransparentBlack);
		
		// draw backcolor
		if((backColor != Colors::kTransparentBlack) && button->isEnabled ())
			port.fillRoundRect (rect, backcolorRadius, backcolorRadius, SolidBrush (backColor));
		
		
		// *** Draw Button Image ***
		if(animationFilmstrip)
		{
			updateAnimationState (button);
			
			if(isAnimationPending () || isAnimationRunning ())
			{
				float phase = isAnimationPending () ? phaseForPendingAnimation : button->getPhase ();
				int phaseFrame = ccl_to_int (animationFilmstrip->getFrameCount () * phase);
				animationFilmstrip->setCurrentFrame (phaseFrame);
				port.drawImage (animationFilmstrip, Rect (0,0, animationFilmstrip->getWidth (), animationFilmstrip->getHeight ()), rect);
			}
			else if(image) // animation stopped - draw button image
			{
				image->setCurrentFrame (frameIndex [button->getCurrentFrame ()]);
				port.drawImage (image, Rect (0, 0, image->getWidth (), image->getHeight ()), rect);
			}
		}
		else if(image)
		{
			image->setCurrentFrame (frameIndex [button->getCurrentFrame ()]);
			Rect src (0, 0, image->getWidth (), image->getHeight ());
			port.drawImage (image, src, rect);

			// draw animated phase overlay
			if(phaseFrameIndex != -1 && button->getPhase () > 0.f)
			{
				image->setCurrentFrame (phaseFrameIndex);
				ImageMode mode (button->getPhase ());
				port.drawImage (image, src, rect, &mode);
			}
		}
		else if(style.isBorder ())
			port.drawRect (rect, Pen (visualStyle->getForeColor ()));
		
		// *** Draw Focus ***
		if(button->isFocused () && !style.isCustomStyle (Styles::kButtonAppearanceHideFocus))
		{
			if(image && image->getFrameIndex (ThemeNames::kFocused) >= 0) // draw focus overlay if available
			{
				image->setCurrentFrame (frameIndex [ThemeElements::kFocused]);
			
				Rect src (0, 0, image->getWidth (), image->getHeight ());
				port.drawImage (image, src, rect);
			}
		}
	}
	
	IImage* icon = button->getIcon ();
	if(button->getTitle ().isEmpty () == false || icon != nullptr)
	{
		if(style.isVertical ())
		{
			port.saveState ();

			Transform t;
			t.translate ((float)rect.left, (float)rect.bottom);
			t.rotate (Math::degreesToRad (-90.f));
			port.addTransform (t);

			rect (0, 0, rect.getHeight (), rect.getWidth ());
		}

		rect.left	+= padding.left;
		rect.right	-= padding.right;
		rect.top	+= padding.top;
		rect.bottom	-= padding.bottom;

		bool buttonIsPressed = ThemeElements::kPressed == button->getThemeElementState ();
		bool buttonMouseOver = ThemeElements::kMouseOver == button->getThemeElementState ();
		
		if(textShiftDownMode)
			if(button->isOn () || buttonIsPressed)
				rect.offset (0,1);

		Point iconSize;
		Point iconPos;
		Rect iconSrc;
		
		if(icon)
		{
			if(icon->getFrameCount ())
			{
				// icon size can depend on current frame...
				// ...select frame beforehand
				int frame = button->getCurrentFrame ();
				if(iconFrameIndex.count () > frame)
					icon->setCurrentFrame (iconFrameIndex.at (frame));
					
				if(MultiImage* multiImageIcon = unknown_cast<MultiImage> (icon)) 	
					icon = multiImageIcon->getFrame (multiImageIcon->getCurrentFrame ()); // current frame as icon
			}
			
			iconSize (icon->getWidth (), icon->getHeight ());
			iconSrc.setSize (iconSize);
						
			if(icon->getType () == IImage::kScalable)
			{
				float iconResize = iconFillSize * ccl_min (button->getHeight (), button->getWidth ());
				float resizeRatio = (iconResize > 0.f) ? (iconResize / ccl_max (iconSize.x, iconSize.y)) : 1;
				iconSize *= resizeRatio;
			}
			
			iconPos = rect.getLeftTop () + ((rect.getSize () - iconSize) * .5f);
		}
		
		if(button->getTitle ().isEmpty ())
		{
			if(leadingIcon)
				iconPos.x = rect.left;
			else if(trailingIcon)
				iconPos.x = rect.right - iconSize.x - iconSpacing;
		}
		else
		{
			int alignH = visualStyle->getTextAlignment ().getAlignH ();
			Font font (visualStyle->getTextFont ().zoom (button->getZoomFactor ()));

			if(icon)
			{
				if(leadingIcon)
				{
					iconPos.x = rect.left;
					rect.left += iconSize.x + iconSpacing;
				}
				else if(trailingIcon)
				{
					iconPos.x = rect.right - iconSize.x - iconSpacing;
					rect.right = iconPos.x;
				}
				else
				{
					if(alignH == Alignment::kLeft)
					{
						iconPos.x = rect.left;
						rect.left += iconSize.x + iconSpacing;
					}
					else
					{
						Rect stringSize;
						port.measureString (stringSize, button->getTitle (), font);
						if(alignH == Alignment::kHCenter)
						{
							Coord margin = (rect.getWidth () - stringSize.getWidth () - iconSpacing - iconSize.x) / 2;
							
							iconPos.x = rect.left + margin;
							
							rect.right -= margin;
							rect.left = rect.right - stringSize.getWidth ();
						}
						else // kRight
							iconPos.x = rect.right - stringSize.getWidth () - iconSize.x - iconSpacing;
					}
				}
			}
			
			// setup text color
			Color textBrushColor;
			if(button->isEnabled ())
				textBrushColor = getTextColor (colorParam, button->isOn (), buttonIsPressed, buttonMouseOver);
			else if(button->getPhase () != 0.)
				textBrushColor = textColorPhaseOn;
			else
				textBrushColor = button->isOn () ? textColorDisabledOn : textColorDisabled;

			SolidBrush textBrush (textBrushColor);
			String title = button->getTitle ();

			if(style.isCustomStyle (Styles::kButtonAppearanceMultiLine))
			{
				AlignmentRef alignment (visualStyle->getTextAlignment ()); // Alignment::kCenter
				port.drawText (rect, title, font, textBrush, TextFormat (alignment, TextFormat::kWordBreak));
			}
			else
			{
				Coord maxTitleWidth = visualStyle->getMetric<Coord> ("text.width", 0);
				if(maxTitleWidth > 0)
				{
					// LATER TODO: replace by Font::kTrimModeTruncate!
					Rect stringSize;
					port.measureString (stringSize, title, font);
					
					const int minCharacters = 2; // display at least 2 characters
					while(title.length () > minCharacters && stringSize.getWidth () > maxTitleWidth)
					{
						title = title.subString(0, title.length () - 1 );
						port.measureString (stringSize, title, font);
					}
				}
				
				if(style.isCustomStyle (Styles::kButtonAppearanceScaleText))
				{
					Font scaledFont = visualStyle->getTextFont ();
					textScaler.scaleTextFont (scaledFont, rect, title);
		
					port.drawString (rect, title, scaledFont, textBrush, alignH);
				}
				else
					port.drawString (rect, title, font, textBrush, alignH);
			}
		}
		
		if(icon)
		{
			auto getModifiedIcon = [&](IImage* icon)
			{
				if(button->isEnabled () == false)
				{
					if(iconFrameIndex[ThemeElements::kDisabled] == iconFrameIndex[ThemeElements::kNormal]) // when the icon has no own 'disabled' frame
						if(iconColorDisabled != Colors::kTransparentBlack)  // when the iconColorDisabled is set
							return ModifiedImageCache::instance ().lookup (icon, iconColorDisabled);   // create a modified icon using the disabled icon color
				}
				else if(useModifiedIcon)
				{
					ColorRef iconColor = getIconColor (colorParam, button->isOn (), buttonIsPressed, buttonMouseOver);
					return ModifiedImageCache::instance ().lookup (icon, iconColor, drawAsTemplate);   // create a modified icon using iconcolor
				}
				return icon;
			};
			
			Rect iconDst (iconPos.x, iconPos.y, iconSize);
			port.drawImage (getModifiedIcon (icon), iconSrc, iconDst);
			
			// draw animated phase overlay
			if(phaseIconFrameIndex != -1 && button->getPhase () > 0.f)
			{
				icon->setCurrentFrame (phaseIconFrameIndex);
				ImageMode mode (button->getPhase ());
				port.drawImage (getModifiedIcon (icon), iconSrc, iconDst, &mode);
			}
		}
		
		if(style.isVertical ())
			port.restoreState ();
	}

	view->View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonRenderer::updateAnimationState (const Button* button)
{
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

ColorRef ButtonRenderer::getTextColor (IColorParam* colorParam, bool isOn, bool pressed, bool mouseover) const
{
	// textColor is dependent on colorParam when this parameter is used
	if(colorParam != nullptr)
	{
		colorParamColor = Colors::kTransparentBlack;
		colorParam->getColor (colorParamColor);
		
		if(colorParamColor.getAlphaF () == 0.f)
		{
			colorParamColor = isOn ? textContrastTransparentColorOn : textContrastTransparentColor;
		}
		else if(textContrastBrightColor != Colors::kTransparentBlack)
		{
			if(colorParamColor.getLuminance () < brightColorThreshold)
				colorParamColor = isOn ? textContrastBrightColorOn : textContrastBrightColor;
			else
				colorParamColor = isOn ? textContrastDarkColorOn : textContrastDarkColor;
		}
		else if(textColorAlphaBlend.getAlphaF () != 0)
		{
			colorParamColor.alphaBlend (textColorAlphaBlend, textColorAlphaBlend.getAlphaF ());
		}
		
		return colorParamColor;
	}

	if(isOn)
		return pressed ? textColorPressedOn : mouseover ? textColorMouseOverOn : textColorOn;
	else
		return pressed ? textColorPressed : mouseover ? textColorMouseOver : textColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef ButtonRenderer::getIconColor (IColorParam* colorParam, bool isOn, bool pressed, bool mouseover) const
{
	// iconColor is dependent on colorParam when this parameter is used
	if(colorParam != nullptr)
	{
		colorParamColor = Colors::kTransparentBlack;
		colorParam->getColor (colorParamColor);
		
		if(colorParamColor.getAlphaF () == 0.f)
		{
			colorParamColor = isOn ? iconContrastTransparentColorOn : iconContrastTransparentColor;
		}
		else if(iconContrastBrightColor != Colors::kTransparentBlack)
		{
			if(colorParamColor.getLuminance () < brightColorThreshold)
				colorParamColor = isOn ? iconContrastBrightColorOn : iconContrastBrightColor;
			else
				colorParamColor = isOn ? iconContrastDarkColorOn : iconContrastDarkColor;
		}
		else if(iconColorAlphaBlend.getAlphaF () != 0)
		{
			colorParamColor.alphaBlend (iconColorAlphaBlend, iconColorAlphaBlend.getAlphaF ());
		}
		
		return colorParamColor;
	}
	
	if(isOn)
		return pressed ? iconPressedColorOn : mouseover ? iconMouseoverColorOn : iconColorOn;
	else
		return pressed ? iconPressedColor : mouseover ? iconMouseoverColor : iconColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ButtonRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	return false;
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

MultiToggleRenderer::MultiToggleRenderer (VisualStyle* visualStyle)
: ButtonRenderer (visualStyle)
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

BEGIN_VISUALSTYLE_CLASS (CheckBox, Button, "CheckBoxStyle")
	ADD_VISUALSTYLE_METRIC ("buttonstyle")			///< if this is set ("1"), the control is drawn like a button
	ADD_VISUALSTYLE_METRIC ("checkboxrightside")	///< if this is set ("1"), the checkbox is drawn algined to the right side of the view rect.
	ADD_VISUALSTYLE_IMAGE ("mixedicon")				///< fallback when no mixedicon at the checkbox is defined
END_VISUALSTYLE_CLASS (CheckBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

CheckBoxRenderer::CheckBoxRenderer (VisualStyle* visualStyle)
: ButtonRenderer (visualStyle),
  useButtonStyle (false),
  checkBoxRightSide (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBoxRenderer::draw (View* view, const UpdateRgn& updateRgn)
{	
	StyleRef style = view->getStyle ();
	if(initialized == false)
		initialize (style, view);

	if(useButtonStyle)
	{
		ButtonRenderer::draw (view, updateRgn);
		return;
	}
	
	CheckBox* checkBox = static_cast<CheckBox*> (view);

	GraphicsPort port (checkBox);
	Rect rect;
	checkBox->getClientRect (rect);

	Theme& theme = checkBox->getTheme ();
	IThemePainter& painter = theme.getPainter ();

	// *** draw checkmark ***
	int titleOffset = 0;
	
	if(image)
	{
		Rect checkSrc (0, 0, image->getWidth (), image->getHeight ());
		Rect checkDst (checkSrc);
		checkDst.centerV (rect);
		
		if(checkBoxRightSide)
			checkDst.offset (rect.getWidth () - image->getWidth (), 0);

		int frame = checkBox->getCurrentFrame ();
		image->setCurrentFrame (frameIndex [frame]);

		port.drawImage (image, checkSrc, checkDst);
		
		IImage* icon = checkBox->getIcon ();
		IImage* mixedIcon = checkBox->getMixedIcon ();
		if(icon && checkBox->isOn ())
		{
			if(icon->getFrameCount ())
				icon->setCurrentFrame (iconFrameIndex.at (frame));
			
			Point iconSize (icon->getWidth (), icon->getHeight ());
			Point iconPos (checkDst.getLeftTop () + (checkDst.getSize () - iconSize) * .5f);

			if(IImage* modifiedIcon = ModifiedImageCache::instance ().lookup (icon, checkBox->isEnabled () ? iconColor : iconColorDisabled))
				port.drawImage (modifiedIcon, iconPos);
		}
		else if(mixedIcon && checkBox->isMixed ())
		{
			if(mixedIcon->getFrameCount ())
				mixedIcon->setCurrentFrame (mixedIconFrameIndex.at (frame));

			Point iconSize (mixedIcon->getWidth (), mixedIcon->getHeight ());
			Point iconPos (checkDst.getLeftTop () + (checkDst.getSize () - iconSize) * .5f);

			if(IImage* modifiedIcon = ModifiedImageCache::instance ().lookup (mixedIcon, checkBox->isEnabled () ? iconColor : iconColorDisabled))
				port.drawImage (modifiedIcon, iconPos);
		}
		
		titleOffset = image->getWidth () + 2 + padding.left;
	}
	else
	{
		int checkH = theme.getThemeMetric (ThemeElements::kCheckBoxSize);
		if(checkBox->getHeight () < checkH)
			checkH = checkBox->getHeight ();

		int state = checkBox->getThemeElementState ();

		Rect checkRect (0, 0, checkH, checkH);
		
		if(checkBoxRightSide)
			checkRect.offset (rect.getWidth () - checkH, 0);
		
		checkRect.centerV (rect);
		painter.drawElement (port, checkRect, getThemeId (checkBox), state);	

		titleOffset = checkH + 2 + padding.left;
	}
	
	
	if(checkBox->isFocused () && !style.isCustomStyle (Styles::kButtonAppearanceHideFocus))
	{
		if(image && image->getFrameIndex (ThemeNames::kFocused) >= 0) // draw focus overlay if available
		{
			image->setCurrentFrame (frameIndex [ThemeElements::kFocused]);
			
			Rect checkSrc (0, 0, image->getWidth (), image->getHeight ());
			Rect checkDst (checkSrc);
			checkDst.centerV (rect);
			
			port.drawImage (image, checkSrc, checkDst);
		}
	}
	
	// *** draw title ***
	String title = view->getTitle ();
	if(!title.isEmpty ())
	{
		Font font (visualStyle->getTextFont ().zoom (checkBox->getZoomFactor ()));
		AlignmentRef alignment (visualStyle->getTextAlignment ());
		
		Rect titleRect (rect);
		if(checkBoxRightSide)
			titleRect.right -= titleOffset;
		else
			titleRect.left += titleOffset;
		
		port.drawString (titleRect, title, font, SolidBrush (checkBox->isEnabled () ? textColor : textColorDisabled), alignment);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBoxRenderer::initialize (StyleRef style, View* view)
{
	useButtonStyle = visualStyle->getMetric<bool> ("buttonstyle", false);
	checkBoxRightSide = visualStyle->getMetric<bool> ("checkboxrightside", false);

	CheckBox* checkBox = static_cast<CheckBox*> (view);
	IImage* mixedIcon = checkBox->getMixedIcon ();

	if(mixedIcon == nullptr)
		if(mixedIcon = visualStyle->getImage ("iconmixed"))
			checkBox->setMixedIcon (mixedIcon);

	if(mixedIcon)
	{
		ASSERT (mixedIcon->getFrameCount ()) // resources are missing!

		int numFrames = ccl_max (10, checkBox->getNumFrames ());
		mixedIconFrameIndex.resize (numFrames);
		mixedIconFrameIndex.setCount (numFrames);

		initFrames (mixedIconFrameIndex, *mixedIcon);
	}

	ButtonRenderer::initialize (style, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeElementID CheckBoxRenderer::getThemeId (CheckBox* checkBox) const
{
	bool checked = checkBox->getParameter ()->getValue () == checkBox->getParameter ()->getMax ();
	return checked ? ThemeElements::kCheckBoxChecked : ThemeElements::kCheckBoxNormal;
}

//************************************************************************************************
// RadioButtonRenderer
/** A Radio button is drawn like a CheckBox. */
//************************************************************************************************

RadioButtonRenderer::RadioButtonRenderer (VisualStyle* visualStyle)
: CheckBoxRenderer (visualStyle)
{}
