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
// Filename    : ccl/text/strings/formatparser.cpp
// Description : String Format Parser
//
//************************************************************************************************

#include "ccl/text/strings/formatparser.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"

using namespace CCL;
using namespace Text;

//************************************************************************************************
// FormatParser
//************************************************************************************************

bool FormatParser::parse (const IString& format)
{
	IString::CharData chars;
	format.getChars (chars);
	const uchar* text = chars.text;
	if(!text)
		return false;

	bool result = parse (text, format.getLength ());
	format.releaseChars (chars);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormatParser::parse (const uchar* format, int length)
{
	const uchar* f = format;
	for(int i = 0; i < length; i++)
	{
		if((f[i] == '%'))
		{
			i++;
			if(f[i] == '%')
			{
				if(!onChar (f[i]))
					return false;
			}
			else
			{
				// parse type
				MutableCString type;
				CStringWriter<20> typeWriter (type); 
				while(f[i] && f[i] != '(')
					typeWriter.append (f[i]), i++;
				typeWriter.flush ();
				if(f[i] == 0) // syntax error!
					return false;
				i++;

				FormatType typeValue = kFormatAny;
				if(!type.isEmpty ())
				{
					if(type == "string" || type == "s")
						typeValue = kFormatString;
					else if(type == "int" || type == "i")
						typeValue = kFormatInt;
					else if(type == "hex" || type == "x")
						typeValue = kFormatHex;
					else if(type == "float" || type == "f")
						typeValue = kFormatFloat;
				}

				// parse index
				MutableCString index;
				CStringWriter<20> indexWriter (index);
				while(f[i] && f[i] >= '0' && f[i] <= '9')
					indexWriter.append (f[i]), i++;
				indexWriter.flush ();

				int iValue = ::atoi (index);
				iValue--; // zero-based

				// parse option
				int optValue = -1;
				if(f[i] == ':')
				{
					i++;
					MutableCString option;
					CStringWriter<20> optionWriter (option);
					while(f[i] && (f[i] >= '0' && f[i] <= '9'))
						optionWriter.append (f[i]), i++;
					optionWriter.flush ();
					if(!option.isEmpty ())
						optValue = ::atoi (option);
				}

				// eat until bracket closed
				while(f[i] && f[i] != ')')
					i++;
				if(f[i] == 0) // syntax error!
					return false;
				
				if(!onFormat (FormatDef (typeValue, iValue, optValue)))
					return false;
			}
		}
		else
		{
			if(!onChar (f[i]))
				return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Variant Parser
//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormatParser::parseVariant (Variant& result, StringRef string)
{
	result.clear ();

	if(string.isEmpty ()) // empty string
	{
		result = string;
		return true;
	}

	// check string content...
	short type = Variant::kInt;
	StringChars chars (string);
	int length = string.length ();
	int signCount = 0;
	int pointCount = 0;
	for(int i = 0; i < length; i++)
	{
		bool sign = isSignChar (chars[i]);
		if(sign)
			signCount++;

		if(chars[i] == '.')
			pointCount++;
		
		if(signCount > 2 || pointCount > 1)
		{
			type = Variant::kString;
			break;
		}
		else if(isFloatChar (chars[i]))
		{
			if(!isDecimalChar (chars[i]) && !sign)
				type = Variant::kFloat;
		}
		else
		{
			type = Variant::kString;
			break;
		}
	}

	// try to scan integer...
	if(type == Variant::kInt && string.getIntValue (result.lValue))
	{
		result.type = type;
		return true;
	}

	// try to scan float...
	if(type == Variant::kFloat && string.getFloatValue (result.fValue))
	{
		result.type = type;
		return true;
	}

	result = string;
	return true; // hmm...?
}
