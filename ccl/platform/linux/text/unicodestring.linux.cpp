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
// Filename    : ccl/platform/linux/text/unicodestring.linux.cpp
// Description : Unicode String Implementation using GNU libunistring
//
//************************************************************************************************

#include "ccl/platform/linux/text/unicodestring.linux.h"

#include "ccl/public/text/cstring.h"

#include "ccl/text/strings/unicode-cross-platform/ucharfunctions.h"
#include "ccl/text/strings/stringstats.h"

#ifndef CCLTEXT_LIBUNISTRING_ENABLED
#define CCLTEXT_LIBUNISTRING_ENABLED 1
#endif 

#if CCLTEXT_LIBUNISTRING_ENABLED
#include <unistr.h>
#include <unicase.h>
#include <unictype.h>
#include <uninorm.h>
#include <uniconv.h>
#else
#include <langinfo.h>
#endif // CCLTEXT_LIBUNISTRING_ENABLED

namespace CCL {
namespace Text {

static CStringPtr getNativeEncoding (TextEncoding encoding)
{
	static const struct { TextEncoding te; CStringPtr cp; } codePageMapping[] =
	{
		{Text::kASCII,			"ASCII"},		// US-ASCII
		{Text::kISOLatin1,		"LATIN1"},		// ISO 8859-1 Latin I
		{Text::kWindowsLatin1,	"MS-ANSI"},		// ANSI Codepage 1252
		{Text::kDOSLatinUS,		"IBM437"},		// IBM PC/MS-DOS Codepage 437
		{Text::kMacRoman,		"MAC"},			// MAC - Roman
		{Text::kShiftJIS,		"SHIFT-JIS"},	// Japanese Codepage 932
		{Text::kUTF8,			"UTF-8"},		// UTF-8
		{Text::kUTF16LE,		"UTF-16LE"},	// UTF-16 Little Endian
		{Text::kUTF16BE,		"UTF-16BE"}		// UTF-16 Big Endian
	};

	if(encoding != Text::kSystemEncoding)
		for(int i = 0; i < ARRAY_COUNT (codePageMapping); i++)
			if(codePageMapping[i].te == encoding)
				return codePageMapping[i].cp;

	#if CCLTEXT_LIBUNISTRING_ENABLED
	return locale_charset ();
	#else
	return nl_langinfo (CODESET);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCLTEXT_LIBUNISTRING_ENABLED
static uninorm_t getNativeNormalizationForm (NormalizationForm form)
{
	static const struct { NormalizationForm form; uninorm_t nativeForm; } normalizationMapping[] =
	{
		{Text::kNormalizationC, 	UNINORM_NFC},	// Canonical Decomposition followed by Canonical Composition
		{Text::kNormalizationD, 	UNINORM_NFD},	// Canonical Decomposition
		{Text::kNormalizationKC, 	UNINORM_NFKC},	// Compatibility Decomposition followed by Canonical Composition
		{Text::kNormalizationKD, 	UNINORM_NFKD}	// Compatibility Decomposition
	};
	
	for(auto mapItem : normalizationMapping)
		if(mapItem.form == form)
			return mapItem.nativeForm;
	
	ASSERT (0);
	return nullptr;
}
#endif // CCLTEXT_LIBUNISTRING_ENABLED

} // namespace Text
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Text functions
//************************************************************************************************

#if CCLTEXT_LIBUNISTRING_ENABLED

int Text::convertToCString (char* cString, int cStringSize, TextEncoding encoding, const uchar* uString, int uStringLength)
{
	if(uString && uStringLength < 0)
		uStringLength = getLength (uString);
	size_t length = cStringSize;
	char* result = cString;
	if(uStringLength == 0)
		length = 0;
	else
		result = u16_conv_to_encoding (getNativeEncoding (encoding), iconveh_question_mark, uString, uStringLength, nullptr, cString, &length);
	if(result == nullptr)
		return -1;
	if(result != cString)
	{
		ASSERT (cString == nullptr)
		string_free (result);
		return int (length + 1);
	}
	if(cString && length < cStringSize)
		cString[length] = '\0';
	return int (length + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Text::convertToUnicode (uchar* uString, int uStringSize, TextEncoding encoding, const char* cString, int cStringLength)
{
	if(cString && cStringLength < 0)
		cStringLength = getLength (cString);
	size_t length = uStringSize;
	uchar* result = uString;
	if(cStringLength == 0)
		length = 0;
	else
		result = u16_conv_from_encoding (getNativeEncoding (encoding), iconveh_question_mark, cString, cStringLength, nullptr, uString, &length);
	if(result == nullptr)
		return -1;
	if(result != uString)
	{
		ASSERT (uString == nullptr)
		string_free (result);
		return int (length + 1);
	}
	if(uString && length < uStringSize)
		uString[length] = u'\0';
	return int (length + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Text::compareStrings (const uchar* s1, int l1, const uchar* s2, int l2, int flags)
{
	if(s1 == nullptr)
		return s2 == nullptr ? Text::kEqual : Text::kLess;
	if(s2 == nullptr)
		return kGreater;

	bool succeeded = false;
	int result = -1;
	if(l1 < 0)
		l1 = Text::getLength (s1) + 1;
	if(l2 < 0)
		l2 = Text::getLength (s2) + 1;
	if(flags & kIgnoreCase)
		succeeded = u16_casecmp (s1, l1, s2, l2, uc_locale_language (), UNINORM_NFC, &result) == 0;
	if(!succeeded)
		result = u16_strncmp (s1, s2, ccl_min (l1, l2));
	if(result < 0)
		return kLess;
	if(result > 0)
		return kGreater;
	return kEqual;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uchar* Text::findString (const uchar* source, const uchar* value, int flags)
{
	if(source == nullptr || value == nullptr)
		return nullptr;

	const uchar* result;
	
	uchar* sourceCopy = nullptr;
	uchar* valueCopy = nullptr;
	
	const uchar* hayStack = source;
	const uchar* needle = value;
	
	int hayStackLength = getLength (source);
	int needleLength = getLength (value);

	bool ignoreCase = (flags & kIgnoreCase) != 0;
	if(ignoreCase)
	{
		sourceCopy = NEW uchar[hayStackLength + 1];
		valueCopy = NEW uchar[needleLength + 1];
		::memcpy (sourceCopy, source, (hayStackLength + 1) * sizeof(uchar));
		::memcpy (valueCopy, value, (needleLength + 1) * sizeof(uchar));
		
		toLowercase (sourceCopy);
		toLowercase (valueCopy);
		hayStack = sourceCopy;
		needle = valueCopy;
	}
	
	if(flags & kReverseFind)
		result = UCharFunctions::findStringReverse (hayStack, hayStackLength, needle, needleLength, ignoreCase);
	else
		result = u16_strstr (hayStack, needle);
	
	if(hayStack != source && result != nullptr)
		result = (result - hayStack) + source;
	
	delete[] sourceCopy;
	delete[] valueCopy;
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::toUppercase (uchar* s)
{
	size_t n = u16_strlen (s);
	u16_toupper (s, n, uc_locale_language (), UNINORM_NFC, s, &n);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::toLowercase (uchar* s)
{
	size_t n = u16_strlen (s);
	u16_tolower (s, n, uc_locale_language (), UNINORM_NFC, s, &n);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::capitalize (uchar* s)
{
	size_t n = u16_strlen (s);
	u16_totitle (s, n, uc_locale_language (), UNINORM_NFC, s, &n);
}

#else 

#include "ccl/text/strings/unicodestringbuffer.impl.h"

#endif // CCLTEXT_LIBUNISTRING_ENABLED

//************************************************************************************************
// UnicodeString
//************************************************************************************************

UnicodeString* UnicodeString::newString ()
{
	return NEW LinuxUnicodeString;
}

//************************************************************************************************
// LinuxUnicodeString
//************************************************************************************************

IString* CCL_API LinuxUnicodeString::cloneString () const
{
	return NEW LinuxUnicodeString (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API LinuxUnicodeString::createNativeString () const
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

void CCL_API LinuxUnicodeString::releaseNativeString (void* nativeString) const
{
	delete[] static_cast<char*> (nativeString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUnicodeString::appendNativeString (const void* _nativeString)
{
	tresult result = kResultFailed;
	
	const char* nativeString = static_cast<const char*> (_nativeString);
	int nativeLength = Text::getLength (nativeString) + 1;
	
	uchar* unicodeText = NEW uchar[nativeLength];
	if(Text::convertToUnicode (unicodeText, nativeLength, Text::kSystemEncoding, nativeString, nativeLength) > 0)
		result = appendChars (unicodeText);
	delete[] unicodeText;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxUnicodeString::isNormalized (NormalizationForm form) const
{
	#if CCLTEXT_LIBUNISTRING_ENABLED

	if(text == nullptr)
		return true;
	
	LinuxUnicodeString copy (*this);
	copy.normalize (form);
	return ::memcmp (text, copy.text, u16_strlen (text) * sizeof(uchar)) == 0;

	#else

	return false;

	#endif // CCLTEXT_LIBUNISTRING_ENABLED
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxUnicodeString::normalize (NormalizationForm form)
{
	#if CCLTEXT_LIBUNISTRING_ENABLED

	if(text == nullptr)
		return kResultOk;

	size_t sourceLength = u16_strlen (text);
	size_t length = textByteSize / sizeof(uchar);
	if(length == 0)
		return kResultOk;

	uchar* sourceCopy = NEW uchar[length];
	::memcpy (sourceCopy, text, textByteSize);
	
	uchar* result = u16_normalize (Text::getNativeNormalizationForm (form), sourceCopy, sourceLength, const_cast<uchar*> (text), &length);

	if(result != nullptr)
	{
		if(result != text)
		{
			resizeInternal (0);
			text = result;
			textByteSize = int ((length + 1) * sizeof(uchar));
		}
		
		ASSERT ((length + 1) * sizeof(uchar) <= textByteSize)
		if((length + 1) * sizeof(uchar) <= textByteSize)
			const_cast<uchar*> (text)[length] = u'\0';
		updateMetadata (length);
	}

	delete[] sourceCopy;

	return result == nullptr ? kResultFailed : kResultOk;

	#else

	return kResultNotImplemented;

	#endif // CCLTEXT_LIBUNISTRING_ENABLED
}

//************************************************************************************************
// UnicodeUtilities
//************************************************************************************************

UnicodeUtilities& UnicodeUtilities::getInstance ()
{
	#if CCLTEXT_LIBUNISTRING_ENABLED
	static LinuxUnicodeUtilities theInstance;
	#else
	static UnicodeUtilities theInstance;
	#endif
	return theInstance;
}

//************************************************************************************************
// LinuxUnicodeUtilities
//************************************************************************************************

#if CCLTEXT_LIBUNISTRING_ENABLED

uint32 LinuxUnicodeUtilities::toUtf32 (uchar c) const
{
	uint32 utf32 = 0;
	size_t n = 1;
	uint32* result = u16_to_u32 (&c, n, &utf32, &n);
	if(result != &utf32 && result != nullptr)
		string_free (result);
	return utf32;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxUnicodeUtilities::isAlpha (uchar c) const
{
	return uc_is_alpha (toUtf32 (c));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxUnicodeUtilities::isAlphaNumeric (uchar c) const
{
	return uc_is_alnum (toUtf32 (c));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxUnicodeUtilities::isLowercase (uchar c) const
{
	bool result = false;
	u16_is_lowercase (&c, 1, uc_locale_language (), &result);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxUnicodeUtilities::isUppercase (uchar c) const
{
	bool result = false;
	u16_is_uppercase (&c, 1, uc_locale_language (), &result);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API LinuxUnicodeUtilities::toLowercase (uchar c) const
{
	size_t n = 1;
	uchar result;
	u16_tolower (&c, n, uc_locale_language (), UNINORM_NFC, &result, &n);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API LinuxUnicodeUtilities::toUppercase (uchar c) const
{
	size_t n = 1;
	uchar result;
	u16_toupper (&c, n, uc_locale_language (), UNINORM_NFC, &result, &n);
	return result;
}

#endif // CCLTEXT_LIBUNISTRING_ENABLED
