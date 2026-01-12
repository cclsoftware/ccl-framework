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
// Filename    : ccl/platform/win/gui/theme.win.cpp
// Description : Windows Theme
//
//************************************************************************************************

#include "ccl/platform/win/gui/theme.win.h"
#include "ccl/platform/win/gui/win32graphics.h"

#include <uxtheme.h>
#include <vssym32.h>

#pragma comment (lib, "uxtheme.lib")

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// NativeThemePainter
//************************************************************************************************

NativeThemePainter& NativeThemePainter::instance ()
{
	static WindowsTheme theNativeTheme;
	return theNativeTheme;
}

//************************************************************************************************
// Win32::WindowsTheme
//************************************************************************************************

bool WindowsTheme::getSystemColor (Color& color, ThemeColorID which) const
{
	bool result = true;
	COLORREF sysColor = 0;

	switch(which)
	{
	case ThemeElements::kSelectionColor :
		sysColor = ::GetSysColor (COLOR_HIGHLIGHT);
		break;

	case ThemeElements::kSelectionTextColor :
		sysColor = ::GetSysColor (COLOR_HIGHLIGHTTEXT);
		break;

	case ThemeElements::kTooltipBackColor :
		sysColor = ::GetSysColor (COLOR_INFOBK);
		break;

	case ThemeElements::kTooltipTextColor :
		sysColor = ::GetSysColor (COLOR_INFOTEXT);
		break;

	case ThemeElements::kListViewBackColor :
		sysColor = GetSysColor (COLOR_WINDOW);
		break;

	default :
		result = false;
	}

	if(result)
		color = GdiInterop::fromSystemColor (sysColor);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsTheme::getSystemFont (Font& font, ThemeFontID which) const
{
	if(which == ThemeElements::kMenuFont)
	{
		LOGFONT logFont = {0};
		HRESULT hr = ::GetThemeSysFont (NULL, TMT_MENUFONT, &logFont);
		ASSERT (SUCCEEDED (hr))
		GdiInterop::fromLogicalFont (font, logFont);
		return true;
	}
	return false;
}
