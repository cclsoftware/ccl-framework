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
// Filename    : core/public/corestringbuffer.h
// Description : C-String Buffer
//
//************************************************************************************************

#ifndef _corestringbuffer_h
#define _corestringbuffer_h

#include "core/public/corestringtraits.h"

namespace Core {

//************************************************************************************************
// ConstString
/** C-String pointer wrapper, safe for ASCII-encoded text only!
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

class ConstString: public CStringTraits<ConstString>,
				   public CStringClassifier
{
public:
	ConstString (CStringPtr text = nullptr)
	: text (safe_str (text))
	{}
	
protected:
	CStringPtr text;

	// required by CStringTraits:
	friend class CStringTraits<ConstString>;
	CStringPtr __str () const { return text; }
};

//************************************************************************************************
// CStringBuffer
/** Fixed-size C-String buffer class, safe for ASCII-encoded text only!
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

template <int maxSize>
class CStringBuffer: public CStringTraits<CStringBuffer<maxSize> >,
					 public MutableCStringTraits<CStringBuffer<maxSize> >,
					 public CStringClassifier
{
public:
	CStringBuffer (CStringPtr text = nullptr);

	/** Empty string. */
	CStringBuffer& empty ();

	/** Truncate string at given position. */
	CStringBuffer& truncate (int index);

	/** Insert string at given position. */
	CStringBuffer& insert (int index, CStringPtr otherString);

	/** Remove characters at given position. */
	CStringBuffer& remove (int index, int count);

	/** Replace range by other string. */
	CStringBuffer& replace (int index, int count, CStringPtr otherString);

	/** Replace all occurrences of one character with another. */
	CStringBuffer& replace (char oldChar, char newChar);

	/** Create substring. */
	void subString (CStringBuffer& result, int index, int count = -1) const;

	/** Remove leading/trailing whitespace, but not in between any other characters. */
	CStringBuffer& trimWhitespace ();
	
	using CStringClassifier::toLowercase;
	using CStringClassifier::toUppercase;

	/** convert to lower case string. */
	CStringBuffer<maxSize>& toLowercase ();
	
	/** convert to upper case string. */
	CStringBuffer<maxSize>& toUppercase ();

	/** Assign C-String. */
	CStringBuffer& operator = (CStringPtr text);

	/** Assign integer value (replaces previous content). */
	CStringBuffer& assignInteger (int value);

	/** Get buffer pointer. */
	char* getBuffer ();

	/** Get buffer size. */
	int getSize () const;

protected:
	char buffer[maxSize];

	// required by CStringTraits:
	friend class CStringTraits<CStringBuffer>;
	CStringPtr __str () const { return buffer; }

	// required by MutableCStringTraits:
	friend class MutableCStringTraits<CStringBuffer>;
	CStringBuffer& __append (CStringPtr string, int count);
	CStringBuffer& __init (CStringPtr string);
};

/** 16 character C-String. \ingroup core_string \ingroup text_string */
typedef CStringBuffer<16> CString16;

/** 32 character C-String. 	\ingroup core_string \ingroup text_string */
typedef CStringBuffer<32> CString32;

/** 64 character C-String. 	\ingroup core_string \ingroup text_string */
typedef CStringBuffer<64> CString64;

/** 128 character C-String. \ingroup core_string \ingroup text_string */
typedef CStringBuffer<128> CString128;

/** 256 character C-String. \ingroup core_string \ingroup text_string */
typedef CStringBuffer<256> CString256;

