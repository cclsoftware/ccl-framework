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
// Filename    : ccl/platform/shared/skia/skiafontresource.cpp
// Description : Skia Font Resource
//
//************************************************************************************************

#include "ccl/gui/system/fontresource.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/base/singleton.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/shared/skia/skiastream.h"
#include "ccl/platform/shared/skia/skiafontmanager.h"
#include "ccl/platform/shared/skia/skiatextlayout.h"

namespace CCL {
	
//************************************************************************************************
// SkiaFontResource
//************************************************************************************************

class SkiaFontResource: public FontResource
{
public:
	SkiaFontResource (CCL::IStream& stream, StringRef name, int fontStyle);
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SkiaFontResource
//************************************************************************************************

FontResource* FontResource::install (CCL::IStream& stream, StringRef name, int fontStyle)
{
	return NEW SkiaFontResource (stream, name, fontStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaFontResource::SkiaFontResource (CCL::IStream& stream, StringRef name, int fontStyle)
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
			
			static_cast<SkiaFontManager*> (fontManager.get ())->addTypeface (typeface, skFamilyName.fString.c_str ());
		}
	}
}
