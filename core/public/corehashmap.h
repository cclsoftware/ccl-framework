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
// Filename    : core/public/corehashmap.h
// Description : Hash map class
//
//************************************************************************************************

#ifndef _corehashmap_h
#define _corehashmap_h

#include "core/public/corevector.h"
#include "core/public/coremap.h"

namespace Core {

template<class TKey, class TValue> class HashMapIterator;

//************************************************************************************************
// HashMap
/** Hash map container class.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class TKey, class TValue>
class HashMap
{
public:
	typedef KeyValue<TKey, TValue> TAssociation;
	typedef Vector<TAssociation> TList;
	typedef VectorIterator<TAssociation> TIterator;

	/** Hash function type. */
	typedef int (*HashFunc) (const TKey& key, int size);

	/** Construct with built-in integer hash function. */
	HashMap (int size, TValue errorValue);

	/** Construct with custom hash function. */
	HashMap (int size, HashFunc hashFunc = hashInt, TValue errorValue = TValue ());

	/** Copy constructor. */
	HashMap (const HashMap& other);

	#ifdef __cpp_rvalue_references
	/** Move constructor. */
	HashMap (HashMap&& other);
	#endif

	/** Assign (copy) other hash map. */
	HashMap& operator = (const HashMap& other);

	~HashMap ();

	/** Check if container is empty. */
	bool isEmpty () const;

	/** Count elements in container. */
	int count () const;

	/** Add key/value pair. */
	void add (const TKey& key, const TValue& value);

	/** Remove key. */
	bool remove (const TKey& key);

	/** Replace value for key. */
	bool replaceValue (const TKey& key, const TValue& value);

	/** Remove all elements from container. */
	void removeAll ();

	/** Look-up value for key. */
	const TValue& lookup (const TKey& key) const;

	/** Get value for key, returns false if not found. */
	bool get (TValue& value, const TKey& key) const;

	/** Check if key is contained in map. */
	bool contains (const TKey& key) const;

	/** Get key for given value (reverse look-up). */
	bool getKey (TKey& key, const TValue& value) const;

	RangeIterator<HashMap<TKey, TValue>, HashMapIterator<TKey, TValue>, TValue> begin () const;
	RangeIterator<HashMap<TKey, TValue>, HashMapIterator<TKey, TValue>, TValue> end () const;

protected:
	int size;
	HashFunc hashFunc;
	TList* table;
	int total;
	TValue error;

	template<class _TKey, class _TValue> friend class HashMapIterator;

	static int hashInt (const TKey& key, int size) { return ((key % size) + size) % size; };
};

//************************************************************************************************
// HashMapIterator
/** Hash map iterator.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class TKey, class TValue>
class HashMapIterator
{
public:
	HashMapIterator (const HashMap<TKey, TValue>& map);
	~HashMapIterator ();

	typedef typename HashMap<TKey, TValue>::TAssociation Association;

	/** Check if iteration is done. */
	bool done () const;	

	/** Seek to first element. */
	void first ();
	
	/** Seek to last element. */
	void last ();
	
	/** Seek and return next key/value pair. */
	const Association& nextAssociation ();
	
	/** Seek and return previous key/value pair. */
	const Association& previousAssociation ();

	/** Seek and return next value. */
	const TValue& next ();
	
	/** Seek and return previous value. */
	const TValue& previous ();
	
	/** Peek at next value (but don't seek). */
	const TValue& peekNext ();

	bool operator == (const HashMapIterator<TKey, TValue>& other) const;
	bool operator != (const HashMapIterator<TKey, TValue>& other) const;

protected:
	const HashMap<TKey, TValue>& map;
	int tableIndex;
	typename HashMap<TKey, TValue>::TIterator* listIter;
	Association error;

	void findNextList ();
	void findPreviousList ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// HashMap implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
