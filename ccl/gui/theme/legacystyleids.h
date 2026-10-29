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
// Filename    : ccl/gui/theme/legacystyleids.h
// Description : Legacy Style ID Constants
//
//************************************************************************************************

#ifndef _ccl_legacystyleids_h
#define _ccl_legacystyleids_h

#include "ccl/public/base/platform.h"

#ifdef CCLGUI_LEGACY_STYLES_SUPPORTED // set via CMake
	#define LEGACY_STYLES_SUPPORTED CCLGUI_LEGACY_STYLES_SUPPORTED
#else
	#define LEGACY_STYLES_SUPPORTED 0
#endif

#if LEGACY_STYLES_SUPPORTED
	#define BEGIN_LEGACY_STYLES_ENABLED_BLOCK	if(LegacyStyleID::enabled == true) {
	#define END_LEGACY_STYLES_ENABLED_BLOCK		}

	#define BEGIN_LEGACY_STYLES_DISABLED_BLOCK	if(LegacyStyleID::enabled == false) {
	#define END_LEGACY_STYLES_DISABLED_BLOCK	}
#else
	#define BEGIN_LEGACY_STYLES_DISABLED_BLOCK	if(1) {
	#define END_LEGACY_STYLES_DISABLED_BLOCK	}
#endif

namespace CCL {

#if LEGACY_STYLES_SUPPORTED
namespace LegacyStyleID
{
	extern bool enabled; ///< check if legacy styles are enabled at runtime

	// Text colors
	// The BIV renderers (Button, SelectBox, ...) read these into the "value" atom, the TextBox
	// renderers into the "text" atom - the replacement is the property of that atom.
	const CStringPtr kTextColorOn = "textcolor.on";							// replaced by the "on" style condition
	const CStringPtr kTextColorTransparent = "textcolor.transparent";		// replaced by the atom's "contrastcolortransparent"
	const CStringPtr kTextColorBright = "textcolor.bright";					// replaced by the atom's "contrastcolorbright"
	const CStringPtr kTextColorDark = "textcolor.dark";						// replaced by the atom's "contrastcolordark"
	const CStringPtr kTextColorTransparentOn = "textcolor.transparentOn";	// replaced by the atom's "contrastcolortransparent" in the "on" style condition
	const CStringPtr kTextColorBrightOn = "textcolor.brightOn";				// replaced by the atom's "contrastcolorbright" in the "on" style condition
	const CStringPtr kTextColorDarkOn = "textcolor.darkOn";					// replaced by the atom's "contrastcolordark" in the "on" style condition
	const CStringPtr kTextColorAlphaBlend = "textcolor.alphablend";			// replaced by the atom's "contrastcoloralphablend"
	const CStringPtr kTextColorThreshold = "textcolor.threshold";			// not replaced yet - the bright color threshold of the color contrast helper has no atom property
	const CStringPtr kTextColorPlaceholder = "textcolor.placeholder";		// replaced by the color of the "placeholder" atom
	const CStringPtr kTextColorPressed = "textcolor.pressed";				// replaced by the "pressed" style condition
	const CStringPtr kTextColorPressedOn = "textcolor.pressedOn";			// replaced by the "pressed on" style condition
	const CStringPtr kTextColorMouseover = "textcolor.mouseover";			// replaced by the "mouseover" style condition
	const CStringPtr kTextColorMouseoverOn = "textcolor.mouseoverOn";		// replaced by the "mouseover on" style condition
	const CStringPtr kTextColorDisabled = "textcolor.disabled";				// replaced by the "disabled" style condition
	const CStringPtr kTextColorDisabledOn = "textcolor.disabledOn";			// replaced by the "disabled on" style condition

