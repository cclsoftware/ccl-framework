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
// Filename    : ccl/public/text/stringbuilder.h
// Description : String Builder
//
//************************************************************************************************

#ifndef _ccl_stringbuilder_h
#define _ccl_stringbuilder_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {

//************************************************************************************************
// StringBuilder
/** Appends text items to a string, stops when a given number of items is reached.
	\ingroup text_string */
//************************************************************************************************

class StringBuilder
{
public:
	StringBuilder (String& string);

	/** Configuration. */
	PROPERTY_VARIABLE (int, maxItems, MaxItems)			///< maximum number of items, default: 20
	PROPERTY_STRING (itemSeparator, ItemSeparator)		///< string between items, default: "\n"
	PROPERTY_STRING (moreItemsMarker, moreItemsMarker)	///< placeholder string for items that were omitted, default: "..."

	/** Check if at least maxItems were added. */
	bool isLimitReached () const;

	/** Add a text item if possible. */
	void addItem (StringRef text);

private:
	String& string;
	int numItems;
};

//************************************************************************************************
// UIDString
/** Convert 16-Byte GUID to string representation (Unicode).
	\ingroup text_string */
//************************************************************************************************

class UIDString: public String
{
public:
	UIDString (UIDRef uid);

	/** Generate new GUID as string. */
	static String generate ();	
	
	/** Check if string is a valid GUID. */
	static bool verify (StringRef uidString);
};

//************************************************************************************************
// UIDCString
/** Convert 16-Byte GUID to string representation (C-String).
	\ingroup text_string */
//************************************************************************************************

class UIDCString: public MutableCString
{
public:
	UIDCString (UIDRef uid);

	static MutableCString generate ();	
};

//************************************************************************************************
// FourCCString
/** Converts a Four-character code to its string representation.
	\ingroup text_string */
//************************************************************************************************

class FourCCString: public String
{
public:
	FourCCString (uint32 fourCC);
	FourCCString (StringRef string): String (string) {}

	void appendFourCC (uint32 fourCC);
	uint32 getFourCC () const;
};

//************************************************************************************************
// WideCharString
/** Converts string to wchar_t representation, size is platform-dependent.
	\ingroup text_string */
//************************************************************************************************

class WideCharString
{
public:
	WideCharString (StringRef string);
	~WideCharString ();

	const wchar_t* str () const;

protected:
	#if CCL_UCHAR_COMPATIBLE_WITH_WCHAR_T
	StringChars chars;
	#else
	wchar_t* buffer;
	#endif
};

//************************************************************************************************
// StringUtils
//************************************************************************************************

namespace StringUtils
{
	/** If the string ends with a delimiter followed by a number, provide the number and optionally the remaining prefix before the delimiter.
		Delimiters are optional, but if they are given, one of them must precede the number for the method to succeed. */
	bool getLastIntValue (StringRef string, int64& value, String* prefix = nullptr, StringRef delimiters = String (" "));
	bool getLastIntValue (StringRef string, int& value, String* prefix = nullptr, StringRef delimiters = String (" "));
	bool isDigitsOnly (StringRef string);
	String strip (StringRef string, bool (*filter) (uchar c)); // keep only chars that match the filter

	class IndexedNameBuilder;
}

//************************************************************************************************
// StringUtils::IndexedNameBuilder
/** Helper for appending a running number to a name (e.g. to avoid duplicate names). 
	Tries to extract an existing number from requestedName as starting point.

	A trailing number in the optional originalName is treated as part of an immutable stem.
	The appended numbers are put in brackets instead in this case.

	The separator is the string between stem name and number (or opening bracket).
*/
//************************************************************************************************

class StringUtils::IndexedNameBuilder
{
public:
	IndexedNameBuilder (StringRef requestedName, StringRef originalName, int startNumber = 2, StringRef separator = " ");

	void nextName (String& name); ///< increases number

private:
	String stemName;
	String separator;
	bool useBrackets;
	int number;
};

//************************************************************************************************
// StringParser
/** Simple helper class for parsing a string. */
//************************************************************************************************

class StringParser
{
public:
	StringParser (StringRef string);

	bool advance (int numChars = 1);
	bool isEndOfString () const;

	uchar peek () const;
	uchar read ();
	bool read (uchar c);
	
	bool readUntil (String& string, StringRef delimiters);
	bool readUntilWhitespace (String& string);

	bool readToken (StringRef token);
	bool peekToken (StringRef token) const;

	bool skipUntil (StringRef token);
	void skipAny (StringRef characters);
	void skipWhitepace ();

	bool skipEmptyLine ();
	bool skipLineEnding ();

private:
	static const String kWhitespace;

	String string;
	int position;
};

} // namespace CCL

#endif // _ccl_stringbuilder_h
