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
// Filename    : ccl/public/text/istring.h
// Description : Unicode String Interface
//
//************************************************************************************************

#ifndef _ccl_istring_h
#define _ccl_istring_h

#include "ccl/public/textservices.h"
#include "ccl/public/text/istringprivate.h"

namespace CCL {

interface IStringTokenizer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// String macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCLSTR(asciiString)		CCL::System::GetConstantString (asciiString)
#define CCLSTRSIZE(charBuffer)	(sizeof(charBuffer)/sizeof(CCL::uchar))

//////////////////////////////////////////////////////////////////////////////////////////////////
// String options
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Text
{
	/** String function options. */
	enum StringOptions
	{
		kIgnoreCase = 1<<0,			///< ignore case when searching/comparing strings
		kReverseFind = 1<<1,		///< search backwards from string end to start
		kIgnoreDiacritic = 1<<2,	///< compare: ignore diacritic markers (umlauts)
		kPreserveEmptyToken = 1<<3,	///< tokenize: preserve empty token
		kCompareNumerically = 1<<4,	///< compare: treat groups of digits as numbers

		kStringOptionsLastFlag = 4
	};

	/** Result of string comparison. */
	enum CompareResult
	{
		kLess = -1,		///< this is less than comperand
		kEqual = 0,		///< this is equal to comperand
		kGreater = 1	///< this is greater than comperand
	};

	/** Unicode normalization form. */
	DEFINE_ENUM (NormalizationForm)
	{
		kNormalizationC,
		kNormalizationD,
		kNormalizationKC,
		kNormalizationKD
	};
}

using Text::NormalizationForm;

//************************************************************************************************
// IString
/** Platform-independent Unicode string interface using 16 bit code units in UTF-16 encoding.
	\ingroup text_string */
//************************************************************************************************

interface IString: IUnknown
{
	/** String character data. */
	struct CharData
	{
		const uchar* text;		///< read-only address of string characters
		int reserved;			///< for internal use

		CharData ()
		: text (nullptr),
		  reserved (0)
		{}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Constant (immutable) string methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Returns true if string is empty. */
	virtual tbool CCL_API isEmpty () const = 0;

	/** Returns string length in code units. */
	virtual int CCL_API getLength () const = 0;

	/** Returns code unit at specified position or null if out of range. */
	virtual uchar CCL_API getCharAt (int index) const = 0;

	/** Direct access to character data, might cause a temporary copy to be created. */
	virtual tresult CCL_API getChars (CharData& data) const = 0;

	/** Release character data, always call in pair after getChars(). */
	virtual tresult CCL_API releaseChars (CharData& data) const = 0;

	/** Copy string to external character buffer, size is in characters (uchar, not bytes). */
	virtual tresult CCL_API copyTo (uchar* charBuffer, int bufferSize) const = 0;

	/** Convert to C-String in specified encoding, size includes 0 termination byte. */
	virtual tresult CCL_API toCString (TextEncoding encoding, char* cString, int cStringSize, int* bytesWritten = nullptr) const = 0;

	/** Convert to Pascal string (with length byte) in specified encoding, size includes length byte. */
	virtual tresult CCL_API toPascalString (TextEncoding encoding, unsigned char* pString, int pStringSize) const = 0;

	/** Compare this with other string, returns CompareResult. */
	virtual int CCL_API compare (const IString* otherString, int flags = 0) const = 0;

	/** Compare this with given Unicode text, returns CompareResult. */
	virtual int CCL_API compareChars (const uchar* charBuffer, int count = -1) const = 0;

	/** Compare for equality with other string (usually faster than full comparison). */
	virtual tbool CCL_API equals (const IString* otherString) const = 0;

	/** Compare for equality with given Unicode text (usually faster than full comparison). */
	virtual tbool CCL_API equalsChars (const uchar* charBuffer, int count = -1) const = 0;

	/** Returns code unit index of substring occurance in this string, or -1 if not found. */
	virtual int CCL_API findSubString (const IString* otherString, int flags = 0) const = 0;

