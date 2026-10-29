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
// Filename    : ccl/platform/wasm/system/localemanager.wasm.cpp
// Description : WebAssembly Locale Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/posix/system/localemanager.posix.h"

#include "ccl/public/text/language.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, getNavigatorLanguage, (char* cString, int cStringSize), 
{
	const intl = window.Intl;
	let locale = null;
	if(intl !== undefined)
		locale = intl.NumberFormat ().resolvedOptions ().locale;
	else if(navigator.languages !== undefined)
		locale = navigator.languages[0];
	else
		locale = navigator.userLanguage || navigator.language || navigator.browserLanguage;

	if(locale)
	{
		stringToUTF8 (locale, cString, cStringSize);
		return true;
	}
	return false;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, getNavigatorRegion, (char* cString, int cStringSize), 
{
	try 
	{
		const locale = Intl.NumberFormat ().resolvedOptions ().locale;
		const region = new Intl.Locale (locale).maximize ().region;
		if(region)
		{
			stringToUTF8 (region, cString, cStringSize);
			return true;
		}
	} 
	catch(e) 
	{}
	return false;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, findCharacterOnKey, (char* cString, int cStringSize, CStringPtr keyCode, bool capsLock), 
{
	try 
	{
		const code = UTF8ToString (keyCode);
		const result = CCL.findCharacterInLayoutMap (code, capsLock);
		if(result)
		{
			stringToUTF8 (result, cString, cStringSize);
			return true;
		}
	}
	catch(e) 
	{}
	return false;
});

} // namespace Wasm

//************************************************************************************************
// WasmLocaleManager
//************************************************************************************************

class WasmLocaleManager: public PosixLocaleManager
{
public:
	WasmLocaleManager ();
	~WasmLocaleManager ();

	// PosixLocaleManager
	StringID CCL_API getSystemLanguage () const override;
	StringID CCL_API getInputLanguage () const override;
	StringID CCL_API getSystemRegion () const override;
	uchar CCL_API getCharacterOnKey (uchar characterUS, tbool withCapsLock = false) const override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmLocaleManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (LocaleManager, WasmLocaleManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmLocaleManager::WasmLocaleManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmLocaleManager::~WasmLocaleManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WasmLocaleManager::getSystemLanguage () const
{
	char language[8] = "";
	if(getNavigatorLanguage (language, ARRAY_COUNT (language)))
		return getPrimaryLanguageCode (language);
	return LanguageCode::English;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WasmLocaleManager::getInputLanguage () const
{
	return LanguageCode::English;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WasmLocaleManager::getSystemRegion () const
{
	static MutableCString systemRegion;
	if(systemRegion.isEmpty ())
	{
		char region[8] = "";
		if(getNavigatorRegion (region, ARRAY_COUNT (region)))
			systemRegion = region;
		else
			systemRegion = CountryCode::kUS;
	}
	return systemRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API WasmLocaleManager::getCharacterOnKey (uchar characterUS, tbool withCapsLock) const
{
	MutableCString input;
	MutableCString character;
	character.append (characterUS);
	
	if(CString::isAlpha (character.firstChar ()))
	{
		character.toUppercase ();
		input = "Key";
		input.append (character);
	}
	else if(CString::isDigit (character.firstChar ()))
	{
		input = "Digit";
		input.append (character);
	}
	else
		return 0; // handle additional keys?

	char result[8] = "";
	if(findCharacterOnKey (result, ARRAY_COUNT (result), input, withCapsLock))
	{
		CCL_PRINTF ("Character on key %s with caps lock %s: %s\n", input.str (), withCapsLock ? "on" : "off", result)

		String unicodeResult;
		unicodeResult.appendCString (Text::kUTF8, result);
		return unicodeResult.firstChar ();
	}
	return characterUS;
}
