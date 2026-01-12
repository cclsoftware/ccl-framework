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
// Filename    : ccl/text/strings/unicodestring.cpp
// Description : Unicode String Implementation
//
//************************************************************************************************

#include "ccl/text/strings/unicodestring.h"
#include "ccl/text/strings/formatparser.h"

#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/collections/vector.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"

#include "core/text/coretexthelper.h"

namespace CCL {

//************************************************************************************************
// CharWriter
/** Helper class for appending characters to an IString. */
//************************************************************************************************

template <int size>
class CharWriter
{
public:
	CharWriter (IString& string)
	: string (string),
	  count (0)
	{}

	void append (uchar c)
	{
		buffer[count] = c;
		if(++count >= size)
			flush ();
	}

	void flush ()
	{
		if(count > 0)
			string.appendChars (buffer, count), count = 0;
	}

protected:
	IString& string;
	uchar buffer[size];
	int count;
};

//************************************************************************************************
// TokenList
/** List of string tokens. */
//************************************************************************************************

class TokenList: public Unknown,
				 public IStringTokenizer
{
public:
	TokenList ()
	: iter (nullptr),
	  index (0)
	{}

	~TokenList ()
	{
		ListForEach (tokens, IString*, string)
			string->release ();
		EndFor
		if(iter)
			delete iter;
	}

	void append (IString* string, uchar delimiter)
	{
		tokens.append (string);
		delimiters.add (delimiter);
	}

	// IStringTokenizer
	tbool CCL_API done () const override
	{
		return getIterator ().done ();
	}

	StringRef CCL_API nextToken (uchar& delimiter) override
	{
		IString* string = getIterator ().next ();
		delimiter = string ? delimiters[index++] : 0;
		result = String (string);
		return result;
	}

	CLASS_INTERFACE (IStringTokenizer, Unknown)

protected:
	typedef LinkedList<IString*> StringList;
	typedef ListIterator<IString*> Iterator;

	StringList tokens;
	Vector<uchar> delimiters;
	mutable Iterator* iter;
	int index;
	String result;

	Iterator& getIterator () const
	{
		if(!iter)
			iter = NEW Iterator (tokens);
		return *iter;
	}
};

//************************************************************************************************
// FormatWriter
/** Helper class for formatted writing to a string. */
//************************************************************************************************

class FormatWriter: public FormatParser
{
public:
	FormatWriter (UnicodeString& result, Variant args[], int argCount);

	void flush () { resultWriter.flush (); }

	// FormatParser
	bool onChar (uchar c) override;
	bool onFormat (const Text::FormatDef& def) override;

protected:
	UnicodeString& result;
	CharWriter<UnicodeString::kTempStringSize> resultWriter;
	Variant* args;
	int argCount;
};

//************************************************************************************************
// FormatReader
/** Helper class for formatted reading from a string. */
//************************************************************************************************

class FormatReader: public FormatParser
{
public:
	FormatReader (const uchar* source, Variant args[], int argCount);

	int getReadCount () const { return numArgsRead; }

	// FormatParser
	bool onChar (uchar c) override;
	bool onFormat (const Text::FormatDef& def) override;

protected:
	const uchar* source;
	int sourceIndex;
	Variant* args;
	int argCount;
	int numArgsRead;

	uchar peekChar () const { return source[sourceIndex]; }
	uchar nextChar () { return source[++sourceIndex]; }
};

} // namespace CCL

using namespace CCL;
using namespace Text;

//************************************************************************************************
// UnicodeString
//************************************************************************************************

