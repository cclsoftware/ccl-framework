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
// Filename    : ccl/platform/android/gui/theme.android.cpp
// Description : Android Theme
//
//************************************************************************************************

#include "ccl/gui/theme/theme.h"

#include "ccl/platform/android/gui/frameworkactivity.h"

#include "ccl/public/gui/graphics/dpiscale.h"

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidTheme
//************************************************************************************************

class AndroidTheme: public NativeThemePainter
{
public:
	// NativeThemePainter
	bool getSystemColor (Color& color, ThemeColorID which) const override;
	bool getSystemFont (Font& font, ThemeFontID which) const override;
	bool getSystemMetric (int& metric, ThemeMetricID which) const override;
};

//************************************************************************************************
// AndroidTheme
//************************************************************************************************

NativeThemePainter& NativeThemePainter::instance ()
{
	static AndroidTheme theNativeTheme;
	return theNativeTheme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidTheme::getSystemColor (Color& color, ThemeColorID which) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidTheme::getSystemFont (Font& font, ThemeFontID which) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidTheme::getSystemMetric (int& metric, ThemeMetricID which) const
{
	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();
	Rect insets = activity->getInsets ();
	DpiScale::toCoordRect (insets, activity->getDensityFactor ());
	if(which == ThemeElements::kSystemStatusBarHeight)
	{
		metric = insets.top;
		return true;
	}
	else if(which == ThemeElements::kSystemNavigationBarHeight)
	{
		metric = insets.bottom;
		return true;
	}
	else if(which == ThemeElements::kSystemMarginLeft)
	{
		metric = insets.left;
		return true;
	}
	else if(which == ThemeElements::kSystemMarginRight)
	{
		metric = insets.right;
		return true;
	}
	return false;
}
