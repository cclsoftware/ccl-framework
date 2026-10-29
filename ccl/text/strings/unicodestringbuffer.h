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
// Filename    : ccl/text/strings/unicodestringbuffer.h
// Description : Unicode String Buffer
//
//************************************************************************************************

#ifndef _ccl_unicodestringbuffer_h
#define _ccl_unicodestringbuffer_h

#include "unicodestring.h"

namespace CCL {

//************************************************************************************************
// Text functions
//************************************************************************************************

namespace Text
{
	// platform-specific:
	int convertToCString (char cString[], int cStringSize, TextEncoding encoding, UStringPtr uString, int uStringLength);
	int convertToUnicode (uchar uString[], int uStringSize, TextEncoding encoding, CStringPtr cString, int cStringLength);
	int compareStrings (UStringPtr s1, int l1, UStringPtr s2, int l2, int flags);
	UStringPtr findString (UStringPtr source, UStringPtr value, int flags);
	void toUppercase (uchar s[]);
	void toLowercase (uchar s[]);
	void capitalize (uchar s[]);
}

//************************************************************************************************
// UnicodeStringBuffer
//************************************************************************************************

class UnicodeStringBuffer: public UnicodeString
{
public:
	UnicodeStringBuffer ();
	UnicodeStringBuffer (const UnicodeStringBuffer& other);
	~UnicodeStringBuffer ();

	UnicodeStringBuffer& operator = (const UnicodeStringBuffer& other);

	tresult assign (const UnicodeStringBuffer& other);

	// UnicodeString
	tresult makeConstant (CStringPtr asciiString) override;
	void releaseInternal () override;
	tbool CCL_API isEmpty () const override;
	int CCL_API getLength () const override;
	unsigned int CCL_API getHashCode () const override;
	uchar CCL_API getCharAt (int index) const override;
	tresult CCL_API getChars (CharData& data) const override;
	tresult CCL_API releaseChars (CharData& data) const override;
	tresult CCL_API copyTo (uchar charBuffer[], int bufferSize) const override;
	tresult CCL_API toCString (TextEncoding encoding, char cString[], int cStringSize, int* bytesWritten = nullptr) const override;
	tresult CCL_API toPascalString (TextEncoding encoding, unsigned char pString[], int pStringSize) const override;
	tbool CCL_API equals (const IString* otherString) const override;
	tbool CCL_API equalsChars (UStringPtr charBuffer, int count = -1) const override;
	int CCL_API compare (const IString* otherString, int flags = 0) const override;
	int CCL_API compareChars (UStringPtr charBuffer, int count = -1) const override;
	int CCL_API findSubString (const IString* otherString, int flags = 0) const override;
	IString* CCL_API createSubString (int index, int count = -1) const override;
	IStringPrivateData CCL_API getPrivateData () const override;
	tresult CCL_API assignChars (UStringPtr charBuffer, int count = -1) override;
	tresult CCL_API appendCString (TextEncoding encoding, CStringPtr cString, int count = -1) override;
	tresult CCL_API appendPascalString (TextEncoding encoding, const unsigned char pString[]) override;
	tresult CCL_API appendChars (UStringPtr charBuffer, int count = -1) override;
	tresult CCL_API appendString (const IString* otherString) override;
	tresult CCL_API appendRepeated (const IString* otherString, int count) override;
	tresult CCL_API insert (int index, const IString* otherString) override;
	tresult CCL_API remove (int index, int count) override;
	tresult CCL_API truncate (int index) override;
	void CCL_API trimWhitespace () override;
	void CCL_API toUppercase () override;
	void CCL_API toLowercase () override;
	void CCL_API capitalize () override;
	int CCL_API replace (const IString* searchString, const IString* replacementString, int flags = 0) override;

protected:
	static constexpr unsigned int kInvalidHashCode = 0xFFFFFFFF;
	static UStringPtr kEmpty;

	uchar* text;
	int textByteSize;
	int textLength;
	mutable unsigned int hashCode;

	bool resizeInternal (int newLength, bool fixed = false);
	tresult assignInternal (UStringPtr charBuffer, int count); ///< assign from buffer with known text length
	tresult appendInternal (UStringPtr charBuffer, int count); ///< append from buffer with known text length
	void updateMetadata (int newLength);
};

} // namespace CCL

#endif // _ccl_unicodestringbuffer_h
