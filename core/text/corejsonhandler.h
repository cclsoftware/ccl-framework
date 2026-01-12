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
// Filename    : core/text/corejsonhandler.h
// Description : JSON / UBJSON Handler
//
//	JSON - http://json.org 
//	Universal Binary JSON (UBJSON) - http://ubjson.org
//
//************************************************************************************************

#ifndef _corejsonhandler_h
#define _corejsonhandler_h

#include "coretexthelper.h"
#include "coreattributehandler.h"

#include "core/public/coreprimitives.h"
#include "core/public/corestringbuffer.h"
#include "core/public/corestreamaccessor.h"
#include "core/public/corememstream.h"

namespace Core {
namespace Text {
namespace Json {

//************************************************************************************************
// Json::ErrorHandler
//************************************************************************************************

struct ErrorHandler
{
	virtual void onError (int64 position, CStringPtr errorMessage) = 0;
};

//************************************************************************************************
// Json::Parser
//************************************************************************************************

class Parser: public TextParser
{
public:
	Parser (IO::Stream* stream, AttributeHandler* attributeHandler, ErrorHandler* errorHandler, bool json5Enabled = false);

	bool parse ();

protected:
	AttributeHandler* attributeHandler;
	ErrorHandler* errorHandler;
	bool suppressErrors;
	bool json5Enabled;

	void skipWhitespace ();
	bool match (char c);
	bool match (CStringPtr string);
	bool readString (TextBuffer& string, bool isKey = false);
	bool readInteger (int64& value);
	bool readNumber (CStringPtr id);
	bool readObject (CStringPtr id);
	bool readArray (CStringPtr id);
	bool readValue (CStringPtr id);
	bool skipComment ();
	static bool isDigit (char c);

	void onError (CStringPtr errorMessage);
};

//************************************************************************************************
// Json::Writer
//************************************************************************************************

class Writer: public TextWriter,
			  public AttributeHandler
{
public:
	Writer (IO::Stream* stream);

	// AttributeHandler
	void startObject (CStringPtr id, int flags = 0) override;
	void endObject (CStringPtr id, int flags = 0) override;
	void startArray (CStringPtr id, int flags = 0) override;
	void endArray (CStringPtr id, int flags = 0) override;
	void setValue (CStringPtr id, int64 value, int flags = 0) override;
	void setValue (CStringPtr id, double value, int flags = 0) override;
	void setValue (CStringPtr id, bool value, int flags = 0) override;
	void setValue (CStringPtr id, CStringPtr value, int flags = 0) override;
	void setNullValue (CStringPtr id, int flags = 0) override;

protected:
	bool isFirstElement;

	void checkSequence ();
	bool writeEscapedString (CStringPtr text);
	void writeKeyValue (CStringPtr key, CStringPtr value = nullptr, bool quoteValue = false);
};

//************************************************************************************************
// Json::StringBufferWriter
//************************************************************************************************

template <class StringType>
class StringBufferWriter: public Writer
{
public:
	StringBufferWriter (StringType& stringBuffer)
	: stream (stringBuffer.getBuffer (), stringBuffer.getSize ()),
	  Writer (&stream)
	{
		stream.setBytesWritten (0);
	}

	~StringBufferWriter ()
	{
		flush ();
		char nullByte = 0;
		stream.writeBytes (&nullByte, 1);
	}

private:
	IO::MemoryStream stream;
};

//************************************************************************************************
// Json::BinaryParser
//************************************************************************************************

class BinaryParser
{
public:
	BinaryParser (IO::Stream* stream, AttributeHandler* attributeHandler, ErrorHandler* errorHandler);

	bool parse ();

protected:
	IO::BinaryStreamAccessor input;
	AttributeHandler* attributeHandler;
	ErrorHandler* errorHandler;
	bool suppressErrors;
	char nextType; 

	bool readString (TextBuffer& string);
	bool readInteger (int64& value);
	bool readFloat (double& value);
	bool readObject (CStringPtr id);
	bool readArray (CStringPtr id);
	bool readValue (CStringPtr id);

	void onError (CStringPtr errorMessage);

	INLINE void readNextType ()
	{
		do
		{
			if(!input.read (nextType))
				nextType = 0;
		} while(nextType == 'N'); // skip no-op
	}

	INLINE bool matchType (char c)
	{
		if(nextType != c)
			return false;

		readNextType ();
		return true;
	}

	template<class Type>
	bool readInt (int64& value)
	{
		Type v = 0;
		if(!input.read (v))
			return false;

		value = v;
		return true;
	}

