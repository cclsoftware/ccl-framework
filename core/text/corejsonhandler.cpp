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
// Filename    : core/text/corejsonhandler.cpp
// Description : JSON / UBJSON Handler
//
//************************************************************************************************

#include "corejsonhandler.h"
#include "coreutfcodec.h"

#if CTL_RTOS // platform doesn't support double precision, i.e. sizeof(double) equals sizeof(float)
#define CORE_DOUBLE_AS_FLOAT 1
#else
#define CORE_DOUBLE_AS_FLOAT 0
#endif

#define SKIP_COMMENTS 1	// text parser can skip C-style comments (otherwise comments are not allowed and result in errors)
#define PRESERVE_FLOATS 1 // write whole-number floats as "xxx.0" in JSON, to distinguish them from integers

#if PRESERVE_FLOATS
#include <math.h>
#endif

using namespace Core;
using namespace Text;
using namespace Json;

//************************************************************************************************
// Json::Parser
//************************************************************************************************

Parser::Parser (IO::Stream* stream, AttributeHandler* attributeHandler, ErrorHandler* errorHandler, bool json5Enabled)
: TextParser (stream),
  attributeHandler (attributeHandler),
  errorHandler (errorHandler),
  suppressErrors (false),
  json5Enabled (json5Enabled)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::parse ()
{
	readChar (); // init peekChar

	if(readObject (""))
		return true;

	if(readArray (""))
		return true;

	onError ("Object or array expected. Btw, UTF-8 BOM is unsupported.");
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parser::onError (CStringPtr errorMessage)
{
	if(suppressErrors == false)
		errorHandler->onError (bytePosition, errorMessage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::match (CStringPtr string)
{
	if(!string)
		return false;

	while(*string)
	{
		if(peekChar != *string)
			return false;

		readChar ();
		string++;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::readString (TextBuffer& string, bool isKey)
{
	bool singleQuoted = false;
	bool doubleQuoted = false;
	
	if(json5Enabled)
	{
		singleQuoted = match ('\'');
		
		if(!singleQuoted)
			doubleQuoted = match ('"');
		
		if(!(singleQuoted || doubleQuoted) && !isKey)
		{
			onError ("'\"' or '\'' expected when reading string.");
			return false;
		}
	}
	else if(!match ('"'))
	{
		onError ("'\"' expected when reading string.");
		return false;
	}
	else
		doubleQuoted = true;

	while(char c = readChar ())
	{
		if(c == '\'' && singleQuoted)
			return true;
		
		if(c == '"' && doubleQuoted)
			return true;
		
		if(c == ':' && json5Enabled && isKey)
		{
			c = readPreviousChar ();
			return true;
		}
		
		if(c ==  '\\')
		{
			// escape sequence
			if((c = readChar ()))
			{
				switch(c)
				{
					case '\\':	string.append ('\\'); break;
					case '/':	string.append ('/'); break;
					case '"':	string.append ('"'); break;
					case '\'':	string.append ('\''); break;
					case 'b':	string.append (0x08); break;
					case 'f':	string.append (0xC); break;
					case 'n':	string.append ('\n'); break;
					case 'r':	string.append ('\r'); break;
					case 't':	string.append ('\t'); break;
					case 'u':
					{
						// 4 hex digits (specifying a unicode character) must follow
						// (note: code points higher than 0xFFFF have to be repesented by two \uxxxx sequences)
						uchar32 codePoint = 0;
						for(int i = 0; i < 4; i++)
						{
							char digit = readChar ();
							int hexValue = StringParser::getHexValue (digit);
							if(hexValue >= 0)
								codePoint = (codePoint << 4) + hexValue;
							else
							{
								onError ("4 hex digits expected after \\u.");
								return false;
							}
						}

						// encode character as UTF-8 byte sequence
						unsigned char charBuffer[6] = {0};
						int numBytes = codePoint = 0 ? 0 : Text::UTFCodec::encodeUTF8 (codePoint, charBuffer, 6);
						if(numBytes > 0)
						{
							for(int i = 0; i < numBytes; i++)
								string.append (charBuffer[i]);
						}
						else
						{
							onError ("Illegal character code after \\u.");
							return false;
						}
					}
					break;
				}
			}
		}
		else
			string.append (c);
	}

	onError ("Unexpected end of string.");
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::readInteger (int64& value)
{
	bool success = false;
	value = 0;
	while(isDigit (peekChar))
	{
		value = value * 10 + (peekChar - '0');

		success = true;
		readChar ();
	}
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::readNumber (CStringPtr id)
{
	skipWhitespace ();

	bool isFloat = false;
	int sign = 1;
	if(match ('-'))
		sign = -1;
	else if(json5Enabled && match ('+'))
		sign = 1;

	int64 intValue = 0;
	if(!readInteger (intValue) && !(json5Enabled && (peekChar == '.')))
	{
		onError ("Digit 0..9 expected when reading number.");
		return false;
	}

	if(json5Enabled && match ('x'))
	{
		TextBuffer hexString;
		if(intValue == 0)
		{
			hexString.append ('0');
			hexString.append ('x');
			while(StringParser::getHexValue (peekChar) >= 0)
			{
				char digit = readChar ();
				hexString.append (digit);
			}
			hexString.appendNull ();
			
			int64 hexValue;
			CStringBuffer<10> (hexString.getBuffer ()).getHexValue (hexValue);
			attributeHandler->setValue (id, hexValue);
			return true;
		}
		else
			return false;
	}
	
	double value = (double)intValue;
	if(match ('.'))
	{
		isFloat = true;
		
		double base = 0.1;
		while(isDigit (peekChar))
		{
			value += base * (peekChar - '0');
			base *= 0.1;
			readChar ();
		}
	}
	intValue *= sign;
	value *= sign;

	if(match ('e') || match ('E'))
	{
		isFloat = true;

		int sign = 1;
		if(match ('-'))
			sign = -1;
		else
			match ('+');

		int64 exponent = 0;
		if(readInteger (exponent))
		{
			double factor = 1;
			for(int i = 0; i < exponent; i++)
				factor *= 10;

			if(sign == -1)
				factor = 1 / factor;

			value *= factor;
		}
	}

	if(isFloat)
		attributeHandler->setValue (id, value);
	else
		attributeHandler->setValue (id, intValue);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::readObject (CStringPtr id)
{
	if(!match ('{'))
		return false;

	attributeHandler->startObject (id);

	bool expectElement = false; // may be empty
	
	TextBuffer key;
	suppressErrors = true; // do not report error when object is empty
	while(readString (key, json5Enabled))
	{
		suppressErrors = false; 

		if(!match (':'))
		{
			onError ("\":\" expected for key.");
			return false;
		}

		key.appendNull ();
		if(!readValue (key.getBuffer ()))
			return false;

		expectElement = match (',');
		if(!expectElement)
			break;
		else if(json5Enabled && match ('}'))
		{
			attributeHandler->endObject (id);
			return true;
		}

		key.empty ();
	}
	suppressErrors = false; 
	if(expectElement)
	{
		onError ("Expected \" after \",\".");
		return false;
	}

	attributeHandler->endObject (id);

	return match ('}');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::readArray (CStringPtr id)
{
	if(!match ('['))
		return false;

	attributeHandler->startArray (id);

	bool expectElement = false; // may be empty
	
	suppressErrors = true; // do not report error if array is empty
	while(readValue (""))
	{
		expectElement = match (',');
		if(!expectElement)
			break;
		else if(json5Enabled && match (']'))
		{
			attributeHandler->endObject (id);
			return true;
		}
	}
	suppressErrors = false;

	if(expectElement)
	{
		onError ("Expected value after \",\".");
		return false;
	}

	attributeHandler->endArray (id);

	return match (']');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::readValue (CStringPtr id)
{
	skipWhitespace ();
	
	if(json5Enabled && peekChar == '\'')
	{
		TextBuffer string;
		if(readString (string))
		{
			string.appendNull ();
			attributeHandler->setValue (id, string.getBuffer ());
			return true;
		}
	}

	switch(peekChar)
	{
	case '"': // string
		{
			TextBuffer string;
			if(readString (string))
			{
				string.appendNull ();
				attributeHandler->setValue (id, string.getBuffer ());
				return true;
			}
		}
		break;

	case '{': // object
		return readObject (id);

	case '[': // array
		return readArray (id);

	case 't':
		if(match ("true"))
		{
			attributeHandler->setValue (id, true);
			return true;
		}
		break;

	case 'f':
		if(match ("false"))
		{
			attributeHandler->setValue (id, false);
			return true;
		}
		break;

	case 'n':
		if(match ("null"))
		{
			attributeHandler->setNullValue (id);
			return true;
		}
		break;

	default:
		return readNumber (id);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parser::skipComment ()
{
	// comments start with // or /*
	if(peekChar == '/')
	{
		readChar ();
		if(peekChar == '/')
		{
			// line comment: skip until end of line
			while(peekChar != '\r' && peekChar != '\n' && peekChar != 0)
				readChar ();
		}
		else if(peekChar == '*')
		{
			// block comment: skip until closing */
			char previousChar = 0;
			readChar ();
			while(peekChar != 0)
			{
				if(peekChar == '/' && previousChar == '*')
				{
					readChar ();
					break;
				}

				previousChar = peekChar;
				readChar ();
			}
		}
		else
			onError ("Invalid start of comment: // or /* expected.");

		return true; // we have consumed something (even if it was an invalid single /)
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parser::skipWhitespace ()
{
	while(true)
	{
		if(peekChar == ' ' || peekChar == '\t' || peekChar == '\r' || peekChar == '\n')
			readChar ();
		else
		{
			#if SKIP_COMMENTS
			if(skipComment ())
				continue;
			#endif
			return;
		}
	} 
}

//************************************************************************************************
// Json::Writer
//************************************************************************************************

Writer::Writer (IO::Stream* stream)
: TextWriter (stream),
  isFirstElement (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::checkSequence ()
{
	if(isFirstElement)
		isFirstElement = false;
	else
		writeString (",", true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Writer::writeEscapedString (CStringPtr text)
{
	if(text)
		while(char c = *text++)
		{
			switch(c)
			{
			case '\\':	writeChar ('\\'); c = '\\'; break;
			//case '/':	writeChar ('\\'); c = '/'; break; escaping forward slashes isn't required by JSON 
			case '"':	writeChar ('\\'); c = '"'; break;
			//case '\'':	writeChar ('\\'); c = '\''; break; not valid json (see http://www.json.org/)
			case '\n':	writeChar ('\\'); c = 'n'; break;
			case '\r':	writeChar ('\\'); c = 'r'; break;
			case '\t':	writeChar ('\\'); c = 't'; break;
			case 0x08:	writeChar ('\\'); c = 'b'; break;
			case 0xC:	writeChar ('\\'); c = 'f'; break;
			}
			// todo: it might be necessary to escape any non-ascii char as \uxxxx, but we would have to decode the already utf8-encoded string here...

			if(!writeChar (c))
				return false;
		}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::writeKeyValue (CStringPtr key, CStringPtr value, bool quoteValue)
{
	if(key && *key)
	{
		writeIndent ();
		writeString ("\"");
		writeString (key);
		writeString ("\": ");
	}
	else if(value && *value)
		writeIndent ();

	if(quoteValue)
	{
		writeString ("\"");
		writeEscapedString (value);
		writeString ("\"");
	}
	else
		writeString (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::startObject (CStringPtr id, int /*flags*/)
{
	checkSequence ();

	writeKeyValue (id);
	if(id && *id)
		writeNewline ();
	writeLine ("{");

	incIndent ();
	isFirstElement = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::endObject (CStringPtr /*id*/, int /*flags*/)
{
	decIndent ();

	writeNewline ();
	writeIndent ();
	writeString ("}");

	isFirstElement = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::startArray (CStringPtr id, int /*flags*/)
{
	checkSequence ();

	writeKeyValue (id);
	if(id && *id)
		writeNewline ();
	writeLine ("[");

	incIndent ();
	isFirstElement = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::endArray (CStringPtr /*id*/, int /*flags*/)
{
	decIndent ();

	writeNewline ();
	writeIndent ();
	writeString ("]");

	isFirstElement = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::setValue (CStringPtr id, int64 value, int /*flags*/)
{
	checkSequence ();

	CString64 string;
	string.appendFormat ("%" FORMAT_INT64 "d", value);
	writeKeyValue (id, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::setValue (CStringPtr id, double value, int /*flags*/)
{
	checkSequence ();

	CString64 string;
	#if PRESERVE_FLOATS
	if(::floor (value) == value) // check if it's a whole number
		string.appendFormat ("%.1f", value); // at least one digit after decimal dot, to mark clearly as "float"
	else
	#endif
		string.appendFormat ("%.50g", value); // best fit with full precision
	
	//ASSERT (!string.contains ("#INF")) // parser will fail on infinity values!
	// TODO: infinity should be encoded as null value!

	writeKeyValue (id, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::setValue (CStringPtr id, bool value, int /*flags*/)
{
	checkSequence ();

	writeKeyValue (id, value ? "true" : "false");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::setValue (CStringPtr id, CStringPtr value, int /*flags*/)
{
	checkSequence ();

	writeKeyValue (id, value, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Writer::setNullValue (CStringPtr id, int /*flags*/)
{
	checkSequence ();

	writeKeyValue (id, "null");
}

//************************************************************************************************
// Json::BinaryParser
//************************************************************************************************

BinaryParser::BinaryParser (IO::Stream* stream, AttributeHandler* attributeHandler, ErrorHandler* errorHandler)
: input (*stream, CORE_BIG_ENDIAN),
  attributeHandler (attributeHandler),
  errorHandler (errorHandler),
  suppressErrors (false),
  nextType (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryParser::parse ()
{
	readNextType ();

	if(readObject (""))
		return true;

	if(readArray (""))
		return true;

	onError ("Object or array expected.");
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryParser::onError (CStringPtr errorMessage)
{
	if(suppressErrors == false)
		errorHandler->onError (input.getStream ().getPosition (), errorMessage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryParser::readString (TextBuffer& string)
{
	// consume the 'S', if present (omitted for "keys" in an object)
	// next type is the (integer) type of length
	if(nextType == 'S')
		readNextType ();

	int64 length = 0;
	if(!readInteger (length))
		return false;

	if(length < kTextBufferSize)
	{
		// read directly into buffer
		if(input.read ((void*)string.getBuffer (), (int)length) != length)
		{
			onError ("String is shorter than expected.");
			return false; // or just take what we got?
		}
		string.setNumChars ((uint32)length);
	}
	else
	{
		// todo: read in blocks of kTextBufferSize (if we ever have such long strings)
		char c = 0;
		for(int i = 0; i < length; i++)
		{
			if(!input.read (c))
			{
				onError ("String is shorter than expected.");
				return false; // or just take what we got?
			}
			string.append (c);
		}
	}
	readNextType ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryParser::readInteger (int64& value)
{
	switch(nextType)
	{
	case 'i': return readInt<int8> (value);
	case 'U': return readInt<uint8> (value);
	case 'I': return readInt<int16> (value);
	case 'l': return readInt<int32> (value);
	case 'L': return readInt<int64> (value);
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryParser::readFloat (double& value)
{
	switch(nextType)
	{
	case 'd': return readFloat<float32> (value);
	case 'D': 
		#if CORE_DOUBLE_AS_FLOAT
		onError ("Platform doesn't support double precision!");
		return false;
		#else
		return readFloat<float64> (value);
		#endif

	case 'H': 
		onError ("High-precision number (H) not supported.");
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryParser::readObject (CStringPtr id)
{
	if(nextType != '{')
		return false;
	
	readNextType ();

	attributeHandler->startObject (id);

	TextBuffer key;
	suppressErrors = true; // do not report error when object is empty
	while(readString (key)) 
	{
		suppressErrors = false; 

		key.appendNull ();
		if(!readValue (key.getBuffer ()))
			return false;

		if(nextType == '}')
			break;

		key.empty ();
	}
	suppressErrors = false;

	attributeHandler->endObject (id);

	return matchType ('}');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryParser::readArray (CStringPtr id)
{
	if(nextType != '[')
		return false;
	
	readNextType ();

	attributeHandler->startArray (id);

	suppressErrors = true; // do not report error if array is empty
	while(readValue (""))
	{
		if(nextType == ']')
			break;
	}
	suppressErrors = false;

	attributeHandler->endArray (id);
	return matchType (']');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryParser::readValue (CStringPtr id)
{
	switch(nextType)
	{
	case 'S': // string
		{
			TextBuffer string;
			if(readString (string))
			{
				string.appendNull ();
				attributeHandler->setValue (id, string.getBuffer ());
				return true;
			}
		}
		break;

	case 'C': // character
		{
			char c[2] = { 0, 0 };
			if(input.read (c[0]))
			{
				attributeHandler->setValue (id, c);
				readNextType ();
				return true;
			}
		}
		break;

	case '{': // object
		return readObject (id);

	case '[': // array
		return readArray (id);

	case 'T':
		attributeHandler->setValue (id, true);
		readNextType ();
		return true;

	case 'F':
		attributeHandler->setValue (id, false);
		readNextType ();
		return true;

	case 'Z':
		attributeHandler->setNullValue (id);
		readNextType ();
		return true;

	case 'i':
	case 'U':
	case 'I':
	case 'l':
	case 'L':
		{
			int64 intValue = 0;
			if(readInteger (intValue))
			{
				attributeHandler->setValue (id, intValue);
				readNextType ();
				return true;
			}
		}
		break;

	case 'd':
	case 'D':
	case 'H': 
		{
			double value = 0;
			if(readFloat (value))
			{
				attributeHandler->setValue (id, value);
				readNextType ();
				return true;
			}
		}
		break;

	case 'N': // no-op
		ASSERT (false)

	default:
		onError ("Invalid Type.");
	}
	return false;
}

//************************************************************************************************
// Json::BinaryInplaceParser
//************************************************************************************************

BinaryInplaceParser::BinaryInplaceParser (IO::Buffer& buffer, AttributeHandler* attributeHandler, ErrorHandler* errorHandler)
: buffer (buffer),
  attributeHandler (attributeHandler),
  errorHandler (errorHandler),
  suppressErrors (false),
  position (0),
  nextType (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryInplaceParser::parse ()
{
	readNextType ();

	if(readObject (""))
		return true;

	if(readArray (""))
		return true;

	onError ("Object or array expected.");
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr BinaryInplaceParser::readString ()
{
	// consume the 'S', if present (omitted for "keys" in an object)
	// next type is the (integer) type of length
	if(nextType == 'S')
		readNextType ();

	int64 length = 0;
	if(!readInteger (length))
		return nullptr;

	if(bytesRemaining () < length)
		return nullptr;

	char* string = buffer + position;
	position += (uint32)length;

	readNextType ();

	string[length] = 0; // terminate string (overwrite the next type byte that we just read)
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryInplaceParser::readInteger (int64& value)
{
	switch(nextType)
	{
	case 'i': return readNumber<int8> (value);
	case 'U': return readNumber<uint8> (value);
	case 'I': return readNumber<int16> (value);
	case 'l': return readNumber<int32> (value);
	case 'L': return readNumber<int64> (value);
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryInplaceParser::readFloat (double& value)
{
	switch(nextType)
	{
	case 'd': return readNumber<float32> (value);
	case 'D': 
		#if CORE_DOUBLE_AS_FLOAT
		onError ("Platform doesn't support double precision!");
		return false;
		#else
		return readNumber<float64> (value);
		#endif

	case 'H': 
		onError ("High-precision number (H) not supported.");
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
bool BinaryInplaceParser::readObject (CStringPtr id)
{
	if(nextType != '{')
		return false;
	
	readNextType ();

	attributeHandler->startObject (id, AttributeHandler::kInplace);

	CStringPtr key;
	suppressErrors = true; // do not report error when object is empty
	while(key = readString ())
	{
		suppressErrors = false;

		if(!readValue (key))
			return false;

		if(nextType == '}')
			break;
	}
	suppressErrors = false;

	attributeHandler->endObject (id, AttributeHandler::kInplace);

	return matchType ('}');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryInplaceParser::readArray (CStringPtr id)
{
	if(nextType != '[')
		return false;
	
	readNextType ();

	attributeHandler->startArray (id, AttributeHandler::kInplace);

	suppressErrors = true; // do not report error if array is empty
	while(readValue (""))
	{
		if(nextType == ']')
			break;
	}
	suppressErrors = false;

	attributeHandler->endArray (id, AttributeHandler::kInplace);
	return matchType (']');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryInplaceParser::readValue (CStringPtr id)
{
	switch(nextType)
	{
	case 'S': // string
		{
			if(CStringPtr string = readString ())
			{
				attributeHandler->setValue (id, string, AttributeHandler::kInplace|AttributeHandler::kInplaceValue);
				return true;
			}
		}
		break;

	case 'C': // character
		{
			if(bytesRemaining () > 0)
			{
				char* string = buffer + position++;
				readNextType ();
				string[1] = 0; // terminate string

				attributeHandler->setValue (id, string, AttributeHandler::kInplace|AttributeHandler::kInplaceValue);
				return true;
			}
		}
		break;

	case '{': // object
		return readObject (id);

	case '[': // array
		return readArray (id);

	case 'T':
		attributeHandler->setValue (id, true, AttributeHandler::kInplace);
		readNextType ();
		return true;

	case 'F':
		attributeHandler->setValue (id, false, AttributeHandler::kInplace);
		readNextType ();
		return true;

	case 'Z':
		attributeHandler->setNullValue (id, AttributeHandler::kInplace);
		readNextType ();
		return true;

	case 'i':
	case 'U':
	case 'I':
	case 'l':
	case 'L':
		{
			int64 intValue = 0;
			if(readInteger (intValue))
			{
				attributeHandler->setValue (id, intValue, AttributeHandler::kInplace);
				readNextType ();
				return true;
			}
		}
		break;

	case 'd':
	case 'D':
	case 'H': 
		{
			double value = 0;
			if(readFloat (value))
			{
				attributeHandler->setValue (id, value, AttributeHandler::kInplace);
				readNextType ();
				return true;
			}
		}
		break;

	case 'N': // no-op
		ASSERT (false)

	default:
		onError ("Invalid Type.");
	}
	return false;
}

//************************************************************************************************
// Json::BinaryWriter
//************************************************************************************************

BinaryWriter::BinaryWriter (IO::Stream* stream)
: output (*stream, CORE_BIG_ENDIAN),
  doublePrecisionEnabled (false), // not supported by all embedded platforms, must be off by default!
  result (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::startObject (CStringPtr id, int /*flags*/)
{
	writeID (id);
	writeChar ('{');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::endObject (CStringPtr /*id*/, int /*flags*/)
{
	writeChar ('}');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::startArray (CStringPtr id, int /*flags*/)
{
	writeID (id);
	writeChar ('[');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::endArray (CStringPtr /*id*/, int /*flags*/)
{
	writeChar (']');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::setValue (CStringPtr id, int64 value, int /*flags*/)
{
	writeID (id);
	writeInt (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CTL_RTOS
CORE_HOT_FUNCTION void BinaryWriter::setValue (CStringPtr id, double value, int /*flags*/)
{
	// ATTENTION: Optimized version, identifier length is limited here!
	char buffer[64];
	char* outPtr = buffer;
	
	int idLength = strlen (id);
	if(idLength > 0)
	{
		// int8
		outPtr[0] = 'i';
		outPtr += 1;
		outPtr[0] = idLength;
		outPtr += 1;
		memcpy (outPtr, id, idLength);
		outPtr += idLength;
	}
	
	#if !CORE_DOUBLE_AS_FLOAT
	if(doublePrecisionEnabled)
	{
		// float64
		outPtr[0] = 'D';
		outPtr += 1;
		if(CORE_NATIVE_BYTEORDER != CORE_BIG_ENDIAN)
			value = byte_swap (value);
		memcpy (outPtr, (char*)&value, sizeof(value));
		outPtr += sizeof(value);
	}
	else
	#endif
	{
		float32 value32 (value);
		// float32
		outPtr[0] = 'd';
		outPtr += 1;
		if(CORE_NATIVE_BYTEORDER != CORE_BIG_ENDIAN)
			value32 = byte_swap (value32);
		memcpy (outPtr, (char*)&value32, sizeof(value32));
		outPtr += sizeof(value32);
	}
	
	output.write(buffer, outPtr - buffer);
}
#else
void BinaryWriter::setValue (CStringPtr id, double value, int /*flags*/)
{
	writeID (id);

	#if !CORE_DOUBLE_AS_FLOAT
	if(doublePrecisionEnabled)
	{
		writeChar ('D');
		output.write (float64 (value));
	}
	else
	#endif
	{
		writeChar ('d');
		output.write (float32 (value));
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::setValue (CStringPtr id, bool value, int /*flags*/)
{
	writeID (id);
	writeChar (value ? 'T' : 'F');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::setValue (CStringPtr id, CStringPtr value, int /*flags*/)
{
	writeID (id);
	writeString (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BinaryWriter::setNullValue (CStringPtr id, int /*flags*/)
{
	writeID (id);
	writeChar ('Z');
}
