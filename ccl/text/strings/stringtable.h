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
// Filename    : ccl/text/strings/stringtable.h
// Description : String Hash Table
//
//************************************************************************************************

#ifndef _ccl_stringtable_h
#define _ccl_stringtable_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/collections/hashtable.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// StringEntry
//************************************************************************************************

struct StringEntry
{
	const char* cString;
	bool ownCString;

	enum OwnerHint
	{
		kNoCopy,
		kCopy,
		kTake
	};

	StringEntry (const char* cString = nullptr, OwnerHint hint = kNoCopy);
	StringEntry (CStringRef string); // no copy!
	virtual ~StringEntry ();

	void release ();
	int getHashCode (int size) const;
	bool equals (const StringEntry& other) const;
};

//************************************************************************************************
// UnicodeStringEntry
//************************************************************************************************

struct UnicodeStringEntry: StringEntry
{
	String theString;

	UnicodeStringEntry (const char* cString = nullptr, StringRef theString = nullptr, OwnerHint hint = kNoCopy);
};

//************************************************************************************************
// StringEntryList
//************************************************************************************************

class StringEntryList: public LinkedList<StringEntry*>
{
public:
	~StringEntryList ();
	StringEntry* lookup (const StringEntry& entryToFind);
};

//************************************************************************************************
// StringTable
//************************************************************************************************

class StringTable: public Unknown,
				   public HashTable<StringEntry*, StringEntryList>
{
public:
	StringTable (int size = 100);

	StringEntry* lookup (const StringEntry& entryToFind) const;

protected:
	static int hashEntry (StringEntry* const& entry, int size);
};

} // namespace CCL

#endif // _ccl_stringtable_h
