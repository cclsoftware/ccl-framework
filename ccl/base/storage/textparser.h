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
// Filename    : ccl/base/storage/textparser.h
// Description : Simple text parser class
//
//************************************************************************************************

#ifndef _ccl_textparser_h
#define _ccl_textparser_h

#include "ccl/public/text/cstring.h"

namespace CCL {

interface IStream;

//************************************************************************************************
// TextParser
//************************************************************************************************

class TextParser
{
public:
	TextParser (IStream& stream);
	virtual ~TextParser ();

	void addWhitespace (uchar c);
	void addIdentifierChar (uchar c);

	// 1 character lookahead
	uchar peek () const;
	bool advance ();

	// characters & strings
	uchar read ();											///< read the next character
	bool read (uchar c);									///< try to read the given character, advances on success
	int read (String& string, int length);					///< read upto length characters
	bool readUntil (CStringPtr delimiters, String& string);	///< read until one of the delimiters characters is found 
	bool readUntil (uchar delimiter, String& string);		///< read until delimiter is found, delimiter is overread but not copied to string
	bool readUntil (StringRef delimiter, String& string);	///< read until delimiter is found, delimiter is overread but not copied to string
	String& readIdentifier (String&);						///< starts with a letter, followed by letters & digits
	void readIdentifier (char* ident, int bufferSize);		///< starts with a letter, followed by letters & digits
	bool readPropertyPath (MutableCString& string);			///< read a property path, may contain letters & digits, and : . /
	String& readStringLiteral (String&, uchar quote = '"');	///< any text in ".." todo: escaping

	// numbers
	bool readFloat (float&, bool withExponent = true);		///< eg. "1.", ".1", "1.1", withExponent: "1e-12", 2.2E6"
	bool readFloat (double&, bool withExponent = true);
	bool readInt (int&);
	bool readInt (int64&);

	void skipWhite ();
	bool skipLine ();              ///< skip the rest of current line
	bool readLine (String& line);  ///< read the rest of current line

private:
	IStream& stream;
	uchar peekChar;
	MutableCString whitespaces;
	MutableCString identChars;

	bool isWhitespace (uchar c) const;
	bool isIdentifierChar (uchar c) const;
	bool isIdentifierChar (char c) const;
	static bool isAlpha (char c);

	template<class Float> void tryExponent (Float& value);
	template<class Float> bool parseFloat (Float& value, bool withExponent);
	template<class Int> bool parseInt (Int& value);
};

} // namespace CCL

#endif // _ccl_textparser_h
