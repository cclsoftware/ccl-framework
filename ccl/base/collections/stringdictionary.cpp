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
// Filename    : ccl/base/collections/stringdictionary.cpp
// Description : String Dictionary
//
//************************************************************************************************

#include "ccl/base/collections/stringdictionary.h"

#include "ccl/base/storage/storage.h"

namespace CCL {

//************************************************************************************************
// Association
//************************************************************************************************

class Association: public Object
{
public:
	DECLARE_CLASS (Association, Object)

	Association (StringRef key = nullptr, StringRef value = nullptr);

	PROPERTY_STRING (key, Key)
	PROPERTY_STRING (value, Value)

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// DictionaryMethods
//************************************************************************************************

template <typename Dictionary>
class DictionaryMethods
{
public:
	static bool equals (const Dictionary& d1, const Dictionary& d2)
	{
		if(d1.countEntries () != d2.countEntries ())
			return false;

		for(int i = 0; i < d1.countEntries (); i++)
		{
			auto& key1 = d1.getKeyAt (i);
			auto& value1 = d1.getValueAt (i);
			if(d2.lookupValue (key1) != value1)
				return false;
		}
	
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	template <typename StringType>
	static bool load (Dictionary& d, Attributes& a)
	{
		d.removeAll ();

		// new approach: use iterator instead of unqueue to preserve other data
		IterForEach (a.newQueueIterator (nullptr), Association, assoc)
			if(assoc != nullptr) // can be null because of filtering
				d.appendEntry (StringType (assoc->getKey ()), StringType (assoc->getValue ()));
		EndFor
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	template <typename StringType>
	static bool save (Attributes& a, const Dictionary& d)
	{
		for(int i = 0; i < d.countEntries (); i++)
			a.queue (nullptr, NEW Association (StringType (d.getKeyAt (i)), StringType (d.getValueAt (i))), Attributes::kOwns);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	static void dump (const Dictionary& d)
	{
		for(int i = 0; i < d.countEntries (); i++)
		{
			Debugger::printf ("[%d] key = \"", i);
			Debugger::print (d.getKeyAt (i));
			Debugger::print ("\" value = \"");
			Debugger::print (d.getValueAt (i));
			Debugger::println ("\"");
		}
	}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Association
//************************************************************************************************

DEFINE_CLASS (Association, Object)
DEFINE_CLASS_NAMESPACE (Association, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Association::Association (StringRef key, StringRef value)
: key (key),
  value (value)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Association::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	key = a.getString ("key");
	value = a.getString ("value");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Association::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("key", key);
	a.set ("value", value);
	return true;
}

//************************************************************************************************
// StringDictionary
//************************************************************************************************

DEFINE_CLASS (StringDictionary, Object)
DEFINE_CLASS_NAMESPACE (StringDictionary, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringDictionary::StringDictionary ()
: dictionary (*System::CreateStringDictionary ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringDictionary::StringDictionary (const StringDictionary& other)
: dictionary (*System::CreateStringDictionary ())
{
	copyFrom (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringDictionary::StringDictionary (const IStringDictionary& other)
: dictionary (*System::CreateStringDictionary ())
{
	copyFrom (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringDictionary::~StringDictionary ()
{
	dictionary.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringDictionary::convertTo (ICStringDictionary& dst, TextEncoding encoding) const
{
	dictionary.convertTo (dst, encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringDictionary::equals (const Object& obj) const
{
	const StringDictionary* other = ccl_cast<StringDictionary> (&obj);
	if(other)
		return DictionaryMethods<IStringDictionary>::equals (dictionary, other->dictionary);
	else
		return SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringDictionary::load (const Storage& storage)
{
	return DictionaryMethods<IStringDictionary>::load<String> (dictionary, storage.getAttributes ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringDictionary::save (const Storage& storage) const
{
	return DictionaryMethods<IStringDictionary>::save<String> (storage.getAttributes (), dictionary);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringDictionary::dump () const
{
	DictionaryMethods<IStringDictionary>::dump (*this);
}

//************************************************************************************************
// CStringDictionary
//************************************************************************************************

DEFINE_CLASS (CStringDictionary, Object)
DEFINE_CLASS_NAMESPACE (CStringDictionary, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringDictionary::CStringDictionary ()
: dictionary (*System::CreateCStringDictionary ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringDictionary::CStringDictionary (const CStringDictionary& other)
: dictionary (*System::CreateCStringDictionary ())
{
	copyFrom (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringDictionary::CStringDictionary (const ICStringDictionary& other)
: dictionary (*System::CreateCStringDictionary ())
{
	copyFrom (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringDictionary::~CStringDictionary ()
{
	dictionary.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CStringDictionary::convertTo (IStringDictionary& dst, TextEncoding encoding) const
{
	dictionary.convertTo (dst, encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CStringDictionary::equals (const Object& obj) const
{
	const CStringDictionary* other = ccl_cast<CStringDictionary> (&obj);
	if(other)
		return DictionaryMethods<ICStringDictionary>::equals (dictionary, other->dictionary);
	else
		return SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CStringDictionary::load (const Storage& storage)
{
	return DictionaryMethods<ICStringDictionary>::load<MutableCString> (dictionary, storage.getAttributes ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CStringDictionary::save (const Storage& storage) const
{
	return DictionaryMethods<ICStringDictionary>::save<String> (storage.getAttributes (), dictionary);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CStringDictionary::dump () const
{
	for(int i = 0; i < countEntries (); i++)
		Debugger::printf ("[%d] key = \"%s\" value = \"%s\"\n", i, getKeyAt (i).str (), getValueAt (i).str ());
}
