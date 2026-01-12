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
// Filename    : ccl/platform/linux/gui/fontresource.linux.cpp
// Description : Linux Font Resource
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/fontresource.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/base/singleton.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/shared/skia/skiafontmanager.h"
#include "ccl/platform/shared/skia/skiatextlayout.h"
#include "ccl/platform/shared/skia/skiastream.h"

namespace CCL {
	
//************************************************************************************************
// LinuxFontResource
//************************************************************************************************

class LinuxFontResource: public FontResource
{
public:
	LinuxFontResource (CCL::IStream& stream, StringRef name, int fontStyle);
	~LinuxFontResource ();
};

//************************************************************************************************
// LinuxFontManager
//************************************************************************************************

class LinuxFontManager: public SkFontMgr 
{
public:
	LinuxFontManager ();
	~LinuxFontManager ();
	
	static sk_sp<SkFontMgr> create ();

	void addTypeface (sk_sp<SkTypeface> typeface, CStringRef name);
	
protected:
	sk_sp<SkFontMgr> fontManager;

	class StyleSet: public SkFontStyleSet
	{
	public:
		PROPERTY_MUTABLE_CSTRING (name, Name)
		
		void add (sk_sp<SkTypeface> typeface);

		// SkFontStyleSet
		int count () override;
		void getStyle (int index, SkFontStyle* style, SkString* styleName) override;
		sk_sp<SkTypeface> createTypeface (int index) override;
		sk_sp<SkTypeface> matchStyle (const SkFontStyle& pattern) override;
		
	private:
		Vector<sk_sp<SkTypeface>> faces;
	};
	
	Vector<sk_sp<StyleSet>> userFonts;
	mutable Vector<sk_sp<StyleSet>> fontCache;
	mutable Vector<MutableCString> missingFontsCache;
	
