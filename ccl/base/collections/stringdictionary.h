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
// Filename    : ccl/base/collections/stringdictionary.h
// Description : String Dictionary
//
//************************************************************************************************

#ifndef _ccl_stringdictionary_h
#define _ccl_stringdictionary_h

#include "ccl/base/object.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// Implementation macros
//************************************************************************************************

#define IMPLEMENT_STRINGDICTIONARY(DictionaryType, StringRefType) \
tbool CCL_API isCaseSensitive () const override { return dictionary.isCaseSensitive (); } \
void CCL_API setCaseSensitive (tbool state) override { dictionary.setCaseSensitive (state); } \
int CCL_API countEntries () const override { return dictionary.countEntries (); } \
StringRefType CCL_API getKeyAt (int index) const override { return dictionary.getKeyAt (index); } \
StringRefType CCL_API getValueAt (int index) const override { return dictionary.getValueAt (index); } \
StringRefType CCL_API lookupValue (StringRefType key) const override { return dictionary.lookupValue (key); } \
void CCL_API setEntry (StringRefType key, StringRefType value) override { dictionary.setEntry (key, value); } \
void CCL_API appendEntry (StringRefType key, StringRefType value) override { dictionary.appendEntry (key, value); } \
void CCL_API removeEntry (StringRefType key) override { dictionary.removeEntry (key); } \
void CCL_API removeAll () override { dictionary.removeAll (); } \
void CCL_API copyFrom (const DictionaryType& _dictionary) override { dictionary.copyFrom (_dictionary); }

//************************************************************************************************
// StringDictionary
//************************************************************************************************

class StringDictionary: public Object,
						public IStringDictionary
{
public:
	DECLARE_CLASS (StringDictionary, Object)

	StringDictionary ();
	StringDictionary (const StringDictionary& other);
	StringDictionary (const IStringDictionary& other);
	~StringDictionary ();

	void dump () const;

	// IStringDictionary
	IMPLEMENT_STRINGDICTIONARY (IStringDictionary, StringRef)
	void CCL_API convertTo (ICStringDictionary& dst, TextEncoding encoding) const override;

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IStringDictionary, Object)

protected:
	IStringDictionary& dictionary;
};

//************************************************************************************************
// CStringDictionary
//************************************************************************************************

class CStringDictionary: public Object,
						 public ICStringDictionary
{
public:
	DECLARE_CLASS (CStringDictionary, Object)

	CStringDictionary ();
	CStringDictionary (const CStringDictionary& other);
	CStringDictionary (const ICStringDictionary& other);
	~CStringDictionary ();

	void dump () const;

	// ICStringDictionary
	IMPLEMENT_STRINGDICTIONARY (ICStringDictionary, CStringRef)
	void CCL_API convertTo (IStringDictionary& dst, TextEncoding encoding) const override;

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (ICStringDictionary, Object)

protected:
	ICStringDictionary& dictionary;
};

#undef IMPLEMENT_STRINGDICTIONARY

} // namespace CCL

#endif // _ccl_stringdictionary_h
