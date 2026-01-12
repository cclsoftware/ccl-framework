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
// Filename    : ccl/text/xml/xmlstringdict.h
// Description : XML String Dictionary
//
//************************************************************************************************

#ifndef _ccl_xmlstringdict_h
#define _ccl_xmlstringdict_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

#include "ccl/text/strings/textdictionary.h"

namespace CCL {

//************************************************************************************************
// Implementation macros
//************************************************************************************************

#define IMPLEMENT_STRINGDICTIONARY(StringRefType) \
tbool CCL_API isCaseSensitive () const override { return caseSensitive; } \
void CCL_API setCaseSensitive (tbool state) override { caseSensitive = state != 0; } \
int CCL_API countEntries () const override { return count (); } \
StringRefType CCL_API getKeyAt (int index) const override { return keyAt (index); } \
StringRefType CCL_API getValueAt (int index) const override { return valueAt (index); } \
StringRefType CCL_API lookupValue (StringRefType key) const override { return lookup (key); } \
void CCL_API setEntry (StringRefType key, StringRefType value) override { set (key, value); } \
void CCL_API appendEntry (StringRefType key, StringRefType value) override { append (key, value); } \
void CCL_API removeEntry (StringRefType key) override { remove (key); } \
void CCL_API removeAll () override { empty (); }

//************************************************************************************************
// XmlStringDictionary
//************************************************************************************************

class XmlStringDictionary: public Unknown,
						   public IStringDictionary,
						   private TextDictionary<String, StringRef>
{
public:
	XmlStringDictionary () {}
	XmlStringDictionary (const XmlStringDictionary&);
	XmlStringDictionary (const IStringDictionary&);

	// IStringDictionary
	IMPLEMENT_STRINGDICTIONARY (StringRef)
	void CCL_API copyFrom (const IStringDictionary& dictionary) override;
	void CCL_API convertTo (ICStringDictionary& dst, TextEncoding encoding) const override;

	CLASS_INTERFACE (IStringDictionary, Unknown)
};

//************************************************************************************************
// XmlCStringDictionary
//************************************************************************************************

class XmlCStringDictionary: public Unknown,
							public ICStringDictionary,
							private TextDictionary<MutableCString, CStringRef>
{
public:
	XmlCStringDictionary () {}
	XmlCStringDictionary (const XmlCStringDictionary&);
	XmlCStringDictionary (const ICStringDictionary&);

	// ICStringDictionary
	IMPLEMENT_STRINGDICTIONARY (CStringRef)
	void CCL_API copyFrom (const ICStringDictionary& dictionary) override;
	void CCL_API convertTo (IStringDictionary& dst, TextEncoding encoding) const override;

	CLASS_INTERFACE (ICStringDictionary, Unknown)
};

#undef IMPLEMENT_STRINGDICTIONARY

} // namespace CCL

#endif // _ccl_xmlstringdict_h
