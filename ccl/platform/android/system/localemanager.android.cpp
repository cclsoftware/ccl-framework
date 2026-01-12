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
// Filename    : ccl/platform/android/system/localemanager.android.cpp
// Description : Locale Manager (Android)
//
//************************************************************************************************

#include "ccl/system/localization/localemanager.h"

#include "ccl/platform/android/system/assetfilesystem.h"
#include "ccl/platform/android/system/system.android.h"

#include "ccl/base/storage/settings.h"

#include "ccl/public/text/language.h"
#include "ccl/public/cclversion.h"

#include <android/configuration.h>

namespace CCL {

//************************************************************************************************
// AndroidLocaleManager
//************************************************************************************************

class AndroidLocaleManager: public LocaleManager
{
public:
	// ILocaleManager
	StringID CCL_API getSystemLanguage () const override;
	StringID CCL_API getInputLanguage () const override;
	StringID CCL_API getSystemRegion () const override;
	void CCL_API getLanguagesFolder (IUrl& url) const override;

protected:
	static const String kSettingsFileName;

	DECLARE_STRINGID_MEMBER (kLanguageCode)
	DECLARE_STRINGID_MEMBER (kLanguagePack)

	mutable MutableCString languageCode;
	mutable String languagePack;

	void storeSettings () const;
	void restoreSettings () const;

	// LocaleManager
	bool getNativeUserLanguage (MutableCString& language) const override;
	void setNativeUserLanguage (StringID language) override;
	void setNativeLanguagePack (StringRef pathString) override;
	bool getNativeLanguagePack (String& pathString) const override;
	void collectGeographicRegions (GeographicRegionList& list) const override;
};

namespace Android {

//************************************************************************************************
// java.util.Locale
//************************************************************************************************

DECLARE_JNI_CLASS (Locale, "java/util/Locale")
	DECLARE_JNI_STATIC_METHOD (jobject, forLanguageTag, jstring)
	DECLARE_JNI_STATIC_METHOD (jobjectArray, getAvailableLocales)
	DECLARE_JNI_STATIC_METHOD (jobject, getDefault)
	DECLARE_JNI_METHOD (jstring, getCountry)
	DECLARE_JNI_METHOD (jstring, getDisplayCountry, jobject)
END_DECLARE_JNI_CLASS (Locale)

DEFINE_JNI_CLASS (Locale)
	DEFINE_JNI_STATIC_METHOD (forLanguageTag, "(Ljava/lang/String;)Ljava/util/Locale;")
	DEFINE_JNI_STATIC_METHOD (getAvailableLocales, "()[Ljava/util/Locale;")
	DEFINE_JNI_STATIC_METHOD (getDefault, "()Ljava/util/Locale;")
	DEFINE_JNI_METHOD (getCountry, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getDisplayCountry, "(Ljava/util/Locale;)Ljava/lang/String;")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;
using namespace Core::Java;

//************************************************************************************************
// AndroidLocaleManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (LocaleManager, AndroidLocaleManager)

const String AndroidLocaleManager::kSettingsFileName = "LocaleManager";

DEFINE_STRINGID_MEMBER_ (AndroidLocaleManager, kLanguageCode, "languageCode")
DEFINE_STRINGID_MEMBER_ (AndroidLocaleManager, kLanguagePack, "languagePack")

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API AndroidLocaleManager::getSystemLanguage () const
{
    static MutableCString systemLanguage;
	if(systemLanguage.isEmpty ())
	{
		char code[2] = {};
		AConfiguration_getLanguage (AndroidSystemInformation::getInstance ().getConfiguration (), code);
		systemLanguage.append (code, 2);
	}
	return systemLanguage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API AndroidLocaleManager::getSystemRegion () const
{
	static MutableCString systemRegion;
	if(systemRegion.isEmpty ())
	{
		char code[3] = {};
		AConfiguration_getCountry (AndroidSystemInformation::getInstance ().getConfiguration (), code);
		systemRegion.append (code);
	}
	return systemRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidLocaleManager::getLanguagesFolder (IUrl& url) const
{
	// "assets/Languages"
	url = AssetUrl ("Languages", IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidLocaleManager::getNativeUserLanguage (MutableCString& language) const
{
	restoreSettings ();

	language = languageCode;

	return !languageCode.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API AndroidLocaleManager::getInputLanguage () const
{
	return LanguageCode::Neutral;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidLocaleManager::setNativeUserLanguage (StringID language)
{
	languageCode = language;

	storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidLocaleManager::setNativeLanguagePack (StringRef pathString)
{
	languagePack = pathString;

	storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidLocaleManager::getNativeLanguagePack (String& pathString) const
{
	restoreSettings ();

	if(languagePack.isEmpty ())
		return false;

	// create proper asset URL from stored language pack path
	AssetUrl path (languagePack);
	path.getUrl (pathString);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidLocaleManager::collectGeographicRegions (GeographicRegionList& list) const
{
	JniAccessor jni;
	JniString englishTag (jni, "en");
	LocalRef englishLocale (jni, Locale.forLanguageTag (englishTag));
	LocalRef systemLocale (jni, Locale.getDefault ());
	JniObjectArray locales (jni, Locale.getAvailableLocales ());
	for(int i = 0; i < locales.getLength (); i++)
	{
		LocalRef locale (jni, locales[i]);
		String iso2Code = fromJavaString (LocalStringRef (jni, Locale.getCountry (locale)));
		if(list.containsRegionISO (iso2Code) || !Unicode::isAlpha (iso2Code.firstChar ()))
			continue;

		if((iso2Code[0] == 'Q' && iso2Code[1] >= 'M') || // reserved codes QM to QZ
		   (iso2Code[0] == 'X' && iso2Code[1] != 'K') || // reserved codes XA to XZ, but XK used for Kosovo
		   iso2Code == "AA" || iso2Code == "ZZ") // reserved codes AA and ZZ
			continue;

		AutoPtr<GeographicRegion> region = NEW GeographicRegion;
		region->iso2Code = iso2Code;
		region->englishName = fromJavaString (LocalStringRef (jni, Locale.getDisplayCountry (locale, englishLocale)));

		region->nativeName = fromJavaString (LocalStringRef (jni, Locale.getDisplayCountry (locale, locale)));
		if(region->nativeName.isEmpty ())
			region->nativeName = region->englishName;

		region->localizedName = fromJavaString (LocalStringRef (jni, Locale.getDisplayCountry (locale, systemLocale)));
		if(region->localizedName.isEmpty ())
			region->localizedName = region->englishName;

		list.addSorted (region.detach ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidLocaleManager::storeSettings () const
{
	XmlSettings settings (kSettingsFileName);
	settings.init (String::kEmpty, CCL_SETTINGS_NAME, kSettingsFileName);
	settings.removeAll ();

	Attributes& a = settings.getAttributes (kSettingsFileName);
	a.set (kLanguageCode, languageCode);
	a.set (kLanguagePack, languagePack);

	settings.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidLocaleManager::restoreSettings () const
{
	XmlSettings settings (kSettingsFileName);
	settings.init (String::kEmpty, CCL_SETTINGS_NAME, kSettingsFileName);
	settings.restore ();

	Attributes& a = settings.getAttributes (kSettingsFileName);
	a.get (languageCode, kLanguageCode);
	a.get (languagePack, kLanguagePack);
}
