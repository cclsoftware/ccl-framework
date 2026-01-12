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
// Filename    : ccl/platform/linux/system/localemanager.linux.cpp
// Description : Linux Locale Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/localization/localemanager.h"

#include "ccl/platform/linux/interfaces/iinputlocale.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/language.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#include <langinfo.h>
#include <locale>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-names.h>

namespace CCL {

//************************************************************************************************
// LinuxLocaleManager
//************************************************************************************************

class LinuxLocaleManager: public LocaleManager,
						  public Linux::IInputLocale
{
public:
	LinuxLocaleManager ();
	~LinuxLocaleManager ();
	
	DECLARE_STRINGID_MEMBER (kUserLanguage)
	DECLARE_STRINGID_MEMBER (kLanguagePack)
	
	// LocaleManager
	StringID CCL_API getSystemLanguage () const override;
	StringID CCL_API getInputLanguage () const override;
	StringID CCL_API getSystemRegion () const override;
	uchar CCL_API getCharacterOnKey (uchar characterUS, tbool withCapsLock = false) const override;
	StringID CCL_API getMeasureSystem () const override;

	// IInputLocale
	void CCL_API setKeyMap (xkb_keymap* keyMap) override;
	const char* CCL_API getInputLocale () const override;
	
	CLASS_INTERFACE (IInputLocale, LocaleManager)
	
protected:
	static const String kSettingsFileName;
	
	mutable FixedSizeVector<xkb_keysym_t, 256> keySymsEnglishUS; // keysyms in English-US layout for key codes 0..255
	mutable xkb_keymap* currentKeyMap;
	mutable xkb_context* xkbContext;
	
	mutable MutableCString userLanguage;
	mutable String languagePack;	
	
	locale_t systemLocale;
	
	void storeSettings () const;
	void restoreSettings () const;

	void getSystemLanguageCode (MutableCString& language) const;
	
	// LocaleManager
	bool getNativeUserLanguage (MutableCString& language) const override;
	void setNativeUserLanguage (StringID language) override;
	void setNativeLanguagePack (StringRef pathString) override;
	bool getNativeLanguagePack (String& pathString) const override;
	void collectGeographicRegions (GeographicRegionList& list) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static StringID getPrimaryLanguageCode (const char* _langid)
{
	CString langid (_langid);
	if(langid.startsWith ("en")) return LanguageCode::English;
	if(langid.startsWith ("de")) return LanguageCode::German;
	if(langid.startsWith ("fr")) return LanguageCode::French;
	if(langid.startsWith ("es")) return LanguageCode::Spanish;
	if(langid.startsWith ("it")) return LanguageCode::Italian;
	if(langid.startsWith ("pt")) return LanguageCode::Portuguese;
	if(langid.startsWith ("ja")) return LanguageCode::Japanese;
	if(langid.startsWith ("zh")) return LanguageCode::Chinese;
	return LanguageCode::Neutral;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static StringID getSubLanguageCode (const char* _langid)
{
	CString langid (_langid);
	if(langid.startsWith ("en_DE")) return LanguageCode::GermanDE;
	if(langid.startsWith ("en_US")) return LanguageCode::EnglishUS;
	return getPrimaryLanguageCode (_langid);
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// LinuxLocaleManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (LocaleManager, LinuxLocaleManager)

const String LinuxLocaleManager::kSettingsFileName = "LocaleManager";

DEFINE_STRINGID_MEMBER_ (LinuxLocaleManager, kUserLanguage, "userLanguage")
DEFINE_STRINGID_MEMBER_ (LinuxLocaleManager, kLanguagePack, "languagePack")
	
//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxLocaleManager::LinuxLocaleManager ()
: currentKeyMap (nullptr),
  xkbContext (nullptr)
{
	systemLocale = ::newlocale (LC_ALL_MASK, "", nullptr);
	
	CCL_PRINTF ("System region: %s\n", getSystemRegion ().str ())
	CCL_PRINTF ("System language: %s\n", getSystemLanguage ().str ())
	CCL_PRINTF ("Input language: %s\n", getInputLanguage ().str ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxLocaleManager::~LinuxLocaleManager ()
{
	if(systemLocale)
		::freelocale (systemLocale);
	if(currentKeyMap != nullptr)
		xkb_keymap_unref (currentKeyMap);
	if(xkbContext != nullptr)
		xkb_context_unref (xkbContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxLocaleManager::getSystemLanguageCode (MutableCString& language) const
{
	CStringPtr lang = ::getenv ("LANGUAGE");
	if(lang == nullptr)
		lang = ::getenv ("LANG");
	if(lang == nullptr && systemLocale != nullptr)
		lang = ::nl_langinfo_l (_NL_ADDRESS_LANG_AB, systemLocale);
	language = lang;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LinuxLocaleManager::getSystemLanguage () const
{
	MutableCString language;
	getSystemLanguageCode (language);
	return getPrimaryLanguageCode (language);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LinuxLocaleManager::getInputLanguage () const
{
	return getPrimaryLanguageCode (getInputLocale ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LinuxLocaleManager::getMeasureSystem () const
{
	CStringPtr measurement = nullptr;
	if(systemLocale != nullptr)
		measurement = ::nl_langinfo_l (_NL_MEASUREMENT_MEASUREMENT, systemLocale);
	if(measurement != nullptr)
		return (measurement[0] == 2) ? MeasureID::kMeasureUS : MeasureID::kMeasureSI;
	
	if(measurement == nullptr && systemLocale != nullptr)
	{
		measurement = systemLocale->__names[LC_MEASUREMENT];
		if(measurement != nullptr)
			return (getSubLanguageCode (measurement) == LanguageCode::EnglishUS) ? MeasureID::kMeasureUS : MeasureID::kMeasureSI;
	}

	MutableCString language;
	getSystemLanguageCode (language);
	return (getSubLanguageCode (language) == LanguageCode::EnglishUS) ? MeasureID::kMeasureUS : MeasureID::kMeasureSI;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API LinuxLocaleManager::getInputLocale () const
{
	if(systemLocale)
		return systemLocale->__names[LC_CTYPE];
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxLocaleManager::getNativeUserLanguage (MutableCString& language) const
{
	restoreSettings ();
	language = userLanguage;
	return !userLanguage.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxLocaleManager::setNativeUserLanguage (StringID language)
{
	userLanguage = language;
	storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxLocaleManager::getNativeLanguagePack (String& pathString) const
{
	restoreSettings ();
	pathString = languagePack;
	return !languagePack.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxLocaleManager::setNativeLanguagePack (StringRef pathString)
{
	languagePack = pathString;
	storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LinuxLocaleManager::getSystemRegion () const
{
	static MutableCString systemRegion;
	if(systemRegion.isEmpty ())
	{
		if(systemLocale)
			systemRegion = ::nl_langinfo_l (_NL_ADDRESS_COUNTRY_AB2, systemLocale);
		else
			systemRegion = CountryCode::kUS; // fall back to US
	}
	return systemRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxLocaleManager::collectGeographicRegions (GeographicRegionList& list) const
{
	Url localesFolder;
	localesFolder.fromPOSIXPath ("/usr/share/i18n/locales", Url::kFolder);
	ForEachFile (System::GetFileSystem ().newIterator (localesFolder, IFileIterator::kFiles), file)
		String localeName;
		file->getName (localeName);
		
		locale_t locale = ::newlocale (LC_ALL_MASK, MutableCString (localeName, Text::kSystemEncoding), nullptr);
		if(locale == nullptr)
		{
			localeName.append (".utf8");
			locale = ::newlocale (LC_ALL_MASK, MutableCString (localeName, Text::kSystemEncoding), nullptr);
		}
		if(locale == nullptr)
			continue;
		
		AutoPtr<GeographicRegion> region = NEW GeographicRegion;
		region->englishName = String (Text::kSystemEncoding, ::nl_langinfo_l (_NL_IDENTIFICATION_TERRITORY, locale));
		region->nativeName = String (Text::kSystemEncoding, ::nl_langinfo_l (_NL_ADDRESS_COUNTRY_NAME, locale));
		if(region->nativeName.isEmpty ())
			region->nativeName = region->englishName;
		region->iso2Code = String (Text::kSystemEncoding, ::nl_langinfo_l (_NL_ADDRESS_COUNTRY_AB2, locale));
		region->localizedName = String (getSystemRegion ()) == region->iso2Code ? region->nativeName : region->englishName;
		
		list.addSorted (region.detach ());
		
		::freelocale (locale);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API LinuxLocaleManager::getCharacterOnKey (uchar characterUS, tbool withCapsLock) const
{
	if(!Unicode::isAlphaNumeric (characterUS))
		return characterUS;

	// on first use, build table with scanCodes in English-US
	if(keySymsEnglishUS.isEmpty ())
	{
		xkb_context* context = xkb_context_new (XKB_CONTEXT_NO_FLAGS);
		
		xkb_rule_names names;
		names.rules = "evdev";
		names.model = "pc105";
		names.layout = "us";
		names.variant = "";
		names.options = "";
		
		xkb_keymap* keyMap = xkb_keymap_new_from_names (context, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
		
		for(xkb_keycode_t keyCode = 0; keyCode < 256; keyCode++)
		{
			const xkb_keysym_t* symbols = nullptr;
			int count = xkb_keymap_key_get_syms_by_level (keyMap, keyCode, 0, 0, &symbols);
			if(count > 0)
				keySymsEnglishUS.add (symbols[0]);
			else
				keySymsEnglishUS.add (-1);
		}
		
		xkb_keymap_unref (keyMap);
		xkb_context_unref (context);
	}
	
	// For the allowed input range of this function, keysyms match directly to ASCII characters
	xkb_keysym_t keySym = characterUS;
	xkb_keycode_t keyCode = keySymsEnglishUS.index (keySym);
	if(keyCode > 255)
		return characterUS;
	
	if(currentKeyMap == nullptr)
	{
		if(xkbContext == nullptr)
			xkbContext = xkb_context_new (XKB_CONTEXT_NO_FLAGS);
		currentKeyMap = xkb_keymap_new_from_names (xkbContext, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
	}
	
	const xkb_keysym_t* symbols = nullptr;
	int count = xkb_keymap_key_get_syms_by_level (currentKeyMap, keyCode, 0, withCapsLock ? 1 : 0, &symbols);
	if(count > 0)
		return xkb_keysym_to_utf32 (symbols[0]);
	
	return characterUS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxLocaleManager::setKeyMap (xkb_keymap* keyMap)
{
	if(currentKeyMap != nullptr)
		xkb_keymap_unref (currentKeyMap);
	currentKeyMap = keyMap;
	if(currentKeyMap)
	{
		xkb_keymap_ref (currentKeyMap);
		CCL_PRINTF ("Received new keymap: %s\n", xkb_keymap_layout_get_name (keyMap, 0));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxLocaleManager::storeSettings () const
{
	XmlSettings settings (kSettingsFileName);
	settings.init ("", CCL_SETTINGS_NAME, kSettingsFileName);
	settings.removeAll ();
	
	Attributes& a = settings.getAttributes (kSettingsFileName);
	a.set (kUserLanguage, userLanguage, Text::kUTF8);
	a.set (kLanguagePack, languagePack);

	settings.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxLocaleManager::restoreSettings () const
{
	XmlSettings settings (kSettingsFileName);
	settings.init ("", CCL_SETTINGS_NAME, kSettingsFileName);
	settings.restore ();
	Attributes& a = settings.getAttributes (kSettingsFileName);
	a.get (userLanguage, kUserLanguage, Text::kUTF8);
	a.get (languagePack, kLanguagePack);
}
