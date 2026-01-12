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
// Filename    : ccl/public/text/cstring.h
// Description : C-String Class
//
//************************************************************************************************

#ifndef _ccl_cstring_h
#define _ccl_cstring_h

#include "ccl/public/textservices.h"

#include "core/public/corestringtraits.h"

namespace CCL {

class MutableCString;

//////////////////////////////////////////////////////////////////////////////////////////////////
// C-String macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CSTR(str) CCL::System::GetConstantCString (str)

//************************************************************************************************
// ICString
/** C-String interface, for internal use only!
	\ingroup text_string */
//************************************************************************************************

interface ICString: IUnknown
{
	/** Resize internal text buffer. */
	virtual tbool CCL_API resize (int newLength) = 0;

	/** Get address of internal text buffer. */
	virtual char* CCL_API getText () = 0;

	/** Clone C-String object. */
	virtual ICString* CCL_API cloneString () const = 0;

	DECLARE_IID (ICString)
};

//************************************************************************************************
// PlainCString
/** The string class below is binary equivalent to this C structure.
	\ingroup text_string */
//************************************************************************************************

struct PlainCString
{
	CStringPtr text;		///< raw string pointer
	ICString* theString;	///< string interface pointer
};

//************************************************************************************************
// CString
/** Immutable C-String class, safe for ASCII-encoded text only!
	\ingroup text_string */
//************************************************************************************************

class CString: protected PlainCString,
			   public Core::CStringTraits<CString>,
			   public Core::CStringClassifier
{
public:
	/** Construct with plain C-String. */
	CString (CStringPtr text = nullptr);

	/** Copy constructor. */
	CString (CStringRef other);

	/** Destructor. */
	~CString ();

	/** Empty C-String. */
	static const CString kEmpty;

	/** Get line ending of given format. */
	static CStringRef getLineEnd (TextLineFormat lineFormat = Text::kSystemLineFormat);

	/** Create substring. */
	MutableCString subString (int index, int count = -1) const;
	
	/** Returns substring between prefix and suffix, string must start with prefix. */
	MutableCString getBetween (CStringPtr prefix, CStringPtr suffix) const;

	/** Sets result to the substring between prefix and suffix, string must start with prefix.
		Returns true if prefix and suffix match, even if the result between is empty. */
	bool getBetween (MutableCString& result, CStringPtr prefix, CStringPtr suffix) const;

	/** Convert to Unicode String. */
	void toUnicode (String& string, TextEncoding encoding = Text::kASCII) const;

	/** Assign plain C-String. */
	CString& operator = (CStringPtr text);

	/** Assign other C-String. */
	CString& operator = (CStringRef other);

protected:
	// required by CStringTraits:
	friend class Core::CStringTraits<CString>;
	CStringPtr __str () const { return safe_str (text); }
};

//************************************************************************************************
// MutableCString
/** Mutable C-String class, safe for ASCII-encoded text only!
	\ingroup text_string */
//************************************************************************************************

class MutableCString: public CString,
					  public Core::MutableCStringTraits<MutableCString>
{
public:
	/** Construct with C-String. */
	MutableCString (CStringPtr text = nullptr);

	/** Construct with C-String. */
	MutableCString (CStringRef string);

	/** Copy constructor. */
	MutableCString (const MutableCString& string);

	/** Construct with Unicode String. */
	MutableCString (StringRef string, TextEncoding encoding = Text::kASCII);

	/** Empty string. */
	MutableCString& empty ();

	/** Append Unicode String. */
	MutableCString& append (StringRef string, TextEncoding encoding = Text::kASCII);

	/** Truncate string at given position. */
	MutableCString& truncate (int index);

	/** Insert string at given position. */
	MutableCString& insert (int index, CStringPtr otherString);

	/** Replace range by other string. */
	MutableCString& replace (int index, int count, CStringPtr otherString);

	/** Replace all occurances of one character with another. */
	MutableCString& replace (char oldChar, char newChar);

	/** Change all alphabetical characters to lowercase. */
	MutableCString& toLowercase ();

	/** Change all alphabetical characters to uppercase. */
	MutableCString& toUppercase ();

	/** Remove all whitespace at beginning and end. */
	MutableCString& trimWhitespace ();

	/** Assign C-String. */
	MutableCString& operator = (CStringPtr text);

	/** Assign C-String. */
	MutableCString& operator = (CStringRef string);

	/** Assign mutable C-String. */
	MutableCString& operator = (const MutableCString& string);

	/** Assign Unicode String. */
	MutableCString& operator = (StringRef string);

	/** Append Unicode String. */
	MutableCString& operator += (StringRef string);

	using Core::MutableCStringTraits<MutableCString>::append;
	using Core::MutableCStringTraits<MutableCString>::operator +=;

protected:
	friend class CString;
	void initString ();
	void writeEnable ();
	bool resize (int newLength);

	// required by MutableCStringTraits:
	friend class Core::MutableCStringTraits<MutableCString>;
	MutableCString& __append (CStringPtr string, int count);
};

//************************************************************************************************
// CStringWriter
/** Helper class for appending characters to mutable C-String.
	\ingroup text_string */
//************************************************************************************************

template <int size>
class CStringWriter
{
public:
	CStringWriter (MutableCString& string, bool emptyFirst = true)
	: string (string),
	  count (0)
	{
		if(emptyFirst)
			string.empty ();
	}

	void append (char c)
	{
		buffer[count] = c;
		if(++count >= size)
			flush ();
	}

	void append (uchar c)
	{
		buffer[count] = c < 255 ? (char)c : '?';
		if(++count >= size)
			flush ();
	}

	void flush ()
	{
		if(count > 0)
			string.append (buffer, count), count = 0;
	}

protected:
	MutableCString& string;
	char buffer[size];
	int count;
};

} // namespace CCL

#endif // _ccl_cstring_h
