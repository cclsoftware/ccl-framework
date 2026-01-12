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
// Filename    : ccl/platform/win/system/localemanager.win.cpp
// Description : Locale Manager (Win32)
//
//************************************************************************************************

#include "ccl/system/localization/localemanager.h"

#include "ccl/public/text/language.h"
#include "ccl/public/cclversion.h"

#include "ccl/platform/win/system/registry.h"
#include "ccl/platform/win/cclwindows.h"

namespace CCL {

//************************************************************************************************
// WindowsLocaleManager
//************************************************************************************************

class WindowsLocaleManager: public LocaleManager
{
public:
	// LocaleManager
	StringID CCL_API getSystemLanguage () const override;
	StringID CCL_API getInputLanguage () const override;
	StringID CCL_API getSystemRegion () const override;
	uchar CCL_API getCharacterOnKey (uchar characterUS, tbool withCapsLock = false) const override;
	StringID CCL_API getMeasureSystem () const override;

protected:
	mutable FixedSizeVector<UINT, 256> scanCodesEnglishUS; // scan codes in English-US layout for virtual-key codes 0..255

	// LocaleManager
	bool getNativeUserLanguage (MutableCString& language) const override;
	void setNativeUserLanguage (StringID language) override;
	void setNativeLanguagePack (StringRef pathString) override;
	bool getNativeLanguagePack (String& pathString) const override;
	void collectGeographicRegions (GeographicRegionList& list) const override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_LOCALE_ROOT "Software\\" CCL_SETTINGS_NAME "\\Locales"
#define CCL_LANGUAGEPACK_ROOT "Software\\" CCL_SETTINGS_NAME "\\LanguagePacks"

//////////////////////////////////////////////////////////////////////////////////////////////////

static StringID getPrimaryLanguageCodeFromLANGID (LANGID langid)
{
	switch(PRIMARYLANGID (langid))
	{
	case LANG_ENGLISH    : return LanguageCode::English;
	case LANG_GERMAN     : return LanguageCode::German;
	case LANG_FRENCH     : return LanguageCode::French;
	case LANG_SPANISH    : return LanguageCode::Spanish;
	case LANG_ITALIAN    : return LanguageCode::Italian;
	case LANG_PORTUGUESE : return LanguageCode::Portuguese;
	case LANG_JAPANESE   : return LanguageCode::Japanese;
	case LANG_CHINESE    : return LanguageCode::Chinese;
	}

	return LanguageCode::Neutral;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static StringID getSubLanguageCodeFromLANGID (LANGID langid)
{
	switch(PRIMARYLANGID (langid))
	{
	case LANG_GERMAN :
		switch(SUBLANGID (langid))
		{
		case SUBLANG_GERMAN : return LanguageCode::GermanDE;
		}
		return LanguageCode::German;

	case LANG_ENGLISH :
		switch(SUBLANGID (langid))
		{
		case SUBLANG_ENGLISH_US : return LanguageCode::EnglishUS;
		}
		return LanguageCode::English;
	}

	return getPrimaryLanguageCodeFromLANGID (langid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool isValidIso2CountryCode (StringRef iso2Code)
{
	static const String invariantCode ("IV");
	if(iso2Code.compare (invariantCode, false) == Text::kEqual)
		return false;

	return Unicode::isAlpha (iso2Code.firstChar ());
}

//************************************************************************************************
// WindowsLocaleManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (LocaleManager, WindowsLocaleManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WindowsLocaleManager::getSystemLanguage () const
{
	// Make sure to use UI language set by user, not locale (e.g. English system in Germany)
	// See https://msdn.microsoft.com/en-us/library/windows/desktop/dd319088(v=vs.85).aspx
	LANGID langid = ::GetUserDefaultUILanguage ();
	return getPrimaryLanguageCodeFromLANGID (langid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WindowsLocaleManager::getInputLanguage () const
{
	HKL hkl = ::GetKeyboardLayout (0);
	LCID lcid = LOWORD (hkl);
	return getSubLanguageCodeFromLANGID (LANGIDFROMLCID (lcid));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsLocaleManager::getNativeUserLanguage (MutableCString& language) const
{
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_LOCALE_ROOT);

	String value;
	String name (Registry::AppValueName ());
	accessor.readString (value, String::kEmpty, name);

	language = MutableCString (value);
	return !language.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsLocaleManager::setNativeUserLanguage (StringID language)
{
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_LOCALE_ROOT);
	String name (Registry::AppValueName ());
	accessor.writeString (String (language), String::kEmpty, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsLocaleManager::setNativeLanguagePack (StringRef pathString)
{
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_LANGUAGEPACK_ROOT);
	String name (Registry::AppValueName ());
	accessor.writeString (pathString, String::kEmpty, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsLocaleManager::getNativeLanguagePack (String& pathString) const
{
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_LANGUAGEPACK_ROOT);

	String value;
	String name (Registry::AppValueName ());
	accessor.readString (value, String::kEmpty, name);

	pathString = value;
	return !pathString.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WindowsLocaleManager::getSystemRegion () const
{
	static MutableCString systemRegion;
	if(systemRegion.isEmpty ())
	{
		String result;
		TCHAR str[128] = {};
		::GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, str, 127);
		result = str;

		// Note: Sometimes UN M49 (https://en.wikipedia.org/wiki/UN_M49) codes are returned 
		// instead of ISO 3166-1 country codes, e.g. 1 for World, 150 for Europe, 419 for Latin America, etc.
		if(!isValidIso2CountryCode (result))
			result.empty ();

		if(!result.isEmpty ())
			systemRegion = MutableCString (result);
		else
			systemRegion = CountryCode::kUS; // fall back to US;
	}
	return systemRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsLocaleManager::collectGeographicRegions (GeographicRegionList& list) const
{
	struct CB
	{
		static BOOL CALLBACK EnumSystemLocaleProc (LPWSTR lpLocaleString,  DWORD dwFlags, LPARAM lParam)
		{
			GeographicRegionList* list = reinterpret_cast<GeographicRegionList*> (lParam);

			TCHAR str[128] = {};
			::GetLocaleInfoEx (lpLocaleString, LOCALE_SENGLISHCOUNTRYNAME, str, 127);
			String englishName (str);

			if(list->containsRegion (englishName) == false)
			{
				AutoPtr<GeographicRegion> region = NEW GeographicRegion;
				region->englishName = englishName;

				str[0] = 0;
				::GetLocaleInfoEx (lpLocaleString, LOCALE_SISO3166CTRYNAME, str, 127);
				region->iso2Code = str;

				// some have only numbers (UN M49), some are pseudo (invariant)...
				if(isValidIso2CountryCode (region->iso2Code))
				{
					str[0] = 0;
					::GetLocaleInfoEx (lpLocaleString, LOCALE_SNATIVECOUNTRYNAME, str, 127);
					region->nativeName = str;

					str[0] = 0;
					::GetLocaleInfoEx (lpLocaleString, LOCALE_SLOCALIZEDCOUNTRYNAME, str, 127);
					region->localizedName = str;

					list->addSorted (region.detach ());
				}
				#if (0 && DEBUG)
				else
				{
					Debugger::print ("Locale ignored: ");
					Debugger::println (region->iso2Code);
				}
				#endif
			}
			return TRUE;
		}
	};

	::EnumSystemLocalesEx (CB::EnumSystemLocaleProc, LOCALE_ALL, reinterpret_cast<LPARAM>(&list), nullptr);

	// EnumSystemLocalesEx does not list all - so use EnumSystemGeoID to get the rest (native and localized name are not available)
	static GeographicRegionList* theList = nullptr;
	struct CB2
	{
		static BOOL CALLBACK EnumGeoInfoProc (GEOID geoID)
		{
			TCHAR iso2 [12] = {};
			::GetGeoInfo (geoID, GEO_ISO2, iso2, 12, LANG_NEUTRAL);
			String iso2Code (iso2);
			if(theList->containsRegionISO (iso2Code) == false)
			{
				TCHAR name [128] = {};
				::GetGeoInfo (geoID, GEO_FRIENDLYNAME, name, 128, 0);

				AutoPtr<GeographicRegion> region = NEW GeographicRegion;
				region->englishName = name;
				region->nativeName = region->englishName;
				region->localizedName = region->englishName;
				region->iso2Code = iso2;

				theList->addSorted (region.detach ());
			}

			return TRUE;
		}
	};

	ScopedVar<GeographicRegionList*> scope (theList, &list);
	::EnumSystemGeoID (GEOCLASS_NATION, 0, CB2::EnumGeoInfoProc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API WindowsLocaleManager::getCharacterOnKey (uchar characterUS, tbool withCapsLock) const
{
	if(!Unicode::isAlphaNumeric (characterUS))
		return characterUS;

	HKL currentKeyboardLayout = ::GetKeyboardLayout (0);

	// on first use, build table with scanCodes in English-US
	if(scanCodesEnglishUS.isEmpty ())
	{
		// get list of all keyboard layouts
		int numLayouts = ::GetKeyboardLayoutList (0, nullptr);
		Vector<HKL> layoutList (numLayouts);
		numLayouts = ::GetKeyboardLayoutList (layoutList.getCapacity (), layoutList);
		layoutList.setCount (numLayouts);

		// load US-Eng keyboard layout
		static const char kLayoutID_US[] = "00000409";
		HKL layoutUS = ::LoadKeyboardLayoutA (kLayoutID_US, KLF_NOTELLSHELL);

		// note: for letters and digits in the US layout, the virtual key code is the same as the character
		for(int vkey = 0; vkey <= 255; vkey++)
		{
			UINT scanCodeUS = ::MapVirtualKeyEx (vkey, MAPVK_VK_TO_VSC, layoutUS);
			scanCodesEnglishUS.add (scanCodeUS);
			CCL_PRINTF ("character %d (%c): \tscanCodeUS us %d\n", vkey, vkey, scanCodeUS)
		}

		// unload if it wasn't in the list before
		if(!layoutList.contains (layoutUS))
		{
			CCL_PRINTF ("unload US-layout\n")
			::UnloadKeyboardLayout (layoutUS);
		}

		// activate previous layout again
		::ActivateKeyboardLayout (currentKeyboardLayout, 0);
	}

	characterUS = Unicode::toUppercase (characterUS);

	// map scan code to virtual key in current layout
	int index = characterUS;
	UINT scanCode = scanCodesEnglishUS.at (index);
	UINT vk = ::MapVirtualKeyEx (scanCode, MAPVK_VSC_TO_VK, currentKeyboardLayout);

	BYTE keyboardState[256] = {};
	if(withCapsLock)
		keyboardState[VK_SHIFT] = 0x80; // (does not work as expected with VK_CAPITAL)

	WCHAR characters[4];
	int result = ::ToUnicodeEx (vk, scanCode, keyboardState, characters, 4, 0, currentKeyboardLayout);

	uchar character = Unicode::toUppercase (characters[0]);
	CCL_PRINTF ("characterUS %d (%c): \tscanCode %d \tcharacter %c (result %d)\n", characterUS, characterUS, scanCode, character, result)
	return character;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WindowsLocaleManager::getMeasureSystem () const
{
	TCHAR str[2] = {0};
	::GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT, LOCALE_IMEASURE, str, 2);
	if(str[0] == '1')
		return MeasureID::kMeasureUS;
	
	return MeasureID::kMeasureSI;
}
