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
// Filename    : ccl/platform/cocoa/system/localemanager.cocoa.mm
// Description : Locale Manager (macOS/iOS)
//
//************************************************************************************************

#include "ccl/system/localization/localemanager.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/public/cclversion.h"
#include "ccl/public/text/language.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#ifndef CCL_PLATFORM_IOS
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/cclcarbon.h"
#endif

#include "ccl/platform/cocoa/interfaces/icocoalocalemanager.h"

namespace CCL {

//************************************************************************************************
// CocoaLocaleManager
//************************************************************************************************

class CocoaLocaleManager: public LocaleManager,
						  public MacOS::ICocoaLocaleManager
{
public:
	CocoaLocaleManager ();
	~CocoaLocaleManager ();

	// ILocaleManager
	StringID CCL_API getSystemLanguage () const override;
	StringID CCL_API getInputLanguage () const override;
	StringID CCL_API getSystemRegion () const override;
	uchar CCL_API getCharacterOnKey (uchar characterUS, tbool withCapsLock = false) const override;

	// ICocoaLocaleManager
	tbool CCL_API getSystemKeyForCharacter (uint16& sysKey, uchar characterLocal) const override;
	
	CLASS_INTERFACE (ICocoaLocaleManager, LocaleManager)

protected:
	struct KeyLayoutMap
	{
		UInt16 sysKey;
		uchar characterUS;
		uchar characterLocal;
		uchar characterLocalCapsLock;
	};
	static KeyLayoutMap keyLayoutMap[];

	MutableCString inputLanguage;
	id<NSObject> observer;

	void updateInputLanguage ();

	// LocaleManager
	bool getNativeUserLanguage (MutableCString& language) const override;
	void setNativeUserLanguage (StringID language) override;
	void setNativeLanguagePack (StringRef pathString) override;
	bool getNativeLanguagePack (String& pathString) const override;
	void collectGeographicRegions (GeographicRegionList& list) const override;
};

} // namespace CCL

using namespace CCL;

static NSString* CreatePreferenceKey (StringRef key);
static void WritePreferenceString (StringRef file, CStringRef data);
static bool ReadPreferenceString (StringRef file, MutableCString& string);
static bool GetGeographicRegion (GeographicRegion& region, NSLocale* locale);

//************************************************************************************************
// CocoaLocaleManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (LocaleManager, CocoaLocaleManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CCL_PLATFORM_IOS

