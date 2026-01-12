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
// Filename    : ccl/platform/win/gui/theme.win.h
// Description : Windows Theme
//
//************************************************************************************************

#ifndef _ccl_theme_win_h
#define _ccl_theme_win_h

#include "ccl/gui/theme/theme.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// Win32::WindowsTheme
//************************************************************************************************

class WindowsTheme: public NativeThemePainter
{
public:
	// NativeThemePainter
	bool getSystemColor (Color& color, ThemeColorID which) const override;
	bool getSystemFont (Font& font, ThemeFontID which) const override;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_theme_win_h
