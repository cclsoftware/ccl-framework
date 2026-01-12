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
// Filename    : ccl/gui/theme/colorreference.h
// Description : Color Scheme Reference
//
//************************************************************************************************

#ifndef _ccl_colorreference_h
#define _ccl_colorreference_h

#include "ccl/public/text/cstring.h"

#include "ccl/public/gui/graphics/color.h"

namespace CCL {

class ColorScheme;

//************************************************************************************************
// ColorSchemeReference
//************************************************************************************************

struct ColorSchemeReference
{
	ColorScheme* scheme = nullptr;
	MutableCString nameInScheme;
};

//************************************************************************************************
// ColorValueReference
//************************************************************************************************

struct ColorValueReference: ColorSchemeReference
{
	Color colorValue;
};

} // namespace CCL

#endif // _ccl_colorreference_h
