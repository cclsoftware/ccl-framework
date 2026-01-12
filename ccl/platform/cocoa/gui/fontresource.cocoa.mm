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
// Filename    : ccl/platform/cocoa/gui/fontresource.cocoa.mm
// Description : Mac OS Font Resource
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/fontresource.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/gui/graphics/font.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/fontcache.h"

#include "ccl/platform/shared/skia/skiafontmanager.h"
#include "ccl/platform/shared/skia/skiatextlayout.h"

#include <CoreText/CoreText.h>
#include <CoreFoundation/CFData.h>

namespace CCL {

//************************************************************************************************
// CocoaFontResource
//************************************************************************************************

class CocoaFontResource: public FontResource
{
public:
	CocoaFontResource (CCL::IStream& stream, StringRef name, int fontStyle);
	~CocoaFontResource ();

private:
	CGFontRef fontRef;
};

//************************************************************************************************
// CocoaFontManager
//************************************************************************************************

class CocoaFontManager: public SkFontMgr
{
public:
	CocoaFontManager ();
	
	static sk_sp<SkFontMgr> create ();

	void addFont (CStringRef name);
	
protected:
	sk_sp<SkFontMgr> fontManager;
	Vector<MutableCString> userFonts;
	
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
	#if CCL_PLATFORM_MAC
	static sk_sp<SkFontMgr> theManager = CocoaFontManager::create ();
	#else
	static sk_sp<SkFontMgr> theManager = SkFontMgr_New_CoreText (NULL);
	#endif
	return theManager;
}

//************************************************************************************************
// CocoaFontResource
//************************************************************************************************

FontResource* FontResource::install (IStream& stream, StringRef name, int fontStyle)
{
	return NEW CocoaFontResource (stream, name, fontStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaFontResource::CocoaFontResource (CCL::IStream& stream, StringRef name, int fontStyle)
: fontRef (0)
{
	AutoPtr<IMemoryStream> fontStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	if(!fontStream)
		return;

	CFDataRef inData = CFDataCreate (NULL, (UInt8 *)fontStream->getMemoryAddress (), fontStream->getBytesWritten ());

	CGDataProviderRef provider = CGDataProviderCreateWithCFData (inData);
	fontRef = CGFontCreateWithDataProvider (provider);
	
	#if DEBUG_LOG
	CFObj<CFStringRef> cfname;
	String nameString;
	cfname = CGFontCopyFullName (fontRef);
	nameString.appendNativeString (cfname);
	CCL_PRINTF("fullName: %s\n", MutableCString (nameString).str ())
	cfname = CGFontCopyPostScriptName (fontRef);
	nameString.empty ();
	nameString.appendNativeString (cfname);
	CCL_PRINTF("postScriptName: %s\n", MutableCString (nameString).str ())
	#endif
	
	CFErrorRef error;
	bool success = CTFontManagerRegisterGraphicsFont (fontRef, &error);
	
	CGDataProviderRelease (provider);
	CFRelease (inData);
	
	ASSERT(success)
	if(success)
	{
		CTFontRef ctFontRef = CTFontCreateWithGraphicsFont (fontRef, 0, NULL, NULL);
		CFStringRef cfFamilyName = CTFontCopyName (ctFontRef, kCTFontFamilyNameKey);
		String familyName;
		familyName.appendNativeString (cfFamilyName);
		CFRelease (cfFamilyName);

		CFStringRef cfFullName = CTFontCopyName (ctFontRef, kCTFontFullNameKey);
		String fullName;
		fullName.appendNativeString (cfFullName);
		CFRelease (cfFullName);
		if(fontStyle != Font::kNormal)
			FontCache::instance ().addStyledFont (familyName, fontStyle, fullName);
		CFStringRef cfStyleName = CTFontCopyName (ctFontRef, kCTFontStyleNameKey);
		String styleName;
		styleName.appendNativeString (cfStyleName);
		CFRelease (cfStyleName);
		#if CCL_PLATFORM_MAC
		if(fontStyle != Font::kNormal || !styleName.isEmpty ())
			SkiaFontCache::instance ().addStyledFont (familyName, fontStyle, fullName, styleName);
		#endif
		CFRelease (ctFontRef);
		FontCache::instance ().addUserFont (familyName);
		#if CCL_PLATFORM_MAC
		SkiaFontCache::instance ().addUserFont (familyName);
		sk_sp<SkFontMgr> fontManager = SkiaFontManagerFactory::createFontManager ();
		static_cast<CocoaFontManager*> (fontManager.get ())->addFont (MutableCString (familyName));
		#endif
	}
	else
	{
		CGFontRelease (fontRef);
		fontRef = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaFontResource::~CocoaFontResource ()
{
	if(fontRef)
	{
		CFErrorRef error;
		CTFontManagerUnregisterGraphicsFont (fontRef, &error);
		CGFontRelease (fontRef);
	}
}

//************************************************************************************************
// CocoaFontManager
//************************************************************************************************

CocoaFontManager::CocoaFontManager ()
: fontManager (nullptr)
{
	fontManager = SkFontMgr_New_CoreText (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaFontManager::addFont (CStringRef name)
{
	for(const auto& font : userFonts)
		if(font == name)
			return;
	
	userFonts.add (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontMgr> CocoaFontManager::create ()
{
	return sk_make_sp<CocoaFontManager> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaFontManager::onCountFamilies () const
{
	return fontManager->countFamilies () + userFonts.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaFontManager::onGetFamilyName (int index, SkString* familyName) const
{
	int systemCount = fontManager->countFamilies ();
	if(index < systemCount)
		fontManager->getFamilyName (index, familyName);
	else if(index < systemCount + userFonts.count ())
		familyName->set (userFonts[index - systemCount].str ());
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontStyleSet> CocoaFontManager::onCreateStyleSet (int index) const
{
	int systemCount = fontManager->countFamilies ();
	if(index < systemCount)
		return fontManager->createStyleSet (index);
	else if(index < systemCount + userFonts.count ())
		return fontManager->matchFamily (userFonts[index - systemCount].str ());

	return nullptr;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontStyleSet> CocoaFontManager::onMatchFamily (const char familyName[]) const
{
	return fontManager->matchFamily (familyName);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
	
sk_sp<SkTypeface> CocoaFontManager::onMatchFamilyStyle (const char familyName[], const SkFontStyle& style) const
{
	return fontManager->matchFamilyStyle (familyName, style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> CocoaFontManager::onMatchFamilyStyleCharacter (const char familyName[], const SkFontStyle& style, const char* bcp47[], int bcp47Count, SkUnichar character) const
{
	return fontManager->matchFamilyStyleCharacter (familyName, style, bcp47, bcp47Count, character);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> CocoaFontManager::onMakeFromStreamIndex (std::unique_ptr<SkStreamAsset> stream, int ttcIndex) const
{
	return fontManager->makeFromStream (std::move (stream), ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> CocoaFontManager::onMakeFromStreamArgs (std::unique_ptr<SkStreamAsset> stream, const SkFontArguments& args) const
{
	return fontManager->makeFromStream (std::move (stream), args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> CocoaFontManager::onMakeFromData (sk_sp<SkData> data, int ttcIndex) const
{
	return fontManager->makeFromData (std::move (data), ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> CocoaFontManager::onMakeFromFile (const char path[], int ttcIndex) const
{
	return fontManager->makeFromFile (path, ttcIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> CocoaFontManager::onLegacyMakeTypeface (const char familyName[], SkFontStyle style) const
{
	return fontManager->legacyMakeTypeface (familyName, style);
}

