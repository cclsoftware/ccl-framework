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
// Filename    : ccl/text/strings/stringtable.cpp
// Description : String Hash Table
//
//************************************************************************************************

#include "ccl/text/strings/stringtable.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// StringEntry
//************************************************************************************************

StringEntry::StringEntry (const char* _cString, OwnerHint hint)
: cString (_cString),
  ownCString (hint == kTake)
{
	if(hint == kCopy)
	{
		cString = NEW char[::strlen (_cString) + 1];
		::strcpy ((char*)cString, _cString);
		ownCString = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry::StringEntry (CStringRef string)
: cString (string.str ()),
  ownCString (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry::~StringEntry ()
{
	if(ownCString)
		delete [] cString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringEntry::release () 
{ 
	delete this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StringEntry::getHashCode (int size) const 
{ 
	return Core::CStringFunctions::hashCFSIndex (cString) % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringEntry::equals (const StringEntry& other) const
{ 
	return strcmp (cString, other.cString) == 0; 
}

//************************************************************************************************
// UnicodeStringEntry
//************************************************************************************************

UnicodeStringEntry::UnicodeStringEntry (const char* cString, StringRef theString, OwnerHint hint)
: StringEntry (cString, hint),
  theString (theString)
{}

//************************************************************************************************
// StringEntryList
//************************************************************************************************

StringEntryList::~StringEntryList ()
{
	ListForEach (*this, StringEntry*, e)
		e->release ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry* StringEntryList::lookup (const StringEntry& entryToFind)
{
	ListForEach (*this, StringEntry*, e)
		if(e->equals (entryToFind))
			return e;
	EndFor
	return nullptr;
}

//************************************************************************************************
// StringTable
//************************************************************************************************

int StringTable::hashEntry (StringEntry* const& entry, int size)
{
	return entry->getHashCode (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTable::StringTable (int size)
: HashTable<StringEntry*, StringEntryList> (size, hashEntry)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringEntry* StringTable::lookup (const StringEntry& entryToFind) const
{
	int idx = entryToFind.getHashCode (size);
	return table[idx].lookup (entryToFind);
}

} // namespace CCL
