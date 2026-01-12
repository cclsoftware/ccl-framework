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
// Filename    : ccl/platform/shared/interfaces/platformtheme.h
// Description : CCL GUI Platform Integration
//
//************************************************************************************************

#ifndef _ccl_platformtheme_h
#define _ccl_platformtheme_h

#include "ccl/public/gui/framework/themeelements.h"

#include "core/public/coreplugin.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// IPlatformTheme
//************************************************************************************************

struct IPlatformTheme: Core::IPropertyHandler
{
	typedef ThemeElements::MetricID MetricID;
	typedef ThemeElements::ColorID ColorID;
	typedef ThemeElements::FontID FontID;
	
	virtual tbool getMetric (int& value, MetricID id) const = 0;
	virtual tbool getColor (Core::Color& color, ColorID id) const = 0;
	virtual tbool getFont (Core::StringResult fontName, FontID id) const = 0;
	
	static const Core::InterfaceID kIID = FOUR_CHAR_ID ('T','h','m','e');
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformtheme_h
 