	// Icons
	const CStringPtr kIcon = "icon";										// replaced by the image of the "icon" atom
	const CStringPtr kIconMixed = "iconmixed";								// replaced by the "iconmixed" attribute of the CheckBox element - the mixed icon has no atom
	const CStringPtr kIconColor = "iconcolor";								// replaced by the color of the "icon" atom
	const CStringPtr kIconColorOn = "iconcolor.on";							// replaced by the "on" style condition
	const CStringPtr kIconColorPressed = "iconcolor.pressed";				// replaced by the "pressed" style condition
	const CStringPtr kIconColorPressedOn = "iconcolor.pressedOn";			// replaced by the "pressed on" style condition
	const CStringPtr kIconColorMouseover = "iconcolor.mouseover";			// replaced by the "mouseover" style condition
	const CStringPtr kIconColorMouseoverOn = "iconcolor.mouseoverOn";		// replaced by the "mouseover on" style condition
	const CStringPtr kIconColorDisabled = "iconcolor.disabled";				// replaced by the "disabled" style condition
	const CStringPtr kIconColorDisabledOn = "iconcolor.disabledOn";			// replaced by the "disabled on" style condition
	const CStringPtr kIconColorTransparent = "iconcolor.transparent";		// replaced by the "contrastcolortransparent" of the "icon" atom
	const CStringPtr kIconColorBright = "iconcolor.bright";					// replaced by the "contrastcolorbright" of the "icon" atom
	const CStringPtr kIconColorDark = "iconcolor.dark";						// replaced by the "contrastcolordark" of the "icon" atom
	const CStringPtr kIconColorTransparentOn = "iconcolor.transparentOn";	// replaced by the "contrastcolortransparent" of the "icon" atom in the "on" style condition
	const CStringPtr kIconColorBrightOn = "iconcolor.brightOn";				// replaced by the "contrastcolorbright" of the "icon" atom in the "on" style condition
	const CStringPtr kIconColorDarkOn = "iconcolor.darkOn";					// replaced by the "contrastcolordark" of the "icon" atom in the "on" style condition
	const CStringPtr kIconColorAlphaBlend = "iconcolor.alphablend";			// replaced by the "contrastcoloralphablend" of the "icon" atom

	const CStringPtr kIconFillFactor = "fill.icon";							// replaced by StyleID::BIV::kIconFillFactor
	const CStringPtr kSpacingIcon = "spacing.icon";							// replaced by StyleID::BIV::kIconValueSpacing

	// Native control padding
	// Not replaced yet - the inset of the native text control inside an EditBox has no atom property.
	const CStringPtr kNativePaddingLeft = "native.padding.left";
	const CStringPtr kNativePaddingTop = "native.padding.top";
	const CStringPtr kNativePaddingRight = "native.padding.right";
	const CStringPtr kNativePaddingBottom = "native.padding.bottom";
	const CStringPtr kNativePadding = "native.padding";

	// Text
	const CStringPtr kScaleTextMaxFont = "scaletext.maxfont";				// not replaced yet - the maximum font size of an atom has no style property
	const CStringPtr kScaleTextMinFont = "scaletext.minfont";				// not replaced yet - the minimum font size of an atom has no style property
	const CStringPtr kTextFontPlaceholder = "textfont.placeholder";			// not replaced yet - the font of an atom has no style property

	// Colors
	const CStringPtr kTransparentColor = "transparentcolor";				// replaced by the "contrastcolortransparent" of the "text" atom
	const CStringPtr kColorAlphaBlend = "color.alphablend";					// replaced by the "contrastcoloralphablend" of the affected atom
	const CStringPtr kBorderColor = "bordercolor";							// replaced by the "bordercolor" of the "background" atom
	const CStringPtr kSelectionColor = "selectioncolor";					// replaced by the color of the "selection" atom
	const CStringPtr kBackcolorOff = "backcolor.off";						// replaced by the color of the "background" atom
	const CStringPtr kBackcolorOn = "backcolor.on";							// replaced by the color of the "background" atom in the "on" style condition
	const CStringPtr kBackcolorTransparent = "backcolor.transparent";		// replaced by the "contrastcolortransparent" of the "background" atom

	// Images
	// The BIV renderers select them by the "left", "middle" and "right" options, the TabView renderer
	// by the position of a tab - the replacement is the image of the "background" atom of a Button or
	// of the "tab.background" atom.
	const CStringPtr kSingleImage = "single";								// replaced by the "left right" style condition - a single tab has no position condition
	const CStringPtr kLeftImage = "left";									// replaced by the "left" style condition
	const CStringPtr kMiddleImage = "middle";								// replaced by the "middle" style condition
	const CStringPtr kRightImage = "right";									// replaced by the "right" style condition
	const CStringPtr kFirstImage = "first";									// replaced by the "left" style condition
	const CStringPtr kLastImage = "last";									// replaced by the "right" style condition

	const CStringPtr kColorizeImage = "colorize.image";						// replaced by the "template" attribute of the image resource
	const CStringPtr kLightAdaptImage = "lightadapt.image";					// replaced by the "adaptive" attribute of the image resource
	const CStringPtr kColorizeIcon = "colorize.icon";						// replaced by the "template" attribute of the icon image resource

