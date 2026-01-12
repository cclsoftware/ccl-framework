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
// Filename    : ccl/text/strings/formatparser.h
// Description : String Format Parser
//
//************************************************************************************************

#ifndef _ccl_formatparser_h
#define _ccl_formatparser_h

#include "ccl/public/text/istring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Format definition
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Text {

/** Format types. */
enum FormatType
{
	kFormatAny = -1,	///< not specified
	kFormatInt,			///< "int" or "i"
	kFormatHex,			///< "hex" or "x"
	kFormatFloat,		///< "float" or "f"
	kFormatString		///< "string" or "s"
};

/** Formatter field. */
struct FormatDef
{
	FormatType type;	///< format type
	int index;			///< argument index
	int option;			///< option value

	FormatDef (FormatType type = kFormatAny, int index = -1, int option = -1)
	: type (type),
	  index (index),
	  option (option)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isSignChar (uchar c)	{ return c == '+' || c == '-'; }
inline bool isDecimalChar (uchar c)	{ return c >= '0' && c <= '9'; }
inline bool isHexChar (uchar c)		{ return isDecimalChar (c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
inline bool isFloatChar (uchar c)	{ return isDecimalChar (c) || c == '.' || c == 'e' || c == 'E' || isSignChar (c); }

} // namespace Text

//************************************************************************************************
// FormatParser
/** String format parser. */
//************************************************************************************************

class FormatParser
{
public:
	virtual bool onChar (uchar c) = 0;
	virtual bool onFormat (const Text::FormatDef& def) = 0;

	bool parse (const IString& format);
	bool parse (const uchar* format, int length);

	static bool isValidChar (uchar c, Text::FormatType type)
	{
		switch(type)
		{
		case Text::kFormatAny :
		case Text::kFormatInt : return Text::isDecimalChar (c);
		case Text::kFormatHex : return Text::isHexChar (c);
		case Text::kFormatFloat : return Text::isFloatChar (c);
		}
		return type == Text::kFormatString;
	}

	static bool parseVariant (Variant& result, StringRef string);
};

} // namespace CCL

#endif // _ccl_formatparser_h