HashMap<TKey, TValue>::HashMap (int size, TValue errorValue)
: HashMap (size, hashInt, errorValue)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
HashMap<TKey, TValue>::HashMap (int size, HashFunc hashFunc, TValue errorValue)
: size (size),
  hashFunc (hashFunc),
  table (nullptr),
  total (0),
  error (errorValue)
{
	if(size > 0)
		table = NEW TList[size];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
HashMap<TKey, TValue>::HashMap (const HashMap& other)
: size (other.size),
  hashFunc (other.hashFunc),
  table (nullptr),
  total (other.total),
  error (other.error)
{
	if(size > 0)
	{
		table = NEW TList[size];
		for(int i = 0; i < other.size; i++)
			table[i] = other.table[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cpp_rvalue_references
template<class TKey, class TValue>
HashMap<TKey, TValue>::HashMap (HashMap&& other)
: size (other.size),
  hashFunc (other.hashFunc),
  table (other.table),
  total (other.total),
  error (other.error)
{
	other.size = 0;
	other.table = 0;
	other.total = 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
HashMap<TKey, TValue>& HashMap<TKey, TValue>::operator = (const HashMap& other)
{
	if(table && (size != other.size))
	{
		delete [] table;
		table = 0;
		if(other.size > 0)
			table = NEW TList[other.size];
	}

	size = other.size;
	hashFunc = other.hashFunc;
		
	if(size > 0)
	{
		for(int i = 0; i < size; i++)
			table[i] = other.table[i];
	}
		
	total = other.total;
	error = other.error;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
HashMap<TKey, TValue>::~HashMap ()
{
	if(table)
		delete [] table;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMap<TKey, TValue>::isEmpty () const
{
	return total == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
int HashMap<TKey, TValue>::count () const
{
	return total;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
void HashMap<TKey, TValue>::add (const TKey& key, const TValue& value)
{
	int index = hashFunc (key, size);
	table[index].add (TAssociation (key, value));
	total++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMap<TKey, TValue>::remove (const TKey& key)
{
	int index = hashFunc (key, size);
	TIterator iter (table[index]);
	while(!iter.done ())
	{
		const TAssociation& a = iter.next ();
		if(a.key == key)
		{
			table[index].remove (a);
			total--;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMap<TKey, TValue>::replaceValue (const TKey& key, const TValue& value)
{
	if(!remove (key))
		return false;
	add (key, value);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
void HashMap<TKey, TValue>::removeAll ()
{
	if(total == 0)
		return;

	for(int i = 0; i < size; i++)
		table[i].removeAll ();
	total = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
const TValue& HashMap<TKey, TValue>::lookup (const TKey& key) const
{
	int index = hashFunc (key, size);
	TIterator iter (table[index]);
	while(!iter.done ())
	{
		const TAssociation& a = iter.next ();
		if(a.key == key)
			return a.value;
	}
	return error;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMap<TKey, TValue>::get (TValue& value, const TKey& key) const
{
	int index = hashFunc (key, size);
	TIterator iter (table[index]);
	while(!iter.done ())
	{
		const TAssociation& a = iter.next ();
		if(a.key == key)
		{
			value = a.value;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMap<TKey, TValue>::contains (const TKey& key) const
{
	int index = hashFunc (key, size);
	TIterator iter (table[index]);
	while(!iter.done ())
	{
		const TAssociation& a = iter.next ();
		if(a.key == key)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMap<TKey, TValue>::getKey (TKey& key, const TValue& value) const
{
	for(int i = 0; i < size; i++)
	{
		TIterator iter (table[i]);
		while(!iter.done ())
		{
			const TAssociation& a = iter.next ();
			if(a.value == value)
			{
				key = a.key;
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
RangeIterator<HashMap<TKey, TValue>, HashMapIterator<TKey, TValue>, TValue> HashMap<TKey, TValue>::begin () const
{
	return RangeIterator<HashMap<TKey, TValue>, HashMapIterator<TKey, TValue>, TValue> (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
RangeIterator<HashMap<TKey, TValue>, HashMapIterator<TKey, TValue>, TValue> HashMap<TKey, TValue>::end () const
{
	static HashMap<TKey, TValue> dummy (0, hashFunc, error);
	return RangeIterator<HashMap<TKey, TValue>, HashMapIterator<TKey, TValue>, TValue> (dummy);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// HashMapIterator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
HashMapIterator<TKey, TValue>::HashMapIterator (const HashMap<TKey, TValue>& map)
: map (map),
  tableIndex (-1),
  listIter (nullptr)
{
	findNextList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
HashMapIterator<TKey, TValue>::~HashMapIterator ()
{
	delete listIter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
void HashMapIterator<TKey, TValue>::first ()
{
	delete listIter;
	listIter = 0;
	tableIndex = -1;

	findNextList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
void HashMapIterator<TKey, TValue>::last ()
{
	delete listIter;
	listIter = 0;
	tableIndex = map.size;

	findPreviousList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMapIterator<TKey, TValue>::done () const
{
	return !listIter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
const typename HashMapIterator<TKey, TValue>::Association& HashMapIterator<TKey, TValue>::nextAssociation ()
{
	if(listIter)
	{
		ASSERT (!listIter->done ())

		Association& assoc = listIter->next ();

		if(listIter->done ())
		{
			delete listIter;
			listIter = nullptr;

			findNextList ();
		}
		return assoc;
	}
	return error;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
const TValue& HashMapIterator<TKey, TValue>::next ()
{
	return nextAssociation ().value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
const typename HashMapIterator<TKey, TValue>::Association& HashMapIterator<TKey, TValue>::previousAssociation ()
{
	if(listIter)
	{
		ASSERT (!listIter->done ())

		Association& assoc = listIter->previous ();

		if(listIter->done ())
		{
			delete listIter;
			listIter = 0;

			findPreviousList ();
		}
		return assoc;
	}
	return error;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
const TValue& HashMapIterator<TKey, TValue>::previous ()
{
	return previousAssociation ().value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
void HashMapIterator<TKey, TValue>::findNextList ()
{
	while(++tableIndex < map.size)
		if(!map.table[tableIndex].isEmpty ())
		{
			listIter = NEW typename HashMap<TKey, TValue>::TIterator (map.table[tableIndex]);
			ASSERT (!listIter->done ())
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
void HashMapIterator<TKey, TValue>::findPreviousList ()
{
	while(--tableIndex >= 0)
		if(!map.table[tableIndex].isEmpty ())
		{
			listIter = NEW typename HashMap<TKey, TValue>::TIterator (map.table[tableIndex]);
			listIter->last ();
			ASSERT (!listIter->done ())
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
const TValue& HashMapIterator<TKey, TValue>::peekNext ()
{
	if(listIter)
	{
		ASSERT (!listIter->done ())
		Association& assoc = listIter->peekNext ();
		return assoc.value;
	}
	return error.value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMapIterator<TKey, TValue>::operator == (const HashMapIterator<TKey, TValue>& other) const
{ return done () == other.done (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue>
bool HashMapIterator<TKey, TValue>::operator != (const HashMapIterator<TKey, TValue>& other) const
{ return !(*this == other); }

} // namespace Core

#endif // _corehashmap_h