	template<class Type>
	bool readFloat (double& value)
	{
		Type v = 0;
		if(!input.read (v))
			return false;

		value = v;
		return true;
	}
};

//************************************************************************************************
// Json::BinaryInplaceParser
//************************************************************************************************

class BinaryInplaceParser
{
public:
	BinaryInplaceParser (IO::Buffer& buffer, AttributeHandler* attributeHandler, ErrorHandler* errorHandler);

	bool parse ();

protected:
	IO::Buffer& buffer;
	AttributeHandler* attributeHandler;
	ErrorHandler* errorHandler;
	bool suppressErrors;
	uint32 position;
	char nextType;

	CStringPtr readString ();
	bool readInteger (int64& value);
	bool readFloat (double& value);
	bool readObject (CStringPtr id);
	bool readArray (CStringPtr id);
	bool readValue (CStringPtr id);

	INLINE void onError (CStringPtr errorMessage)
	{
		if(suppressErrors == false)
			errorHandler->onError (position, errorMessage);
	}

	INLINE uint32 bytesRemaining () const
	{
		return buffer.getSize () - position;
	}

	INLINE void readNextType ()
	{
		do
		{
			if(bytesRemaining () > 0)
				nextType = buffer[position++];
			else
				nextType = 0;
		} while(nextType == 'N'); // skip no-op
	}

	INLINE bool matchType (char c)
	{
		if(nextType != c)
			return false;

		readNextType ();
		return true;
	}

	// poor man's BinaryAccessor
	enum { kIsByteSwap = CORE_NATIVE_BYTEORDER != CORE_BIG_ENDIAN };

	template<class StoredType, class ResultType>
	INLINE bool readNumber (ResultType& value)
	{
		if(bytesRemaining () < sizeof(StoredType))
			return false;

		StoredType v = *(StoredType*)(buffer + position);
		position += sizeof(StoredType);

		if(kIsByteSwap)
			v = byte_swap (v);

		value = v;
		return true;
	}
};

//************************************************************************************************
// Json::BinaryWriter
//************************************************************************************************

class BinaryWriter: public AttributeHandler
{
public:
	BinaryWriter (IO::Stream* stream);

	void setDoublePrecisionEnabled (bool state) { doublePrecisionEnabled = state; }
	bool getResult () const { return result; }

	// AttributeHandler
	void startObject (CStringPtr id, int flags = 0) override;
	void endObject (CStringPtr id, int flags = 0) override;
	void startArray (CStringPtr id, int flags = 0) override;
	void endArray (CStringPtr id, int flags = 0) override;
	void setValue (CStringPtr id, int64 value, int flags = 0) override;
	void setValue (CStringPtr id, double value, int flags = 0) override;
	void setValue (CStringPtr id, bool value, int flags = 0) override;
	void setValue (CStringPtr id, CStringPtr value, int flags = 0) override;
	void setNullValue (CStringPtr id, int flags = 0) override;

protected:
	IO::BinaryStreamAccessor output;
	bool doublePrecisionEnabled;
	bool result;

	INLINE void writeChar (char c)
	{
		result = output.write (&c, 1) == 1;
	}

	INLINE void writeInt (int value)
	{
		if(value >= -128 && value <= 127)
		{
			int8 data[2] = {'i', int8(value)};
			output.write (data, 2);
		}
		else if(value >= 0 && value <= 255)
		{
			uint8 data[2] = {'U', uint8(value)};
			output.write (data, 2);
		}
		else if(value >= -32768 && value <= 32767)
		{
			writeChar ('I');
			output.write (int16(value));
		}
		else
		{
			writeChar ('l'); // small L
			output.write (int32(value));
		}
	}

	INLINE void writeInt (int64 value)
	{
		static const int kMaxInt = 0x7fffffff;
		static const int kMinInt = -0x7fffffff;

		if(value >= kMinInt && value <= kMaxInt)
		{
			writeInt (int(value));
		}
		else
		{
			writeChar ('L');
			output.write (value);
		}
	}

	INLINE void writeString (CStringPtr string)
	{
		int length = ConstString (string).length ();
		writeChar ('S');
		writeInt (length); // not optional
		if(length > 0)
			output.write (string, length);
	}

	INLINE void writeID (CStringPtr string)
	{
		int length = ConstString (string).length ();
		if(length > 0) // omit empty id (e.g. inside an array)
		{
			writeInt (length);
			output.write (string, length);
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Parser::isDigit (char c)
{ return c >= '0' && c <= '9'; }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Parser::match (char c)
{
	skipWhitespace ();

	if(peekChar != c)
		return false;

	readChar ();
	return true;
}

} // namespace Json
} // namespace Text
} // namespace Core

#endif // _corejsonhandler_h