CocoaLocaleManager::KeyLayoutMap CocoaLocaleManager::keyLayoutMap[] =
{
		{kVK_ANSI_1, '1', '1', '1'},
		{kVK_ANSI_2, '2', '2', '2'},
		{kVK_ANSI_3, '3', '3', '3'},
		{kVK_ANSI_4, '4', '4', '4'},
		{kVK_ANSI_5, '5', '5', '5'},
		{kVK_ANSI_6, '6', '6', '6'},
		{kVK_ANSI_7, '7', '7', '7'},
		{kVK_ANSI_8, '8', '8', '8'},
		{kVK_ANSI_9, '9', '9', '9'},
		{kVK_ANSI_0, '0', '0', '0'},

		{kVK_ANSI_Q, 'Q', 'Q', 'Q'},
		{kVK_ANSI_W, 'W', 'W', 'W'},
		{kVK_ANSI_E, 'E', 'E', 'E'},
		{kVK_ANSI_R, 'R', 'R', 'R'},
		{kVK_ANSI_T, 'T', 'T', 'T'},
		{kVK_ANSI_Y, 'Y', 'Y', 'Y'},
		{kVK_ANSI_U, 'U', 'U', 'U'},
		{kVK_ANSI_I, 'I', 'I', 'I'},
		{kVK_ANSI_O, 'O', 'O', 'O'},
		{kVK_ANSI_P, 'P', 'P', 'P'},

		{kVK_ANSI_A, 'A', 'A', 'A'},
		{kVK_ANSI_S, 'S', 'S', 'S'},
		{kVK_ANSI_D, 'D', 'D', 'D'},
		{kVK_ANSI_F, 'F', 'F', 'F'},
		{kVK_ANSI_G, 'G', 'G', 'G'},
		{kVK_ANSI_H, 'H', 'H', 'H'},
		{kVK_ANSI_J, 'J', 'J', 'J'},
		{kVK_ANSI_K, 'K', 'K', 'K'},
		{kVK_ANSI_L, 'L', 'L', 'L'},

		{kVK_ANSI_Z, 'Z', 'Z', 'Z'},
		{kVK_ANSI_X, 'X', 'X', 'X'},
		{kVK_ANSI_C, 'C', 'C', 'C'},
		{kVK_ANSI_V, 'V', 'V', 'V'},
		{kVK_ANSI_B, 'B', 'B', 'B'},
		{kVK_ANSI_N, 'N', 'N', 'N'},
		{kVK_ANSI_M, 'M', 'M', 'M'}
};

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaLocaleManager::CocoaLocaleManager ()
{
	updateInputLanguage ();

	#ifndef CCL_PLATFORM_IOS
	__block CocoaLocaleManager* manager = this;
	observer = [[NSNotificationCenter defaultCenter] addObserverForName:NSTextInputContextKeyboardSelectionDidChangeNotification object:nil queue:nil usingBlock:
				^(NSNotification* _Nonnull note)
				{
					manager->updateInputLanguage ();
					SignalSource (Signals::kLocales).signal (Message (Signals::kInputLanguageChanged));
				}
				];
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaLocaleManager::~CocoaLocaleManager ()
{
	#ifndef CCL_PLATFORM_IOS
	[[NSNotificationCenter defaultCenter] removeObserver:observer];
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CocoaLocaleManager::getSystemLanguage () const
{
    static MutableCString systemLanguage;
    if(systemLanguage.isEmpty ())
    {
		NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    	NSArray* languages = defaults ? [defaults objectForKey:@"AppleLanguages"] : nil;
	    NSString* language = languages ? [languages objectAtIndex: 0] : nil;

    	if(language == nil)
			systemLanguage = LanguageCode::English;
    	else
        {
	        String str;
		    str.appendNativeString (language);
        	str.truncate (2);
            systemLanguage = MutableCString (str);
		}
    }
    return systemLanguage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CocoaLocaleManager::getSystemRegion () const
{
	static MutableCString systemRegion;
	if(systemRegion.isEmpty ())
	{
		GeographicRegion region;
		GetGeographicRegion (region, [NSLocale currentLocale]);
		systemRegion = MutableCString (region.getISO2Code ());
	}
	return systemRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API CocoaLocaleManager::getCharacterOnKey (uchar characterUS, tbool withCapsLock) const
{
	#if CCL_PLATFORM_IOS
	return characterUS;
	#else
	if(!Unicode::isAlphaNumeric (characterUS))
		return characterUS;

	for(int i = 0; i < ARRAY_COUNT (keyLayoutMap); i++)
	{
		KeyLayoutMap& mapping = keyLayoutMap[i];
		if(mapping.characterUS == characterUS)
			return withCapsLock ? mapping.characterLocalCapsLock : mapping.characterLocal;
	}

	return characterUS;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaLocaleManager::getSystemKeyForCharacter (uint16& sysKey, uchar characterLocal) const
{
	#ifndef CCL_PLATFORM_IOS
	for(int i = 0; i < ARRAY_COUNT (keyLayoutMap); i++)
	{
		KeyLayoutMap& mapping = keyLayoutMap[i];
		if(mapping.characterLocal == characterLocal
		|| mapping.characterLocalCapsLock == characterLocal)
		{
			sysKey = mapping.sysKey;
			return true;
		}
	}
	#endif

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaLocaleManager::getNativeUserLanguage (MutableCString& language) const
{
	return ReadPreferenceString (CCLSTR("language"), language);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CocoaLocaleManager::getInputLanguage () const
{
	return inputLanguage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaLocaleManager::updateInputLanguage ()
{
#ifndef CCL_PLATFORM_IOS
	inputLanguage = LanguageCode::Neutral;

	// since macOS 10.12 calling the TextInputSources API from a background process leads to an apparently unresponsive app state (jumping icon in dock)
	if(NSNumber* value = (NSNumber*)[[NSBundle mainBundle] objectForInfoDictionaryKey:@"dev.ccl.isDaemon"])
		if(value.boolValue)
			return;

	CFObj<TISInputSourceRef> source = TISCopyCurrentKeyboardInputSource ();
	if(source == NULL)
		return;

	if(NSArray* languages = (NSArray*)TISGetInputSourceProperty (source, kTISPropertyInputSourceLanguages))
	{
		if(NSString* language = [languages objectAtIndex:0])
		{
			String str;
			str.appendNativeString (language);
			str.truncate (2);

			inputLanguage = MutableCString (str);

			if(inputLanguage == LanguageCode::English)
				inputLanguage = LanguageCode::EnglishUS;
			else if(inputLanguage == LanguageCode::German)
				inputLanguage = LanguageCode::GermanDE;
		}
	}

	CFDataRef uchr = (CFDataRef)TISGetInputSourceProperty (source, kTISPropertyUnicodeKeyLayoutData);
	if(uchr == NULL)
		return;

	const UCKeyboardLayout* keyboardLayout = (const UCKeyboardLayout*)CFDataGetBytePtr (uchr);
	UInt32 deadKeyState = 0;
	const UniCharCount kMaxStringLength = 1;
	const UInt32 kCapsLockState = (alphaLock >> 8) & 0xFF;
	UniCharCount actualStringLength = 0;
	UniChar unicodeBuffer[kMaxStringLength];

	for(int i = 0; i < ARRAY_COUNT (keyLayoutMap); i++)
	{
		KeyLayoutMap& mapping = keyLayoutMap[i];
		mapping.characterLocal = mapping.characterLocalCapsLock = mapping.characterUS;

		// without modifier
		OSStatus status = UCKeyTranslate (keyboardLayout, mapping.sysKey, kUCKeyActionDown, 0, LMGetKbdType (), 0, &deadKeyState, kMaxStringLength, &actualStringLength, unicodeBuffer);
		if(status == noErr && actualStringLength == 1)
			mapping.characterLocal = Unicode::toUppercase (unicodeBuffer[0]);

		// with caps lock
		status = UCKeyTranslate (keyboardLayout, mapping.sysKey, kUCKeyActionDown, kCapsLockState, LMGetKbdType (), 0, &deadKeyState, kMaxStringLength, &actualStringLength, unicodeBuffer);
		if(status == noErr && actualStringLength == 1)
			mapping.characterLocalCapsLock = Unicode::toUppercase (unicodeBuffer[0]);
	}
#else
	inputLanguage = LanguageCode::Neutral;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaLocaleManager::setNativeUserLanguage (StringID language)
{
	WritePreferenceString (CCLSTR("language"), language);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaLocaleManager::setNativeLanguagePack (StringRef pathString)
{
	#ifndef CCL_PLATFORM_IOS
	if(!pathString.isEmpty ())
	{
		Url pathUrl;
		pathUrl.fromDisplayString (pathString);
		if(pathUrl.isAbsolute ())
		{
			Url bundleUrl;
			if(MacUtils::urlFromNSUrl (bundleUrl, [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]], IUrl::kFolder))
				if(pathUrl.makeRelative (bundleUrl))
				{
					WritePreferenceString (CCLSTR ("Language_Pack"), MutableCString (pathUrl.getPath (), Text::kUTF8));
					return;
				}
		}
	}
	#endif

	WritePreferenceString (CCLSTR ("Language_Pack"), MutableCString (pathString, Text::kUTF8));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaLocaleManager::getNativeLanguagePack (String& pathString) const
{
	MutableCString pathCString;

	if(ReadPreferenceString (CCLSTR ("Language_Pack"), pathCString))
	{
		#ifndef CCL_PLATFORM_IOS
		if(!pathCString.isEmpty ())
		{
			Url pathUrl;
			if(pathUrl.fromDisplayString (pathCString.str ()))
				if(pathUrl.isRelative ())
				{
					Url bundleUrl;
					if(MacUtils::urlFromNSUrl (bundleUrl, [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]], IUrl::kFolder))
					{
						pathUrl.makeAbsolute (bundleUrl);
						pathCString = pathUrl.getPath ();
					}
				}
		}
		#endif

		pathString.appendCString (Text::kUTF8, pathCString.str (), pathCString.length ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaLocaleManager::collectGeographicRegions (GeographicRegionList& list) const
{
	for(NSString* localeID in [NSLocale availableLocaleIdentifiers])
	{
		NSLocale* locale = [NSLocale localeWithLocaleIdentifier:localeID];
		AutoPtr<GeographicRegion> region = NEW GeographicRegion;
			if(GetGeographicRegion (*region, locale))
				if(!list.containsRegion (region->englishName))
					list.addSorted (region.detach ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static NSString* CreatePreferenceKey (StringRef key)
{
	NSMutableString* result = [[NSMutableString alloc] init];
	NSString* prefix = key.createNativeString<NSString*> ();
	[result appendString:prefix];
	[prefix release];

	// avoid clash of preferences for different (major) versions of the same application
	if(NSBundle* bundle = [NSBundle mainBundle])
	{
		if(NSString* appName = [bundle objectForInfoDictionaryKey:@"CFBundleName"])
		{
			[result appendString:@"_"];
			[result appendString:appName];
			if(NSString* appVersion = [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"])
				if(NSString* majorVersion = [NSString stringWithFormat:@"_%i", [appVersion intValue]])
					[result appendString:majorVersion];
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WritePreferenceString (StringRef file, CStringRef data)
{
	#ifndef CCL_PLATFORM_IOS
	NSString* key = CreatePreferenceKey (file);
	NSString* value = String (data).createNativeString<NSString*> ();

	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	[defaults setObject:value forKey:key];
    [defaults synchronize];

    [key release];
	[value release];
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ReadPreferenceString (StringRef file, MutableCString& string)
{
	#ifdef CCL_PLATFORM_IOS
	return false;
	#else
	NSString* key = CreatePreferenceKey (file);
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	NSString* value = [defaults stringForKey:key];
    [key release];

	if(value == nil)
		return false;

	String v;
	v.appendNativeString (value);
	string = v;
	return true;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GetGeographicRegion (GeographicRegion& region, NSLocale* locale)
{
	NSLocale* englishLocale = [NSLocale localeWithLocaleIdentifier:@"en_US"];

	if(NSString* countryCode = [locale objectForKey:NSLocaleCountryCode])
	{
		NSString* countryID = [NSLocale localeIdentifierFromComponents:@{NSLocaleCountryCode:countryCode}];
		String string;
		string.appendNativeString ([englishLocale displayNameForKey:NSLocaleIdentifier value:countryID]);
		region.englishName = string;
		string.empty ();
		string.appendNativeString (countryCode);
		region.iso2Code = string;
		string.empty ();
		string.appendNativeString ([locale displayNameForKey:NSLocaleIdentifier value:countryID]);
		region.nativeName = string;
		string.empty ();
		string.appendNativeString ([[NSLocale currentLocale] displayNameForKey:NSLocaleIdentifier value:countryID]);
		region.localizedName = string;
		return true;
	}

	return false;
}
