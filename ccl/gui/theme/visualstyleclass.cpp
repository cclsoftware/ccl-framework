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
// Filename    : ccl/gui/theme/visualstyleclass.cpp
// Description : VisualStyle MetaClass
//
//************************************************************************************************

#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/gui/theme/theme.h"
#include "ccl/gui/graphics/graphicshelper.h"

#include "ccl/base/singleton.h"

#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace VisualStyleClass;

//************************************************************************************************
// VisualStyleClass::Library
//************************************************************************************************

DEFINE_SINGLETON (VisualStyleClass::Library)

//////////////////////////////////////////////////////////////////////////////////////////////////

Library::Library ()
: TypeLibrary (CCL_STYLES_TYPELIB_NAME),
  metrics ("ThemeMetrics", Theme::metricNames, ThemeElements::kNumMetrics),
  colors ("ThemeColors", Theme::colorNames, ThemeElements::kNumColors),
  cursors ("ThemeCursors", Theme::cursorNames, ThemeElements::kNumCursors),
  fonts ("ThemeFonts", Theme::fontNames, ThemeElements::kNumFonts),
  standardStyles ("StandardStyles", ThemePainter::standardStyleNames, ThemePainter::kNumStandardStyles),
  elementImages ("ThemeElementImages", ThemePainter::uniqueImageNames, ThemePainter::kUniqueImageCount),
  elementStates ("ThemeElementStates", ThemePainter::stateNames, ThemeElements::kNumElementStates),
  elementStatesOn ("ThemeElementStatesOn", ThemePainter::stateNamesOn, ThemeElements::kNumElementStates)
{
	addEnum (&metrics);
	addEnum (&colors);
	addEnum (&cursors);
	addEnum (&fonts);
	addEnum (&standardStyles);
	addEnum (&elementImages);
	addEnum (&elementStates);
	addEnum (&elementStatesOn);
	addEnum (&GraphicsHelper::instance ().getDefaultColors ());
}

//************************************************************************************************
// VisualStyle
/** Basic visual style attributes inherited by all other styles. */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY (VisualStyle, "BasicStyle", VISUALSTYLES_TYPELIB) //  see also: StyleID
BEGIN_TYPELIB_MEMBERS (VisualStyle)
	ADD_TYPELIB_MEMBER_COLOR	("forecolor")	///< color used for foreground elements
	ADD_TYPELIB_MEMBER_COLOR	("hilitecolor")	///< color used for a hilite state
	ADD_TYPELIB_MEMBER_COLOR	("backcolor")	///< backgound color
	ADD_TYPELIB_MEMBER_COLOR	("textcolor")	///< color used for drawing text
	ADD_TYPELIB_MEMBER_METRIC	("strokewidth")	///< width in pixels used when lines are drawn
	ADD_TYPELIB_MEMBER_FONT		("textfont")	///< font used for drawing text
	ADD_TYPELIB_MEMBER_ALIGN	("textalign")	///< alignment of text
	ADD_TYPELIB_MEMBER_OPTIONS	("textoptions")	///< additional options for drawing text. Supported option: "wordbreak" (for multiline text)
	ADD_TYPELIB_MEMBER_IMAGE	("background")	///< background image
END_TYPELIB_MEMBERS (VisualStyle)
