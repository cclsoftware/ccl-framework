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
// Filename    : ccl/base/collections/stringlist.h
// Description : String List
//
//************************************************************************************************

#ifndef _ccl_stringlist_h
#define _ccl_stringlist_h

#include "ccl/base/boxedtypes.h"

#include "ccl/base/collections/objectlist.h"

namespace CCL {

class StringBuilder;

//************************************************************************************************
// StringList
/** List of Strings. \ingroup base_collect */
//************************************************************************************************

class StringList: public Object
{
public:
	DECLARE_CLASS (StringList, Object)

	StringList ();
	StringList (const InitializerList<String>& list);

	void addAllFrom (const StringList& stringList);

	void add (StringRef string);
	void addOnce (StringRef string); ///< add if not contained
	void addSorted (StringRef string);
	void addSortedOnce (StringRef string);
	void prepend (StringRef string);
	bool moveToHead (StringRef string);
	bool replaceAt (int index, StringRef string);
	bool remove (StringRef string);
	void removeAll ();
	bool removeFirst ();
	bool removeLast ();

	bool contains (StringRef string, bool caseSensitive = true) const;
	bool containsAnyOf (const StringList& stringList, bool caseSensitive = true) const;
	bool containsSubStringOf (StringRef string, bool caseSensitive) const;
	bool isEmpty () const;
	int count () const;
	Iterator* newIterator () const;
	String operator [] (int index) const;
	String at (int index) const;
	int index (StringRef string) const;

	template<class Lambda> void forEach (const Lambda& visit) const; // void visit (StringRef)

	StringList& operator = (const StringList&);
	bool operator == (const StringList&) const;

	String concat (StringRef delimiter) const;
	void addToBuilder (StringBuilder& builder) const;

	RangeIterator<ObjectList, ObjectListIterator, Boxed::String*> begin () const;
	RangeIterator<ObjectList, ObjectListIterator, Boxed::String*> end () const;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectList list; ///< of Boxed::String
};

//************************************************************************************************
// IAutoComplete
//************************************************************************************************

interface IAutoComplete: IUnknown
{
	// source for auto complete suggestions
	virtual tbool suggestCompletions (StringList& completions, StringRef input) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// StringList inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringList::StringList () { list.objectCleanup (); }
inline void StringList::removeAll () { list.removeAll (); }
inline bool StringList::isEmpty () const { return list.isEmpty (); }
inline int StringList::count () const { return list.count (); }
inline Iterator* StringList::newIterator () const { return list.newIterator (); }
inline int StringList::index (StringRef string) const { Boxed::String s (string); return list.index (s); }
inline RangeIterator<ObjectList, ObjectListIterator, Boxed::String*> StringList::begin () const	{ return list.begin<Boxed::String> (); }
inline RangeIterator<ObjectList, ObjectListIterator, Boxed::String*> StringList::end () const	{ return list.end<Boxed::String> (); }

template<class Lambda>
inline void StringList::forEach (const Lambda& visit) const
{
	ListForEachObject (list, Boxed::String, str)
		visit (*str);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_stringlist_h
