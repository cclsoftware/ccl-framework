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
// Filename    : core/public/coreurlencoding.h
// Description : URL Encoding
//
//************************************************************************************************

#ifndef _coreurlencoding_h
#define _coreurlencoding_h

#include "core/public/corestringtraits.h"

namespace Core {

//************************************************************************************************
// URLEncoding
//************************************************************************************************

namespace URLEncoding
{
	/** URL encoding scheme. */
	enum Scheme
	{
		kRFC3986,	///< compatible to RFC3986
		kWebForm	///< compatible to WWW Forms
	};

	inline char toHexChar (char v)
	{
		if(v >= 0 && v <= 9)
			return '0' + v;
		if(v >= 0xA && v <= 0xF)
			return 'A' + v - 0xA;
		return 0; // not allowed!
	}

	inline char fromHexChar (char c)
	{
		if(c >= '0' && c <= '9')
			return c - '0';
		if(c >= 'A' && c <= 'F')
			return 0xA + c - 'A';
		return 0; // not allowed!
	}

	inline bool isUnreservedChar_RFC3986 (char c)
	{
		switch(c)
		{
		case '_' : case '~' : case '.' : case '-' :
			return true;
		default :
			return CStringClassifier::isAlphaNumeric (c);
		}
	}
	
	/** URL-encode string, works with C-string clases. */
	template <typename TString>	
	void encode (TString& result, CStringPtr string, Scheme scheme)
	{
		for(int i = 0; string[i]; i++)
		{
			char c = string[i];
			bool reserved = false;

			if(scheme == kRFC3986)
			{
				if(isUnreservedChar_RFC3986 (c))
					result.append (c);
				else
					reserved = true;
			}
			else // scheme = kWebForm
			{
				if(CStringClassifier::isAlphaNumeric (c))
					result.append (c);
				else if(c == ' ')
					result.append ('+');
				else
					reserved = true;
			}

			if(reserved)
			{
				result.append ('%');
				int value = c; // char can be > 0x7F (negative)!
				result.append (toHexChar ((value >> 4) & 0xF));
				result.append (toHexChar (value & 0xF));
			}
		}
	}

	/** URL-decode string, works with C-string clases. */
	template <typename TString>	
	void decode (TString& result, CStringPtr string)
	{
		for(int i = 0; string[i]; i++)
		{
			char c = string[i];
			if(c == '%')
			{
				char c1 = string[++i];
				if(c1 == 0)
					break;

				char c2 = string[++i];
				if(c2 == 0)
					break;

				char value = (char)(fromHexChar (c1) << 4) | fromHexChar (c2);
				result.append (value);
			}
			else if(c == '+')
				result.append (' ');
			else
				result.append (c);
		}
	}
}

} // namespace Core

#endif // _coreurlencoding_h