DEFINE_IID_ (IUnicodeStringInternal, 0xba41bd76, 0x4a6d, 0x4b84, 0xbb, 0xce, 0x65, 0x7b, 0xae, 0x6a, 0x87, 0x39)

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnicodeString::makeFormatString (TempString format, const char* prefix, int value, const char* suffix)
{
	::strcpy (format, prefix);
	TempString numString;
	::snprintf (numString, kTempStringSize, "%d", value);
	::strcat (format, numString);
	::strcat (format, suffix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::queryInterface (UIDRef iid, void** ptr)
{
	if(ccl_iid<IUnicodeStringInternal> ().equals (iid))
	{
		*ptr = static_cast<IUnicodeStringInternal*> (this); // do not retain!
		return kResultOk;
	}
	QUERY_INTERFACE (IString)
	QUERY_INTERFACE (IFormattedString)
	return Unknown::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStringTokenizer* CCL_API UnicodeString::tokenize (const IString* delimiters, int flags) const
{
	if(!delimiters)
		return nullptr;

	CharData chars;
	getChars (chars);
	if(!chars.text)
		return nullptr;

	CharData delimChars;
	int delimLength = delimiters->getLength ();
	delimiters->getChars (delimChars);
	if(!delimChars.text)
		return nullptr;

	TokenList* list = NEW TokenList;
	AutoPtr<UnicodeString> result = newString ();
	CharWriter<kTempStringSize> resultWriter (*result);
	bool preserveEmptyToken = (flags & kPreserveEmptyToken) != 0;

	int length = getLength ();
	for(int iChar = 0; iChar < length; iChar++)
	{
		uchar c = chars.text[iChar];

		bool isDelimiter = false;
		for(int iDelim = 0; iDelim < delimLength; iDelim++)
			if(c == delimChars.text[iDelim])
			{
				isDelimiter = true;
				break;
			}

		if(isDelimiter)
		{
			resultWriter.flush ();
			if(!result->isEmpty () || preserveEmptyToken)
				list->append (result->cloneString (), c);
			result->releaseInternal ();
		}
		else
			resultWriter.append (c);
	}

	// add last element
	resultWriter.flush ();
	if(!result->isEmpty ())
		list->append (result->cloneString (), 0);

	delimiters->releaseChars (delimChars);
	releaseChars (chars);
	return list;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API UnicodeString::getHashCode () const
{
	if(isEmpty ())
		return 0;

	CharData chars;
	getChars (chars);
	if(!chars.text)
		return 0;

	// Note: Hash implementation must not change, because other code might rely on it
	// (e.g. storing hashed string as UID)!!!

	// hash(i) = hash(i - 1) * 65599 + str[i];
	int length = getLength ();
	unsigned int hash = 0;
	for(int i = 0; i < length; i++)
		hash = chars.text[i] + (hash << 6) + (hash << 16) - hash;

	releaseChars (chars);
	return hash;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnicodeString::substitute (int flags)
{
	struct SubstituteEntry
	{
		uchar original;
		uchar replacementOne;
		uchar replacementTwo;
	};

	static const SubstituteEntry table[] = 
	{
		// quotes
		{0x00AB, 0x0022, 0x0000},
		{0x00B4, 0x0027, 0x0000},
		{0x00BB, 0x0022, 0x0000},
		{0x02B9, 0x0027, 0x0000},
		{0x02BA, 0x0022, 0x0000},
		{0x02BC, 0x0027, 0x0000},
		{0x02C8, 0x0027, 0x0000},
		{0x02CB, 0x0060, 0x0000},
		{0x0300, 0x0060, 0x0000},
		{0x0301, 0x0027, 0x0000},
		{0x030B, 0x0022, 0x0000},
		{0x030E, 0x0022, 0x0000},
		{0x2018, 0x0027, 0x0000},
		{0x2019, 0x0027, 0x0000},
		{0x201A, 0x0027, 0x0000},
		{0x201B, 0x0027, 0x0000},
		{0x201C, 0x0022, 0x0000},
		{0x201D, 0x0022, 0x0000},
		{0x201E, 0x0022, 0x0000},
		{0x201F, 0x0022, 0x0000},
		{0x2032, 0x0027, 0x0000},
		{0x2033, 0x0022, 0x0000},
		{0x2034, 0x0027, 0x0000},
		{0x2035, 0x0060, 0x0000},
		{0x2036, 0x0022, 0x0000},
		{0x2037, 0x0027, 0x0000},
		{0x3003, 0x0022, 0x0000},
		{0x301D, 0x0022, 0x0000},
		{0x301E, 0x0022, 0x0000},
		{0x301F, 0x0022, 0x0000},
		{0xFF02, 0x0022, 0x0000},
		{0xFF07, 0x0027, 0x0000},
		// umlauts
		{0x00c4, 0x0041, 0x0065}, // Capital A, umlaut mark -> Ae
		{0x00d6, 0x004F, 0x0065},
		{0x00dc, 0x0055, 0x0065},		
		{0x00e4, 0x0061, 0x0065},
		{0x00f6, 0x006f, 0x0065},
		{0x00fc, 0x0075, 0x0065},		
		// eszett
		{0x00df, 0x0073, 0x0073}, // Small sharp s -> ss
		// diacritics
		{0x00c0, 0x0041, 0x0000}, // Capital A, grave accent -> A
		{0x00c1, 0x0041, 0x0000},
		{0x00c2, 0x0041, 0x0000},
		{0x00c3, 0x0041, 0x0000},
		{0x00c5, 0x0041, 0x0000},
		{0x00c6, 0x0041, 0x0000},
		{0x00c7, 0x0043, 0x0000},
		{0x00c8, 0x0045, 0x0000},
		{0x00c9, 0x0045, 0x0000},
		{0x00ca, 0x0045, 0x0000},
		{0x00cb, 0x0045, 0x0000},
		{0x00cc, 0x0049, 0x0000},
		{0x00cd, 0x0049, 0x0000},
		{0x00ce, 0x0049, 0x0000},
		{0x00cf, 0x0049, 0x0000},
		{0x00d0, 0x0044, 0x0000},
		{0x00d1, 0x004e, 0x0000},
		{0x00d2, 0x004f, 0x0000},
		{0x00d3, 0x004f, 0x0000},
		{0x00d4, 0x004f, 0x0000},
		{0x00d5, 0x004f, 0x0000},		
		{0x00d7, 0x0058, 0x0000},		
		{0x00d8, 0x004f, 0x0000},		
		{0x00d9, 0x0055, 0x0000},
		{0x00da, 0x0055, 0x0000},
		{0x00db, 0x0055, 0x0000},		
		{0x00dd, 0x0059, 0x0000},
		{0x00e0, 0x0061, 0x0000},
		{0x00e1, 0x0061, 0x0000},
		{0x00e2, 0x0061, 0x0000},
		{0x00e3, 0x0061, 0x0000},
		{0x00e5, 0x0061, 0x0000},
		{0x00e6, 0x0061, 0x0000},
		{0x00e7, 0x0063, 0x0000},		
		{0x00e8, 0x0065, 0x0000},
		{0x00e9, 0x0065, 0x0000},
		{0x00ea, 0x0065, 0x0000},
		{0x00eb, 0x0065, 0x0000},		
		{0x00ec, 0x0069, 0x0000},
		{0x00ed, 0x0069, 0x0000},
		{0x00ee, 0x0069, 0x0000},
		{0x00ef, 0x0069, 0x0000},		
		{0x00f1, 0x006e, 0x0000},		
		{0x00f2, 0x006f, 0x0000},
		{0x00f3, 0x006f, 0x0000},
		{0x00f4, 0x006f, 0x0000},
		{0x00f5, 0x006f, 0x0000},
		{0x00f8, 0x006f, 0x0000},		
		{0x00f9, 0x0075, 0x0000},
		{0x00fa, 0x0075, 0x0000},
		{0x00fb, 0x0075, 0x0000}
	};

	auto findEntry = [&] (uchar c) -> const SubstituteEntry*
	{
		if(c >= 0x0080) // non ASCII character?
			for(int i = 0; i < ARRAY_COUNT (table); i++)
				if(c == table[i].original)
					return &table[i];
		return nullptr;
	};

	if(isEmpty ())
		return;
	
	CharData chars;
	getChars (chars);
	if(!chars.text)
		return;

	const uchar* original = chars.text;
	int originalLength = getLength ();
	int maxSubstituteLength = 2 * originalLength + 1; // replace one char by up to two chars
	
	uchar* substituteBuffer = nullptr;
	static const int kMaxStackLength = STRING_STACK_SPACE_MAX;
	uchar stackBuffer[kMaxStackLength];
	VectorDeleter<uchar> substituteDeleter (nullptr);
	if(maxSubstituteLength <= kMaxStackLength)
		substituteBuffer = stackBuffer;
	else
		substituteBuffer = substituteDeleter._ptr = NEW uchar[maxSubstituteLength];
	
	int substituteLength = 0;	
	bool stringChanged = false;
	for(int originalIndex = 0; originalIndex < originalLength; originalIndex++)
	{
		uchar theChar = original[originalIndex];
		if(const SubstituteEntry* entry = findEntry (theChar))
		{
			stringChanged = true;
			substituteBuffer[substituteLength++] = entry->replacementOne;
			if(entry->replacementTwo != 0x0000)
				substituteBuffer[substituteLength++] = entry->replacementTwo;
		}
		else
			substituteBuffer[substituteLength++] = theChar;
	}

	releaseChars (chars);
	
	if(stringChanged)
		assignChars (substituteBuffer, substituteLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IFormattedString
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::getFloatValue (float& value) const
{
	value = 0.;
	TempString temp;
	toCString (kASCII, temp, kTempStringSize);
	char* comma = ::strchr (temp, ','); // allow comma as decimal separator
	if(comma)
		*comma = '.';
	return ::sscanf (temp, "%f", &value) == 1 ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::getFloatValue (double& value) const
{
	value = 0.;
	TempString temp;
	toCString (kASCII, temp, kTempStringSize);
	char* comma = ::strchr (temp, ','); // allow comma as decimal separator
	if(comma)
		*comma = '.';
	return ::sscanf (temp, "%lf", &value) == 1 ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::getIntValue (int32& value) const
{
	value = 0;
	TempString temp;
	toCString (kASCII, temp, kTempStringSize);
	
	Core::Text::StringParser parser (temp);
	parser.skip (' ');
	return parser.parseInt (value) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::getIntValue (int64& value) const
{
	value = 0;
	TempString temp;
	toCString (kASCII, temp, kTempStringSize);
	
	Core::Text::StringParser parser (temp);
	parser.skip (' ');
	return parser.parseInt (value) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::getHexValue (int64& value) const
{
	value = 0;
	TempString temp;
	toCString (kASCII, temp, kTempStringSize);
	return ::sscanf (temp, "%" FORMAT_INT64 "x", &value) == 1 ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UnicodeString::scanFormat (const IString* format, Variant args[], int count) const
{
	if(!format)
		return 0;

	CharData chars;
	getChars (chars);
	if(!chars.text)
		return 0;

	FormatReader reader (chars.text, args, count);
	reader.parse (*format);

	releaseChars (chars);
	return reader.getReadCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::appendIntValue (int64 value, int numPaddingZeros)
{
	TempString temp = {0};

	if(numPaddingZeros >= 1)
	{
		TempString format;
		makeFormatString (format, "%0", numPaddingZeros, FORMAT_INT64 "d");
		::snprintf (temp, kTempStringSize, format, value);
	}
	else
		::snprintf (temp, kTempStringSize, "%" FORMAT_INT64 "d", value);

	return appendCString (kASCII, temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::appendHexValue (int64 value, int numPaddingZeros)
{
	TempString temp = {0};

	if(numPaddingZeros >= 1)
	{
		TempString format;
		makeFormatString (format, "%0", numPaddingZeros, FORMAT_INT64 "X");
		::snprintf (temp, kTempStringSize, format, value);
	}
	else
		::snprintf (temp, kTempStringSize, "%" FORMAT_INT64 "X", value);

	return appendCString (kASCII, temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::appendFloatValue (double value, int numDecimalDigits)
{
	TempString temp = {0};
	if(numDecimalDigits >= 0)
		::snprintf (temp, kTempStringSize, "%.*lf", numDecimalDigits, value); // or "%.*f"
	else
		::snprintf (temp, kTempStringSize, "%.50g", value); // best fit with full precision

	return appendCString (kASCII, temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UnicodeString::appendFormat (const IString* format, Variant args[], int count)
{
	if(!format)
		return kResultInvalidArgument;

	AutoPtr<UnicodeString> result = newString ();
	FormatWriter writer (*result, args, count);
	if(!writer.parse (*format))
		return kResultFalse;

	writer.flush ();
	return appendString (result);
}

//************************************************************************************************
// FormatWriter
//************************************************************************************************

FormatWriter::FormatWriter (UnicodeString& result, Variant args[], int argCount)
: result (result),
  resultWriter (result),
  args (args),
  argCount (argCount)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormatWriter::onChar (uchar c)
{
	resultWriter.append (c);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormatWriter::onFormat (const FormatDef& def)
{
	resultWriter.flush (); // flush any pending characters

	// fetch argument
	Variant* var = def.index >= 0 && def.index < argCount ? &args[def.index] : nullptr;
	if(!var)
		return false;

	FormatType type = def.type;
	if(type == kFormatAny) // use argument type
	{
		switch(var->getType ())
		{
		case Variant::kInt    : type = kFormatInt; break;
		case Variant::kFloat  : type = kFormatFloat; break;
		case Variant::kString : type = kFormatString; break;
		// TODO: convert Variant::kObject to string???
		}
	}

	tresult ok = kResultOk;
	switch(type)
	{
	case kFormatString :
		ok = result.appendString (var->string);
		break;

	case kFormatInt :
		ok = result.appendIntValue (*var, def.option);
		break;

	case kFormatHex :
		ok = result.appendHexValue (*var, def.option);
		break;

	case kFormatFloat :
		ok = result.appendFloatValue (*var, def.option);
		break;
	}
	return ok == kResultOk;
}

//************************************************************************************************
// FormatReader
//************************************************************************************************

FormatReader::FormatReader (const uchar* source, Variant args[], int argCount)
: source (source),
  sourceIndex (0),
  args (args),
  argCount (argCount),
  numArgsRead (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormatReader::onChar (uchar c)
{
	if(!peekChar () || peekChar () != c) // end of string or difference
		return false;

	nextChar ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormatReader::onFormat (const FormatDef& def)
{
	// fetch argument
	Variant* var = def.index >= 0 && def.index < argCount ? &args[def.index] : nullptr;
	if(!var)
		return false;

	var->clear (); // empty first!

	if(def.type == kFormatString)
	{
		UnicodeString* result = UnicodeString::newString ();
		CharWriter<UnicodeString::kTempStringSize> resultWriter (*result);
		int count = 0;
		for(uchar c = peekChar (); c != 0; c = nextChar ())
		{
			if(def.option >= 0 && count >= def.option) // stop if limit reached
				break;
			if(System::GetUnicodeUtilities ().isWhitespace (c)) // stop at whitespace
				break;

			resultWriter.append (c);
			count++;
		}
		resultWriter.flush ();

		var->type = Variant::kString|Variant::kShared;
		var->string = result;
		numArgsRead++;
	}
	else // any, int, hex, float
	{
		MutableCString temp;
		CStringWriter<100> tempWriter (temp);
		uchar c = peekChar ();
		if(isSignChar (c))
		{
			tempWriter.append (c);
			nextChar ();
		}

		for(c = peekChar (); c != 0; c = nextChar ())
		{
			if(!isValidChar (c, def.type))
				break;
			tempWriter.append (c);
		}

		tempWriter.flush ();
		if(temp.isEmpty ())
			return false;

		if(def.type == kFormatInt || def.type == kFormatAny)
		{
			int64 iValue = 0;
			::sscanf (temp, "%" FORMAT_INT64 "d", &iValue);
			*var = iValue;
			numArgsRead++;
		}
		else if(def.type == kFormatHex)
		{
			int64 iValue = 0;
			::sscanf (temp, "%" FORMAT_INT64 "x", &iValue);
			*var = iValue;
			numArgsRead++;
		}
		else if(def.type == kFormatFloat)
		{
			double fValue = 0.;
			::sscanf (temp, "%lf", &fValue);
			*var = fValue;
			numArgsRead++;
		}
	}
	return true;
}

//************************************************************************************************
// UnicodeUtilities
//************************************************************************************************

tbool CCL_API UnicodeUtilities::isAlpha (uchar c) const
{
	return ::iswalpha (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isAlphaNumeric (uchar c) const
{
	return ::iswalnum (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isWhitespace (uchar c) const
{
	return ::iswspace (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isDigit (uchar c) const
{
	return ::iswdigit (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isASCII (uchar c) const
{
	return isascii (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isPrintable (uchar c) const
{
	// non-ASCII characters are considered printable for now
	return !isASCII (c) || ::isprint (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isLowercase (uchar c) const
{
	return ::iswlower (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isUppercase (uchar c) const
{
	return ::iswupper (c) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnicodeUtilities::isFullWidth (uchar c) const
{
	return c >= 0x3000 && c <= 0x30ff
		|| c >= 0x4e00 && c <= 0x9fcc
		|| c >= 0xff01 && c <= 0xff9f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API UnicodeUtilities::toLowercase (uchar c) const
{
	return (uchar)::towlower (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API UnicodeUtilities::toUppercase (uchar c) const
{
	return (uchar)::towupper (c);
}
