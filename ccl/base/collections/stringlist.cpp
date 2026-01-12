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
// Filename    : ccl/base/collections/stringlist.cpp
// Description : String List
//
//************************************************************************************************

#include "ccl/base/collections/stringlist.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/text/stringbuilder.h"

using namespace CCL;

//************************************************************************************************
// StringList
//************************************************************************************************

DEFINE_CLASS (StringList, Object)
DEFINE_CLASS_NAMESPACE (StringList, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringList::StringList (const InitializerList<String>& list)
: StringList ()
{
	for(auto s : list)
		add (s);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringList::addAllFrom (const StringList& stringList)
{
	ListForEachObject (stringList.list, Boxed::String, str)
		add (*str);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringList::add (StringRef string)
{
	list.add (NEW Boxed::String (string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringList::addOnce (StringRef string)
{
	if(!contains (string))
		add (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringList::addSorted (StringRef string)
{
	list.addSorted (NEW Boxed::String (string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringList::addSortedOnce (StringRef string)
{
	if(!contains (string))
		addSorted (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringList::prepend (StringRef string)
{
	list.prepend (NEW Boxed::String (string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::moveToHead (StringRef string)
{
	bool first = true;
	ListForEachObject (list, Boxed::String, str)
		if(*str == string)
		{
			if(first)
				return true;

			list.remove (__iter);
			list.prepend (str);
			return true;
		}
		first = false;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::remove (StringRef string)
{
	ListForEachObject (list, Boxed::String, str)
		if(*str == string)
		{
			list.remove (__iter);
			str->release ();
			return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::removeFirst ()
{
	Boxed::String* str = (Boxed::String*)list.removeFirst ();
	if(str)
	{
		str->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::removeLast ()
{
	Boxed::String* str = (Boxed::String*)list.removeLast ();
	if(str)
	{
		str->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::contains (StringRef string, bool caseSensitive) const
{
	ListForEachObject (list, Boxed::String, str)
		if(string.compare (*str, caseSensitive) == Text::kEqual)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::containsSubStringOf (StringRef string, bool caseSensitive) const
{
	ListForEachObject (list, Boxed::String, str)
		if(string.contains (*str, caseSensitive))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::containsAnyOf (const StringList& stringList, bool caseSensitive) const
{
	ListForEachObject (list, Boxed::String, str)
		if(stringList.contains (*str, caseSensitive))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringList& StringList::operator = (const StringList& other)
{
	removeAll ();
	addAllFrom (other);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::operator == (const StringList& other) const
{
	ObjectListIterator otherIter (other.list);

	ListForEachObject (list, Boxed::String, str)
		Boxed::String* otherStr = (Boxed::String*)otherIter.next ();
		if(!otherStr || *otherStr != *str)
			return false;
	EndFor

	if(!otherIter.done ())
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringList::at (int index) const
{
	String result;
	Boxed::String* str = (Boxed::String*)list.at (index);
	if(str)
		result = *str;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::replaceAt (int index, StringRef string)
{
	if(Boxed::String* str = (Boxed::String*)list.at (index))
	{
		*str = string;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringList::operator [] (int index) const
{
	return at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringList::concat (StringRef delimiter) const
{
	String result;
	ListForEachObject (list, Boxed::String, str)
		if(!result.isEmpty ())
			result << delimiter;
		result << *str;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringList::addToBuilder (StringBuilder& builder) const
{
	ListForEachObject (list, Boxed::String, str)
		builder.addItem (*str);
		if(builder.isLimitReached ())
			break;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::load (const Storage& storage)
{
	storage.getAttributes ().unqueue (list, nullptr, ccl_typeid<Boxed::String> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringList::save (const Storage& storage) const
{
	storage.getAttributes ().queue (nullptr, list, Attributes::kShare);
	return true;
}
