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
// Filename    : core/public/coredeque.h
// Description : Deque classes
//
//************************************************************************************************

#ifndef _coredeque_h
#define _coredeque_h

#include "core/public/corelinkedlist.h"

namespace Core {

//************************************************************************************************
// Deque
/**	Double-ended queue container class.
	Elements can be added to or removed from both, front and back.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class T>
class Deque: protected LinkedList<T>
{
public:
	/** Check if container is empty. */
	using LinkedList<T>::isEmpty;

	/** Count elements in container. */
	using LinkedList<T>::count;

	/** Add element to front. */
	void addFront (const T& data) { LinkedList<T>::prepend (data); }
	
	/** Add element to back. */
	void addBack (const T& data) { LinkedList<T>::append (data); }
	
	/** Remove element from front. */
	T popFront () { return LinkedList<T>::removeFirst (); }
	
	/** Remove element from back. */
	T popBack () { return LinkedList<T>::removeLast (); }
	
	/** Peek at front-most element. */
	const T& peekFront () const { return LinkedList<T>::getFirst (); }
	
	/** Peek at back-most element. */
	const T& peekBack () const { return LinkedList<T>::getLast (); }
};

//************************************************************************************************
// FixedDeque
/**	Double-ended queue container class with fixed size and externally managed memory.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T>
class FixedDeque
{
public:
	FixedDeque ();
	~FixedDeque ();

	/** Initialize with external memory. */
	void initialize (T* _memoryStart, int _memorySize);	
	
	/** Get capacitiy, i.e. max. number of elements. */
	int getCapacity () const;
	
	/** Count elements in container. */
	int count () const;	
	
	/** Check if container is empty. */
	bool isEmpty () const;
	
	/** Add element to front. */
	bool addFront (const T& newItem);
	
	/** Add element to back. */
	bool addBack (const T& newItem);
	
	/** Remove element from front. */
	bool popFront (T& result);
	
	/** Remove element from back. */
	bool popBack (T& result);
	
	/** Peek at front-most element. */
	bool peekFront (T& result);
	
	/** Peek at back-most element. */
	bool peekBack (T& result);

private:
	T* memoryStart;
	int capacity;
	T* memoryEnd;
	T* firstItem;
	T* lastItem;
	int numItems;

	bool addEmpty (const T& item);
	void incrementPointer (T*& pointer);
	void decrementPointer (T*& pointer);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// FixedDeque implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
FixedDeque<T>::FixedDeque ()
: memoryStart (nullptr),
  capacity (0),
  memoryEnd (nullptr),
  firstItem (nullptr),
  lastItem (nullptr),
  numItems (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
FixedDeque<T>::~FixedDeque ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void FixedDeque<T>::initialize (T* _memoryStart, int _memorySize)
{
	if(_memoryStart == nullptr || _memorySize == 0)
	{
		memoryStart = nullptr;
		capacity = 0;
		memoryEnd = nullptr;
	}
	else
	{
		memoryStart = _memoryStart;
		capacity = _memorySize / sizeof(T);
		memoryEnd = memoryStart + capacity;
	}

	numItems = 0;
	firstItem = nullptr;
	lastItem = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int FixedDeque<T>::getCapacity () const
{
	return capacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int FixedDeque<T>::count () const
{
	return numItems;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::isEmpty () const
{
	return numItems == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::addFront (const T& item)
{
	if(numItems >= capacity)
		return false;
	if(addEmpty (item))
		return true;
	decrementPointer (firstItem);
	*firstItem = item;
	++numItems;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::addBack (const T& item)
{
	if(numItems >= capacity)
		return false;
	if(addEmpty (item))
		return true;
	incrementPointer (lastItem);
	*lastItem = item;
	++numItems;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::addEmpty (const T& item)
{
	if(firstItem == nullptr)
	{
		*memoryStart = item;
		firstItem = memoryStart;
		lastItem = memoryStart;
		numItems = 1;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::popFront (T& result)
{
	if(numItems == 0)
		return false;

	result = *firstItem;
	incrementPointer (firstItem);
	--numItems;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::popBack (T& result)
{
	if(numItems == 0)
		return false;

	result = *lastItem;
	decrementPointer (lastItem);
	--numItems;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::peekFront (T& result)
{
	if(numItems == 0)
		return false;

	result = *firstItem;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool FixedDeque<T>::peekBack (T& result)
{
	if(numItems == 0)
		return false;

	result = *lastItem;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void FixedDeque<T>::incrementPointer (T*& pointer)
{
	++pointer;
	if(pointer == memoryEnd)
		pointer = memoryStart;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void FixedDeque<T>::decrementPointer (T*& pointer)
{
	--pointer;
	if(pointer < memoryStart)
		pointer = memoryEnd - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coredeque_h
