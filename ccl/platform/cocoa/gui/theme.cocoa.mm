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
// Filename    : ccl/platform/cocoa/gui/theme.cocoa.mm
// Description : Mac OSX Theme
//
//************************************************************************************************

#include "ccl/gui/theme/theme.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// MacOSXTheme
//************************************************************************************************

class MacOSXTheme: public NativeThemePainter
{
public:	
	// NativeThemePainter
	bool getSystemColor (Color& color, ThemeColorID which) const;
	bool getSystemFont (Font& font, ThemeFontID which) const;
};

} // namespace MacOS
} // namespace CCL

using namespace CCL;
using namespace MacOS;
using namespace ThemeElements;

//************************************************************************************************
// NativeThemePainter
//************************************************************************************************

NativeThemePainter& NativeThemePainter::instance ()
{
	static MacOSXTheme theNativeTheme;
	return theNativeTheme;
}

//************************************************************************************************
// MacOSXTheme
//************************************************************************************************

bool MacOSXTheme::getSystemColor (Color& color, ThemeColorID which) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MacOSXTheme::getSystemFont (Font& font, ThemeFontID which) const
{
	if(which == kMenuFont)
	{
		font.setFace ("Lucida Grande");
		font.setSize (13);
		return true;
	}
	return false;
}
