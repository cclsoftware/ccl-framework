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
// Filename    : ccl/text/strings/unicode-cross-platform/ucharfunctions.h
// Description : Platform-independent Unicode functions
//
//************************************************************************************************

#ifndef _ucharfunctions_h
#define _ucharfunctions_h

#include "ccl/public/collections/vector.h"

#include "core/text/coreutfcodec.h"

namespace CCL {

//************************************************************************************************
// UCharSet - sorted set of UTF16 characters
//************************************************************************************************

class UCharSet
{
public:
	UCharSet (const uchar* data, int count)
	: vector (data, count)
	{}

	bool contains (uchar c) const
	{
		return vector.search (c) != nullptr;
	}

	int index (uchar c) const
	{
		const uchar* result = vector.search (c);
		return result ? (int)(result - vector.getItems ()) : -1;
	}

	operator const uchar* () const
	{
		return vector.getItems ();
	}

protected:
	ConstVector<uchar> vector;
};

//************************************************************************************************
// UCharMapping - sorted mapping of UTF16 characters
//************************************************************************************************

class UCharMapping
{
public:
	struct Item
	{
		uchar key;
		uchar value;

		bool operator == (const Item& other) const
		{
			return key == other.key;
		}

		bool operator > (const Item& other) const
		{
			return key > other.key;
		}
	};

	UCharMapping (const Item* items, int count)
	: vector (items, count)
	{}

	uchar lookup (uchar key) const
	{
		Item keyItem = {key, 0};
		Item* result = vector.search (keyItem);
		return result ? result->value : 0;
	}

protected:
	ConstVector<Item> vector;
};

//************************************************************************************************
// UCharFunctions - UTF16 character and string functions, safe for BMP only... or even less!
//************************************************************************************************

namespace UCharFunctions
{
	using namespace Core::Text::UTFFunctions; // import corelib functions

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Tables
	//////////////////////////////////////////////////////////////////////////////////////////////

	extern const UCharSet lowercaseCharacterSet;
	extern const UCharSet uppercaseCharacterSet;
	extern const UCharSet numericCharacterSet;
	extern const UCharSet whitespaceCharacterSet;
	extern const UCharMapping encodingTableASCII;
	extern const UCharMapping encodingTableLatin1;
	extern const UCharMapping encodingTableCP437;
	extern const UCharMapping decodingTableCP437;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Character classification
	//////////////////////////////////////////////////////////////////////////////////////////////

	inline bool isUppercase (uchar c)
	{
		return uppercaseCharacterSet.contains (c);
	}

	inline bool isLowercase (uchar c)
	{
		return lowercaseCharacterSet.contains (c);
	}

	inline bool isAlpha (uchar c)
	{
		// TODO: this doesn't include all alphabetic characters...
		return uppercaseCharacterSet.contains (c) || lowercaseCharacterSet.contains (c);
	}

	inline bool isNumeric (uchar c)
	{
		return numericCharacterSet.contains (c);
	}

	inline bool isAlphaNumeric (uchar c)
	{
		return isAlpha (c) || isNumeric (c);
	}

	inline bool isDigit (uchar c) // checks for decimal digits only (0..9)
	{
		return c >= 0x0030 && c <= 0x0039;
	}

	inline bool isWhitespace (uchar c)
	{
		return whitespaceCharacterSet.contains (c);
	}

