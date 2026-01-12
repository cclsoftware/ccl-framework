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
// Filename    : ccl/platform/cocoa/text/unicodestring.cocoa.h
// Description : Unicode String Implementation with CFStrings
//
//************************************************************************************************

#ifndef _unicodestring_cocoa_h
#define _unicodestring_cocoa_h

#include "ccl/text/strings/unicodestring.h"

namespace CCL {
	
//************************************************************************************************
// UnicodeCFString
//************************************************************************************************

class UnicodeCFString: public UnicodeString
{
public:
	UnicodeCFString ();
	UnicodeCFString (const UnicodeCFString& other);
	~UnicodeCFString ();

	UnicodeCFString& operator = (const UnicodeCFString& other);

	// UnicodeString
	tresult makeConstant (const char* asciiString);
	void releaseInternal ();
	tbool CCL_API isEmpty () const;
	int CCL_API getLength () const;
	uchar CCL_API getCharAt (int index) const;
	tresult CCL_API getChars (CharData& data) const;
	tresult CCL_API releaseChars (CharData& data) const;
	tresult CCL_API copyTo (uchar* charBuffer, int bufferSize) const;
	tresult CCL_API toCString (TextEncoding encoding, char* cString, int cStringSize, int* bytesWritten = nullptr) const;
	tresult CCL_API toPascalString (TextEncoding encoding, unsigned char* pString, int pStringSize) const;
	tbool CCL_API equals (const IString* otherString) const;
	tbool CCL_API equalsChars (const uchar* charBuffer, int count = -1) const;
	int CCL_API compare (const IString* otherString, int flags = 0) const;
	int CCL_API compareChars (const uchar* charBuffer, int count = -1) const;
	int CCL_API findSubString (const IString* otherString, int flags = 0) const;
	IString* CCL_API createSubString (int index, int count = -1) const;
	IString* CCL_API cloneString () const;
	void* CCL_API createNativeString () const;
	void CCL_API releaseNativeString (void* nativeString) const;
	IStringPrivateData CCL_API getPrivateData () const;
	tresult CCL_API assignChars (const uchar* charBuffer, int count = -1);
	tresult CCL_API appendCString (TextEncoding encoding, const char* cString, int count = -1);
	tresult CCL_API appendPascalString (TextEncoding encoding, const unsigned char* pString);
	tresult CCL_API appendChars (const uchar* charBuffer, int count = -1);
	tresult CCL_API appendString (const IString* otherString);
	tresult CCL_API appendRepeated (const IString* otherString, int count);
	tresult CCL_API appendNativeString (const void* nativeString);
	tresult CCL_API insert (int index, const IString* otherString);
	tresult CCL_API remove (int index, int count);
	tresult CCL_API truncate (int index);
	void CCL_API trimWhitespace ();
	void CCL_API toUppercase ();
	void CCL_API toLowercase ();
	void CCL_API capitalize ();
	int CCL_API replace (const IString* searchString, const IString* replacementString, int flags = 0);
	tbool CCL_API isNormalized (NormalizationForm form) const;
	tresult CCL_API normalize (NormalizationForm form);
	
protected:
	void* cfString;
	#if DEBUG
	char __private[128];
	#endif
	
	void* getCFString ();
	void updatePrivate ();

	static unsigned int getNativeEncoding (TextEncoding encoding);
	static CFStringNormalizationForm getNativeNormalizationForm (NormalizationForm form);
};

//************************************************************************************************
// CocoaUnicodeUtilities
//************************************************************************************************

class CocoaUnicodeUtilities: public UnicodeUtilities
{
public:
	// UnicodeUtilities
	tbool CCL_API isAlpha (uchar c) const;
	tbool CCL_API isAlphaNumeric (uchar c) const;
	tbool CCL_API isLowercase (uchar c) const;
	tbool CCL_API isUppercase (uchar c) const;
	uchar CCL_API toLowercase (uchar c) const;
	uchar CCL_API toUppercase (uchar c) const;
};

} // namespace CCL

#endif // _unicodestring_cocoa_h
