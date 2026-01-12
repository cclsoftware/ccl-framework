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
// Filename    : ccl/text/strings/unicodestringbuffer.cpp
// Description : Unicode String Buffer
//
//************************************************************************************************

#define OPTIMIZE_STRING 1 // enable string optimizations

#include "ccl/text/strings/unicodestringbuffer.h"
#include "ccl/text/strings/stringstats.h"

using namespace CCL;

#if STRING_STATS
static StringStatistics<uchar> theStats ("Unicode String Statistics");
#endif

//************************************************************************************************
// UnicodeStringBuffer
//************************************************************************************************

static const uchar emptyString[1] = {0};
const uchar* UnicodeStringBuffer::kEmpty = emptyString;

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeStringBuffer::UnicodeStringBuffer ()
: text (nullptr),
  textByteSize (0),
  textLength (0),
  hashCode (kInvalidHashCode)
{
	STRING_ADDED
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeStringBuffer::UnicodeStringBuffer (const UnicodeStringBuffer& other)
: text (nullptr),
  textByteSize (0),
  textLength (0),
  hashCode (kInvalidHashCode)
{
	STRING_ADDED

	if(other.text)
		assign (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeStringBuffer::~UnicodeStringBuffer ()
{
	releaseInternal ();

	STRING_REMOVED
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnicodeStringBuffer& UnicodeStringBuffer::operator = (const UnicodeStringBuffer& other)
{
	assign (other);

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult UnicodeStringBuffer::assign (const UnicodeStringBuffer& other)
{
	assignInternal (other.text, other.textLength);

	textLength = other.textLength;
	hashCode = other.hashCode;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnicodeStringBuffer::releaseInternal ()
{
	resizeInternal (0);
	updateMetadata (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constant (immutable) string methods
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult UnicodeStringBuffer::makeConstant (const char* asciiString)
{
	ASSERT (text == nullptr) // must be called only once!
	if(Text::isEmpty (asciiString))
		return kResultOk;

	int length = Text::getLength (asciiString);
	if(!resizeInternal (length, true))
		return kResultOutOfMemory;

	for(int i = 0; i < length + 1; i++)
		text[i] = asciiString[i];

	updateMetadata (length);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeStringBuffer::isEmpty () const
{
	return Text::isEmpty (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeStringBuffer::getLength () const
{
	// assert that textLength is up to date (no missing updateMetadata() call)
	ASSERT (textLength == (isEmpty () ? 0 : Text::getLength (text)))

	return textLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API UnicodeStringBuffer::getHashCode () const
{
	if(hashCode == kInvalidHashCode)
		hashCode = UnicodeString::getHashCode ();
	#if DEBUG
	else
		// assert that hashCode is up to date (no missing updateMetadata() call)
		ASSERT (hashCode == UnicodeString::getHashCode ())
	#endif

	return hashCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API UnicodeStringBuffer::getCharAt (int index) const
{
	if(index >= 0 && index < getLength ())
		return text[index];
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::getChars (CharData& data) const
{
	data.text = text;
	data.reserved = 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::releaseChars (CharData& data) const
{
	data.text = nullptr;
	data.reserved = 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::copyTo (uchar* charBuffer, int bufferSize) const
{
	if(!charBuffer || bufferSize <= 0)
		return kResultInvalidArgument;

	ASSERT (ccl_is_aligned (charBuffer))

	if(text)
	{
		Text::copyTo (charBuffer, text, bufferSize);
		charBuffer[bufferSize-1] = 0;
	}
	else
		charBuffer[0] = 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::toCString (TextEncoding encoding, char* cString, int cStringSize, int* bytesWritten) const
{
	if(!Text::isValidCStringEncoding (encoding) || !cString || cStringSize <= 0)
		return kResultInvalidArgument;

	if(!isEmpty ())
	{
		int result = Text::convertToCString (cString, cStringSize, encoding, text, textLength + 1);
		cString[cStringSize - 1] = 0;
		if(result <= 0)
			return kResultOutOfMemory;

		if(bytesWritten)
			*bytesWritten = result;
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

tresult CCL_API UnicodeStringBuffer::toPascalString (TextEncoding encoding, unsigned char* pString, int pStringSize) const
{
	if(!pString || pStringSize <= 0)
		return kResultInvalidArgument;

	// Problem: toCString() adds a null terminator, we could loose one character here!
	tresult result = toCString (encoding, (char*)pString + 1, pStringSize - 1);
	if(result != kResultOk)
	{
		pString[0] = 0;
		return result;
	}

	int length = getLength ();
	pString[0] = length < 254 ? length : 254;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeStringBuffer::equals (const IString* otherString) const
{
	if(this == otherString)
		return true;

	if(otherString == nullptr)
		return isEmpty ();

	if(getLength () != otherString->getLength () || getHashCode () != otherString->getHashCode ())
		return false;

	CharData otherData;
	if(otherString->getChars (otherData) == kResultOk)
		return Text::stringsEqual (text, otherData.text, getLength ());

	return compare (otherString) == Text::kEqual;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeStringBuffer::equalsChars (const uchar* charBuffer, int count) const
{
	ASSERT (ccl_is_aligned (charBuffer))

	if(charBuffer == nullptr)
		return isEmpty ();

	if(count < 0)
		count = Text::getLength (charBuffer);

	return Text::stringsEqual (text, charBuffer, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeStringBuffer::compare (const IString* _otherString, int flags) const
{
	if(this == _otherString)
		return Text::kEqual;

	UnicodeStringBuffer* otherString = castToString<UnicodeStringBuffer> (_otherString);
	const uchar* s1 = text ? text : kEmpty;
	const uchar* s2 = otherString && otherString->text ? otherString->text : kEmpty;

	return Text::compareStrings (s1, -1, s2, -1, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeStringBuffer::compareChars (const uchar* charBuffer, int count) const
{
	ASSERT (ccl_is_aligned (charBuffer))

	const uchar* s1 = text ? text : kEmpty;
	const uchar* s2 = charBuffer ? charBuffer : kEmpty;

	return Text::compareStrings (s1, -1, s2, count, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeStringBuffer::findSubString (const IString* _otherString, int flags) const
{
	UnicodeStringBuffer* otherString = castToString<UnicodeStringBuffer> (_otherString);
	if(!isEmpty () && otherString && !otherString->isEmpty ())
	{
		const uchar* ptr = Text::findString (text, otherString->text, flags);
		if(ptr)
			return (int)(ptr - text);
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IString* CCL_API UnicodeStringBuffer::createSubString (int index, int count) const
{
	if(count == 0)
		return nullptr;
	int thisLength = getLength ();
	if(index < 0 || index >= thisLength)
		return nullptr;
	if(count < 0)
		count = thisLength - index;
	if(count <= 0)
		return nullptr;

	UnicodeStringBuffer* result = (UnicodeStringBuffer*)newString ();
	if(!result->resizeInternal (count))
	{
		result->release ();
		return nullptr;
	}

	Text::copyTo (result->text, text + index, count);
	result->text[count] = 0;
	result->updateMetadata (count);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStringPrivateData CCL_API UnicodeStringBuffer::getPrivateData () const
{
	return (IStringPrivateData)&text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Mutable string methods
//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnicodeStringBuffer::resizeInternal (int newLength, bool fixed)
{
	unsigned int byteSize = newLength > 0 ? (newLength + 1) * sizeof(uchar) : 0;

	#if OPTIMIZE_STRING
	if(byteSize > 0 && fixed == false)
	{
		static const unsigned int delta = 16 * sizeof(uchar);

		byteSize = (byteSize / delta + 1) * delta;
		if(byteSize == textByteSize)
			return true;
	}
	#endif

	if(byteSize == 0)
	{
		if(text)
			string_free ((void*)text);
		text = nullptr;
	}
	else
	{
		void* temp = text ? string_realloc ((void*)text, byteSize) : string_malloc (byteSize);
		if(temp == nullptr)
			return false;

		text = (uchar*)temp;
	}

	STRING_RESIZED (textByteSize, byteSize)

	textByteSize = byteSize;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult UnicodeStringBuffer::assignInternal (const uchar* charBuffer, int count)
{
	ASSERT (count >= 0)

	if(count == 0 || Text::isEmpty (charBuffer))
		releaseInternal ();
	else
	{
		if(!text || count > getLength ())
			if(!resizeInternal (count))
				return kResultOutOfMemory;

		Text::copyTo (text, charBuffer, count);
		text[count] = 0;
		updateMetadata (count);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult UnicodeStringBuffer::appendInternal (const uchar* charBuffer, int count)
{
	ASSERT (count >= 0)

	if(isEmpty ())
		return assignInternal (charBuffer, count);

	if(count == 0 || Text::isEmpty (charBuffer))
		return kResultOk;

	int oldLength = getLength ();
	int newLength = oldLength + count;
	if(!resizeInternal (newLength))
		return kResultOutOfMemory;

	Text::copyTo (text + oldLength, charBuffer, count);
	text[newLength] = 0;
	updateMetadata (newLength);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnicodeStringBuffer::updateMetadata (int newLength)
{
	textLength = newLength;
	hashCode = kInvalidHashCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::assignChars (const uchar* charBuffer, int count)
{
	ASSERT (ccl_is_aligned (charBuffer))

	if(count < 0)
		count = Text::getLength (charBuffer);
	else
		count = Text::getLength (charBuffer, count);

	return assignInternal (charBuffer, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::appendCString (TextEncoding encoding, const char* cString, int count)
{
	if(count == 0 || Text::isEmpty (cString))
		return kResultOk;

	#if 1 // simplified variant for ASCII, probably a little bit faster
	if(encoding == Text::kASCII)
	{
		int unicodeLength = count < 0 ? Text::getLength (cString) : Text::getLength (cString, count);

		int oldLength = getLength ();
		int newLength = oldLength + unicodeLength;
		if(!resizeInternal (newLength))
			return kResultOutOfMemory;

		uchar* dst = text + oldLength;
		for(int i = 0; i < unicodeLength; i++)
			dst[i] = cString[i];

		text[newLength] = 0;
		updateMetadata (newLength);
	}
	else
	#endif
	{
		if(!Text::isValidCStringEncoding (encoding))
			return kResultInvalidArgument;

		int unicodeLength = Text::convertToUnicode (nullptr, 0, encoding, cString, count);
		if(count == -1)
			unicodeLength--; // without null char
		if(unicodeLength < 1)
			return kResultUnexpected;

		int oldLength = getLength ();
		int newLength = oldLength + unicodeLength;
		if(!resizeInternal (newLength))
			return kResultOutOfMemory;

		uchar* dst = text + oldLength;
		int result = Text::convertToUnicode (dst, unicodeLength + 1, encoding, cString, count);
		text[newLength] = 0;
		if(result <= 0)
			return kResultFalse;

		newLength = oldLength + Text::getLength (text + oldLength, unicodeLength);
		updateMetadata (newLength);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::appendPascalString (TextEncoding encoding, const unsigned char* pString)
{
	if(!pString || (pString && !pString[0]))
		return kResultOk;

	unsigned char count = pString[0];
	return appendCString (encoding, (char*)pString + 1, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::appendChars (const uchar* charBuffer, int count)
{
	ASSERT (ccl_is_aligned (charBuffer))

	if(count < 0)
		count = Text::getLength (charBuffer);
	else
		count = Text::getLength (charBuffer, count);

	return appendInternal (charBuffer, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::appendString (const IString* _otherString)
{
	UnicodeStringBuffer* otherString = castToString<UnicodeStringBuffer> (_otherString);
	if(otherString && otherString->text)
		return appendInternal (otherString->text, otherString->textLength);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::appendRepeated (const IString* _otherString, int count)
{
	UnicodeStringBuffer* otherString = castToString<UnicodeStringBuffer> (_otherString);
	if(otherString && !otherString->isEmpty () && count > 0)
	{
		int oldLength = getLength ();
		int otherLength = otherString->getLength ();
		int newLength = oldLength + count * otherLength;
		if(!resizeInternal (newLength))
			return kResultOutOfMemory;

		uchar* dst = text + oldLength;
		for(int i = 0; i < count; i++, dst += otherLength)
			Text::copyTo (dst, otherString->text, otherLength + 1); // copy with null terminator
		updateMetadata (newLength);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::insert (int index, const IString* _otherString)
{
	UnicodeStringBuffer* otherString = castToString<UnicodeStringBuffer> (_otherString);
	if(!otherString)
		return kResultInvalidArgument;

	if(isEmpty ())
		return assign (*otherString);

	int oldLength = getLength ();
	int insertLength = otherString->getLength ();
	if(!insertLength || index < 0)
		return kResultOk;

	if(index >= oldLength)
		return appendInternal (otherString->text, insertLength);

	if(!resizeInternal (oldLength + insertLength))
		return kResultOutOfMemory;

	uchar* src = text + index;
	uchar* dst = text + index + insertLength;
	::memmove (dst, src, (oldLength - index + 1) * sizeof(uchar));
	Text::copyTo (src, otherString->text, insertLength);
	updateMetadata (oldLength + insertLength);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::remove (int index, int count)
{
	int length = getLength ();
	if(count < 0)
		count = length - index;

	if(index < 0 || index + count > length || count <= 0)
		return kResultOk;

	::memmove (text + index, text + index + count, (length - index - count) * sizeof(uchar));
	text[length - count] = 0;
	updateMetadata (length - count);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeStringBuffer::truncate (int index)
{
	if(index >= 0 && index < getLength ())
	{
		text[index] = 0;
		updateMetadata (index);
		return kResultOk;
	}
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeStringBuffer::trimWhitespace ()
{
	if(Text::isEmpty (text))
		return;

	auto isWhitespace = [] (uchar c)
	{
		switch(c)
		{
		case ' ' : case '\t' : case '\n' : case '\r' : return true;
		default : return false;
		}
	};

	int oldLength = getLength ();
	const uchar* start = text;
	while(*start && isWhitespace (*start))
		start++;

	const uchar* end = text + oldLength - 1;
	while(end > start && isWhitespace(*end))
		end--;

	int newLength = (int)(end+1 - start);
	if(newLength != oldLength)
	{
		if(start != text && newLength > 0)
			::memmove (text, start, newLength * sizeof(uchar));
		text[newLength] = 0;
		updateMetadata (newLength);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeStringBuffer::toUppercase ()
{
	if(isEmpty ())
		return;

	Text::toUppercase (text);
	updateMetadata (textLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeStringBuffer::toLowercase ()
{
	if(isEmpty ())
		return;

	Text::toLowercase (text);
	updateMetadata (textLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeStringBuffer::capitalize ()
{
	if(isEmpty ())
		return;

	Text::capitalize (text);
	updateMetadata (textLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeStringBuffer::replace (const IString* searchString, const IString* replacementString, int flags)
{
	if(!searchString || !replacementString || isEmpty ())
		return 0;

	struct SearchFuncs
	{
		static const uchar* searchMatchCase (const uchar* str, const uchar* searchStr)	{ return Text::findString (str, searchStr, 0); }
		static const uchar* searchIgnoreCase (const uchar* str, const uchar* searchStr)	{ return Text::findString (str, searchStr, Text::kIgnoreCase); }

		typedef const uchar* (Search) (const uchar*, const uchar*);
	};

	SearchFuncs::Search* searchFunc = flags & Text::kIgnoreCase ? &SearchFuncs::searchIgnoreCase : &SearchFuncs::searchMatchCase;

	UnicodeStringBuffer* searchStr = castToString<UnicodeStringBuffer> (searchString);
	int numReplaced = 0;

	if(const uchar* matchPtr = searchFunc (text, searchStr->text))
	{
		int matchPos = (int)(matchPtr - text);

		UnicodeStringBuffer* replaceStr = castToString<UnicodeStringBuffer> (replacementString);
		int replaceLength = replaceStr->getLength ();
		int searchLength = searchString->getLength ();

		// temp copy of old string
		AutoPtr<IString> source = cloneString ();
		CharData sourceData;
		source->getChars (sourceData);
		const uchar* sourcePtr = sourceData.text + matchPos; // first match pos in copy

		// start with chars before first match
		truncate (matchPos);

		// copy replacement for first match
		appendInternal (replaceStr->text, replaceLength);
		sourcePtr += searchLength;
		numReplaced++;

		while(matchPtr = searchFunc (sourcePtr, searchStr->text))
		{
			int matchPos = (int)(matchPtr - sourcePtr); // relative to current source pos

			// copy matchPos chars from source (chars before the match)
			appendInternal (sourcePtr, matchPos);
			sourcePtr += matchPos;

			// copy replacement for match
			appendInternal (replaceStr->text, replaceLength);
			sourcePtr += searchLength;
			numReplaced++;
		}

		if(*sourcePtr)
		{
			int remainingLength = (int)(sourceData.text + source->getLength () - sourcePtr);

			// copy remaining source
			appendInternal (sourcePtr, remainingLength);
		}

		releaseChars (sourceData);
	}
	return numReplaced;
}
