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
// Filename    : ccl/platform/shared/skia/skiafontmanager.cpp
// Description : Skia Font Manager
//
//************************************************************************************************

#include "ccl/platform/shared/skia/skiafontmanager.h"

using namespace CCL;

//************************************************************************************************
// SkiaFontManager
//************************************************************************************************

SkiaFontManager::SkiaFontManager ()
: fontManager (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontManager::~SkiaFontManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaFontManager::addTypeface (sk_sp<SkTypeface> typeface, CStringRef name)
{
	for(const auto& font : userFonts)
	{
		if(font->getName () == name)
		{
			font->add (typeface);
			return;
		}
	}
	
	sk_sp<StyleSet> set = sk_make_sp<StyleSet> ();
	set->setName (name);
	set->add (typeface);
	userFonts.add (set);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkiaFontManager::onCountFamilies () const
{
	return fontManager->countFamilies () + userFonts.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaFontManager::onGetFamilyName (int index, SkString* familyName) const
{
	int systemCount = fontManager->countFamilies ();
	if(index < systemCount)
		fontManager->getFamilyName (index, familyName);
	else if(index < systemCount + userFonts.count ())
		familyName->set (userFonts[index - systemCount]->getName ().str ());
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontStyleSet> SkiaFontManager::onCreateStyleSet (int index) const
{
	int systemCount = fontManager->countFamilies ();
	if(index < systemCount)
	{
		SkString familyName;
		fontManager->getFamilyName (index, &familyName);
		return onMatchFamily (familyName.c_str ());
		
	}
	else if(index < systemCount + userFonts.count ())
	{
		return userFonts[index - systemCount];
	}
	return nullptr;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontStyleSet> SkiaFontManager::onMatchFamily (const char familyName[]) const
{	
	if(familyName[0] == '\0')
		return nullptr;
	
	for(const auto& entry : userFonts)
	{
		if(entry->getName () == familyName)
			return entry;
	}
	for(const auto& entry : fontCache)
	{
		if(entry->getName () == familyName)
			return entry;
	}
	
	int index = missingFontsCache.index (CString (familyName));
	if(index >= 0)
		return nullptr;
	
	CCL_PRINTF ("Could not find %s in cache\n", MutableCString (familyName).str ())
	
	sk_sp<SkFontStyleSet> styleSet (fontManager->matchFamily (familyName));
	if(styleSet && styleSet->count () > 0)
	{
		sk_sp<StyleSet> set = sk_make_sp<StyleSet> ();
		set->setName (familyName);
		for(int i = 0; i < styleSet->count (); i++)
			set->add (sk_sp<SkTypeface> (styleSet->createTypeface (i)));
		fontCache.add (set);
		return styleSet;
	}
	
	missingFontsCache.add (MutableCString (familyName));
	
	CCL_PRINTF ("Could not find %s at all!\n", MutableCString (familyName).str ())
	
	return nullptr;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
	
sk_sp<SkTypeface> SkiaFontManager::onMatchFamilyStyle (const char familyName[], const SkFontStyle& style) const
{
	return fontManager->matchFamilyStyle (familyName, style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::onMatchFamilyStyleCharacter (const char familyName[], const SkFontStyle& style, const char* bcp47[], int bcp47Count, SkUnichar character) const
{
	if(character == '\t' || character == '\n' || character == '\r')
		return nullptr;
	return fontManager->matchFamilyStyleCharacter (familyName, style, bcp47, bcp47Count, character);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::onMakeFromStreamIndex (std::unique_ptr<SkStreamAsset> stream, int ttcIndex) const
{
	ASSERT (0)
	return fontManager->makeFromStream (std::move (stream), ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::onMakeFromStreamArgs (std::unique_ptr<SkStreamAsset> stream, const SkFontArguments& args) const
{
	return fontManager->makeFromStream (std::move (stream), args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::onMakeFromData (sk_sp<SkData> data, int ttcIndex) const
{
	return fontManager->makeFromData (std::move (data), ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::onMakeFromFile (const char path[], int ttcIndex) const
{
	return fontManager->makeFromFile (path, ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::onLegacyMakeTypeface (const char familyName[], SkFontStyle style) const
{
	return fontManager->legacyMakeTypeface (familyName, style);
}

//************************************************************************************************
// SkiaFontManager::StyleSet
//************************************************************************************************

void SkiaFontManager::StyleSet::add (sk_sp<SkTypeface> typeface)
{
	faces.add (typeface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkiaFontManager::StyleSet::count ()
{
	return faces.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaFontManager::StyleSet::getStyle (int index, SkFontStyle* style, SkString* styleName)
{
	const auto& face = faces.at (index);
	if(face != nullptr && style != nullptr)
		*style = face->fontStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::StyleSet::createTypeface (int index)
{	
	const auto& face = faces.at (index);
	if(face != nullptr)
		return face;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> SkiaFontManager::StyleSet::matchStyle (const SkFontStyle& pattern)
{
	bool matchBold = pattern.weight () >= SkFontStyle::kBold_Weight;
	bool matchItalic = pattern.slant () >= SkFontStyle::kItalic_Slant;
	
	sk_sp<SkTypeface> bestMatch;
	int weightDifference = -1;
	
	for(const auto& face : faces)
	{
		bool isBold = face->fontStyle ().weight () >= SkFontStyle::kBold_Weight;
		bool isItalic = face->fontStyle ().slant () >= SkFontStyle::kItalic_Slant;
		
		int difference = ccl_abs (face->fontStyle ().weight () - pattern.weight ());
		
		if(face != nullptr && isBold == matchBold && isItalic == matchItalic && (weightDifference < 0 || difference < weightDifference))
		{
			bestMatch = face;
			weightDifference = difference;
			if(weightDifference == 0)
				break;
		}
	}
	
	if(bestMatch)
		return bestMatch;

	return nullptr;
}
