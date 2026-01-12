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
// Filename    : ccl/public/gui/graphics/font.cpp
// Description : Font classes
//
//************************************************************************************************

#include "ccl/public/gui/graphics/font.h"
#include "ccl/public/gui/graphics/igraphicshelper.h"

using namespace CCL;

//************************************************************************************************
// Font
//************************************************************************************************

const Font& Font::getDefaultFont ()
{
	static Font defaultFont (System::GetGraphicsHelper ().Font_getDefaultFont ());
	return defaultFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Font::getStringWidth (StringRef text, FontRef font)
{
	Rect size;
	measureString (size, text, font);
	return size.getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoordF Font::getStringWidthF (StringRef text, FontRef font)
{
	RectF size;
	measureString (size, text, font);
	return size.getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Font::measureString (Rect& size, StringRef text, FontRef font, int flags)
{
	System::GetGraphicsHelper ().Font_measureString (size, text, font, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Font::measureString (RectF& size, StringRef text, FontRef font, int flags)
{
	System::GetGraphicsHelper ().Font_measureString (size, text, font, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Font::measureStringImage (RectF& size, StringRef text, FontRef font, bool shiftToBaseline)
{
	System::GetGraphicsHelper ().Font_measureStringImage (size, text, font, shiftToBaseline);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Font::measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font, TextFormatRef format)
{
	System::GetGraphicsHelper ().Font_measureText (size, lineWidth, text, font, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Font::measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font, TextFormatRef format)
{
	System::GetGraphicsHelper ().Font_measureText (size, lineWidth, text, font, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Font::collapseString (String& string, Coord maxWidth, FontRef font, int trimMode, bool exact)
{
	System::GetGraphicsHelper ().Font_collapseString (string, CoordF (maxWidth), font, trimMode, exact);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Font::collapseString (String& string, CoordF maxWidth, FontRef font, int trimMode, bool exact)
{
	System::GetGraphicsHelper ().Font_collapseString (string, maxWidth, font, trimMode, exact);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* Font::collectFonts (int flags)
{
	return System::GetGraphicsHelper ().Font_collectFonts (flags);
}
