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
// Filename    : ccl/public/text/cclstring.h
// Description : Unicode String Class
//
//************************************************************************************************

#ifndef _ccl_cclstring_h
#define _ccl_cclstring_h

#include "ccl/public/text/istring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// String macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Helper macro for iterating string tokens. */
#define ForEachStringToken(string, delimiters, result) \
	ForEachStringTokenWithFlags (string, delimiters, result, 0)

/** Helper macro for iterating string tokens. */
#define ForEachStringTokenWithFlags(string, delimiters, result, flags) \
{ CCL::AutoPtr<CCL::IStringTokenizer> __tokenizer = (string).tokenize (delimiters, flags); \
  CCL::uchar __delimiter = 0; \
  if(__tokenizer) while(!__tokenizer->done ()) \
   { CCL::String result = __tokenizer->nextToken (__delimiter);

#if CCL_PLATFORM_WINDOWS
#define CCL_UCHAR_COMPATIBLE_WITH_WCHAR_T 1 ///< uchar is compatible to wchar_t
#endif

//************************************************************************************************
// PlainString
/** The string class below is binary equivalent to this C structure. 
	\ingroup text_string */
//************************************************************************************************

struct PlainString
{
	IString* theString;				///< IString pointer
	IStringPrivateData __private;	///< private data for debugging
};

//************************************************************************************************
// String
/** Unicode string class with "copy-on-write" semantics. 
	\ingroup text_string */
//************************************************************************************************

class String: protected PlainString
{
public:
	friend class StringChars;
	friend struct Variant;
	
	String (nullptr_t unused = nullptr);                    ///< default ctor, allows StringRef s = nullptr
	String (int unused);                                    ///< deprecated: allows StringRef s = 0
	String (IString* string);								///< IString* is shared!
	String (const uchar* charBuffer);						///< characters are copied!
	String (const char* asciiString);						///< construct from ASCII string
	String (TextEncoding encoding, const char* cString);	///< construct from C string
	String (StringRef string, int count);					///< construct with string repetition
	String (const String&);									///< copy ctor
	~String ();												///< dtor must not be virtual!
	
	String& operator = (const String&);						///< string content is shared!

	static const String kEmpty;
	static StringRef getLineEnd (TextLineFormat lineFormat = Text::kSystemLineFormat);
	
	bool isEmpty () const;
	int length () const;

	uchar at (int index) const;
	uchar firstChar () const;
	uchar lastChar () const;

	uchar operator [] (int index) const;

	bool copyTo (uchar* charBuffer, int bufferSize) const;
	bool toCString (TextEncoding encoding, char* cString, int cStringSize, int* bytesWritten = nullptr) const;
	bool toPascalString (TextEncoding encoding, unsigned char* pString, int pStringSize) const;
	bool toASCII (char* asciiString, int asciiStringSize, int* bytesWritten = nullptr) const;

	bool equals (StringRef otherString) const;
	bool equalsChars (const uchar* charBuffer, int count = -1) const;

	int compare (StringRef otherString, bool caseSensitive = true) const;
	int compareWithOptions (StringRef otherString, int flags) const;
	int compareChars (const uchar* charBuffer, int count = -1) const;

	bool operator == (StringRef s) const;
	bool operator != (StringRef s) const;
	bool operator >  (StringRef s) const;
	bool operator >= (StringRef s) const;
	bool operator <  (StringRef s) const;
	bool operator <= (StringRef s) const;

	int index (StringRef otherString, bool caseSensitive = true) const;
	int lastIndex (StringRef otherString, bool caseSensitive = true) const;
	bool contains (StringRef otherString, bool caseSensitive = true) const;
	bool startsWith (StringRef otherString, bool caseSensitive = true) const;
	bool endsWith (StringRef otherString, bool caseSensitive = true) const;

	String subString (int index, int count = -1) const;
	IStringTokenizer* tokenize (StringRef delimiters, int flags = 0) const;

	unsigned int getHashCode () const;

	bool getFloatValue (float& value) const;
	bool getFloatValue (double& value) const;
	float scanFloat (float fallback = 0) const;
	double scanDouble (double fallback = 0) const;

	bool getIntValue (int32& value) const;
	bool getIntValue (int64& value) const;
	int scanInt (int fallback = 0) const;
	int64 scanLargetInt (int64 fallback = 0) const;

	bool getHexValue (int64& value) const;
	
	int scanFormat (StringRef format, Variant args[], int count) const;

	template <typename Type> Type createNativeString () const;
	template <typename Type> void releaseNativeString (Type nativeString);

	void writeEnable ();
	void empty ();

	String& assign (const uchar* charBuffer, int count = -1);
	String& operator = (const uchar* charBuffer);

	String& append (StringRef otherString);
	String& append (StringRef otherString, int count);
	String& append (const uchar* charBuffer, int count = -1);
	String& append (uchar uc);

	bool appendCString (TextEncoding encoding, const char* cString, int count = -1);
	bool appendPascalString (TextEncoding encoding, const unsigned char* pString);
	bool appendNativeString (const void* nativeString);

	String& appendASCII (const char* asciiString, int count = -1);

	String& appendFormat (StringRef format, Variant args[], int count);
	String& appendFormat (StringRef format, VariantRef arg1);
	String& appendFormat (StringRef format, VariantRef arg1, VariantRef arg2);
	String& appendFormat (StringRef format, VariantRef arg1, VariantRef arg2, VariantRef arg3);
	String& appendFormat (StringRef format, VariantRef arg1, VariantRef arg2, VariantRef arg3, VariantRef arg4);

	String& appendIntValue (int v, int numPaddingZeros = -1);
	String& appendIntValue (int64 v, int numPaddingZeros = -1);
	String& appendHexValue (int v, int numPaddingZeros = -1);
	String& appendHexValue (int64 v, int numPaddingZeros = -1);
	String& appendFloatValue (float v, int numDecimalDigits = -1);
	String& appendFloatValue (double v, int numDecimalDigits = -1);

	String& operator << (int v);
	String& operator << (int64 v);
	String& operator << (float v);
	String& operator << (double v);
	String& operator << (StringRef s);
	String& operator << (const uchar* charBuffer);
	String& operator << (const char* asciiString);

	String& insert (int index, StringRef otherString);
	String& prepend (StringRef otherString);
	String& remove (int index, int count);
	String& remove (StringRef otherString, bool caseSensitive = true);
	String& truncate (int index);	
	String& trimWhitespace ();
	String& toUppercase ();
	String& toLowercase ();
	String& capitalize ();
	String& substitute (int flags = 0);
	
	bool isNormalized (NormalizationForm form) const;
	String& normalize (NormalizationForm form);
	
	int replace (StringRef searchString, StringRef replacementString, bool caseSensitive = true);
};

//************************************************************************************************
// StringChars
/** Helper class for read-only access to a string's characters.
	\ingroup text_string */
//************************************************************************************************

class StringChars
{
public:
	StringChars (StringRef string)
	: string (string)
	{
		string.theString->getChars (data);
	}

	~StringChars ()
	{
		string.theString->releaseChars (data);
	}

	operator const uchar* () const
	{
		static uchar emptyText[1] = {0};
		return data.text ? data.text : emptyText;
	}

protected:
	StringRef string;
	IString::CharData data;

	// copy & assignement not allowed!
	StringChars (const StringChars& sc): string (sc.string) {}
	StringChars& operator = (const StringChars&) { return *this; }
};

//************************************************************************************************
// NativeString
/** Helper class encapsulating a native string representation. 
	\ingroup text_string */
//************************************************************************************************

template <typename Type>
struct NativeString
{
	Type nativeString;

	NativeString (StringRef string)
	: nativeString (string.createNativeString<Type> ())
	{}

	~NativeString ()
	{ String ().releaseNativeString<Type> (nativeString); }

	operator Type () const { return nativeString; }
};

//************************************************************************************************
// StringWriter
/** Helper class for appending characters to a string. 
	\ingroup text_string */
//************************************************************************************************

template <int size = 512>
class StringWriter
{
public:
	StringWriter (String& string, bool emptyFirst = true)
	: string (string),
	  count (0)
	{
		buffer[0] = '\0';
		if(emptyFirst)
			string.empty ();
	}

	StringWriter& append (uchar c)
	{
		buffer[count] = c;
		if(++count >= size)
			flush ();
		return *this;
	}

	StringWriter& append (const wchar_t* s, int charCount = -1)
	{
		if(charCount >= 0)
		{			
			for(int i = 0; i < charCount; i++)
			{
				#if CCL_UCHAR_COMPATIBLE_WITH_WCHAR_T
				append (s[i]);
				#else
				append (uchar(s[i]));
				#endif
			}
		}
		else if(s) // null-terminated
		{
			for(int i = 0; s[i] != '\0'; i++)
			{
				#if CCL_UCHAR_COMPATIBLE_WITH_WCHAR_T
				append (s[i]);
				#else
				append (uchar(s[i]));
				#endif
			}
		}
		return *this;
	}
	
	StringWriter& flush ()
	{
		if(count > 0)
		{
			string.append (buffer, count);
			count = 0;
		}
		return *this;
	}

	static String fromWideChars (const wchar_t* s, int charCount = -1)
	{
		String string;
		#if CCL_UCHAR_COMPATIBLE_WITH_WCHAR_T
		string.append (s, charCount);
		#else
		StringWriter<size> (string).append (s, charCount).flush ();
		#endif
		return string;
	}

protected:
	String& string;
	uchar buffer[size];
	int count;
};

//************************************************************************************************
/** Unicode character classification and conversion. 
	\ingroup text_string */
//************************************************************************************************

namespace Unicode
{
	inline IUnicodeUtilities& Utilities ()	{ return System::GetUnicodeUtilities (); }
	inline bool isAlpha (uchar c)			{ return Utilities ().isAlpha (c) != 0; }
	inline bool isAlphaNumeric (uchar c)	{ return Utilities ().isAlphaNumeric (c) != 0; }
	inline bool isWhitespace (uchar c)		{ return Utilities ().isWhitespace (c) != 0; }
	inline bool isDigit (uchar c)			{ return Utilities ().isDigit (c) != 0; }
	inline bool isASCII (uchar c)			{ return Utilities ().isASCII (c) != 0; }
	inline bool isPrintable (uchar c)		{ return Utilities ().isPrintable (c) != 0; }
	inline bool isLowercase (uchar c)		{ return Utilities ().isLowercase (c) != 0; }
	inline bool isUppercase (uchar c)		{ return Utilities ().isUppercase (c) != 0; }
	inline bool isFullWidth (uchar c)		{ return Utilities ().isFullWidth (c) != 0; }
	inline uchar toLowercase (uchar c)		{ return Utilities ().toLowercase (c); }
	inline uchar toUppercase (uchar c)		{ return Utilities ().toUppercase (c); }

	const uchar kZeroWidthSpace = 0x200B;
	const uchar kInfinity = 0x221E;
	const uchar kIdeographicSpace = 0x3000; ///< part of CJK Symbols and Punctuation Unicode block
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// String inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline uchar String::firstChar () const
{ return at (0); }

inline uchar String::lastChar () const
{ return at (length ()-1); }

inline bool String::contains (StringRef otherString, bool caseSensitive) const
{ return index (otherString, caseSensitive) != -1; }

inline bool String::startsWith (StringRef otherString, bool caseSensitive) const
{ return index (otherString, caseSensitive) == 0; }

inline bool String::endsWith (StringRef otherString, bool caseSensitive) const
{
    int index = lastIndex (otherString, caseSensitive);
    return index >= 0 && index == (length () - otherString.length ()); 
}

inline uchar String::operator [] (int index) const
{ return at (index); }

inline bool String::operator == (StringRef s) const
{ return equals (s); }

inline bool String::operator != (StringRef s) const
{ return !equals (s); }

inline bool String::operator > (StringRef s) const
{ return compare (s) == Text::kGreater; }

inline bool String::operator >= (StringRef s) const
{ int result = compare (s); return result == Text::kGreater || result == Text::kEqual; }

inline bool String::operator < (StringRef s) const
{ return compare (s) == Text::kLess; }

inline bool String::operator <= (StringRef s) const
{ int result = compare (s); return result == Text::kLess || result == Text::kEqual; }

template <class Type> 
Type String::createNativeString () const
{ return (Type)theString->createNativeString (); }

template <typename Type> 
void String::releaseNativeString (Type nativeString)
{ theString->releaseNativeString ((void*)nativeString); }

inline String& String::prepend (StringRef otherString)
{ return insert (0, otherString); }

inline String& String::operator = (const uchar* charBuffer)
{ return assign (charBuffer); }

inline String& String::operator << (int v)
{ return appendIntValue (v); }

inline String& String::operator << (int64 v)
{ return appendIntValue (v); }

inline String& String::operator << (float v)
{ return appendFloatValue (v); }

inline String& String::operator << (double v)
{ return appendFloatValue (v); }

inline String& String::operator << (StringRef s)
{ return append (s); }

inline String& String::operator << (const uchar* charBuffer)
{ return append (charBuffer); }

inline String& String::operator << (const char* asciiString)
{ return appendASCII (asciiString); }

inline float String::scanFloat (float fallback) const
{ float f; return getFloatValue (f) ? f : fallback; }
	
inline double String::scanDouble (double fallback) const
{ double d; return getFloatValue (d) ? d : fallback; }

inline int String::scanInt (int fallback) const
{ int i; return getIntValue (i) ? i : fallback; }

inline int64 String::scanLargetInt (int64 fallback) const
{ int64 i; return getIntValue (i) ? i : fallback; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_cclstring_h
