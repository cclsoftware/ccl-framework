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
// Filename    : ccl/public/text/cclstring.cpp
// Description : Unicode String Class
//
//************************************************************************************************

#include "ccl/public/base/variant.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IString, 0x75590d21, 0xd91f, 0x4dd0, 0xb8, 0x52, 0xed, 0xfe, 0xb3, 0xda, 0x65, 0xd3)
DEFINE_IID_ (IStringTokenizer, 0xebacb468, 0xb4c9, 0x4d98, 0x84, 0x14, 0x23, 0xf1, 0xf6, 0x1f, 0xc2, 0xc4)
DEFINE_IID_ (IFormattedString, 0x2a761be7, 0xb704, 0x4fdf, 0xbe, 0xd0, 0x92, 0xeb, 0xc1, 0xce, 0x40, 0xde)
DEFINE_IID_ (IUnicodeUtilities, 0x9f2ad0bf, 0x5c5b, 0x43f1, 0xbb, 0x4a, 0xf3, 0x29, 0xd6, 0x36, 0xd6, 0x98)

//************************************************************************************************
// String
//************************************************************************************************

const String String::kEmpty;

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef String::getLineEnd (TextLineFormat lineFormat)
{
	static const String CRLFLineEnd ("\r\n");
	static const String CRLineEnd ("\r");
	static const String LFLineEnd ("\n");

	return	lineFormat == Text::kCRLFLineFormat ? CRLFLineEnd : 
			lineFormat == Text::kCRLineFormat ? CRLineEnd : 
			LFLineEnd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (nullptr_t unused)
{
	theString = &System::GetEmptyString ();
	theString->retain ();
	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (int unused)
{
	// Debug check: ctor must not be used for anything else than StringRef s = 0!!!
	ASSERT (unused == 0)

	theString = &System::GetEmptyString ();
	theString->retain ();
	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (IString* string)
{
	theString = string ? string : &System::GetEmptyString ();
	theString->retain ();
	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (const uchar* charBuffer)
{
	theString = &System::GetEmptyString ();

	if(charBuffer && charBuffer[0] != '\0')
	{
		theString = theString->cloneString ();
		theString->assignChars (charBuffer);
	}
	else
		theString->retain ();

	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (const char* asciiString)
{
	theString = &System::GetEmptyString ();

	if(asciiString && asciiString[0] != '\0')
	{
		theString = theString->cloneString ();
		theString->appendCString (Text::kASCII, asciiString);
	}
	else
		theString->retain ();

	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (TextEncoding encoding, const char* cString)
{
	theString = &System::GetEmptyString ();

	if(cString)
	{
		theString = theString->cloneString ();
		theString->appendCString (encoding, cString);
	}
	else
		theString->retain ();

	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (StringRef string, int count)
{
	theString = &System::GetEmptyString ();
	theString = theString->cloneString ();
	theString->appendRepeated (string.theString, count);
	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::String (const String& other)
{
	theString = other.theString;
	theString->retain ();
	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String::~String ()
{
	theString->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::operator = (const String& other)
{
	other.theString->retain ();
	theString->release ();
	theString = other.theString;
	__private = theString->getPrivateData ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::isEmpty () const
{ 
	return theString->isEmpty () != 0; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::length () const
{ 
	return theString->getLength (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar String::at (int index) const
{
	return theString->getCharAt (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::copyTo (uchar* charBuffer, int bufferSize) const
{
	return theString->copyTo (charBuffer, bufferSize) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::toCString (TextEncoding encoding, char* cString, int cStringSize, int* bytesWritten) const
{
	return theString->toCString (encoding, cString, cStringSize, bytesWritten) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::toPascalString (TextEncoding encoding, unsigned char* pString, int pStringSize) const
{
	return theString->toPascalString (encoding, pString, pStringSize) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::toASCII (char* asciiString, int asciiStringSize, int* bytesWritten) const
{
	return toCString (Text::kASCII, asciiString, asciiStringSize, bytesWritten);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::equals (StringRef otherString) const
{
	return theString->equals (otherString.theString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::equalsChars (const uchar* charBuffer, int count) const
{
	return theString->equalsChars (charBuffer, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::compare (StringRef otherString, bool caseSensitive) const
{
	return theString->compare (otherString.theString, caseSensitive ? 0 : Text::kIgnoreCase);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::compareWithOptions (StringRef otherString, int flags) const
{
	return theString->compare (otherString.theString, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::compareChars (const uchar* charBuffer, int count) const
{
	return theString->compareChars (charBuffer, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::index (StringRef otherString, bool caseSensitive) const 
{ 
	return theString->findSubString (otherString.theString, caseSensitive ? 0 : Text::kIgnoreCase);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::lastIndex (StringRef otherString, bool caseSensitive) const 
{ 
	int flags = Text::kReverseFind;
	if(!caseSensitive)
		flags |= Text::kIgnoreCase;
	return theString->findSubString (otherString.theString, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String String::subString (int index, int count) const
{
	AutoPtr<IString> theSubString = theString->createSubString (index, count);
	if(theSubString)
		return String (theSubString);
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStringTokenizer* String::tokenize (StringRef delimiters, int flags) const
{
	return theString->tokenize (delimiters.theString, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int String::getHashCode () const
{
	return theString->getHashCode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::getFloatValue (float& value) const
{
	UnknownPtr<IFormattedString> fString (theString);
	return fString->getFloatValue (value) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::getFloatValue (double& value) const
{
	UnknownPtr<IFormattedString> fString (theString);
	return fString->getFloatValue (value) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::getIntValue (int32& value) const
{
	UnknownPtr<IFormattedString> fString (theString);
	return fString->getIntValue (value) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::getIntValue (int64& value) const
{
	UnknownPtr<IFormattedString> fString (theString);
	return fString->getIntValue (value) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::getHexValue (int64& value) const
{
	UnknownPtr<IFormattedString> fString (theString);
	return fString->getHexValue (value) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::scanFormat (StringRef format, Variant args[], int count) const
{
	UnknownPtr<IFormattedString> fString (theString);
	return fString->scanFormat (format.theString, args, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void String::writeEnable ()
{
	AutoPtr<IString> releaser (theString);
	if(theString->retain () == 2)
		return;

	theString->release ();
	theString = theString->cloneString ();
	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void String::empty ()
{
	theString->release ();
	theString = &System::GetEmptyString ();
	theString->retain ();
	__private = theString->getPrivateData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::assign (const uchar* charBuffer, int count)
{
	writeEnable ();
	theString->assignChars (charBuffer, count);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::append (StringRef otherString)
{
	if(isEmpty ())
		*this = otherString;
	else
	{
		writeEnable ();
		theString->appendString (otherString.theString);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::append (StringRef otherString, int count)
{
	writeEnable ();
	theString->appendRepeated (otherString.theString, count);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::append (const uchar* charBuffer, int count)
{
	if(count == 0)
		return *this;

	writeEnable ();
	theString->appendChars (charBuffer, count);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::append (uchar c)
{
	uchar charBuffer[2] = {c, 0};
	return append (charBuffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::appendCString (TextEncoding encoding, const char* cString, int count)
{
	writeEnable ();
	return theString->appendCString (encoding, cString, count) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::appendPascalString (TextEncoding encoding, const unsigned char* pString)
{
	writeEnable ();
	return theString->appendPascalString (encoding, pString) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::appendNativeString (const void* nativeString)
{
	writeEnable ();
	return theString->appendNativeString (nativeString) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendASCII (const char* asciiString, int count)
{
	appendCString (Text::kASCII, asciiString, count);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendFormat (StringRef format, Variant args[], int count)
{
	writeEnable ();
	UnknownPtr<IFormattedString> (theString)->appendFormat (format.theString, args, count);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendFormat (StringRef format, VariantRef arg1)
{
	Variant args[1] = {arg1};
	return appendFormat (format, args, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendFormat (StringRef format, VariantRef arg1, VariantRef arg2)
{
	Variant args[2] = {arg1, arg2};
	return appendFormat (format, args, 2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendFormat (StringRef format, VariantRef arg1, VariantRef arg2, VariantRef arg3)
{
	Variant args[3] = {arg1, arg2, arg3};
	return appendFormat (format, args, 3);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendFormat (StringRef format, VariantRef arg1, VariantRef arg2, VariantRef arg3, VariantRef arg4)
{
	Variant args[4] = {arg1, arg2, arg3, arg4};
	return appendFormat (format, args, 4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::insert (int index, StringRef otherString)
{
	writeEnable ();
	theString->insert (index, otherString.theString);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::remove (int index, int count)
{
	writeEnable ();
	theString->remove (index, count);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::remove (StringRef otherString, bool caseSensitive)
{
	replace (otherString, kEmpty, caseSensitive);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::truncate (int index)
{
	writeEnable ();
	theString->truncate (index);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::trimWhitespace ()
{
	writeEnable ();
	theString->trimWhitespace ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::toUppercase ()
{
	writeEnable ();
	theString->toUppercase ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::toLowercase ()
{
	writeEnable ();
	theString->toLowercase ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::capitalize ()
{
	writeEnable ();
	theString->capitalize ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::substitute (int flags)
{
	writeEnable ();
	theString->substitute ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool String::isNormalized (NormalizationForm form) const
{
	return theString->isNormalized (form);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::normalize (NormalizationForm form)
{
	writeEnable ();
	theString->normalize (form);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int String::replace (StringRef searchString, StringRef replacementString, bool caseSensitive)
{
	writeEnable ();
	return theString->replace (searchString.theString, replacementString.theString, caseSensitive ? 0 : Text::kIgnoreCase);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendIntValue (int v, int numPaddingZeros)
{
	return appendIntValue ((int64)v, numPaddingZeros);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendIntValue (int64 v, int numPaddingZeros)
{
	writeEnable ();
	UnknownPtr<IFormattedString> (theString)->appendIntValue (v, numPaddingZeros);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendHexValue (int v, int numPaddingZeros)
{
	return appendHexValue ((int64)v, numPaddingZeros);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendHexValue (int64 v, int numPaddingZeros)
{
	writeEnable ();
	UnknownPtr<IFormattedString> (theString)->appendHexValue (v, numPaddingZeros);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendFloatValue (float v, int numDecimalDigits)
{
	return appendFloatValue ((double)v, numDecimalDigits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& String::appendFloatValue (double v, int numDecimalDigits)
{
	writeEnable ();
	UnknownPtr<IFormattedString> (theString)->appendFloatValue (v, numDecimalDigits);
	return *this;
}
