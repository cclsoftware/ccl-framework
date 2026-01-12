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
// Filename    : ccl/platform/shared/skia/skiafonttable.cpp
// Description : Skia Font Table
//
//************************************************************************************************

#include "ccl/platform/shared/skia/skiafontmanager.h"
#include "ccl/platform/shared/skia/skiafonttable.h"

#include "ccl/public/text/cstring.h"

using namespace CCL;

//************************************************************************************************
// SkiaFontTable
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaFontTable, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontTable::SkiaFontTable (int flags)
{
	// TODO: handle flags
	
	sk_sp<SkFontMgr> manager = SkiaFontManagerFactory::createFontManager ();
	if(!manager)
		return;
	
	for(int i = 0; i < manager->countFamilies (); i++)
	{
		SkString skFamilyName;
		manager->getFamilyName (i, &skFamilyName);
		if(skFamilyName.startsWith (".")) // these are usually hidden for UI
			continue;
		
		FontFamily family;
		family.name = String (Text::kUTF8, skFamilyName.c_str ());
		fonts.add (family);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
int CCL_API SkiaFontTable::countFonts ()
{
	return fonts.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaFontTable::getFontName (String& name, int index)
{
	if(index < 0 || index >= fonts.count ())
		return kResultInvalidArgument;
	
	name = fonts[index].name;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SkiaFontTable::countFontStyles (int index)
{
	if(index < 0 || index >= fonts.count ())
		return 0;
	
	FontFamily& family = fonts[index];
	
	if(family.styles.isEmpty ())
	{
		sk_sp<SkFontMgr> manager = SkiaFontManagerFactory::createFontManager ();
		if(!manager)
			return 0;
		
		sk_sp<SkFontStyleSet> styleSet = manager->matchFamily (MutableCString (family.name, Text::kUTF8));
		//styleset is guaranteed to be not null
		for(int i = 0; i < styleSet->count (); i++)
		{
			SkFontStyle style;
			SkString skStyleName;
			styleSet->getStyle (i, &style, &skStyleName);
			String styleName (Text::kUTF8, skStyleName.c_str ());
			family.styles.add (styleName);
		}
	}
	
	return family.styles.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaFontTable::getFontStyleName (String& name, int index, int styleIndex)
{
	if(styleIndex < 0 || styleIndex >= countFontStyles (index))
		return kResultInvalidArgument;
	
	const FontFamily& family = fonts[index];
	
	name = family.styles.at (styleIndex);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaFontTable::getExampleText (String& text, int fontIndex, int styleIndex)
{
	return kResultNotImplemented;
}