	/** Creates a copy of this string, starting at the specified index with count characters. */
	virtual IString* CCL_API createSubString (int index, int count = -1) const = 0;

	/** Break string into tokens at given delimiters. */
	virtual IStringTokenizer* CCL_API tokenize (const IString* delimiters, int flags = 0) const = 0;

	/** Creates a copy of this string. */
	virtual IString* CCL_API cloneString () const = 0;

	/**	Create an OS-specific representation of this string
		Windows: BSTR
		macOS/iOS: CFStringRef
		Android: jstring 
		Linux: 8 bit characters in system encoding */
	virtual void* CCL_API createNativeString () const = 0;

	/** Release OS-specific string representation created via createNativeString(). */
	virtual void CCL_API releaseNativeString (void* nativeString) const = 0;

	/** Returns private data for debugging purpose. */
	virtual IStringPrivateData CCL_API getPrivateData () const = 0;
	
	/** Hash string to integer value. */
	virtual unsigned int CCL_API getHashCode () const = 0;

	/** Check if string is normalized according to given form. */
	virtual tbool CCL_API isNormalized (NormalizationForm form) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Mutable string methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Assign a piece of Unicode text. */
	virtual tresult CCL_API assignChars (const uchar* charBuffer, int count = -1) = 0;

	/** Append C-String in specified encoding. */
	virtual tresult CCL_API appendCString (TextEncoding encoding, const char* cString, int count = -1) = 0;

	/** Append Pascal string (with length byte) in specified encoding. */
	virtual tresult CCL_API appendPascalString (TextEncoding encoding, const unsigned char* pString) = 0;

	/** Append a piece of Unicode text. */
	virtual tresult CCL_API appendChars (const uchar* charBuffer, int count = -1) = 0;

	/** Append another string's text. */
	virtual tresult CCL_API appendString (const IString* otherString) = 0;

	/** Appends count repetitions of a string. */
	virtual tresult CCL_API appendRepeated (const IString* otherString, int count) = 0;

	/** Append text from native string, see createNativeString() for OS-specific types. */
	virtual tresult CCL_API appendNativeString (const void* nativeString) = 0;

	/** Insert string at specified position. */
	virtual tresult CCL_API insert (int index, const IString* otherString) = 0;

	/** Remove a range of characters, -1 truncates the string at specified position */
	virtual tresult CCL_API remove (int index, int count = -1) = 0;

	/** Truncate string at specified position. */
	virtual tresult CCL_API truncate (int index) = 0;

	/** Remove leading and trailing whitespaces. */
	virtual void CCL_API trimWhitespace () = 0;

	/** Change all alphabetical characters to uppercase. */
	virtual void CCL_API toUppercase () = 0;

	/** Change all alphabetical characters to lowercase. */
	virtual void CCL_API toLowercase () = 0;

	/** Changes the first character in each word to uppercase. */
	virtual void CCL_API capitalize () = 0;
	
	/** Replace all occurances of a substring, returns number of instances replaced. */
	virtual int CCL_API replace (const IString* searchString, const IString* replacementString, int flags = 0) = 0;

	/** Changes characters to similar ones within the ASCII set, can change length. */
	virtual void CCL_API substitute (int flags = 0) = 0;

	/** Normalize characters according to to given form. */
	virtual tresult CCL_API normalize (NormalizationForm form) = 0;
	
	DECLARE_IID (IString)
};

//************************************************************************************************
// IStringTokenizer
/** Break string into tokens, created via IString::tokenize. 
	\ingroup text_string */
//************************************************************************************************

interface IStringTokenizer: IUnknown
{
	/** Returns true when all tokens delivered. */
	virtual tbool CCL_API done () const = 0;

	/** Returns next token string and its delimiter. */
	virtual StringRef CCL_API nextToken (uchar& delimiter) = 0;

