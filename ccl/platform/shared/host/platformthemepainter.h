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
// Filename    : ccl/platform/shared/host/platformthemepainter.h
// Description : Platform Theme
//
//************************************************************************************************

#ifndef _ccl_platformthemepainter_h
#define _ccl_platformthemepainter_h

#include "ccl/gui/theme/theme.h"

#include "ccl/platform/shared/interfaces/platformtheme.h"
#include "ccl/platform/shared/host/iplatformintegrationloader.h"

namespace CCL {
namespace PlatformIntegration {
    
//************************************************************************************************
// PlatformThemePainter
//************************************************************************************************

class PlatformThemePainter: public NativeThemePainter
{
public:
	PlatformThemePainter ();
	
	void initialize ();
	
	// NativeThemePainter
	bool getSystemColor (Color& color, ThemeColorID which) const;
	bool getSystemFont (Font& font, ThemeFontID which) const;
	bool getSystemMetric (int& metric, ThemeMetricID which) const;
	
protected:
	mutable PlatformIntegration::PlatformImplementationPtr<PlatformIntegration::IPlatformTheme> platformTheme;
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformthemepainter_h
