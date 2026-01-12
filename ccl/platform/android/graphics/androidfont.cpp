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
// Filename    : ccl/platform/android/graphics/androidfont.cpp
// Description : Android Font
//
//************************************************************************************************

#include "androidfont.h"

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidFont
//************************************************************************************************

AndroidFont::AndroidFont (JNIEnv* jni, jobject object)
: JniObject (jni, object),
  style (0),
  symbolFont (false)
{}

//************************************************************************************************
// AndroidSystemFont
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AndroidSystemFont, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidSystemFont::AndroidSystemFont (JNIEnv* jni, jobject object, FontRef font)
: JniObject (jni, object),
  face (font.getFace ()),
  styleName (font.getStyleName ()),
  style (font.getStyle ())
{}

//************************************************************************************************
// AndroidFontTable
//************************************************************************************************

AndroidFontTable::AndroidFontFamily* AndroidFontTable::findFamily (StringRef name)
{
	for(FontFamily* family : fonts)
		if(family->name == name)
			return static_cast<AndroidFontFamily*> (family);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidFontTable::getFontPath (Url& path, int fontIndex, int styleIndex)
{
	if(fontIndex < 0 || fontIndex >= fonts.count ())
		return kResultInvalidArgument;

	AndroidFontFamily* font = static_cast<AndroidFontFamily*> (*fonts.at (fontIndex).as_ppv ());
	if(styleIndex < 0 || styleIndex >= font->styles.count ())
		return kResultInvalidArgument;

	path = font->paths.at (styleIndex);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidFontTable::isSymbolFont (int fontIndex)
{
	if(fontIndex < 0 || fontIndex >= fonts.count ())
		return false;

	AndroidFontFamily* font = static_cast<AndroidFontFamily*> (*fonts.at (fontIndex).as_ppv ());
	return font->symbolFont;
}
