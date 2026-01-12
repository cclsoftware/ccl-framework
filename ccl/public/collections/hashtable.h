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
// Filename    : ccl/public/collections/hashtable.h
// Description : Hash table
//
//************************************************************************************************

#ifndef _ccl_hashtable_h
#define _ccl_hashtable_h

#include "ccl/public/collections/linkedlist.h"

namespace CCL {

//************************************************************************************************
// HashTable
/** \ingroup base_collect */
//************************************************************************************************

template<class T, class TList = LinkedList<T> >
class HashTable
{
public:
	typedef int (*HashFunc) (const T& data, int size);

	HashTable (int size, HashFunc hashFunc = hashInt);
	HashTable (const HashTable<T, TList>& other);
	HashTable (HashTable<T, TList>&& other);
	virtual ~HashTable ();

	HashTable<T, TList>& operator= (const HashTable<T, TList>& other);

	void add (T data);
	bool remove (T data);
	void removeAll ();
	
	bool isEmpty () const;
	int count () const;

	const T& lookup (const T& data) const;
	bool contains (const T& data) const;
	
	int getSize () const;
	TList& getList (int index);
	const TList& getList (int index) const;

protected:
	int size;
	TList* table;
	HashFunc hashFunc;
	int total;

	static int hashInt (const T& key, int size) { return ((key % size) + size) % size; };
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// HashTable implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
HashTable<T, TList>::HashTable (int size, HashFunc hashFunc)
: size (size),
  hashFunc (hashFunc),
  table (nullptr),
  total (0)
{
	if(size > 0)
		table = NEW TList[size];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
HashTable<T, TList>::HashTable (const HashTable<T, TList>& other)
: size (other.size),
  hashFunc (other.hashFunc),
  table (nullptr),
  total (other.total)
{
	if(size > 0)
	{
		table = NEW TList[size];
		for(int i = 0; i < other.size; i++)
		{
			ListForEach (other.table[i], T, item)
				table[i].append (item);
			EndFor
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
HashTable<T, TList>::HashTable (HashTable<T, TList>&& other)
: size (other.size),
  hashFunc (other.hashFunc),
  table (other.table),
  total (other.total)
{
	other.size = 0;
	other.table = nullptr;
	other.total = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
HashTable<T, TList>::~HashTable ()
{
	if(table)
		delete [] table;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
HashTable<T, TList>& HashTable<T, TList>::operator= (const HashTable<T, TList>& other)
{
	if(table && (size != other.size))
	{
		delete [] table;
		table = nullptr;
		if(other.size > 0)
			table = NEW TList[other.size];
	}

	size = other.size;
	hashFunc = other.hashFunc;
	if(size > 0)
	{
		for(int i = 0; i < size; i++)
		{
			ListForEach (other.table[i], T, item)
				table[i].append (item);
			EndFor
		}
	}
	total = other.total;

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
void HashTable<T, TList>::add (T data)
{
	int idx = hashFunc (data, size);
	table[idx].append (data);
	total++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
bool HashTable<T, TList>::remove (T data)
{
	int idx = hashFunc (data, size);
	if(!table[idx].remove (data))
		return false;
	total--;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
void HashTable<T, TList>::removeAll ()
{
	for(int i = 0; i < size; i++)
		table[i].removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
bool HashTable<T, TList>::isEmpty () const
{ return total == 0; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
int HashTable<T, TList>::count () const
{ return total; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
const T& HashTable<T, TList>::lookup (const T& data) const
{
	int idx = hashFunc (data, size);
	return table[idx].lookup (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
template<class T, class TList>
bool HashTable<T, TList>::contains (const T& data) const
{
	int idx = hashFunc (data, size);
	return table[idx].contains (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
int HashTable<T, TList>::getSize () const
{
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
TList& HashTable<T, TList>::getList (int idx)
{
	return table[idx];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class TList>
const TList& HashTable<T, TList>::getList (int idx) const
{
	return table[idx];
}
} // namespace CCL

#endif // _ccl_hashtable_h
