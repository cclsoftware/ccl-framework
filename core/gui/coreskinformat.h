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
// Filename    : core/gui/coreskinformat.h
// Description : Core Skin Format Definitions
//
//************************************************************************************************

#ifndef _coreskinformat_h
#define _coreskinformat_h

#include "core/public/coreenumdef.h"
#include "core/public/gui/corerect.h"

namespace Core {
namespace Skin {

namespace FileNames
{
	const CStringPtr kFontFile1 = "fonts.json";
	const CStringPtr kFontFile2 = "fonts.ubj";
	const CStringPtr kStyleFile1 = "styles.json";
	const CStringPtr kStyleFile2 = "styles.ubj";
	const CStringPtr kBitmapFile1 = "bitmaps.json";
	const CStringPtr kBitmapFile2 = "bitmaps.ubj";
	const CStringPtr kViewFile1 = "views.json";
	const CStringPtr kViewFile2 = "views.ubj";

	// naming conventions for subfolders in skin package
	const CStringPtr kViewsFolder = "views";
	const CStringPtr kBitmapsFolder = "bitmaps";
	const CStringPtr kFontsFolder = "fonts";
}

namespace ResourceAttributes
{
	const CStringPtr kName = "name";
	const CStringPtr kFile = "file";
	const CStringPtr kMonochrome = "monochrome";
	const CStringPtr kFrames = "frames";
	const CStringPtr kSize = "size";
	const CStringPtr kWidth = "width";
	const CStringPtr kHeight = "height";
	const CStringPtr kAlwaysCached = "alwayscached";
	const CStringPtr kComment = "comment"; // reserved for developers and tool support
	const CStringPtr kFontNumber = "fontnumber";
	const CStringPtr kDefault = "default"; // used as default font
	const CStringPtr kFontFace = "fontface"; // reserved for use with TrueType fonts

	bool parseSize (Rect& size, CStringPtr sizeString);
}

namespace ViewClasses
{
	const CStringPtr kView = "View";
	const CStringPtr kContainerView = "ContainerView";
	const CStringPtr kRootView = "RootView";
	const CStringPtr kDelegate = "Delegate";

	const CStringPtr kLabel = "Label";
	const CStringPtr kMultiLineLabel = "MultiLineLabel";
	const CStringPtr kImageView = "ImageView";
	const CStringPtr kVariantView = "VariantView";
	const CStringPtr kAlignView = "AlignView";
	const CStringPtr kControl = "Control";
	const CStringPtr kButton = "Button";
	const CStringPtr kToggle = "Toggle";
	const CStringPtr kRadioButton = "RadioButton";
	const CStringPtr kValueBar = "ValueBar";
	const CStringPtr kSlider = "Slider";
	const CStringPtr kTextBox = "TextBox";
	const CStringPtr kEditBox = "EditBox";
	const CStringPtr kSelectBox = "SelectBox";

	const CStringPtr kListView = "ListView";
}

namespace ViewAttributes
{
	const CStringPtr kType = "type";
	const CStringPtr kName = "name";
	const CStringPtr kChildren = "children";
	const CStringPtr kDefines = "def";
	const CStringPtr kController = "controller";
	const CStringPtr kViewName = "viewname"; // used with ViewClasses::kDelegate

	const CStringPtr kInherit = "inherit";
	const CStringPtr kOptions = "options";
	const CStringPtr kStyle = "style";
	const CStringPtr kBackColor = "backcolor";
	const CStringPtr kBackColorDisabled = "backcolor.disabled";
	const CStringPtr kForeColor = "forecolor";
	const CStringPtr kForeColorDisabled = "forecolor.disabled";
	const CStringPtr kTextColor = "textcolor";
	const CStringPtr kTextColorOn = "textcolor.on";
	const CStringPtr kTextColorDisabled = "textcolor.disabled";
	const CStringPtr kHiliteColor = "hilitecolor";
	const CStringPtr kFont = "font";
	const CStringPtr kTextAlign = "textalign";

	using ResourceAttributes::kSize;
	using ResourceAttributes::kWidth;
	using ResourceAttributes::kHeight;