	inline bool isSurrogatePair (uchar c)
	{
		return c >= 0xD800 && c <= 0xDBFF;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Case conversion
	//////////////////////////////////////////////////////////////////////////////////////////////

	inline uchar toUppercase (uchar c)
	{
		int index = lowercaseCharacterSet.index (c);
		return index != -1 ? uppercaseCharacterSet[index] : c;
	}

	inline uchar toLowercase (uchar c)
	{
		int index = uppercaseCharacterSet.index (c);
		return index != -1 ? lowercaseCharacterSet[index] : c;
	}

	inline void toUppercase (uchar* s, int l)
	{
		ASSERT (l >= 0)

		for(int i = 0; i < l; i++)
		{
			*s = toUppercase (*s);
			s++;
		}
	}

	inline void toLowercase (uchar* s, int l)
	{
		ASSERT (l >= 0)

		for(int i = 0; i < l; i++)
		{
			*s = toLowercase (*s);
			s++;
		}
	}

	inline void capitalize (uchar* s, int l)
	{
		ASSERT (l >= 0)

		uchar lastChar = 0x20;
		for(int i = 0; i < l; i++, lastChar = *s++)
		{
			if(!isAlpha (lastChar))
			{
				if(isLowercase (*s))
					*s = toUppercase (*s);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Comparison
	//////////////////////////////////////////////////////////////////////////////////////////////

	inline int compareStrings (const uchar* s1, int l1, const uchar* s2, int l2, bool ignoreCase)
	{
		ASSERT (l1 >= 0 && l2 >= 0)

		for(int i = 0; ; i++)
		{
			uchar c1 = i < l1 ? s1[i] : 0;
			uchar c2 = i < l2 ? s2[i] : 0;

			if(ignoreCase == true)
			{
				c1 = toLowercase (c1);
				c2 = toLowercase (c2);
			}

			int diff = c1 - c2;
			if(diff != 0)
				return diff < 0 ? -1 : 1;
			if(c1 == 0)
				break;
		}
		return 0;
	}

	int compareStringsNumerically (const uchar* a, const uchar* b, bool ignoreCase);
	
	inline const uchar* findString (const uchar* source, int sLength, const uchar* value, int vLength, bool ignoreCase)
	{
		ASSERT (sLength >= 0 && vLength >= 0)

		int offset = 0;
		for(const uchar* src = source; offset < sLength; src++, offset++)
		{
			int count = sLength-offset;
			if(vLength < count)
				count = vLength;
			if(compareStrings (src, count, value, vLength, ignoreCase) == 0)
				return src;
		}
		return nullptr;
	}

	inline const uchar* findStringReverse (const uchar* source, int sLength, const uchar* value, int vLength, bool ignoreCase)
	{
		const uchar* lastResult = nullptr;
		const uchar* startInSource;
		while((startInSource = findString (source, sLength, value, vLength, ignoreCase)) != nullptr)
		{
			lastResult = startInSource;
			int offset = (int)(startInSource - source);
			sLength -= offset + 1;
			source += offset + 1;
		}
		return lastResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// C-String encoding
	//////////////////////////////////////////////////////////////////////////////////////////////

	typedef uchar (*UCharFunction) (uchar c);

	template <UCharFunction encode>
	int encodeCString (char* cString, int cStringSize, const uchar* uString, int uStringLength)
	{
		ASSERT (uStringLength >= 0)

		unsigned char* dest = (unsigned char*)cString;
		int used = 0;
		for(int i = 0; i < uStringLength; i++)
		{
			unsigned char c = (unsigned char)encode (uString[i]);
			if(dest)
			{
				if(used + 1 >= cStringSize)
					break;
				dest[used] = c;
			}
			used++;
		}

		if(dest && used < cStringSize)
			dest[used++] = 0; // null terminator

		return used;
	}

	template <UCharFunction decode>
	int decodeCString (uchar* uString, int uStringSize, const char* cString, int cStringLength)
	{
		ASSERT (cStringLength >= 0)

		Core::Text::UTF16Writer writer (uString, uStringSize);
		for(int i = 0; i < cStringLength; i++)
			if(!writer.writeNext (decode (cString[i])))
				break;

		writer.finish ();
		return writer.getLength ();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// ASCII encoding
	//////////////////////////////////////////////////////////////////////////////////////////////

	inline uchar encodeASCII (uchar c)
	{
		if(c < 0x80)
			return c;
		else if(!isSurrogatePair (c))
		{
			uchar result = encodingTableASCII.lookup (c);
			if(result)
				return result;
		}
		return '?';
	}

	inline uchar decodeASCII (uchar c)
	{
		ASSERT (c < 0x80)
		return c;
	}

	inline int encodeASCII (char* cString, int cStringSize, const uchar* uString, int uStringLength)
	{
		return encodeCString<encodeASCII> (cString, cStringSize, uString, uStringLength);
	}

	inline int decodeASCII (uchar* uString, int uStringSize, const char* cString, int cStringLength)
	{
		return decodeCString<decodeASCII> (uString, uStringSize, cString, cStringLength);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// ISO Latin 1 encoding
	//////////////////////////////////////////////////////////////////////////////////////////////

	inline uchar encodeISOLatin1 (uchar c)
	{
		if(c < 0x100)
			return c;
		else if(!isSurrogatePair (c))
		{
			uchar result = encodingTableLatin1.lookup (c);
			if(result)
				return result;
		}
		return '?';
	}

	inline uchar decodeISOLatin1 (uchar c)
	{
		ASSERT (c < 0x100)
		return c;
	}

	inline int encodeISOLatin1 (char* cString, int cStringSize, const uchar* uString, int uStringLength)
	{
		return encodeCString<encodeISOLatin1> (cString, cStringSize, uString, uStringLength);
	}

	inline int decodeISOLatin1 (uchar* uString, int uStringSize, const char* cString, int cStringLength)
	{
		return decodeCString<decodeISOLatin1> (uString, uStringSize, cString, cStringLength);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// DOS Latin US (CP 437) encoding
	//////////////////////////////////////////////////////////////////////////////////////////////

	inline uchar encodeDOSLatinUS (uchar c)
	{
		if(c < 0x80)
			return c;
		else if(!isSurrogatePair (c))
		{
			uchar result = encodingTableCP437.lookup (c);
			if(result)
				return result;
		}
		return '?';
	}

	inline uchar decodeDOSLatinUS (uchar c)
	{
		ASSERT (c < 0x100)
		if(c < 0x80)
			return c;
		else
		{
			uchar result = decodingTableCP437.lookup (c);
			if(result)
				return result;
		}
		ASSERT (0) // must not get here!
		return '?';
	}

	inline int encodeDOSLatinUS (char* cString, int cStringSize, const uchar* uString, int uStringLength)
	{
		return encodeCString<encodeDOSLatinUS> (cString, cStringSize, uString, uStringLength);
	}

	inline int decodeDOSLatinUS (uchar* uString, int uStringSize, const char* cString, int cStringLength)
	{
		return decodeCString<decodeDOSLatinUS> (uString, uStringSize, cString, cStringLength);
	}
}

} // namespace CCL

#endif // _ucharfunctions_h
