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
// Filename    : core/gui/coreskinformat.impl.h
// Description : Core Skin Format Definitions
//
//************************************************************************************************

#include "core/gui/coreskinformat.h"

#include "core/text/coretexthelper.h"

namespace Core {
namespace Skin {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ResourceAttributes
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ResourceAttributes::parseSize (Rect& rect, CStringPtr sizeString)
{
	if(!sizeString)
		return false;

	static ConstString skipChars (", ");

	Text::StringParser p (sizeString);
	if(!p.parseInt (rect.left))
		return false;

	p.skip (skipChars);
	if(p.parseInt (rect.top))
	{
		p.skip (skipChars);
		if(p.parseInt (rect.right)) // width
		{
			rect.right += rect.left;

			p.skip (skipChars);
			p.parseInt (rect.bottom); // height
			rect.bottom += rect.top;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Enumerations
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_ENUMINFO (Enumerations::alignment)
	{"hcenter",	Alignment::kHCenter},
	{"left", Alignment::kLeft},
	{"right", Alignment::kRight},
	{"vcenter", Alignment::kVCenter},
	{"top", Alignment::kTop},
	{"bottom", Alignment::kBottom},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::viewOptions)
	{"disabled", kViewBehaviorDisabled},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::labelOptions)
	{"colorize", kLabelAppearanceColorize},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::imageViewOptions)
	{"colorize", kImageViewAppearanceColorize},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::buttonOptions)
	{"wantfocus", kButtonBehaviorWantsFocus},
	{"deferred", kButtonBehaviorDeferred},
	{"transparent", kButtonAppearanceTransparent},
	{"silenttracking", kButtonBehaviorSilentTracking},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::valueBarOptions)
	{"vertical", kValueBarAppearanceVertical},
	{"centered", kValueBarAppearanceCentered},
	{"filmstrip", kValueBarAppearanceFilmstrip},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::textTrimModes)
	{"none", kTextTrimNone},
	{"middle", kTextTrimMiddle},
	{"right", kTextTrimRight},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::textBoxOptions)
	{"multiline", kTextBoxAppearanceMultiLine},
	{"hidetext", kTextBoxAppearanceHideText},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::keyboardLayouts)
	{"letters", KeyboardLayout::kLetters},
	{"numbers", KeyboardLayout::kNumbers},
	{"symbols", KeyboardLayout::kSymbols},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::keyboardCapitalizationModes)
	{"none", KeyboardCapitalization::kNone},
	{"first", KeyboardCapitalization::kFirst},
	{"words", KeyboardCapitalization::kWords},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::border)
	{"none", kBorderNone},
	{"left", kBorderLeftEdge},
	{"right", kBorderRightEdge},
	{"top", kBorderTopEdge},
	{"bottom", kBorderBottomEdge},
	{"all",	kBorderAllEdges},
END_ENUMINFO

BEGIN_ENUMINFO (Enumerations::listViewOptions)
	{"wheelselection", kListViewBehaviorWheelSelection},
	{"deselect", kListViewBehaviorDeselectAllowed},
END_ENUMINFO

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Skin
} // namespace Core