	// SkFontMgr
	int onCountFamilies () const override;
	void onGetFamilyName (int index, SkString* familyName) const override;
	sk_sp<SkFontStyleSet> onCreateStyleSet (int index) const override;
	sk_sp<SkFontStyleSet> onMatchFamily (const char familyName[]) const override;
	sk_sp<SkTypeface> onMatchFamilyStyle (const char familyName[], const SkFontStyle& style) const override;
	sk_sp<SkTypeface> onMatchFamilyStyleCharacter (const char familyName[], const SkFontStyle& style, const char* bcp47[], int bcp47Count, SkUnichar character) const override;
	sk_sp<SkTypeface> onMakeFromStreamIndex (std::unique_ptr<SkStreamAsset> stream, int ttcIndex) const override;
	sk_sp<SkTypeface> onMakeFromStreamArgs (std::unique_ptr<SkStreamAsset> stream, const SkFontArguments& args) const override;
	sk_sp<SkTypeface> onMakeFromData (sk_sp<SkData> data, int ttcIndex) const override;
	sk_sp<SkTypeface> onMakeFromFile (const char path[], int ttcIndex) const override;
	sk_sp<SkTypeface> onLegacyMakeTypeface (const char familyName[], SkFontStyle style) const override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SkiaFontManagerFactory
//************************************************************************************************

sk_sp<SkFontMgr> SkiaFontManagerFactory::createFontManager ()
{
	static sk_sp<SkFontMgr> theManager = LinuxFontManager::create ();
	return theManager;
}

//************************************************************************************************
// LinuxFontResource
//************************************************************************************************

FontResource* FontResource::install (CCL::IStream& stream, StringRef name, int fontStyle)
{
	return NEW LinuxFontResource (stream, name, fontStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFontResource::LinuxFontResource (CCL::IStream& stream, StringRef name, int fontStyle)
{
	AutoPtr<IMemoryStream> fontStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	if(!fontStream)
		return;
	
	SkiaStream skStream (fontStream);
	sk_sp<SkData> data = SkData::MakeFromStream (&skStream, skStream.getLength ());
	
	sk_sp<SkFontMgr> fontManager = SkiaFontManagerFactory::createFontManager ();
	if(fontManager == nullptr)
		return;
	
	for(int i = 0; ; i++)
	{
		sk_sp<SkTypeface> typeface = fontManager->makeFromData (data, i);
		if(typeface == nullptr)
			break;
		
		SkString skPostscriptName;
		typeface->getPostScriptName (&skPostscriptName);
		String postscriptName (Text::kUTF8, skPostscriptName.c_str ());
		
		int style = SkiaFontCache::fromSkFontStyle (typeface->fontStyle ());
		ASSERT (fontStyle == style)
		
		sk_sp<SkTypeface::LocalizedStrings> skFamilyNames (typeface->createFamilyNameIterator ());
		SkTypeface::LocalizedString skFamilyName;
		while(skFamilyNames && skFamilyNames->next (&skFamilyName))
		{
			String familyName (Text::kUTF8, skFamilyName.fString.c_str ());
		
			if(style != Font::kNormal)
				SkiaFontCache::instance ().addStyledFont (familyName, style, name, postscriptName);
			
			SkiaFontCache::instance ().addUserFont (familyName);
			
			static_cast<LinuxFontManager*> (fontManager.get ())->addTypeface (typeface, skFamilyName.fString.c_str ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFontResource::~LinuxFontResource ()
{
}

//************************************************************************************************
// LinuxFontManager
//************************************************************************************************

LinuxFontManager::LinuxFontManager ()
: fontManager (nullptr)
{
	fontManager = SkFontMgr_New_FontConfig (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFontManager::~LinuxFontManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFontManager::addTypeface (sk_sp<SkTypeface> typeface, CStringRef name)
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

sk_sp<SkFontMgr> LinuxFontManager::create ()
{
	return sk_make_sp<LinuxFontManager> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LinuxFontManager::onCountFamilies () const
{
	return fontManager->countFamilies () + userFonts.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFontManager::onGetFamilyName (int index, SkString* familyName) const
{
	int systemCount = fontManager->countFamilies ();
	if(index < systemCount)
		fontManager->getFamilyName (index, familyName);
	else if(index < systemCount + userFonts.count ())
		familyName->set (userFonts[index - systemCount]->getName ().str ());
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontStyleSet> LinuxFontManager::onCreateStyleSet (int index) const
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

sk_sp<SkFontStyleSet> LinuxFontManager::onMatchFamily (const char familyName[]) const
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
	
sk_sp<SkTypeface> LinuxFontManager::onMatchFamilyStyle (const char familyName[], const SkFontStyle& style) const
{
	return fontManager->matchFamilyStyle (familyName, style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::onMatchFamilyStyleCharacter (const char familyName[], const SkFontStyle& style, const char* bcp47[], int bcp47Count, SkUnichar character) const
{
	if(character == '\t' || character == '\n' || character == '\r')
		return nullptr;
	return fontManager->matchFamilyStyleCharacter (familyName, style, bcp47, bcp47Count, character);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::onMakeFromStreamIndex (std::unique_ptr<SkStreamAsset> stream, int ttcIndex) const
{
	ASSERT (0)
	return fontManager->makeFromStream (std::move (stream), ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::onMakeFromStreamArgs (std::unique_ptr<SkStreamAsset> stream, const SkFontArguments& args) const
{
	return fontManager->makeFromStream (std::move (stream), args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::onMakeFromData (sk_sp<SkData> data, int ttcIndex) const
{
	return fontManager->makeFromData (std::move (data), ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::onMakeFromFile (const char path[], int ttcIndex) const
{
	return fontManager->makeFromFile (path, ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::onLegacyMakeTypeface (const char familyName[], SkFontStyle style) const
{
	return fontManager->legacyMakeTypeface (familyName, style);
}

//************************************************************************************************
// LinuxFontManager::StyleSet
//************************************************************************************************

void LinuxFontManager::StyleSet::add (sk_sp<SkTypeface> typeface)
{
	faces.add (typeface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LinuxFontManager::StyleSet::count ()
{
	return faces.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFontManager::StyleSet::getStyle (int index, SkFontStyle* style, SkString* styleName)
{
	const auto& face = faces.at (index);
	if(face != nullptr && style != nullptr)
		*style = face->fontStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::StyleSet::createTypeface (int index)
{	
	const auto& face = faces.at (index);
	if(face != nullptr)
		return face;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> LinuxFontManager::StyleSet::matchStyle (const SkFontStyle& pattern)
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