	// Names used by more than one renderer, with a different meaning each
	const CStringPtr kContextColor = "contextcolor";						// replaced by the color of the atom that draws the affected image
	const CStringPtr kImageFillFactor = "fill.image";						// SelectBox: sizes a value image from the parameter - replaced by the "padding" of the value
																			// ImageView: sizes the image within the view - replaced by the "scale" of the "background" atom

	// SelectBox
	const CStringPtr kSelectBoxButton = "button";							// replaced by the image of the "icon" atom
	const CStringPtr kSelectBoxStates = "states";							// replaced by the image of the "background" atom
	const CStringPtr kSelectBoxContextColorOn = "contextcolor.on";			// replaced by the "on" style condition
	const CStringPtr kSelectBoxStateColorFormat = "state%d";				// replaced by style conditions for the visual state
	const CStringPtr kSelectBoxButtonFillFactor = "fill.button";			// replaced by StyleID::BIV::kIconFillFactor
	const CStringPtr kSelectBoxButtonLeading = "button.leading";			// replaced by a left horizontal StyleID::BIV::kIconAlignment
	const CStringPtr kSelectBoxButtonTrailing = "button.trailing";			// replaced by a right horizontal StyleID::BIV::kIconAlignment

	// ImageView
	const CStringPtr kImageColor = "imagecolor";							// replaced by the color of the "background" atom
	const CStringPtr kImageColorOn = "imagecolor.on";						// replaced by the "on" style condition
	const CStringPtr kImageColorContext = "imagecolor.context";				// replaced by the color of the "background" atom
	const CStringPtr kImageColorTransparent = "imagecolor.transparent";		// replaced by the "contrastcolortransparent" of the "background" atom
	const CStringPtr kImageColorBright = "imagecolor.bright";				// replaced by the "contrastcolorbright" of the "background" atom
	const CStringPtr kImageColorAlphaBlend = "imagecolor.alphablend";		// replaced by the "contrastcoloralphablend" of the "background" atom
	const CStringPtr kImageColorThreshold = "imagecolor.threshold";			// not replaced yet - the bright color threshold of the color contrast helper has no atom property

	// TabView
	const CStringPtr kTabViewHeaderBackground = "headerBackground";			// replaced by the image of the "header" atom
	const CStringPtr kTabViewMenuBackground = "menuBackground";				// replaced by the image of the "tab.background" atom in the "menu" style condition
	const CStringPtr kTabViewMenuIcon = "menuIcon";							// replaced by the image of the "tab.icon" atom in the "menu" style condition
	const CStringPtr kTabViewTabMenuIcon = "tabMenuIcon";					// replaced by the image of the "tab.menuicon" atom
	const CStringPtr kTabViewActiveTextColor = "activetextcolor";			// replaced by the color of the "tab.value" atom in the "on" style condition

	// Other
	const CStringPtr kBackgroundRadius = "backcolor.radius";				// replaced by the border radius properties of the "background" atom
	const CStringPtr kPhaseOn = "phaseOn";									// image frame name - the atom equivalent is an animation of the "opacity" and "imageframe" properties
	const CStringPtr kCheckBoxRightSide = "checkboxrightside";				// replaced by StyleID::BIV::kIconLeading
	const CStringPtr kAnimationFilmstrip = "animation.filmstrip";			// not replaced exactly - an animation filmstrip could be used to show intermediate button states - the visualstyle is responisble to define the appropriate animiation triggers for the phase property

	// ID sets
	// The color of an atom in the element states, each with a variant for the "on" state.
	struct StateColorIDs
	{
		CStringPtr normal = nullptr;
		CStringPtr on = nullptr;
		CStringPtr pressed = nullptr;
		CStringPtr pressedOn = nullptr;
		CStringPtr mouseover = nullptr;
		CStringPtr mouseoverOn = nullptr;
		CStringPtr disabled = nullptr;
		CStringPtr disabledOn = nullptr;
	};

	// The contrast colors of an atom. The alpha blend color falls back to kColorAlphaBlend.
	struct ContrastColorIDs
	{
		CStringPtr bright = nullptr;
		CStringPtr dark = nullptr;
		CStringPtr transparent = nullptr;
		CStringPtr transparentFallback = nullptr; ///< optional
		CStringPtr alphaBlend = nullptr;
	};

} // namespace LegacyStyleID
#endif // LEGACY_STYLES_SUPPORTED

} // namespace CCL

#endif // _ccl_legacystyleids_h
