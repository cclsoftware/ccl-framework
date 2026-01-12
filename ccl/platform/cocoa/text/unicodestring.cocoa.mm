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
// Filename    : ccl/platform/cocoa/text/unicodestring.cocoa.mm
// Description : Unicode String Implementation with CFStrings
//
//************************************************************************************************

#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFStringEncodingExt.h>
#import <Foundation/NSCharacterSet.h>

#include "ccl/platform/cocoa/system/cfallocator.h"
#include "ccl/platform/cocoa/text/unicodestring.cocoa.h"

using namespace CCL;

#if 1
#define CF_ALLOCATOR kCFAllocatorDefault
#else
#define CF_ALLOCATOR MacOS::GetAllocator()
#endif

#define TRY_USE_POINTER 0

#define myCFString ((CFMutableStringRef)cfString)
#define getMyCFString ((CFMutableStringRef)getCFString ())

//************************************************************************************************
// UnicodeString
//************************************************************************************************

UnicodeString* UnicodeString::newString ()
{
	return NEW UnicodeCFString;
}

//************************************************************************************************
// UnicodeCFString
//************************************************************************************************

unsigned int UnicodeCFString::getNativeEncoding (TextEncoding encoding)
{
	static const struct { TextEncoding te; CFStringEncoding cfe; } cfStringMapping[] =
	{
		{Text::kASCII,			kCFStringEncodingASCII},			// US-ASCII
		{Text::kISOLatin1,		kCFStringEncodingISOLatin1},		// ISO 8859-1 Latin I
		{Text::kWindowsLatin1,	kCFStringEncodingWindowsLatin1},	// ANSI Codepage 1252
		{Text::kDOSLatinUS,		kCFStringEncodingDOSLatinUS},		// IBM PC/MS-DOS Codepage 437
		{Text::kMacRoman,		kCFStringEncodingMacRoman},			// MAC - Roman
		{Text::kShiftJIS,		kCFStringEncodingDOSJapanese},		// Japanese Codepage 932
		{Text::kUTF8,			kCFStringEncodingUTF8},				// UTF-8
		{Text::kUTF16LE,		kCFStringEncodingUTF16LE},			// UTF-16 Little Endian
		{Text::kUTF16BE,		kCFStringEncodingUTF16BE}			// UTF-16 Big Endian
	};

	if(encoding != Text::kSystemEncoding)
		for(int i = 0; i < ARRAY_COUNT (cfStringMapping); i++)
			if(cfStringMapping[i].te == encoding)
				return cfStringMapping[i].cfe;

	return kCFStringEncodingMacRoman;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CFStringNormalizationForm UnicodeCFString::getNativeNormalizationForm (NormalizationForm form)
{
	static const struct { NormalizationForm form; CFStringNormalizationForm cfForm; } normalizationMapping[] =
	{
		{Text::kNormalizationC, 	kCFStringNormalizationFormC},	// Canonical Decomposition followed by Canonical Composition
		{Text::kNormalizationD, 	kCFStringNormalizationFormD},	// Canonical Decomposition
		{Text::kNormalizationKC, 	kCFStringNormalizationFormKC},	// Compatibility Decomposition followed by Canonical Composition
		{Text::kNormalizationKD, 	kCFStringNormalizationFormKD}	// Compatibility Decomposition
	};
	
	for(auto mapItem : normalizationMapping)
		if(mapItem.form == form)
			return mapItem.cfForm;
	
	ASSERT (0);
	return kCFStringNormalizationFormC;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeCFString::UnicodeCFString ()
: cfString (NULL)
{
	updatePrivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeCFString::UnicodeCFString (const UnicodeCFString& other)
: cfString (NULL)
{
	if(other.cfString)
		cfString = ::CFStringCreateMutableCopy (CF_ALLOCATOR, 0, (CFStringRef)other.cfString);
		
	updatePrivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeCFString::~UnicodeCFString ()
{
	releaseInternal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeCFString& UnicodeCFString::operator = (const UnicodeCFString& other)
{
	releaseInternal ();

	if(other.cfString)
	{
		cfString = ::CFStringCreateMutableCopy (CF_ALLOCATOR, 0, (CFStringRef)other.cfString);
		updatePrivate ();
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnicodeCFString::releaseInternal ()
{
	if(cfString)
		::CFRelease (myCFString);
	cfString = NULL;
	updatePrivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* UnicodeCFString::getCFString ()
{
	if(!cfString)
		cfString = ::CFStringCreateMutable (CF_ALLOCATOR, 0);
	return cfString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IString* CCL_API UnicodeCFString::cloneString () const
{
	return NEW UnicodeCFString (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constant (immutable) string methods
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult UnicodeCFString::makeConstant (const char* asciiString)
{
	// TODO: use immutable CFString here???
	return appendCString (Text::kASCII, asciiString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeCFString::isEmpty () const
{
	if(!cfString)
		return true;
	return ::CFStringGetLength (myCFString) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeCFString::getLength () const
{
	if(!cfString)
		return 0;
	return (int)::CFStringGetLength (myCFString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API UnicodeCFString::getCharAt (int index) const
{
	if(index < 0 || index >= getLength ())
		return 0;
	
	if(cfString)
		return ::CFStringGetCharacterAtIndex (myCFString, index);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::getChars (CharData& data) const
{
	data.reserved = 0;
	if(cfString)
	{
		#if TRY_USE_POINTER
		// currently (OSX 10.10), this returns always null
		data.text = (const uchar*)CFStringGetCStringPtr (myCFString, kCFStringEncodingUnicode);
		if(data.text == NULL) 
		{	
		#endif
			int bufferSize = getLength () + 1;
			data.text = NEW uchar[bufferSize];
			copyTo ((uchar*)data.text, bufferSize);
			data.reserved = 1; // flag as temporary
			
		#if TRY_USE_POINTER
		}
		#endif
	}
	else
		data.text = NULL;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::releaseChars (CharData& data) const
{
	if((data.reserved & 1) != 0)
	{
		if(data.text)
			delete [] data.text;
		data.text = NULL;
		data.reserved = 0;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::copyTo (uchar* charBuffer, int bufferSize) const
{
	if(!charBuffer || bufferSize <= 0)
		return kResultInvalidArgument;

	if(cfString)
	{
		CFIndex range = ccl_min<CFIndex> (bufferSize - 1, ::CFStringGetLength (myCFString));
		::CFStringGetCharacters (myCFString, ::CFRangeMake (0, range), (UniChar*)charBuffer);
		charBuffer[range] = 0;
	}
	else
		charBuffer[0] = 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::toCString (TextEncoding encoding, char* cString, int cStringSize, int* bytesWritten) const
{
	if(!Text::isValidCStringEncoding (encoding) || !cString || cStringSize <= 0)
		return kResultInvalidArgument;

	if(cfString)
	{
		CFStringEncoding cfEncoding = getNativeEncoding (encoding);
		CFRange range = {0, ::CFStringGetLength (myCFString)};
		CFIndex usedBufLen = 0;
		CFStringGetBytes (myCFString, range, cfEncoding, '?', false, (UInt8*)cString, cStringSize - 1, &usedBufLen);
		ASSERT (usedBufLen < cStringSize)
		cString[usedBufLen] = 0;

		if(bytesWritten)
			*bytesWritten = static_cast<int> (usedBufLen);
	}
	else
	{
		cString[0] = 0;
	
		if(bytesWritten)
			*bytesWritten = 0;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::toPascalString (TextEncoding encoding, unsigned char* pString, int pStringSize) const
{
	if(!pString || pStringSize <= 0)
		return kResultInvalidArgument;

	if(cfString)
	{
		CFStringEncoding cfEncoding = getNativeEncoding (encoding);
		Boolean result = ::CFStringGetPascalString (myCFString, (unsigned char*)pString, pStringSize, cfEncoding);
		if(!result)
			return kResultOutOfMemory;
	}
	else
		pString[0] = 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeCFString::equals (const IString* otherString) const
{		
	return compare (otherString) == Text::kEqual;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeCFString::equalsChars (const uchar* charBuffer, int count) const
{
	return compareChars (charBuffer, count) == Text::kEqual;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeCFString::compare (const IString* _otherString, int flags) const
{		
	UnicodeCFString* otherString = castToString<UnicodeCFString> (_otherString);
	
	if(cfString && otherString && otherString->cfString)
	{
		CFOptionFlags optionFlags = 0;
		if(flags & Text::kIgnoreCase)
			optionFlags |= kCFCompareCaseInsensitive;
		if(flags & Text::kIgnoreDiacritic)
			optionFlags |= kCFCompareDiacriticInsensitive;
		if(flags & Text::kCompareNumerically)
			optionFlags |= kCFCompareNumerically;
		
		return (int)::CFStringCompare (myCFString, (CFStringRef)otherString->cfString, optionFlags);
	}

	if(isEmpty ())
		return (!otherString || otherString->isEmpty ()) ? Text::kEqual : Text::kLess;
	else
	{
		// other must be empty, otherwise the CFStringCompare case above would apply
		ASSERT (!otherString || otherString->isEmpty ())
		return Text::kGreater;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeCFString::compareChars (const uchar* charBuffer, int count) const
{
	// ATTENTION: This function *must not* allocate temporary heap memory!
	if(cfString && charBuffer)
	{
		CFStringInlineBuffer inlineBuffer;
		CFStringInitInlineBuffer (myCFString, &inlineBuffer, CFRangeMake (0, CFStringGetLength (myCFString)));

		int i = 0;
		UniChar c1;		
		while((c1 = CFStringGetCharacterFromInlineBuffer (&inlineBuffer, i)) != 0)
		{
			UniChar c2 = charBuffer[i];
			if(c2 == 0 || (count != -1 && i >= count))
				return Text::kGreater;
				
			if(c2 > c1)
				return Text::kLess;
			if(c1 > c2)
				return Text::kGreater;
			i++;
		}
		
		if(charBuffer[i] != 0)
			return Text::kLess;

		return Text::kEqual;
	}
	else if(isEmpty ())
	{
		if(count == 0 || Text::isEmpty (charBuffer))
			return Text::kEqual;
		else
			return Text::kLess;
	}

	return Text::kGreater;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeCFString::findSubString (const IString* _otherString, int flags) const
{
	UnicodeCFString* otherString = castToString<UnicodeCFString> (_otherString);
	if(cfString && otherString && otherString->cfString)
	{
		CFOptionFlags optionFlags = 0;
		if(flags & Text::kReverseFind)
			optionFlags |= kCFCompareBackwards;
		if(flags & Text::kIgnoreCase)
			optionFlags |= kCFCompareCaseInsensitive;

		CFRange result = ::CFStringFind (myCFString, (CFStringRef)otherString->cfString, optionFlags);
		return (int)result.location; // kCFNotFound is -1 ;-)
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IString* CCL_API UnicodeCFString::createSubString (int index, int count) const
{
	if(cfString == NULL)
		return nullptr;
		
	if(count < 0)
		count = (int)::CFStringGetLength (myCFString) - index;

	if(count <= 0)
		return nullptr;

	UnicodeCFString* result = nullptr;
	CFStringRef temp = ::CFStringCreateWithSubstring (CF_ALLOCATOR, myCFString, ::CFRangeMake (index, count));
	if(temp)
	{
		// create a mutable copy...
		result = NEW UnicodeCFString;
		result->cfString = ::CFStringCreateMutableCopy (CF_ALLOCATOR, 0, temp);
		result->updatePrivate ();
		::CFRelease (temp);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API UnicodeCFString::createNativeString () const
{
	CFStringRef nativeString = myCFString;
	if(!nativeString)
		nativeString = CFSTR ("");
	
	::CFRetain (nativeString); // owned by caller
	return (void*)nativeString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeCFString::releaseNativeString (void* nativeString) const
{
	if(nativeString)
		::CFRelease ((CFStringRef)nativeString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStringPrivateData CCL_API UnicodeCFString::getPrivateData () const
{
#if DEBUG
	return (IStringPrivateData)__private;
#else
	return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnicodeCFString::updatePrivate ()
{
#if DEBUG
	if(cfString)
	{
		CFIndex length = CFStringGetLength (myCFString);
		CFRange range = { 0, length };
		CFIndex usedBufLen = 0;
		CFStringGetBytes (myCFString, range, kCFStringEncodingUTF8, '-', false, (UInt8*)__private, 127, &usedBufLen);
		__private[usedBufLen > 127 ? 127 : usedBufLen] = 0;
	}
	else
		__private[0] = 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Mutable string methods
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::assignChars (const uchar* charBuffer, int count)
{
	releaseInternal ();
	return appendChars (charBuffer, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::appendCString (TextEncoding encoding, const char* cString, int count)
{
	if(!Text::isValidCStringEncoding (encoding))
		return kResultInvalidArgument;

	if(Text::isEmpty (cString) || count == 0)
		return kResultOk;

	CFStringEncoding cfEncoding = getNativeEncoding (encoding);
	if(count > 0)
	{
		for(int i = 0; i < count; i++)
		{
			if(cString[i] == 0)
			{
				count = -1;
				break;
			}
		}
	}
	
	if(count == -1)
	{
		// CFStringAppendCString silently accepts non-ASCII bytes when specifying ASCII encoding, and returns these when asked for an UTF8 string,
		// which yields an ill-formed UTF8 string. CFStringCreateWithCString interprets such bytes as WindowsLatin1 and converts to Unicode.
		CFStringRef temp = ::CFStringCreateWithCString (CF_ALLOCATOR, cString, cfEncoding);
		if(!temp)
			return kResultFailed;
		::CFStringAppend (getMyCFString, temp);
		::CFRelease (temp);
	}
	else // no null terminator
	{
		CFStringRef temp = ::CFStringCreateWithBytes (CF_ALLOCATOR, (const UInt8*)cString, count, cfEncoding, false);
		if(!temp)
			return kResultFailed;
		::CFStringAppend (getMyCFString, temp);
		::CFRelease (temp);
	}
	
	updatePrivate ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::appendPascalString (TextEncoding encoding, const unsigned char* pString)
{
	if(pString && pString[0])
	{
		CFStringEncoding cfEncoding = getNativeEncoding (encoding);
		::CFStringAppendPascalString (getMyCFString, (unsigned char*)pString, cfEncoding);
		updatePrivate ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::appendChars (const uchar* charBuffer, int count)
{
	if(Text::isEmpty (charBuffer) || count == 0)
		return kResultOk;

	if(count < 0)
		count = Text::getLength (charBuffer);
	else
		count = ccl_min (count, Text::getLength (charBuffer, count));

	::CFStringAppendCharacters (getMyCFString, (UniChar*)charBuffer, count);
	updatePrivate ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::appendString (const IString* _otherString)
{
	UnicodeCFString* otherString = castToString<UnicodeCFString> (_otherString);
	if(otherString && otherString->cfString)
	{
		if(!cfString)
			cfString = ::CFStringCreateMutableCopy (CF_ALLOCATOR, 0, (CFStringRef)otherString->cfString);
		else
			::CFStringAppend (myCFString, (CFStringRef)otherString->cfString);

		updatePrivate ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::appendRepeated (const IString* _otherString, int count)
{
	UnicodeCFString* otherString = castToString<UnicodeCFString> (_otherString);
	if(otherString && otherString->cfString && count > 0)
	{
		if(!cfString)
			cfString = ::CFStringCreateMutable (CF_ALLOCATOR, 0);
		int newLength = getLength () + count * otherString->getLength ();
		::CFStringPad (myCFString, (CFStringRef)otherString->cfString, newLength, 0);
		updatePrivate ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::appendNativeString (const void* nativeString)
{
	if(nativeString)
	{
		::CFStringAppend (getMyCFString, (CFStringRef)nativeString);
		updatePrivate ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::insert (int index, const IString* _otherString)
{
	UnicodeCFString* otherString = castToString<UnicodeCFString> (_otherString);
	if(otherString && otherString->cfString)
	{
		// TODO: check if behavior is same as on Windows!!!
		::CFStringInsert (getMyCFString, index, (CFStringRef)otherString->cfString);
		updatePrivate ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::remove (int index, int count)
{
	// TODO: check if behavior is same as on Windows!!!
	if(cfString)
	{
		::CFStringDelete (myCFString, ::CFRangeMake (index, count));
		updatePrivate ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::truncate (int index)
{
	if(index >= 0 && index < getLength ())
	{
		::CFStringPad (myCFString, NULL, index, 0);
		updatePrivate ();
		return kResultOk;
	}
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeCFString::trimWhitespace ()
{
	if(cfString)
	{
		::CFStringTrimWhitespace (myCFString);
		updatePrivate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeCFString::toUppercase ()
{
	if(cfString)
	{
		::CFStringUppercase (myCFString, 0); // TODO: CFLocaleRef???
		updatePrivate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeCFString::toLowercase ()
{
	if(cfString)
	{
		::CFStringLowercase (myCFString, 0); // TODO: CFLocaleRef???
		updatePrivate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeCFString::capitalize ()
{
	if(cfString)
	{
		::CFStringCapitalize (myCFString, 0); // TODO: CFLocaleRef???
		updatePrivate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeCFString::isNormalized (NormalizationForm form) const
{
	if(isEmpty ())
		return true;
		
	// I did not find an efficient way to determine the normalization of a string
	Boolean result = false;
	if(CFMutableStringRef stringCopy = ::CFStringCreateMutableCopy (CFAllocatorGetDefault (), 0, myCFString))
	{
		::CFStringNormalize (stringCopy, getNativeNormalizationForm (form));
		// CFEqual is supposed to do literal Unicode-based comparison, i.e. decomposition rules are not applied
		result = ::CFEqual (stringCopy, myCFString);
		::CFRelease (stringCopy);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeCFString::normalize (NormalizationForm form)
{
	if(isEmpty ())
		return kResultOk;
		
	::CFStringNormalize (myCFString, getNativeNormalizationForm (form));

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeCFString::replace (const IString* _searchString, const IString* _replacementString, int flags)
{
	UnicodeCFString* searchString = castToString<UnicodeCFString> (_searchString);
	UnicodeCFString* replacementString = castToString<UnicodeCFString> (_replacementString);
	
	// allow replace with empty string
	CFStringRef cfSearchString = searchString && searchString->cfString ? (CFStringRef)searchString->cfString : CFSTR ("");
	CFStringRef cfReplacementString = replacementString && replacementString->cfString ? (CFStringRef)replacementString->cfString : CFSTR ("");
	
	int numReplaced = 0;
	if(cfString)
	{
		CFRange range = ::CFRangeMake (0, ::CFStringGetLength (myCFString));

		CFOptionFlags optionFlags = 0;
		if(flags & Text::kReverseFind)
			optionFlags |= kCFCompareBackwards;
		if(flags & Text::kIgnoreCase)
			optionFlags |= kCFCompareCaseInsensitive;

		numReplaced = (int)::CFStringFindAndReplace (myCFString, cfSearchString, cfReplacementString, range, optionFlags);
		updatePrivate ();
	}
	return numReplaced;
}

//************************************************************************************************
// UnicodeUtilities
//************************************************************************************************

UnicodeUtilities& UnicodeUtilities::getInstance ()
{
	static CocoaUnicodeUtilities theInstance;
	return theInstance;
}

//************************************************************************************************
// CocoaUnicodeUtilities
//************************************************************************************************

tbool CCL_API CocoaUnicodeUtilities::isAlpha (uchar c) const
{
	return [[NSCharacterSet alphanumericCharacterSet] characterIsMember:c] && ![[NSCharacterSet decimalDigitCharacterSet] characterIsMember:c];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaUnicodeUtilities::isAlphaNumeric (uchar c) const
{
	return [[NSCharacterSet alphanumericCharacterSet] characterIsMember:c];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaUnicodeUtilities::isLowercase (uchar c) const
{
	return [[NSCharacterSet lowercaseLetterCharacterSet] characterIsMember:c];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaUnicodeUtilities::isUppercase (uchar c) const
{
	return [[NSCharacterSet uppercaseLetterCharacterSet] characterIsMember:c];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API CocoaUnicodeUtilities::toLowercase (uchar c) const
{
	return UnicodeUtilities::toLowercase (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API CocoaUnicodeUtilities::toUppercase (uchar c) const
{
	return UnicodeUtilities::toUppercase (c);
}
