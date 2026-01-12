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
// Filename    : ccl/public/text/cstring.cpp
// Description : C-String Class
//
//************************************************************************************************

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/base/debug.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (ICString, 0x793cf11a, 0xed86, 0x4913, 0x93, 0xe, 0x8, 0xd, 0x8f, 0x39, 0x15, 0xd)

//************************************************************************************************
// CString
//************************************************************************************************

const CString CString::kEmpty;

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef CString::getLineEnd (TextLineFormat lineFormat)
{
	static const CString CRLFLineEnd ("\r\n");
	static const CString CRLineEnd ("\r");
	static const CString LFLineEnd ("\n");

	return	lineFormat == Text::kCRLFLineFormat ? CRLFLineEnd :
			lineFormat == Text::kCRLineFormat ? CRLineEnd :
			LFLineEnd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString::CString (CStringPtr _text)
{
	text = _text;
	theString = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString::CString (CStringRef other)
{
	text = other.text;
	theString = other.theString;
	if(theString)
		theString->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString::~CString ()
{
	if(theString)
		theString->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString& CString::operator = (CStringPtr _text)
{
	text = _text;
	safe_release (theString);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString& CString::operator = (CStringRef other)
{
	text = other.text;
	take_shared (theString, other.theString);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString CString::subString (int index, int count) const
{
	MutableCString result;

	if(index < 0 || index >= length ())
		return result;
	if(count < 0)
		count = length () - index;
	if(count < 0)
		return result;

	if(!result.resize (count))
		return result;

	if(count)
	{
		char* dst = (char*)result.str ();
		::memcpy (dst, str () + index, count);
		dst[count] = 0;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString CString::getBetween (CStringPtr prefix, CStringPtr suffix) const
{
	MutableCString result;
	getBetween (result, prefix, suffix);
	return result;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CString::getBetween (MutableCString& result, CStringPtr prefix, CStringPtr suffix) const
{
	result.empty ();
	if(startsWith (prefix))
	{
		int startIndex = index (suffix);
		if(startIndex > 0)
		{
			int prefixLength = CString (prefix).length ();
			result = subString (prefixLength, startIndex - prefixLength);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CString::toUnicode (String& string, TextEncoding encoding) const
{
	string.empty ();
	string.appendCString (encoding, *this);
}

//************************************************************************************************
// MutableCString
//************************************************************************************************

MutableCString::MutableCString (CStringPtr text)
: CString (text)
{
	if(!isEmpty ())
		initString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString::MutableCString (CStringRef string)
: CString (string)
{
	if(!isEmpty ())
		initString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString::MutableCString (const MutableCString& string)
: CString (string)
{
	if(!isEmpty ())
		initString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString::MutableCString (StringRef string, TextEncoding encoding)
{
	if(!string.isEmpty ())
		append (string, encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::operator = (CStringPtr text)
{
	CString::operator = (text);
	initString ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::operator = (CStringRef string)
{
	CString::operator = (string);
	initString ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::operator = (const MutableCString& string)
{
	CString::operator = (string);
	initString ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::operator = (StringRef string)
{
	empty ();
	if(!string.isEmpty ())
		append (string);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::operator += (StringRef string)
{
	return append (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::empty ()
{
	safe_release (theString);
	text = nullptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MutableCString::initString ()
{
	// force copy if plain string...
	if(!theString)
	{
		theString = System::CreateMutableCString (text);
		ASSERT (theString != nullptr)
		text = theString->getText ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MutableCString::writeEnable ()
{
	initString ();

	AutoPtr<ICString> releaser (theString);
	if(theString->retain () == 2)
		return;

	theString->release ();
	theString = theString->cloneString ();
	ASSERT (theString != nullptr)
	text = theString->getText ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MutableCString::resize (int newLength)
{
	writeEnable ();

	if(!theString->resize (newLength))
		return false;

	text = theString->getText ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::__append (CStringPtr _string, int count)
{
	CString string (_string);

	int stringLength = count < 0 ? string.length () : count;
	if(stringLength <= 0)
		return *this;

	int thisLength = length ();
	int newLength = thisLength + stringLength;
	if(!resize (newLength))
		return *this;

	::memcpy ((void*)(text + thisLength), string, stringLength);
	((char*)text)[newLength] = 0;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::append (StringRef string, TextEncoding encoding)
{
	// buffer estimation for variable-length encoding
	int encodingFactor = Text::getMaxEncodingBytesPerCharacter (encoding);

	int stringLength = string.length ();
	if(stringLength <= 0)
		return *this;

	int thisLength = length ();
	int newLength = thisLength + encodingFactor * stringLength;
	if(!resize (newLength))
		return *this;

	string.toCString (encoding, (char*)(text + thisLength), encodingFactor * stringLength + 1);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::truncate (int index)
{
	writeEnable ();

	if(index >= 0 && index < length ())
		((char*)text)[index] = 0;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::insert (int index, CStringPtr otherString)
{
	if(isEmpty ())
		return append (otherString);

	int oldLength = length ();
	int insertLength = CString (otherString).length ();
	if(!insertLength || index < 0)
		return *this;

	if(index >= oldLength)
		return append (otherString);

	if(!resize (oldLength + insertLength))
		return *this;

	char* src = (char*)text + index;
	char* dst = (char*)text + index + insertLength;
	::memmove (dst, src, oldLength - index + 1);
	::strncpy (src, otherString, insertLength);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::replace (int index, int count, CStringPtr otherString)
{
	int length = this->length ();
	if(count < 0)
		count = length - index;

	if(index < 0 || index + count > length || count <= 0)
		return *this;

	int insertLength = CString (otherString).length ();
	if(insertLength > count)
	{
		if(!resize (length + insertLength - count))
			return *this;
	}
	else
		writeEnable ();

	// move remainder up / down, including terminating 0
	int replaceEnd = index + count;
	::memmove ((char*)text + index + insertLength, (char*)text + replaceEnd, length - replaceEnd + 1);

	// copy replacement into the gap
	::strncpy ((char*)text + index, otherString, insertLength);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::replace (char oldChar, char newChar)
{
	writeEnable ();

	if(text)
		for(char* ptr = (char*)text; *ptr; ptr++)
			if(*ptr == oldChar)
				*ptr = newChar;

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::toLowercase ()
{
	writeEnable ();

	if(text)
	{
		char *a = (char*)text;
		while(*a != '\0')
		{
		  if(isupper (*a))
			*a = (char)tolower (*a);
		  ++a;
		}
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::toUppercase ()
{
	writeEnable ();

	if(text)
	{
		char *a = (char*)text;

		while(*a != '\0')
		{
			if(islower (*a))
				*a = (char)toupper (*a);
			++a;
		}
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString& MutableCString::trimWhitespace ()
{
	int len = length ();
	if(len == 0)
		return *this;

	CStringPtr startPtr = text;
	CStringPtr s1 = startPtr;
	CStringPtr endPtr = text + len - 1;
	CStringPtr s2 = endPtr;
	while(*s1 && isWhitespace (*s1))
		s1++;

	while(s2 > startPtr && isWhitespace (*s2))
		s2--;

	if(s1 > s2)
	{
		empty ();
		return *this;
	}

	long whiteSpaceAtStart = long (s1 - startPtr);
	long whiteSpaceAtEnd = long (endPtr - s2);
	if(whiteSpaceAtStart > 0 || whiteSpaceAtEnd > 0)
	{
		writeEnable ();
		startPtr = text; // refresh startPtr (writeEnable can change it!)

		if(whiteSpaceAtEnd > 0) // trim end
			((char*)s2)[1] = 0;

		if(whiteSpaceAtStart > 0)
			memmove ((char*)startPtr, s1, len + 1 - whiteSpaceAtEnd - whiteSpaceAtStart);
	}
	return *this;
}

