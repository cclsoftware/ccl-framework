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
// Filename    : ccl/platform/win/gui/fontresource.wasm.cpp
// Description : WebAssembly Font Resource
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/skia/skiafontmanager.h"
#include "ccl/platform/shared/skia/skiatextlayout.h"
#include "ccl/platform/wasm/gui/systemevent.wasm.h"

#include "ccl/gui/windows/desktop.h"

#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, loadFallbackFont, (CStringPtr name),
{
	CCL.loadFallbackFont (UTF8ToString (name));
});

} // namespace Wasm

//************************************************************************************************
// WasmFontManager
//************************************************************************************************

class WasmFontManager: public SkiaFontManager,
					   public SystemEventHandler
{
public:
	WasmFontManager ();
	~WasmFontManager ();

	DECLARE_STRINGID_MEMBER (kFontsPath)

	DECLARE_STRINGID_MEMBER (kFontFallbackJP)
	DECLARE_STRINGID_MEMBER (kFontFallbackSC)
	DECLARE_STRINGID_MEMBER (kFontFallbackTC)
	DECLARE_STRINGID_MEMBER (kFontFallbackKR)
	DECLARE_STRINGID_MEMBER (kFontFallbackEmoji)
	DECLARE_STRINGID_MEMBER (kFontFallbackSymbols)
	DECLARE_STRINGID_MEMBER (kFontFallbackSymbols2)
	DECLARE_STRINGID_MEMBER (kFontFallbackGeneric)

	static sk_sp<SkFontMgr> create ();

	// SkiaFontManager
	sk_sp<SkTypeface> onMatchFamilyStyleCharacter (const char familyName[], const SkFontStyle& style, const char* bcp47[], int bcp47Count, SkUnichar character) const override;

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;

protected:
	void registerFallback (StringRef name);
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (FontManager, kFrameworkLevelLast)
{
	SkiaFontCache::instance ().setFontChangesSupported (true);
	return true;
}

//************************************************************************************************
// SkiaFontManagerFactory
//************************************************************************************************

sk_sp<SkFontMgr> SkiaFontManagerFactory::createFontManager ()
{
	static sk_sp<SkFontMgr> theManager = WasmFontManager::create ();
	return theManager;
}

//************************************************************************************************
// WasmFontManager
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontsPath, "/fonts")

DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackJP, "CCLFallbackJP")
DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackSC, "CCLFallbackSC")
DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackTC, "CCLFallbackTC")
DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackKR, "CCLFallbackKR")
DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackEmoji, "CCLFallbackEmoji")
DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackSymbols, "CCLFallbackSymbols")
DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackSymbols2, "CCLFallbackSymbols2")
DEFINE_STRINGID_MEMBER_ (WasmFontManager, kFontFallbackGeneric, "CCLFallbackGeneric")

//////////////////////////////////////////////////////////////////////////////////////////////

WasmFontManager::WasmFontManager ()
{
	SystemEventDelegate::instance ().registerEventHandler ("fonts", *this);

	fontManager = SkFontMgr_New_Custom_Directory (kFontsPath);
	#if DEBUG && DEBUG_LOG
	for(int i = 0; i < fontManager->countFamilies (); i++)
	{
		SkString familyName;
		fontManager->getFamilyName (i, &familyName);
		CCL_PRINTF ("Found font family \"%s\"\n", familyName.c_str ())
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////

WasmFontManager::~WasmFontManager ()
{
	SystemEventDelegate::instance ().unregisterEventHandler (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontMgr> WasmFontManager::create ()
{
	return sk_make_sp<WasmFontManager> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkTypeface> WasmFontManager::onMatchFamilyStyleCharacter (const char familyName[], const SkFontStyle& style, const char* bcp47[], int bcp47Count, SkUnichar character) const
{
	if(character == '\t' || character == '\n' || character == '\r')
		return nullptr;

	MutableCString fallbackName;
	for(int i = 0; i < bcp47Count; i++)
	{
		CString code (bcp47[i]);
		if(code.contains ("Zsye"))
		{
			fallbackName = kFontFallbackEmoji;
			break;
		}
		if(code.startsWith ("ja"))
		{
			fallbackName = kFontFallbackJP;
			break;
		}
		if(code.contains ("Hans"))
		{
			fallbackName = kFontFallbackSC;
			break;
		}
		if(code.contains ("Hant"))
		{
			fallbackName = kFontFallbackTC;
			break;
		}
		if(code.startsWith ("ko"))
		{
			fallbackName = kFontFallbackKR;
			break;
		}
	}
	if(fallbackName.isEmpty ())
	{
		if(character >= 0x3040 && character <= 0x30FF)
			fallbackName = kFontFallbackJP;
		else if(character >= 0xAC00 && character <= 0xD7AF)
			fallbackName = kFontFallbackKR;
		else if(character >= 0x1F300 && character <= 0x1FAFF)
			fallbackName = kFontFallbackEmoji;
		else if(character >= 0x2460 && character <= 0x24FF) // Enclosed Alphanumerics
			fallbackName = kFontFallbackSymbols;
		else if((character >= 0x25A0 && character <= 0x25FF) // Geometric Shapes
			 || (character >= 0x2800 && character <= 0x28FF)) // Braille Patterns
			fallbackName = kFontFallbackSymbols2;
		else if(character >= 0x4E00 && character <= 0x9FFF)
		{
			StringID language = System::GetLocaleManager ().getSystemLanguage ();
			if(language.startsWith ("ko"))
				fallbackName = kFontFallbackKR;
			else if(language.startsWith ("zh"))
				fallbackName = kFontFallbackSC;
			else
				fallbackName = kFontFallbackJP;
		}
	}

	if(fallbackName.isEmpty ())
		fallbackName = kFontFallbackGeneric;

	for(const auto& entry : fontCache)
	{
		if(entry->getName () == fallbackName)
		{
			if(entry->count () > 0)
				return entry->createTypeface (0);
			return nullptr; // already requested
		}
	}
	
	// font not found, try to load via js
	sk_sp<StyleSet> set = sk_make_sp<StyleSet> ();
	set->setName (fallbackName);
	fontCache.add (set);
	loadFallbackFont (fallbackName);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void WasmFontManager::registerFallback (StringRef _name)
{
	MutableCString name (_name, Text::kUTF8);

	MutableCString path (kFontsPath);
	path.append ("/");
	path.append (name);
	path.append (".ttf");
	sk_sp<SkTypeface> typeface = fontManager->makeFromFile (path);
	if(typeface)
	{
		for(const auto& entry : fontCache)
		{
			if(entry->getName () == name)
			{
				entry->add (typeface);
				SkiaFontCache::instance ().invalidate ();
				Desktop.redrawAll ();
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

EventResult WasmFontManager::handleEvent (SystemEvent& e)
{
	if(e.eventId == "fallback-loaded")
		registerFallback (e.arg1);
	return nullptr;
}