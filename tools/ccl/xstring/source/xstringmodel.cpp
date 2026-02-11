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
// Filename    : xstringmodel.cpp
// Description : Data Model
//
//************************************************************************************************

#include "xstringmodel.h"

using namespace CCL;
using namespace XString;

//************************************************************************************************
// Reference
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Reference, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Reference::Reference (StringID scope, StringRef fileName, int lineNumber)
: scope (scope),
  fileName (fileName),
  lineNumber (lineNumber)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Reference::equals (const Object& obj) const
{
	const Reference& r = (const Reference&)obj;
	return scope == r.scope && fileName == r.fileName;// && lineNumber == r.lineNumber;
}

//************************************************************************************************
// Translated
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Translated, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Translated::Translated (StringID key)
: key (key)
{
	references.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Translated::addReference (const Reference& _r)
{
	Reference* r = (Reference*)references.findEqual (_r);
	//ASSERT (r == 0)
	if(r == nullptr)
		references.add (_r.clone ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Translated::getReferences () const
{
	return references.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Translated::getScopes (ScopeList& scopes) const
{
	ForEach (references, Reference, r)
		if(!r->getScope ().isEmpty ())
		{
			if(!scopes.contains (r->getScope ()))
				scopes.add (r->getScope ());
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Translated::getScopeReferences (Container& result, StringID scope) const
{
	ForEach (references, Reference, r)
		if(r->getScope () == scope)
			result.add (r);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Translated::equals (const Object& obj) const
{
	return compare (obj) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Translated::compare (const Object& obj) const
{
	const Translated& t = (const Translated&)obj;
	return key.compare (t.key);
}

//************************************************************************************************
// Bundle
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Bundle, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Bundle::Bundle ()
{
	entries.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Translated* Bundle::addOccurance (StringID key, const Reference& r)
{
	Translated* t = (Translated*)entries.search (Translated (key));
	if(t == nullptr)
		entries.addSorted (t = NEW Translated (key));
	
	t->addReference (r);
	return t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Bundle::countEntries () const
{
	return entries.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Bundle::newIterator () const
{
	return entries.newIterator ();
}
