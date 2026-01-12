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
// Filename    : ccl/public/gui/graphics/color.h
// Description : Color class
//
//************************************************************************************************

#ifndef _ccl_color_h
#define _ccl_color_h

#include "ccl/public/base/primitives.h"

#include "core/public/gui/corecolor.h"

namespace CCL {

using Core::Color;
using Core::ColorRef;
using Core::ColorF;
using Core::ColorFRef;
using Core::ColorHSL;
using Core::ColorHSV;

//************************************************************************************************
// Colors
/**	Color definitions and utilities.
	\ingroup gui_graphics */
//************************************************************************************************

namespace Colors
{
	extern Color kTransparentBlack;		///< black with alpha zero
	extern ColorF kTransparentBlackF;

	extern Color kBlack;	///< black
	extern Color kWhite;	///< white
	extern Color kRed;		///< red
	extern Color kGreen;	///< green
	extern Color kBlue;		///< blue
	extern Color kYellow;	///< yellow
	extern Color kGray;		///< gray
	extern Color kLtGray;	///< lightgray

	/** Convert color to C-String. */
	void toCString (ColorRef color, char* cString, int cStringSize, bool withAlpha = true);

	/** Get color from C-String. */
	bool fromCString (Color& color, CStringPtr cString);

	/** Convert color to Unicode string. */
	void toString (ColorRef color, String& string, bool withAlpha = true);

	/** Get color from Unicode string. */
	bool fromString (Color& color, StringRef string);
}

} // namespace CCL

#endif // _ccl_color_h