	DECLARE_IID (IStringTokenizer)
};

//************************************************************************************************
// IFormattedString
/** Interface for scanning and printing formatted values to and from a string.
	\ingroup text_string */
//************************************************************************************************
/*
	Format Specification Syntax: %[type]([index]:[option])

	Type		Option				See also
	----------+--------------------+----------------
	hex, x		numPaddingZeros		appendHexValue
	int, i		numPaddingZeros		appendIntValue
	float, f	numDecimalDigits	appendFloatValue
	string, s	none				appendString

	Examples:

	Input		Format				Result
	----------+--------------------+----------------
	100			%hex(1:8)			"00000064"
	100			%int(1)				"100"
	100			%float(1:2)			"100.00"
	100			%(1)				"100"
	100			%string(1)			"" (not a string)
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

interface IFormattedString: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Constant (immutable) formatting methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Scan floating-point value represented by this string. */
	virtual tresult CCL_API getFloatValue (double& value) const = 0;

	/** Scan floating-point value represented by this string. */
	virtual tresult CCL_API getFloatValue (float& value) const = 0;

	/** Scan integer value represented by this string. */
	virtual tresult CCL_API getIntValue (int64& value) const = 0;

	/** Scan integer value represented by this string. */
	virtual tresult CCL_API getIntValue (int32& value) const = 0;

	/** Scan hexadecimal value represented by this string. */
	virtual tresult CCL_API getHexValue (int64& value) const = 0;

	/** Scan formatted arguments, similar to 'scanf' in C Library. */
	virtual int CCL_API scanFormat (const IString* format, Variant args[], int count) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Mutable formatting methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Append integer value, optionally padded with leading zeros to given number of digits. */
	virtual tresult CCL_API appendIntValue (int64 value, int numPaddingZeros = -1) = 0;

	/** Append hexadecimal value, optionally padded with leading zeros to given number of digits. */
	virtual tresult CCL_API appendHexValue (int64 value, int numPaddingZeros = -1) = 0;

	/** Append floating-point value, optionally limited to given number of digits after decimal point. */
	virtual tresult CCL_API appendFloatValue (double value, int numDecimalDigits = -1) = 0;

	/** Append formatted arguments, similar to 'printf' in C Library. */
	virtual tresult CCL_API appendFormat (const IString* format, Variant args[], int count) = 0;
	
	DECLARE_IID (IFormattedString)
};

//************************************************************************************************
// IUnicodeUtilities
/** Character classification and conversion for 2-Byte UTF-16 characters.
	\ingroup text_string */
//************************************************************************************************

interface IUnicodeUtilities: IUnknown
{
	/** Returns true if c is an alphabetic character. */
	virtual tbool CCL_API isAlpha (uchar c) const = 0;

	/** Returns true if c is an alphanumeric character. */
	virtual tbool CCL_API isAlphaNumeric (uchar c) const = 0;

	/** Returns true if c is a whitespace. */
	virtual tbool CCL_API isWhitespace (uchar c) const = 0;

	/** Returns true if c is digit. */
	virtual tbool CCL_API isDigit (uchar c) const = 0;

	/** Returns true if c is a valid ASCII character. */
	virtual tbool CCL_API isASCII (uchar c) const = 0;
	
	/** Returns true if c is a printable character. */
	virtual tbool CCL_API isPrintable (uchar c) const = 0;

	/** Returns true if c is a lowercase character. */
	virtual tbool CCL_API isLowercase (uchar c) const = 0;

	/** Returns true if c is an uppercase character. */
	virtual tbool CCL_API isUppercase (uchar c) const = 0;

	/** Returns true if c is a full width character. */
	virtual tbool CCL_API isFullWidth (uchar c) const = 0;

	/** Converts an uppercase character to lowercase. */
	virtual uchar CCL_API toLowercase (uchar c) const = 0;

	/** Converts a lowercase character to uppercase. */
	virtual uchar CCL_API toUppercase (uchar c) const = 0;

	DECLARE_IID (IUnicodeUtilities)
};

} // namespace CCL

#endif // _ccl_istring_h
