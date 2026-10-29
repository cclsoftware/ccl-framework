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
// Filename    : ccl/platform/shared/posix/system/localemanager.posix.cpp
// Description : POSIX Locale Manager
//
//************************************************************************************************

#include "ccl/platform/shared/posix/system/localemanager.posix.h"

#include "ccl/base/storage/settings.h"

#include "ccl/public/text/language.h"
#include "ccl/public/cclversion.h"

using namespace CCL;

//************************************************************************************************
// PosixLocaleManager
//************************************************************************************************

const String PosixLocaleManager::kSettingsFileName = "LocaleManager";

DEFINE_STRINGID_MEMBER_ (PosixLocaleManager, kUserLanguage, "userLanguage")
DEFINE_STRINGID_MEMBER_ (PosixLocaleManager, kLanguagePack, "languagePack")

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID PosixLocaleManager::getPrimaryLanguageCode (CStringPtr _langid)
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

StringID PosixLocaleManager::getSubLanguageCode (CStringPtr _langid)
{
	CString langid (_langid);
	if(langid.startsWith ("de_DE") || langid.startsWith ("de-DE")) return LanguageCode::GermanDE;
	if(langid.startsWith ("en_US") || langid.startsWith ("en-US")) return LanguageCode::EnglishUS;
	return getPrimaryLanguageCode (_langid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixLocaleManager::getNativeUserLanguage (MutableCString& language) const
{
	restoreSettings ();
	language = userLanguage;
	return !userLanguage.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixLocaleManager::setNativeUserLanguage (StringID language)
{
	userLanguage = language;
	storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixLocaleManager::getNativeLanguagePack (String& pathString) const
{
	restoreSettings ();
	pathString = languagePack;
	return !languagePack.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixLocaleManager::setNativeLanguagePack (StringRef pathString)
{
	languagePack = pathString;
	storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixLocaleManager::storeSettings () const
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

void PosixLocaleManager::restoreSettings () const
{
	XmlSettings settings (kSettingsFileName);
	settings.init ("", CCL_SETTINGS_NAME, kSettingsFileName);
	settings.restore ();
	Attributes& a = settings.getAttributes (kSettingsFileName);
	a.get (userLanguage, kUserLanguage, Text::kUTF8);
	a.get (languagePack, kLanguagePack);
}
