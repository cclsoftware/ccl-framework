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
// Filename    : core/public/corestringtraits.h
// Description : C-String Traits
//
//************************************************************************************************

#ifndef _corestringtraits_h
#define _corestringtraits_h

#include "core/public/coretypes.h"

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////
// C-String macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Helper macro for iterating C-String tokens (copies string). */
#define ForEachCStringToken(string, delimiters, result) \
{ Core::CStringTokenizer __tokenizer (string, delimiters); \
  Core::CStringPtr result = 0; \
  while((result = __tokenizer.next ()) != 0) {

/** Helper macro for iterating C-String tokens (modifies string inplace).  */
#define ForEachCStringTokenInplace(string, delimiters, result) \
{ Core::CStringTokenizerInplace __tokenizer (string, delimiters); \
  Core::CStringPtr result = 0; \
  while((result = __tokenizer.next ()) != 0) {

//************************************************************************************************
// CStringTraits
/** C-String traits class, safe for ASCII-encoded text only!
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

template <typename T> 
class CStringTraits
{
public:
	/** Get plain C-String. */
	CStringPtr str () const;

	/** Check if string is empty. */
	bool isEmpty () const;

	/** Get string length. */
	int length () const;

	/** Copy to buffer. */
	bool copyTo (char* charBuffer, int bufferSize) const;

	/** Get position of other C-String. */
	int index (CStringPtr other) const;

	/** Check if this contains other C-String. */
	bool contains (CStringPtr other) const;

	/** Check if this starts with other C-String. */
	bool startsWith (CStringPtr other) const;

	/** Check if this ends with other C-String. */
	bool endsWith (CStringPtr other) const;

	/** Get first position of ASCII character. */
	int index (char c) const;

	/** Get first position of Unicode character. */
	int index (uchar c) const;

	/** Get last position of ASCII character. */
	int lastIndex (char c) const;

	/** Get last position of Unicode character. */
	int lastIndex (uchar c) const;

	/** Check if this contains other ASCII character. */
	bool contains (char c) const;

	/** Compare with other C-String. */
	int compare (CStringPtr other, bool caseSensitive = true) const;

	/** Get integer from string. */
	bool getIntValue (int32& value) const;
	
	/** Get integer from string. */
	bool getIntValue (int64& value) const;

	int scanInt (int fallback = 0) const;
	int64 scanLargetInt (int64 fallback = 0) const;

	/** Get hexadecimal integer from string. */
	bool getHexValue (int64& value) const;

	/** Get double from string. */
	bool getFloatValue (double& value) const;

	/** Get float from string. */
	bool getFloatValue (float& value) const;

	float scanFloat (float fallback = 0) const;
	double scanDouble (double fallback = 0) const;

	/** Hash string to integer value. */
	unsigned int getHashCode () const;

	/** Compare with other C-String. */
	bool operator == (CStringPtr other) const;

	/** Compare with other C-String. */
	bool operator != (CStringPtr other) const;

	/** Compare with other CStringTraits (with possibly different type S). */
	template <class S> bool operator == (const CStringTraits<S>& other) const;

	/** Compare with other CStringTraits (with possibly different type S). */
	template <class S> bool operator != (const CStringTraits<S>& other) const;

	/** Compare with other C-String. Does not check if other is null. */
	bool equalsUnsafe (CStringPtr other) const;
	
	/** Get the character at the specified index. */
	char at (int index) const;
	
	/** Get the first character in the string. */
	char firstChar () const;
	
	/** Get the last character in the string. */
	char lastChar () const;
	
	/** Get the character at the specified index. */
	char operator [] (int index) const;

	/** Cast to plain C-String. */
	operator CStringPtr () const;

protected:
	// helper to handle null pointers to strings
	static inline CStringPtr safe_str (CStringPtr str) { return str ? str : ""; }
};

//************************************************************************************************
// MutableCStringTraits
/** Mutable C-String traits class, safe for ASCII-encoded text only!
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

template <typename T> 
class MutableCStringTraits
{
public:
	/** Init with C-String (this must be empty before). */
	T& init (CStringPtr string);

	/** Append C-String. */
	T& append (CStringPtr string, int count = -1);

	/** Append 8-Bit character. */
	T& append (char c);

	/** Append Unicode character. */
	T& append (uchar uc);

	/** Append printf-style formatted text. */
	T& appendFormat (const char* format, ...);

	/** Append printf-style formatted text. */
	T& appendFormatArgs (const char* format, va_list marker);

	/** Append signed 32 bit integer value. */
	T& appendInteger (int32 value);

	/** Append signed 64 bit integer value. */
	T& appendInteger (int64 value);

	/** Append unsigned 32 bit integer value. */
	T& appendInteger (uint32 value);

	/** Append unsigned 64 bit integer value. */
	T& appendInteger (uint64 value);

	/** Append 8-Bit character. */
	T& operator += (char c);

	/** Append Unicode character. */
	T& operator += (uchar uc);

	/** Append C-String. */
	T& operator += (CStringPtr text);

private:
	/** Write an unsigned integer value to the buffer from back to front. */
	static int writeInteger (char buffer[], int bufferSize, uint64 value);
};

//************************************************************************************************
// CStringTokenizerInplace
/** Break C-string into tokens at given delimiters.
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

class CStringTokenizerInplace
{
public:
	CStringTokenizerInplace (char* string, CStringPtr delimiters, bool preserveEmptyTokens = false);
	~CStringTokenizerInplace ();

	/** Get next token. */
	CStringPtr next ();

protected:
	char* string;
	CStringPtr delimiters;
	bool preserveEmptyTokens;
	char savedChar; // delimiter at *string that was replaced by 0
};

//************************************************************************************************
// CStringTokenizer
/** Break C-string into tokens at given delimiters.
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

class CStringTokenizer: public CStringTokenizerInplace
{
public:
	CStringTokenizer (CStringPtr string, CStringPtr delimiters, bool preserveEmptyTokens = false);
	~CStringTokenizer ();

protected:
	static const int kBufferSize = STRING_STACK_SPACE_MAX;
	char buffer[kBufferSize];
	char* allocatedBuffer;
};

//************************************************************************************************
// CStringClassifier
/** C-String character classification, safe for ASCII only!
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

class CStringClassifier
{
public:
	/** Check if character is alphabetical. */
	static bool isAlpha (char c) { return c > 0 ? ::isalpha (c) != 0 : false; }
	
	/** Check if character is alpha-numeric. */
	static bool isAlphaNumeric (char c) { return c > 0 ? ::isalnum (c) != 0 : false; }
	
	/** Check if character is whitespace. */
	static bool isWhitespace (char c) { return c > 0 ? ::iswspace (c) != 0 : false; }
	
	/** Check if character is a digit. */
	static bool isDigit (char c) { return c > 0 ? ::isdigit (c) != 0 : false; }
	
	/** Check if character is in ASCII range. */
	static bool isASCII (char c) { return c > 0 ? isascii (c) != 0 : false; }
	
	/** Check if character is lowercase. */
	static bool isLowercase (char c) { return c > 0 ? ::islower (c) != 0 : false; }

	/** Check if character is uppercase. */
	static bool isUppercase (char c) { return c > 0 ? ::isupper (c) != 0 : false; }
	
	/** Convert to lowercase character. */
	static char toLowercase (char c) { return c > 0 ? (char)::tolower (c) : 0; }
	
	/** Convert to uppercase character. */
	static char toUppercase (char c){ return c > 0 ? (char)::toupper (c) : 0; }
};

