//************************************************************************************************
//
// CCL String Extractor
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
// Filename    : xstringmodel.h
// Description : Data Model
//
//************************************************************************************************

#ifndef _xstringmodel_h
#define _xstringmodel_h

#include "ccl/base/storage/xmltree.h"

#include "ccl/base/collections/objectarray.h"

namespace XString {

//////////////////////////////////////////////////////////////////////////////////////////////////
using CCL::StringID;
using CCL::StringRef;
using CCL::UrlRef;
//////////////////////////////////////////////////////////////////////////////////////////////////

//************************************************************************************************
// Reference
//************************************************************************************************

class Reference: public CCL::Object
{
public:
	DECLARE_CLASS (Reference, Object)

	Reference (StringID scope = nullptr, StringRef fileName = nullptr, int lineNumber = 0);

	PROPERTY_MUTABLE_CSTRING (scope, Scope)
	PROPERTY_STRING (fileName, FileName)
	PROPERTY_VARIABLE (int, lineNumber, LineNumber)

	// Object
	bool equals (const Object& obj) const override;
};

//************************************************************************************************
// Translated
//************************************************************************************************

class Translated: public CCL::Object
{
public:
	DECLARE_CLASS (Translated, Object)

	Translated (StringID key = nullptr);

	PROPERTY_MUTABLE_CSTRING (key, Key)

	void addReference (const Reference& r);
	CCL::Iterator* getReferences () const;

	typedef CCL::Vector<CCL::MutableCString> ScopeList;
	void getScopes (ScopeList& scopes) const;
	void getScopeReferences (CCL::Container& result, StringID scope) const;

	// Object
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;

protected:
	CCL::ObjectArray references;
};

//************************************************************************************************
// Bundle
//************************************************************************************************

class Bundle: public CCL::Object
{
public:
	DECLARE_CLASS (Bundle, Object)

	Bundle ();

	Translated* addOccurance (StringID key, const Reference& r);

	int countEntries () const;
	CCL::Iterator* newIterator () const;

protected:
	CCL::ObjectArray entries;
};

} // namespace XString

#endif // _xstringmodel_h
