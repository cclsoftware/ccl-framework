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
// Filename    : ccl/platform/shared/host/platformthemepainter.cpp
// Description : Platform Theme
//
//************************************************************************************************

#include "ccl/platform/shared/host/platformthemepainter.h"

#include "ccl/gui/theme/thememanager.h"

#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace PlatformIntegration;

//************************************************************************************************
// NativeThemePainter
//************************************************************************************************

NativeThemePainter& NativeThemePainter::instance ()
{
	static PlatformThemePainter thePlatformThemePainter;
	return thePlatformThemePainter;
}

//************************************************************************************************
// LinuxTheme
//************************************************************************************************

PlatformThemePainter::PlatformThemePainter ()
: platformTheme (CCLGUI_PACKAGE_ID)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformThemePainter::initialize ()
{
	platformTheme.load ();
	if(platformTheme)
	{
		ThemeManager::instance ().onSystemMetricsChanged ();
		ThemeManager::instance ().onSystemColorsChanged ();
		ThemeManager::instance ().onSystemFontsChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlatformThemePainter::getSystemMetric (int& metric, ThemeMetricID which) const
{
	if(platformTheme)
		return platformTheme->getMetric (metric, which);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlatformThemePainter::getSystemColor (Color& color, ThemeColorID which) const
{	
	if(platformTheme)
		return platformTheme->getColor (color, which);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlatformThemePainter::getSystemFont (Font& font, ThemeFontID which) const
{	
	if(platformTheme)
	{
		char stringBuffer[STRING_STACK_SPACE_MAX] = {0};
		StringResult fontName (stringBuffer, sizeof(stringBuffer));
		if(!platformTheme->getFont (fontName, which))
			return false;
		
		font.setFace (String (Text::kUTF8, fontName.charBuffer));
		return true;
		
	}
	return false;
}