	const CStringPtr kTitle = "title";
	const CStringPtr kImage = "image";
	const CStringPtr kIcon = "icon";
	const CStringPtr kBackground = "background";
	const CStringPtr kTextTrimMode = "texttrim";
	const CStringPtr kRadioValue = "radiovalue";
	const CStringPtr kKeyboardLayout = "keyboardlayout";
	const CStringPtr kKeyboardCapitalization = "capitalization";

	const CStringPtr kRowHeight = "rowheight";
	const CStringPtr kSelectColor = "selectcolor";
	const CStringPtr kSeparatorColor = "separatorcolor";
	const CStringPtr kItemInset = "iteminset";
	const CStringPtr kScrollerSize = "scrollersize";
	const CStringPtr kContentSize = "contentsize";
	const CStringPtr kFocusBorder = "focusborder";
}

enum ViewOptions
{
	kViewBehaviorDisabled = 1<<0,
	kLastViewOption = 15 ///< reserve room for private view options
};

enum ControlStyles
{
	// ValueBar
	kValueBarAppearanceVertical = 1<<(kLastViewOption+1),
	kValueBarAppearanceCentered = 1<<(kLastViewOption+2),
	kValueBarAppearanceFilmstrip = 1<<(kLastViewOption+3),

	// ImageView
	kImageViewAppearanceColorize = 1<<(kLastViewOption+1),

	// Button
	kButtonBehaviorWantsFocus = 1<<(kLastViewOption+1),
	kButtonBehaviorDeferred = 1<<(kLastViewOption+2), ///< parameter is changed on mouse/touch up instead of down
	kButtonAppearanceTransparent = 1<<(kLastViewOption+3),
	kButtonBehaviorSilentTracking = 1<<(kLastViewOption+4),

	// Label
	kLabelAppearanceColorize = 1<<(kLastViewOption+1),

	// TextBox
	kTextBoxAppearanceMultiLine = 1<<(kLastViewOption+1),
	kTextBoxAppearanceHideText = 1<<(kLastViewOption+2),

	// ListView
	kListViewBehaviorWheelSelection = 1<<(kLastViewOption+1), ///< wheel changes selection
	kListViewBehaviorDeselectAllowed = 1<<(kLastViewOption+2) ///< allow deselection on click/touch
};

namespace KeyboardLayout
{
	enum Mode
	{
		kLetters,
		kNumbers,
		kSymbols
	};
}

namespace KeyboardCapitalization
{
	enum Mode
	{
		kNone = 0,
		kFirst,
		kWords
	};
}

enum TextTrimMode
{
	kTextTrimNone = 0,
	kTextTrimMiddle,	///< 'middle': put ".." in the middle, strip spaces
	kTextTrimRight		///< 'right': put ".." to the right, strip spaces
};

enum BorderStyles
{
	kBorderNone = 0,
	kBorderLeftEdge = 1<<1,
	kBorderRightEdge = 1<<2,
	kBorderTopEdge = 1<<3,
	kBorderBottomEdge = 1<<4,
	kBorderAllEdges = kBorderLeftEdge|kBorderRightEdge|kBorderTopEdge|kBorderBottomEdge
};

enum ListViewDefaults
{
	kListViewDefaultRowHeight = 18,
	kListViewDefaultItemInset = 2,
	kListViewDefaultScrollerSize = 12
};

struct Enumerations
{
	DECLARE_ENUMINFO (alignment)
	DECLARE_ENUMINFO (viewOptions)
	DECLARE_ENUMINFO (labelOptions)
	DECLARE_ENUMINFO (imageViewOptions)
	DECLARE_ENUMINFO (buttonOptions)
	DECLARE_ENUMINFO (valueBarOptions)
	DECLARE_ENUMINFO (textTrimModes)
	DECLARE_ENUMINFO (textBoxOptions)
	DECLARE_ENUMINFO (keyboardLayouts)
	DECLARE_ENUMINFO (keyboardCapitalizationModes)
	DECLARE_ENUMINFO (border)
	DECLARE_ENUMINFO (listViewOptions)
};

} // namespace Skin
} // namespace Core

#endif // _coreskinformat_h
