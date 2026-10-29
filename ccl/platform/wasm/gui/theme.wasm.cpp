//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/theme.wasm.cpp
// Description : WebAssembly Theme
//
//************************************************************************************************

#include "ccl/gui/theme/theme.h"

using namespace CCL;

//************************************************************************************************
// WasmTheme
//************************************************************************************************

class WasmTheme: public NativeThemePainter
{
public:
	// NativeThemePainter
	bool getSystemColor (Color& color, ThemeColorID which) const override { return false; }
	bool getSystemFont (Font& font, ThemeFontID which) const override { return false; }
	bool getSystemMetric (int& metric, ThemeMetricID which) const override { return false; }
};

//************************************************************************************************
// NativeThemePainter
//************************************************************************************************

NativeThemePainter& NativeThemePainter::instance ()
{
	static WasmTheme theNativeTheme;
	return theNativeTheme;
}
