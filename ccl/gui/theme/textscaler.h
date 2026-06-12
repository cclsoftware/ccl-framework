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
// Filename    : ccl/gui/theme/textscaler.h
// Description : Text Scaler
//
//************************************************************************************************

#ifndef _ccl_textscaler_h
#define _ccl_textscaler_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

//************************************************************************************************
// TextScaler
/** Helper class that caches the latest string for a given rect and provides the appropriate font size. */
//************************************************************************************************

struct TextScaler
{
public:
	TextScaler ();

	enum Options
	{
		kMarkupText = 1<<0,
		kDisableCache = 1<<1
	};

	void scaleTextFont (Font& font, RectRef r, StringRef text, int options = 0);
	
	PROPERTY_VARIABLE (float, explicitMaximalFontSize, ExplicitMaximalFontSize);
	PROPERTY_VARIABLE (float, explicitMinimalFontSize, ExplicitMinimalFontSize);

private:
	String cachedText;
	Rect cachedRect;
	float cachedFontSize;
};

} // namespace CCL

#endif // _ccl_textscaler_h
