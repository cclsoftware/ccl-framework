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
// Filename    : ccl/platform/shared/skia/skiafonttable.h
// Description : Skia Font Table
//
//************************************************************************************************

#ifndef _ccl_skia_fonttable_h
#define _ccl_skia_fonttable_h

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/gui/graphics/font.h"

namespace CCL {

//************************************************************************************************
// SkiaFontTable
//************************************************************************************************

class SkiaFontTable: public Object,
					 public IFontTable
{
public:
	SkiaFontTable (int flags = 0);
	
	DECLARE_CLASS (SkiaFontTable, Object)
	
	struct FontFamily
	{
		String name;
		Vector<String> styles;
	};

	// IFontTable
	int CCL_API countFonts () override; 
	tresult CCL_API getFontName (String& name, int index) override;
	int CCL_API countFontStyles (int fontIndex) override;
	tresult CCL_API getFontStyleName (String& name, int fontIndex, int styleIndex) override;
	tresult CCL_API getExampleText (String& text, int fontIndex, int styleIndex) override;

	CLASS_INTERFACE (IFontTable, Object)
	
private:
	Vector<FontFamily> fonts;
};

} // namespace CCL

#endif // _ccl_skia_fonttable_h