//////////////////////////////////////////////////////////////////////////////////////////////////
// CStringBuffer inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <int maxSize>
CStringBuffer<maxSize>::CStringBuffer (CStringPtr text)
{
	buffer[0] = 0;
	if(text)
		CStringBuffer<maxSize>::init (text);
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::empty ()
{
	buffer[0] = 0;
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::truncate (int index)
{
	if(index >= 0 && index < CStringBuffer<maxSize>::length ())
		buffer[index] = 0;
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::insert (int index, CStringPtr otherString)
{
	if(CStringBuffer<maxSize>::isEmpty ())
		return CStringBuffer<maxSize>::append (otherString);

	int oldLength = CStringBuffer<maxSize>::length ();
	int insertLength = (int)::strlen (CStringBuffer<maxSize>::safe_str (otherString));
	if(!insertLength || index < 0)
		return *this;

	if(index >= oldLength)
		return CStringBuffer<maxSize>::append (otherString);

	if(oldLength + insertLength >= maxSize)
		return *this;

	char* src = buffer + index;
	char* dst = buffer + index + insertLength;
	::memmove (dst, src, oldLength - index + 1);
	::strncpy (src, otherString, insertLength);
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::remove (int index, int count)
{
	int length = this->length ();
	if(count < 0)
		count = length - index;

	if(index < 0 || index + count > length || count <= 0)
		return *this;

	::memmove (buffer + index, buffer + index + count, length - index - count);
	buffer[length - count] = 0;
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::replace (int index, int count, CStringPtr otherString)
{
	int length = this->length ();
	if(count < 0)
		count = length - index;

	if(index < 0 || index + count > length || count <= 0)
		return *this;

	int insertLength = (int)::strlen (CStringBuffer<maxSize>::safe_str (otherString));
	if(length + insertLength - count >= maxSize)
		return *this;

	// move remainder up / down, including terminating 0
	int replaceEnd = index + count;
	::memmove (buffer + index + insertLength, buffer + replaceEnd, length - replaceEnd + 1);

	// copy replacement into the gap
	::strncpy (buffer + index, otherString, insertLength);
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::replace (char oldChar, char newChar)
{
	for(char* ptr = buffer; *ptr; ptr++)
		if(*ptr == oldChar)
			*ptr = newChar;

	return *this;
}

template <int maxSize>
void CStringBuffer<maxSize>::subString (CStringBuffer<maxSize>& result, int index, int count) const
{
	if(index < 0 || index >= CStringBuffer<maxSize>::length ())
		return;
	if(count < 0)
		count = CStringBuffer<maxSize>::length () - index;
	if(count < 0)
		return;

	if(count)
	{
		char* dst = result.buffer;
		::memmove (dst, CStringBuffer<maxSize>::str () + index, count);
		dst[count] = 0;
	}
}
	
template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::trimWhitespace ()
{
	// remove leading/trailing whitespace, but not in between any other characters
	int len = CStringBuffer<maxSize>::length ();
	if(len == 0)
		return *this;
	
	CStringPtr startPtr = buffer;
	CStringPtr s1 = startPtr;
	CStringPtr endPtr = buffer + len - 1;
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
		if(whiteSpaceAtEnd > 0) // trim end
			((char*)s2)[1] = 0;
		
		if(whiteSpaceAtStart > 0)
			::memmove ((char*)startPtr, s1, len + 1 - whiteSpaceAtEnd - whiteSpaceAtStart);
	}
	return *this;

}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::toLowercase ()
{
	int len = CStringBuffer<maxSize>::length ();
	for (int pos = 0; pos < len; pos++)
	{
		CStringBuffer<maxSize>::getBuffer ()[pos] = (char)::tolower (CStringBuffer<maxSize>::getBuffer ()[pos]);
	}
	return *this;
}
	
template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::toUppercase ()
{
	int len = CStringBuffer<maxSize>::length ();
	for (int pos = 0; pos < len; pos++)
	{
		CStringBuffer<maxSize>::getBuffer ()[pos] = (char)::toupper (CStringBuffer<maxSize>::getBuffer ()[pos]);
	}
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::__append (CStringPtr string, int count)
{
	if(count == -1)
		count = (int)::strlen (CStringBuffer<maxSize>::safe_str (string));
	if(count <= 0)
		return *this;

	int thisLength = CStringBuffer<maxSize>::length ();
	int maxCount = maxSize-1 - thisLength;
	if(maxCount <= 0)
		return *this;
	if(count > maxCount)
		count = maxCount;

	::strncat (buffer, string, count);
	buffer[maxSize-1] = 0;
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::__init (CStringPtr string)
{
	int	count = (int)::strlen (CStringBuffer<maxSize>::safe_str (string));
	if(count <= 0)
		return *this;

	int maxCount = maxSize-1;
	if(count > maxCount)
		count = maxCount;

	::strncpy (buffer, string, count);
	buffer[count] = 0;
	return *this;
}

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::assignInteger (int value)
{
	int numDigits = 1;
	bool isNegative = value < 0;
	if(isNegative)
	{
		value = -value;
		numDigits++;
	}

	// determine number of digits
	int dimension = 10;
	while(value >= dimension)
	{
		dimension *= 10;
		numDigits++;
	}

	int pos = numDigits;
	buffer[numDigits] = 0;
	do
	{
		char digit = value % 10;
		value /= 10;
		buffer[--pos] = '0' + digit;
	} while(value > 0);

	if(isNegative)
		buffer[--pos] = '-';

	return *this;
};

template <int maxSize>
CStringBuffer<maxSize>& CStringBuffer<maxSize>::operator = (CStringPtr text)
{ empty (); return CStringBuffer<maxSize>::init (text); }

template <int maxSize>
char* CStringBuffer<maxSize>::getBuffer () { return buffer; }

template <int maxSize>
int CStringBuffer<maxSize>::getSize () const { return maxSize; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _cstringbuffer_h
