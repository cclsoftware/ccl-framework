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
// Filename    : ccl/platform/wasm/text/unicodestring.wasm.cpp
// Description : Unicode String Implementation for WebAssembly
//
//************************************************************************************************

#include "ccl/platform/wasm/text/unicodestring.wasm.h"

#include "ccl/public/text/cstring.h"

#include "ccl/text/strings/unicode-cross-platform/ucharfunctions.h"
#include "ccl/text/strings/stringstats.h"

using namespace CCL;

//************************************************************************************************
// Text functions
//************************************************************************************************

#include "ccl/text/strings/unicodestringbuffer.impl.h"

//************************************************************************************************
// UnicodeString
//************************************************************************************************

UnicodeString* UnicodeString::newString ()
{
	return NEW WasmUnicodeString;
}

//************************************************************************************************
// WasmUnicodeString
//************************************************************************************************

IString* CCL_API WasmUnicodeString::cloneString () const
{
	return NEW WasmUnicodeString (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API WasmUnicodeString::createNativeString () const
{
	char* nativeString = nullptr;
	if(text)
	{
		nativeString = NEW char[textByteSize];
		Text::convertToCString (nativeString, textByteSize, Text::kSystemEncoding, text, getLength () + 1);
	}
	else
	{
		nativeString = NEW char[1];
		nativeString[0] = '\0';
	}
	return nativeString; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmUnicodeString::releaseNativeString (void* nativeString) const
{
	delete[] static_cast<char*> (nativeString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmUnicodeString::appendNativeString (const void* _nativeString)
{
	tresult result = kResultFailed;
	
	CStringPtr nativeString = static_cast<CStringPtr> (_nativeString);
	int nativeLength = Text::getLength (nativeString) + 1;
	
	uchar* unicodeText = NEW uchar[nativeLength];
	if(Text::convertToUnicode (unicodeText, nativeLength, Text::kSystemEncoding, nativeString, nativeLength) > 0)
		result = appendChars (unicodeText);
	delete[] unicodeText;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmUnicodeString::isNormalized (NormalizationForm form) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmUnicodeString::normalize (NormalizationForm form)
{
	return kResultNotImplemented;
}

//************************************************************************************************
// UnicodeUtilities
//************************************************************************************************

UnicodeUtilities& UnicodeUtilities::getInstance ()
{
	static UnicodeUtilities theInstance;
	return theInstance;
}
