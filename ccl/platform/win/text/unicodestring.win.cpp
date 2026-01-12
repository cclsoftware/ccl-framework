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
// Filename    : ccl/platform/win/text/unicodestring.win.cpp
// Description : Windows-specific Unicode String Implementation
//
//************************************************************************************************

#define USE_CROSSPLATFORM_UNICODE_FUNCTIONS (0 && DEBUG) // used during development only!

#include "ccl/platform/win/text/unicodestring.win.h"

#if USE_CROSSPLATFORM_UNICODE_FUNCTIONS
#include "ccl/text/strings/unicode-cross-platform/ucharfunctions.h"
#endif
#include "ccl/text/strings/stringstats.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {
namespace Text {

static unsigned int getNativeEncoding (TextEncoding encoding)
{
	static const struct { TextEncoding te; UINT cp; } codePageMapping[] =
	{
		{Text::kASCII,			20127},		// US-ASCII
		{Text::kISOLatin1,		28591},		// ISO 8859-1 Latin I
		{Text::kWindowsLatin1,	1252},		// ANSI Codepage 1252
		{Text::kDOSLatinUS,		437},		// IBM PC/MS-DOS Codepage 437
		{Text::kMacRoman,		10000},		// MAC - Roman
		{Text::kShiftJIS,		932},		// Japanese Codepage 932
		{Text::kUTF8,			65001},		// UTF-8
		{Text::kUTF16LE,		1200},		// UTF-16 Little Endian
		{Text::kUTF16BE,		1201}		// UTF-16 Big Endian
	};

	if(encoding != Text::kSystemEncoding)
		for(int i = 0; i < ARRAY_COUNT (codePageMapping); i++)
			if(codePageMapping[i].te == encoding)
				return codePageMapping[i].cp;

	return CP_ACP;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static NORM_FORM getNativeNormalizationForm (NormalizationForm form)
{
	static const struct { NormalizationForm form; NORM_FORM windowsForm; } normalizationMapping[] =
	{
	    {Text::kNormalizationC,     NormalizationC},
	    {Text::kNormalizationD,     NormalizationD},
	    {Text::kNormalizationKC,    NormalizationKC},
	    {Text::kNormalizationKD,    NormalizationKD}
	};
	   
	for(auto mapItem : normalizationMapping)
		if(mapItem.form == form)
			return mapItem.windowsForm;

	ASSERT (0)
	return NormalizationC;
}

} // namespace Text
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Text functions
//************************************************************************************************

#if USE_CROSSPLATFORM_UNICODE_FUNCTIONS
#include "ccl/text/strings/unicodestringbuffer.impl.h"
#else

int Text::convertToCString (char* cString, int cStringSize, TextEncoding encoding, const uchar* uString, int uStringLength)
{
	UINT codePage = getNativeEncoding (encoding);
	return ::WideCharToMultiByte (codePage, 0, uString, uStringLength, cString, cStringSize, nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Text::convertToUnicode (uchar* uString, int uStringSize, TextEncoding encoding, const char* cString, int cStringLength)
{
	UINT codePage = getNativeEncoding (encoding);
	return ::MultiByteToWideChar (codePage, 0, cString, cStringLength, uString, uStringSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Text::compareStrings (const uchar* s1, int l1, const uchar* s2, int l2, int flags)
{
	DWORD compareFlags = 0;
	if(flags & kIgnoreCase)
		compareFlags |= NORM_IGNORECASE;
	if(flags & kIgnoreDiacritic)
		compareFlags |= LINGUISTIC_IGNOREDIACRITIC;
	if(flags & kCompareNumerically)
		compareFlags |= SORT_DIGITSASNUMBERS;

	int result = ::CompareStringEx (LOCALE_NAME_INVARIANT, compareFlags, s1, l1, s2, l2, nullptr, nullptr, 0);
	ASSERT (result != 0) // 0 means error
	return result - CSTR_EQUAL; // subtract 2 for C runtime compatibility
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uchar* Text::findString (const uchar* source, const uchar* value, int flags)
{
	DWORD findFlags = 0;
	if(flags & kReverseFind)
		findFlags |= FIND_FROMEND;
	else
		findFlags |= FIND_FROMSTART;
	if(flags & kIgnoreCase)
		findFlags |= NORM_IGNORECASE;
	//if(flags & kIgnoreDiacritic) not used
	//	findFlags |= LINGUISTIC_IGNOREDIACRITIC;

	int index = ::FindNLSStringEx (LOCALE_NAME_INVARIANT, findFlags, source, -1, value, -1, nullptr, nullptr, nullptr, 0);
	const uchar* ptr = index >= 0 ? source + index : nullptr;
	return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::toUppercase (uchar* s)
{
	::CharUpperBuff (s, getLength (s));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::toLowercase (uchar* s)
{
	::CharLowerBuff (s, getLength (s));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Text::capitalize (uchar* s)
{
	uchar prevChar = 0x20;
	for(uchar* ptr = s; *ptr != 0; prevChar = *ptr++)
	{
		if(!::IsCharAlpha (prevChar))
		{
			if(::IsCharLower (*ptr))
				::CharUpperBuff (ptr, 1);
		}
	}
}

#endif // !USE_CROSSPLATFORM_UNICODE_FUNCTIONS

//************************************************************************************************
// UnicodeString
//************************************************************************************************

UnicodeString* UnicodeString::newString ()
{
	return NEW WindowsUnicodeString;
}

//************************************************************************************************
// WindowsUnicodeString
//************************************************************************************************

IString* CCL_API WindowsUnicodeString::cloneString () const
{
	return NEW WindowsUnicodeString (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API WindowsUnicodeString::createNativeString () const
{
	return ::SysAllocString (text ? text : L"");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsUnicodeString::releaseNativeString (void* nativeString) const
{
	if(nativeString)
		::SysFreeString ((BSTR)nativeString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsUnicodeString::appendNativeString (const void* nativeString)
{
	return nativeString ? appendChars ((BSTR)nativeString) : kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUnicodeString::isNormalized (NormalizationForm form) const
{
	return ::IsNormalizedString (Text::getNativeNormalizationForm (form), text, -1) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsUnicodeString::normalize (NormalizationForm form)
{
	if(isEmpty ())
		return kResultOk;

	NORM_FORM nativeForm = Text::getNativeNormalizationForm (form);

	// first call estimates the required length
	int estimatedSize = ::NormalizeString (nativeForm, text, -1, nullptr, 0);
	if(estimatedSize <= 0)
		return kResultFailed;

	// "detach" current buffer (keep as source), then allocate a new buffer for destination string
	const uchar* sourceText = text;
	text = nullptr;
	textByteSize = 0;
	resizeInternal (estimatedSize);

	int newLength = ::NormalizeString (nativeForm, sourceText, -1, ccl_const_cast (text), estimatedSize);
	ASSERT (newLength > 0)
	updateMetadata (newLength - 1);

	// release source buffer
	if(sourceText)
		string_free ((void*)sourceText);

	return kResultOk;
}

//************************************************************************************************
// UnicodeUtilities
//************************************************************************************************

UnicodeUtilities& UnicodeUtilities::getInstance ()
{
	static WindowsUnicodeUtilities theInstance;
	return theInstance;
}

//************************************************************************************************
// WindowsUnicodeUtilities
//************************************************************************************************

#if USE_CROSSPLATFORM_UNICODE_FUNCTIONS
tbool CCL_API WindowsUnicodeUtilities::isAlpha (uchar c) const
{
	return UCharFunctions::isAlpha (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUnicodeUtilities::isAlphaNumeric (uchar c) const
{
	return UCharFunctions::isAlphaNumeric (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUnicodeUtilities::isLowercase (uchar c) const
{
	return UCharFunctions::isLowercase (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUnicodeUtilities::isUppercase (uchar c) const
{
	return UCharFunctions::isUppercase (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API WindowsUnicodeUtilities::toLowercase (uchar c) const
{
	return UCharFunctions::toLowercase (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API WindowsUnicodeUtilities::toUppercase (uchar c) const
{
	return UCharFunctions::toUppercase (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#else // !USE_CROSSPLATFORM_UNICODE_FUNCTIONS

tbool CCL_API WindowsUnicodeUtilities::isAlpha (uchar c) const
{
	return ::IsCharAlpha (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUnicodeUtilities::isAlphaNumeric (uchar c) const
{
	return ::IsCharAlphaNumeric (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUnicodeUtilities::isLowercase (uchar c) const
{
	return ::IsCharLower (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUnicodeUtilities::isUppercase (uchar c) const
{
	return ::IsCharUpper (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API WindowsUnicodeUtilities::toLowercase (uchar c) const
{
	::CharLowerBuff (&c, 1);
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API WindowsUnicodeUtilities::toUppercase (uchar c) const
{
	::CharUpperBuff (&c, 1);
	return c;
}
#endif // !USE_CROSSPLATFORM_UNICODE_FUNCTIONS
