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
// Filename    : ccl/text/strings/unicodestring.h
// Description : Unicode String Implementation
//
//************************************************************************************************

#ifndef _ccl_unicodestring_h
#define _ccl_unicodestring_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/istring.h"

namespace CCL {

class UnicodeString;

//************************************************************************************************
// Text functions
//************************************************************************************************

namespace Text
{
	/** Returns true if text is empty. */
	template <typename CharType> bool isEmpty (const CharType* text)
	{
		return !text || text[0] == 0;
	}

	/** Returns length of null-terminated string. */
	template <typename CharType> int getLength (const CharType* text)
	{
		int length = 0;
		for(; *text; text++, length++);
		return length;
	}

	/** Returns length of a string that might not be terminated. */
	template <class CharType> int getLength (const CharType* text, int count)
	{
		int length = 0;
		for(; length < count && *text; text++, length++);
		return length;
	}

	/** Returns true if strings are equal. */
	template <class CharType> bool stringsEqual (const CharType* t1, const CharType* t2, int count)
	{
		bool equal = true;
		for(int i = 0; i < count && equal; i++)
			equal = (t1[i] == t2[i]);
		return equal;
	}

	/** Copies the first count characters of source to destination. */
	template <typename CharType> CharType* copyTo (CharType* dst, const CharType* src, int count)
	{
		for(int i = 0; i < count; i++)
		{
			dst[i] = src[i];
			if(src[i] == 0)
				break;
		}
		return dst;
	}

	/** Append characters. */
	template <typename CharType> void append (CharType* dst, const CharType* src, int count)
	{
		while(*dst)
			dst++;

		for(int i = 0; i < count; i++)
		{
			if(src[i] == 0)
				break;
			*dst++ = src[i];
		}
	}
}

//************************************************************************************************
// IUnicodeStringInternal
//************************************************************************************************

interface IUnicodeStringInternal: IUnknown
{
	virtual UnicodeString* getObject () = 0;

	DECLARE_IID (IUnicodeStringInternal)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Cast IString to internal representation. */
template <class T>
inline T* castToString (const IString* iString)
{
	IUnicodeStringInternal* iStringInternal = nullptr;
	if(iString) const_cast<IString*> (iString)->queryInterface (ccl_iid<IUnicodeStringInternal> (), (void**)&iStringInternal);
	return iStringInternal ? (T*)iStringInternal->getObject () : nullptr;
}

//************************************************************************************************
// UnicodeString
//************************************************************************************************

class UnicodeString: public Unknown,
					 public IString,
					 public IFormattedString,
					 public IUnicodeStringInternal
{
public:
	static UnicodeString* newString (); ///< platform-specific!

	virtual tresult makeConstant (const char* asciiString) = 0;
	virtual void releaseInternal () = 0;

	// IString
	IStringTokenizer* CCL_API tokenize (const IString* delimiters, int flags = 0) const override;
	unsigned int CCL_API getHashCode () const override;
	void CCL_API substitute (int flags = 0) override;

	// IFormattedString
	tresult CCL_API getFloatValue (float& value) const override;
	tresult CCL_API getFloatValue (double& value) const override;
	tresult CCL_API getIntValue (int32& value) const override;
	tresult CCL_API getIntValue (int64& value) const override;
	tresult CCL_API getHexValue (int64& value) const override;
	int CCL_API scanFormat (const IString* format, Variant args[], int count) const override;
	tresult CCL_API appendIntValue (int64 value, int numPaddingZeros = 0) override;
	tresult CCL_API appendHexValue (int64 value, int numPaddingZeros = 0) override;
	tresult CCL_API appendFloatValue (double value, int numDecimalDigits = -1) override;
	tresult CCL_API appendFormat (const IString* format, Variant args[], int count) override;

	// IUnicodeStringInternal
	UnicodeString* getObject () override { return this; }

	CLASS_INTERFACES (Unknown)

	static const int kTempStringSize = 256;
	typedef char TempString[kTempStringSize];
	static void makeFormatString (TempString format, const char* prefix, int value, const char* suffix);
};

//************************************************************************************************
// UnicodeUtilities
//************************************************************************************************

class UnicodeUtilities: public Unknown,
						public IUnicodeUtilities
{
public:
	static UnicodeUtilities& getInstance (); ///< platform-specific!

	// IUnicodeUtilities
	tbool CCL_API isAlpha (uchar c) const override;
	tbool CCL_API isAlphaNumeric (uchar c) const override;
	tbool CCL_API isWhitespace (uchar c) const override;
	tbool CCL_API isDigit (uchar c) const override;
	tbool CCL_API isASCII (uchar c) const override;
	tbool CCL_API isPrintable (uchar c) const override;
	tbool CCL_API isLowercase (uchar c) const override;
	tbool CCL_API isUppercase (uchar c) const override;
	tbool CCL_API isFullWidth (uchar c) const override;
	uchar CCL_API toLowercase (uchar c) const override;
	uchar CCL_API toUppercase (uchar c) const override;

	CLASS_INTERFACE (IUnicodeUtilities, Unknown)
};

} // namespace CCL

#endif // _ccl_unicodestring_h
