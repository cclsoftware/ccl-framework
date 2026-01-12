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
// Filename    : ccl/platform/cocoa/gui/theme.ios.mm
// Description : iOS Theme
//
//************************************************************************************************

#include "ccl/gui/theme/theme.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/platform/cocoa/cclcocoa.h"

namespace CCL {

//************************************************************************************************
// IOSTheme
//************************************************************************************************

class IOSTheme: public NativeThemePainter
{
public:
	// NativeThemePainter
	bool getSystemColor (Color& color, ThemeColorID which) const;
	bool getSystemFont (Font& font, ThemeFontID which) const;
	bool getSystemMetric (int& metric, ThemeMetricID which) const;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// NativeThemePainter
//************************************************************************************************

NativeThemePainter& NativeThemePainter::instance ()
{
	static IOSTheme theNativeTheme;
	return theNativeTheme;
}

//************************************************************************************************
// IOSTheme
//************************************************************************************************

bool IOSTheme::getSystemColor (Color& color, ThemeColorID which) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSTheme::getSystemFont (Font& font, ThemeFontID which) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSTheme::getSystemMetric (int& metric, ThemeMetricID which) const
{
	UIEdgeInsets insets;
	UIView* rootView = [[[[UIApplication sharedApplication] keyWindow] rootViewController] view];
	insets = rootView.safeAreaInsets;
	
	if(which == ThemeElements::kSystemStatusBarHeight)
	{
		metric = (int)insets.top;
		return true;
	}
	else if(which == ThemeElements::kSystemNavigationBarHeight)
	{
		bool hideHomeIndicator = false;
		CCL::Configuration::Registry::instance ().getValue (hideHomeIndicator, "CCL.iOS", "HideHomeIndicator");
		
		metric = hideHomeIndicator ? 0 : (int)insets.bottom;
		return true;
	}
	else if(which == ThemeElements::kSystemMarginLeft)
	{
		metric = (int)insets.left;
		return true;
	}
	else if(which == ThemeElements::kSystemMarginRight)
	{
		metric = (int)insets.right;
		return true;
	}
	
	return false;
}
