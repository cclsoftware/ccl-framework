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
// Filename    : ccl/platform/android/graphics/androidfont.h
// Description : Android Font
//
//************************************************************************************************

#ifndef _ccl_androidfont_h
#define _ccl_androidfont_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/base/storage/url.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// AndroidFont
//************************************************************************************************

class AndroidFont: public Object,
				   public JniObject
{
public:
	AndroidFont (JNIEnv* jni, jobject object);

	PROPERTY_STRING (familyName, FamilyName)
	PROPERTY_STRING (fullName, FullName)
	PROPERTY_VARIABLE (int, style, Style)
	PROPERTY_BOOL (symbolFont, SymbolFont)

	bool isBold () const { return (style & Font::kBold) != 0; }
	bool isItalic () const { return (style & Font::kItalic) != 0; }

	bool matches (StringRef name) const
	{
		return name == familyName || name == fullName;
	}
};

//************************************************************************************************
// AndroidSystemFont
//************************************************************************************************

class AndroidSystemFont: public Object,
						 public JniObject
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidSystemFont, Object)
	AndroidSystemFont (JNIEnv* jni, jobject object, FontRef font);

	PROPERTY_STRING (face, Face)
	PROPERTY_STRING (styleName, StyleName)
	PROPERTY_VARIABLE (int, style, Style)

	bool matches (FontRef font) const
	{
		return face == font.getFace () && (font.getStyleName ().isEmpty () ? style == font.getStyle () : styleName == font.getStyleName ());
	}
};

//************************************************************************************************
// AndroidFontTable
//************************************************************************************************

class AndroidFontTable: public SimpleFontTable
{
public:
	struct AndroidFontFamily : public FontFamily
	{
		Vector<Url> paths;
		bool symbolFont;
	};

	AndroidFontFamily* findFamily (StringRef name);
	tresult getFontPath (Url& path, int fontIndex, int styleIndex);
	bool isSymbolFont (int fontIndex);
};

} // namespace Android
} // namespace CCL

#endif // _ccl_androidfont_h