//************************************************************************************************
// CStringFunctions
/** C-String helper functions
	\ingroup core_string
	\ingroup text_string */
//************************************************************************************************

class CStringFunctions
{
public:
	/** Hash function compatible to Apple CFString. */
	static uint32 hashCFS (CStringPtr cString)
	{
		uint32 result = 0;
		int len = (int)::strlen (cString);
		if(len < 4)
		{
			int count = len;
			while(count--) 
				result += (result << 8) + *cString++;
		}
		else
		{
			result += (result << 8) + cString[0];
			result += (result << 8) + cString[1];
			result += (result << 8) + cString[len-2];
			result += (result << 8) + cString[len-1];
		}
		result += (result << (len & 31));
		return result;
	}

	/** Use CFString hash as positive integer index. */
	static INLINE int hashCFSIndex (CStringPtr cString)
	{
		return (int)(hashCFS (cString) & 0x7FFFFFFF); // must not be negative!
	}

	/** Hash function by Daniel J. Bernstein from comp.lang.c. */
	static uint32 hashDJB (CStringPtr cString)
	{
		uint32 hash = 5381;
		int c;
		while((c = *cString++))
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		return hash;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// CStringTraits implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline CStringPtr CStringTraits<T>::str () const
{ return static_cast<const T*> (this)->__str (); } // __str() needs to be available in specialized class!

template <class T>
inline bool CStringTraits<T>::isEmpty () const
{ return str ()[0] == 0; }

template <class T>
inline int CStringTraits<T>::length () const 
{ return (int)::strlen (str ()); }

template <class T>
inline bool CStringTraits<T>::copyTo (char* charBuffer, int bufferSize) const
{
	if(!charBuffer || bufferSize <= 0)
		return false;
	int count = length ();
	if(count >= bufferSize)
		count = bufferSize-1;
	::strncpy (charBuffer, str (), count);
	charBuffer[count] = 0;
	return true;
}

template <class T>
inline int CStringTraits<T>::index (CStringPtr other) const
{ CStringPtr ptr = ::strstr (str (), safe_str (other)); return ptr ? (int)(ptr - str ()) : -1; }

template <class T>
inline bool CStringTraits<T>::contains (CStringPtr other) const
{ return index (other) != -1; }

template <class T>
inline bool CStringTraits<T>::startsWith (CStringPtr other) const
{ return index (other) == 0; }

template <class T>
inline bool CStringTraits<T>::endsWith (CStringPtr other) const
{
	int otherLength = other ? (int)::strlen (other) : 0;
	if(otherLength > 0)
	{
		int thisLength = length ();
		if(thisLength >= otherLength)
		{
			int charOffset = thisLength - otherLength;
			return ::memcmp (str () + charOffset, other, otherLength) == 0;
		}
	}
	return false;
}

template <class T>
inline int CStringTraits<T>::index (char c) const
{ CStringPtr ptr = ::strchr (str (), c); return ptr ? (int)(ptr - str ()) : -1; }

template <class T>
inline int CStringTraits<T>::lastIndex (char c) const
{ CStringPtr ptr = ::strrchr (str (), c); return ptr ? (int)(ptr - str ()) : -1; }

template <class T>
inline int CStringTraits<T>::index (uchar c) const
{ return c > 255 ? -1 : index ((char)c); }

template <class T>
inline int CStringTraits<T>::lastIndex (uchar c) const
{ return c > 255 ? -1 : lastIndex ((char)c); }

template <class T>
inline bool CStringTraits<T>::contains (char c) const
{ return index (c) != -1; }

template <class T>
inline int CStringTraits<T>::compare (CStringPtr other, bool caseSensitive) const
{ return caseSensitive ? ::strcmp (str (), safe_str (other)) : strcasecmp (str (), safe_str (other)); }

template <class T>
inline bool CStringTraits<T>::getIntValue (int32& value) const
{ value = 0; return ::sscanf (str (), "%d", &value) == 1; }

template <class T>
inline bool CStringTraits<T>::getIntValue (int64& value) const
{ value = 0; return ::sscanf (str (), "%" FORMAT_INT64 "d", &value) == 1; }

template <class T>
inline bool CStringTraits<T>::getHexValue (int64& value) const
{ value = 0; return ::sscanf (str (), "%" FORMAT_INT64 "x", &value) == 1; }

template <class T>
inline bool CStringTraits<T>::getFloatValue (double& value) const
{ value = 0; return ::sscanf (str (), "%lf", &value) == 1; }

template <class T>
inline bool CStringTraits<T>::getFloatValue (float& value) const
{ value = 0; return ::sscanf (str (), "%f", &value) == 1; }

template <class T>
inline float CStringTraits<T>::scanFloat (float fallback) const
{ float f; return getFloatValue (f) ? f : fallback; }
	
template <class T>
inline double CStringTraits<T>::scanDouble (double fallback) const
{ double d; return getFloatValue (d) ? d : fallback; }

template <class T>
inline int CStringTraits<T>::scanInt (int fallback) const
{ int i; return getIntValue (i) ? i : fallback; }

template <class T>
inline int64 CStringTraits<T>::scanLargetInt (int64 fallback) const
{ int64 i; return getIntValue (i) ? i : fallback; }

template <class T>
inline unsigned int CStringTraits<T>::getHashCode () const
{ return CStringFunctions::hashCFSIndex (str ()); /* use positive integers for backwards compatibility. */ }

template <class T>
inline bool CStringTraits<T>::operator == (CStringPtr other) const
{ return ::strcmp (str (), safe_str (other)) == 0; }

template <class T>
inline bool CStringTraits<T>::operator != (CStringPtr other) const
{ return ::strcmp (str (), safe_str (other)) != 0; }

template <class T> template <class S> 
inline bool CStringTraits<T>::operator == (const CStringTraits<S>& other) const
{ return ::strcmp (str (), other.str ()) == 0; } // no safe_str required, both are safe

template <class T> template <class S> 
inline bool CStringTraits<T>::operator != (const CStringTraits<S>& other) const
{ return ::strcmp (str (), other.str ()) != 0; } // no safe_str required, both are safe

template <class T>
inline bool CStringTraits<T>::equalsUnsafe (CStringPtr other) const
{ return ::strcmp (str (), other) == 0; }

template <class T>
inline char CStringTraits<T>::at (int index) const
{ return str () [index]; }

template <class T>
char CStringTraits<T>::firstChar () const
{ return at (0); }
	
template <class T>
char CStringTraits<T>::lastChar () const
{  if(length () <= 0) return 0; return at (length() - 1); }
	
template <class T>
char CStringTraits<T>::operator [] (int index) const
{	return at (index); }
	  
template <class T>
inline CStringTraits<T>::operator CStringPtr () const 
{ return str ();  }

//////////////////////////////////////////////////////////////////////////////////////////////////
// MutableCStringTraits implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline T& MutableCStringTraits<T>::append (CStringPtr string, int count)
{ return static_cast<T*> (this)->__append (string, count); } // __append() needs to be available in specialized class!

template <class T>
inline T& MutableCStringTraits<T>::init (CStringPtr string)
{ return static_cast<T*> (this)->__init (string); } // __init() needs to be available in specialized class!

template <class T>
inline T& MutableCStringTraits<T>::append (char c)
{ char string[2] = {c, 0}; return append (string); }

template <class T>
inline T& MutableCStringTraits<T>::append (uchar uc)
{ char string[2] = {uc <= 255 ? (char)uc : '?', 0}; return append (string); }

template <class T>
inline T& MutableCStringTraits<T>::appendFormat (const char* format, ...)
{
	va_list marker;
	va_start (marker, format);

	T& result = appendFormatArgs (format, marker);
	va_end(marker);
	return result;
}

template <class T>
inline T& MutableCStringTraits<T>::appendFormatArgs (const char* format, va_list marker)
{
	char buffer[STRING_STACK_SPACE_MAX];

	#if defined(DSP_TI32)
	::vsprintf (buffer, format, marker);
	#else
	::vsnprintf (buffer, sizeof(buffer)-1, format, marker);
	#endif

	buffer[sizeof(buffer)-1] = 0;

	return append (buffer);
}

template <typename T>
inline T& MutableCStringTraits<T>::appendInteger (int32 value)
{
	return appendInteger (int64(value));
}

template <class T>
inline T& MutableCStringTraits<T>::appendInteger (int64 value)
{
	enum { kBufferSize = 32 };
	char buffer[kBufferSize];

	bool isNegative = value < 0;
	if(isNegative)
		value = -value;

	int pos = writeInteger (buffer, kBufferSize, uint64(value));

	if(isNegative)
		buffer[--pos] = '-';

	return static_cast<T*> (this)->__append (buffer + pos, kBufferSize - pos);
};

template <typename T>
inline T& MutableCStringTraits<T>::appendInteger (uint32 value)
{
	return appendInteger (uint64(value));
}

template <class T>
inline T& MutableCStringTraits<T>::appendInteger (uint64 value)
{
	enum { kBufferSize = 32 };
	char buffer[kBufferSize];
	int pos = writeInteger (buffer, kBufferSize, value);
	return static_cast<T*> (this)->__append (buffer + pos, kBufferSize - pos);
}

template <class T>
inline T& MutableCStringTraits<T>::operator += (char c)
{ return append (c); }

template <class T>
inline T& MutableCStringTraits<T>::operator += (uchar uc)
{ return append (uc); }

template <class T>
inline T& MutableCStringTraits<T>::operator += (CStringPtr text)
{ return append (text); }

template <typename T>
inline int MutableCStringTraits<T>::writeInteger (char buffer[], int bufferSize, uint64 value)
{
	int pos = bufferSize;
	do
	{
		char digit = value % 10;
		value /= 10;
		buffer[--pos] = '0' + digit;
	} while(value > 0);
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CStringTokenizerInplace implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline CStringTokenizerInplace::CStringTokenizerInplace (char* string, CStringPtr delimiters, bool preserveEmptyTokens)
: string (string),
  delimiters (delimiters),
  preserveEmptyTokens (preserveEmptyTokens),
  savedChar (0)
{}

inline CStringTokenizerInplace::~CStringTokenizerInplace ()
{
	// restore last replaced delimiter
	if(savedChar && string)
		*string = savedChar;
}

inline CStringPtr CStringTokenizerInplace::next ()
{
	if(!string)
		return nullptr;
	else if(savedChar)
	{
		// restore last replaced delimiter
		*string = savedChar;
		string++;
		savedChar = 0;
	}

	if(!preserveEmptyTokens)
	{
		// inspired by strtok from glibc:
		// skip leading delimiters
		string += ::strspn (string, delimiters);
		if(*string == 0)
		{
			string = nullptr; // reached the end
			return nullptr;
		}
	}

	// find next delimiter after token
	char* token = string;
	string = ::strpbrk (token, delimiters);
	if(string)
	{
		// save delimiter char and overwrite with terminator
		savedChar = *string;
		*string = 0;
	}
	else
	{
		// no more delimiter: return last token
		string = nullptr;
	}
	return token;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CStringTokenizer implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline CStringTokenizer::CStringTokenizer (CStringPtr string, CStringPtr delimiters, bool preserveEmptyTokens)
: CStringTokenizerInplace (nullptr, delimiters, preserveEmptyTokens),
  allocatedBuffer (nullptr)
{
	// Passing nullptr is undefined behavior
	ASSERT (string != nullptr)

	// strtok needs a mutable copy of the string
	int length = (int)::strlen (string);
	if(length < kBufferSize)
	{
		::strcpy (buffer, string);
		this->string = buffer;
	}
	else
	{
		buffer[0] = 0; // not used
		allocatedBuffer = NEW char[length + 1];
		::strcpy (allocatedBuffer, string);
		this->string = allocatedBuffer;
	}
}

inline CStringTokenizer::~CStringTokenizer ()
{
	this->string = nullptr;
	if(allocatedBuffer)
		delete[] allocatedBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _corestringtraits_h

