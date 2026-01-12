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
// Filename    : ccl/public/plugins/scripthelper.h
// Description : Scripting API Helper
//
//************************************************************************************************

#ifndef _ccl_scripthelper_h
#define _ccl_scripthelper_h

#include "ccl/public/plugins/iscriptengine.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/base/primitives.h"

namespace CCL {
namespace Scripting {

//************************************************************************************************
// Scripting::ScriptString
/** Base class to help with encoding conversions without heap allocations.*/
//************************************************************************************************

template<class T>
class ScriptString
{
public:
	ScriptString ()
	: length (0)
	{
		buffer[0] = 0;
	}

	bool isEmpty () const
	{
		return length == 0;
	}
		
protected:
	static constexpr int kMaxSize = 128;
	SharedPtr<IStringValue> stringValue;
	T buffer[kMaxSize];
	int length;
};

//************************************************************************************************
// Scripting::ScriptCString
//************************************************************************************************

class ScriptCString: public ScriptString<char>
{
public:
	ScriptCString (VariantRef value, TextEncoding _encoding = Text::kASCII)
	: encoding (_encoding)
	{
		if(UnknownPtr<IStringValue> string = value.asUnknown ())
			assign (*string);
	}
	
	ScriptCString& assign (IStringValue& value)
	{
		length = value.getLength ();
		if(length > 0)
		{
			TextEncoding sourceEncoding = value.getEncoding ();
			if(Text::isUTF16Encoding (sourceEncoding))
			{
				length = ccl_min (length, kMaxSize - 1);
				const uchar* data = value.getUCharData ();
				uchar mask = 0x7F; // ASCII character range
				if(encoding == Text::kISOLatin1) // ISOLatin1 and UTF16 are also numerically identical from 0x80 to 0xFF
					mask = 0XFF;
				
				for(int i = 0; i < length; i++)
					buffer[i] = char(data[i] & mask);
				buffer[length] = 0;
			}
			else if(Text::isValidCStringEncoding (sourceEncoding))
				stringValue = &value;
		}
		return *this;
	}
	
	operator CStringPtr ()
	{
		if(length > 0 && buffer[0] == 0)
		{
			// need to copy characters to add null termination
			ASSERT (stringValue)
			ASSERT (Text::isValidCStringEncoding (stringValue->getEncoding ()))
			int safeLength = ccl_min (length, kMaxSize - 1);
			const char* data = stringValue->getCharData ();
			for(int i = 0; i < safeLength; i++)
				buffer[i] = data[i];
			buffer[safeLength] = 0;
		}
		return buffer;
	}
	
	operator CString ()
	{
		return CString (operator CStringPtr ());
	}
		
	operator String () const
	{
		String result;
		result.appendCString (encoding, getData (), length);
		return result;
	}

protected:
	TextEncoding encoding;
	
	const char* getData () const
	{
		if(stringValue)
			return stringValue->getCharData ();
		else
			return buffer;
	}
};

//************************************************************************************************
// Scripting::Utf16String
//************************************************************************************************

class Utf16String: public ScriptString<uchar>
{
public:
	Utf16String (VariantRef value)
	{
		if(UnknownPtr<IStringValue> stringValue = value.asUnknown ())
			assign (*stringValue);
	}

	Utf16String& assign (IStringValue& value)
	{
		length = value.getLength ();
		if(length > 0)
		{
			TextEncoding sourceEncoding = value.getEncoding ();
			if(Text::isUTF16Encoding (sourceEncoding))
				stringValue = &value;
			else if(Text::isValidCStringEncoding (sourceEncoding))
			{
				length = ccl_min (length, kMaxSize - 1);
				const char* data = value.getCharData ();
				char mask = 0x7F;
				if(sourceEncoding == Text::kISOLatin1)
					mask = char (0xFF);
				for(int i = 0; i < length; i++)
					buffer[i] = uchar(data[i] & mask);
				buffer[length] = 0;
			}
		}
		return *this;
	}
		
	operator String () const
	{
		return String ().append (getData (), length);
	}
	
	bool operator == (StringRef s) const
	{
		return s.equalsChars (getData (), length);
	}

protected:
	const uchar* getData () const
	{
		if(stringValue)
			return stringValue->getUCharData ();
		else
			return buffer;
	}
};

} // namespace Scripting
} // namespace CCL

#endif // _ccl_scripthelper_h
