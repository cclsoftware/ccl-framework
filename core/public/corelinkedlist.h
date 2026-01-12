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
// Filename    : core/public/corelinkedlist.h
// Description : Double-Linked List
//
//************************************************************************************************

#ifndef _corelinkedlist_h
#define _corelinkedlist_h

#include "core/public/coreplatform.h"
#include "core/public/corecontainer.h"

namespace Core {

template <class T> class LinkedList;
template <class T> class ListIterator;

//************************************************************************************************
// List macros
//************************************************************************************************

/** Iterate trough list (realtime safe, no memory allocations). */
#define ListForEach(list, T, var) \
{ Core::ListIterator<T> __iter (list); \
  while(!__iter.done ()) { \
   T var = __iter.next ();

/** Iterate trough list in reverse order (realtime safe, no memory allocations). */
#define ListForEachReverse(list, T, var) \
{ Core::ListIterator<T> __iter (list); \
  __iter.last (); \
  while(!__iter.done ()) { \
   T var = __iter.previous ();

//************************************************************************************************
// ListLink
/** A list link element.
	\ingroup core_collect
	\ingroup base_collect  */
//************************************************************************************************

template <class T>
class ListLink
{
public:
	ListLink (const T& data);

	/** Get data in element. */
	const T& getData () const;

protected:
	friend class LinkedList<T>;
	friend class ListIterator<T>;

	ListLink (const ListLink&);

	void releaseLink ();

	T data;
	ListLink* next;
	ListLink* prev;
};

//************************************************************************************************
// LinkedList
/** Double-linked list container class.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T>
class LinkedList
{
public:
	LinkedList ();
	virtual ~LinkedList ();

	/** Append list element. */
	void append (const T& data);
	
	/** Prepend list element. */
	void prepend (const T& data);
	
	/** Insert before existing element. */
	bool insertBefore (const T& before, const T& data);
	
	/** Insert before previous element. */
	bool insertBefore (ListIterator<T>& iter, const T& data); 
	
	/** Insert after previous element. */
	bool insertAfter (ListIterator<T>& iter, const T& data);
	
	/** Insert element at index. */
	bool insertAt (int idx, const T& data);
	
	/** Add element sorted. */
	bool addSorted (const T& data);
	
	/** Replace previous element. */	
	bool replace (ListIterator<T>& iter, const T& newData); 
	
	/** Swap content with other list. */
	void swapContent (LinkedList<T>& other);

	/** Remove element. */
	bool remove (const T& data);
	
	/** Remove element at index. */
	bool removeAt (int idx);
	
	/** Remove previous element. */
	bool remove (ListIterator<T>& iter);
	
	/**	Remove elements if condition is met. 
		predicate: (T&) -> bool */
	template <class Predicate> int removeIf (const Predicate& recognize);
	
	/**	Remove elements if condition is met. 
		Use DEFINE_CONTAINER_PREDICATE. */
	int removeIf (ContainerPredicateFunction recognize);
	
	/** Remove all elements. */
	void removeAll ();
	
	/** Remove first element. */
	T removeFirst ();
	
	/** Remove last element. */
	T removeLast ();

	/** Check if container is empty. */
	bool isEmpty () const;

	/** Check if container holds more than one element. */
	bool isMultiple () const;

	/** Count elements in container. */
	int count () const;

	/** Sort elements in container. */
	void sort ();

	/**	Sort elements in container with custom predicate. 
		predicate: (const T& a, const T& b) -> a > b */
	template <class Predicate> void sort (Predicate greater);
	
	/** Get element at index. */
	const T& at (int idx) const;
	
	/** Check if container holds given data. */
	bool contains (const T& data) const;
	
	/** Lookup element with same data. */
	const T& lookup (const T& data) const;
	
	/** Get first element. */
	const T& getFirst () const;
	
	/** Get last element. */
	const T& getLast () const;
	
	/**	Find element based on predicate.
		predicate: (T&) -> bool */
	template <class Predicate> const T& findIf (const Predicate& recognize) const;
	
	/**	Find element based on predicate.
		Use DEFINE_CONTAINER_PREDICATE. */
	const T& findIf (ContainerPredicateFunction recognize) const;

	RangeIterator<LinkedList<T>, ListIterator<T>, T&> begin () const;
	RangeIterator<LinkedList<T>, ListIterator<T>, T&> end () const;

protected:
	friend class ListIterator<T>;
	
	ListLink<T>* _head;
	ListLink<T>* _tail;

	void appendLink (ListLink<T>* newItem);
	void prependLink (ListLink<T>* newItem);
	void insertBeforeLink (ListLink<T>* item, ListLink<T>* newItem);
	void insertAfterLink (ListLink<T>* item, ListLink<T>* newItem);
	void removeLink (ListLink<T>* item);
	void removeAllLinks ();
	ListLink<T>* removeFirstLink ();
	ListLink<T>* removeLastLink ();
	ListLink<T>* linkAt (int idx) const;
	ListLink<T>* lookupLink (const T& data) const;
	template <class Predicate> void sortInternal (Predicate greater);
	
	static T& error ();
};

//************************************************************************************************
// ListIterator
/** List iterator.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T>
class ListIterator
{
public:
	ListIterator (const LinkedList<T>& list);

	/** Check if iteration is done. */
	bool done () const;
	
	/** Seek to first element. */
	void first ();
	
	/** Seek to last element. */
	void last ();
	
	/** Seek and return next element. */
	T& next ();
	
	/** Seek and return previous element. */
	T& previous ();
	
	/** Peek at next element (but don't seek). */
	T& peekNext () const;

	bool operator == (const ListIterator& other) const;
	bool operator != (const ListIterator& other) const;

protected:
	friend class LinkedList<T>;
	const LinkedList<T>& list;
	ListLink<T>* _next;

	ListLink<T>* nextLink ();
	ListLink<T>* previousLink ();
	static T& error ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListIterator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListIterator<T>::ListIterator (const LinkedList<T>& list)
: list (list),
  _next (list._head)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void ListIterator<T>::first ()
{
	_next = list._head;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void ListIterator<T>::last ()
{
	_next = list._tail;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>* ListIterator<T>::nextLink ()
{
	ListLink<T>* _item = _next;
	_next = _item ? _item->next : nullptr;
	return _item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>* ListIterator<T>::previousLink ()
{
	ListLink<T>* _item = _next;
	_next = _item ? _item->prev : nullptr;
	return _item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& ListIterator<T>::next ()
{
	ListLink<T>* _item = _next;
	if(_item)
	{
		_next = _item->next;
		return _item->data;
	}
	else
		return error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& ListIterator<T>::previous ()
{
	ListLink<T>* _item = _next;
	if(_item)
	{
		_next = _item->prev;
		return _item->data;
	}
	else
		return error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& ListIterator<T>::peekNext () const
{
	return _next ? _next->data : error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ListIterator<T>::done () const
{
	return _next == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline T& ListIterator<T>::error ()
{
	static T e;
	return e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ListIterator<T>::operator == (const ListIterator<T>& other) const
{
	return _next == other._next;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ListIterator<T>::operator != (const ListIterator<T>& other) const
{
	return _next != other._next;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListLink implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>::ListLink (const T& d)
: data (d),
  next (nullptr),
  prev (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>::ListLink (const ListLink& other)
: data (other.data),
  next (0),
  prev (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
const T& ListLink<T>::getData () const
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void ListLink<T>::releaseLink ()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// LinkedList implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
LinkedList<T>::LinkedList ()
: _head (nullptr),
  _tail (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
LinkedList<T>::~LinkedList ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::append (const T& data)
{
	appendLink (NEW ListLink<T> (data));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::appendLink (ListLink<T>* newItem)
{
	if(!_head)
		_head = _tail = newItem;
	else
	{
		ListLink<T>* last = _tail;
		last->next = newItem;
		newItem->prev = last;
		_tail = newItem;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::prepend (const T& data)
{
	prependLink (NEW ListLink<T> (data));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::prependLink (ListLink<T>* newItem)
{
	ListLink<T>* oldHead = _head;
	_head = newItem;
	_head->next = oldHead;
	if(oldHead)
		oldHead->prev = _head;

	if(!_tail) // if list was empty before
		_tail = _head;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::insertBefore (const T& before, const T& data)
{
	for(ListLink<T>* item = _head; item; item = item->next)
		if(item->data == before)
		{
			insertBeforeLink (item, NEW ListLink<T> (data));
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::insertBeforeLink (ListLink<T>* item, ListLink<T>* newItem)
{
	if(item->prev)
	{
		item->prev->next = newItem;
		newItem->prev = item->prev;
	}
	else
	{
		ASSERT (item == _head)
		_head = newItem;
	}

	item->prev = newItem;
	newItem->next = item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::insertAfterLink (ListLink<T>* item, ListLink<T>* newItem)
{
	if(item->next)
	{
		item->next->prev = newItem;
		newItem->next = item->next;
	}
	else
	{
		ASSERT (item == _tail)
		_tail = newItem;
	}

	item->next = newItem;
	newItem->prev = item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::insertAt (int idx, const T& data)
{
	ListLink<T>* item = (idx >= 0) ? linkAt (idx) : nullptr;
	if(item)
		insertBeforeLink (item, NEW ListLink<T> (data));
	else
		appendLink (NEW ListLink<T> (data));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::addSorted (const T& data)
{
	for(ListLink<T>* item = _head; item; item = item->next)
		if(item->data > data)
		{
			insertBeforeLink (item, NEW ListLink<T> (data));
			return true;
		}

	append (data);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::insertBefore (ListIterator<T>& iter, const T& data)
{
	ListLink<T>* link = nullptr;
	if(iter._next)
		link = iter._next->prev;
	else
		link = iter.list._tail;

	if(link)
	{
		insertBeforeLink (link, NEW ListLink<T> (data));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::insertAfter (ListIterator<T>& iter, const T& data)
{
	if(iter._next)
		insertBeforeLink (iter._next, NEW ListLink<T> (data));
	else
		appendLink (NEW ListLink<T> (data));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::replace (ListIterator<T>& iter, const T& newData)
{
	ListLink<T>* link = nullptr;
	if(iter._next)
		link = iter._next->prev;
	else
		link = iter.list._tail;

	if(link)
	{
		link->data = newData;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::swapContent (LinkedList<T>& other)
{
	swap_vars (_head, other._head);
	swap_vars (_tail, other._tail);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::remove (const T& data)
{
	for(ListLink<T>* item = _head; item; item = item->next)
		if(item->data == data)
		{
			removeLink (item);
			item->releaseLink ();
			return true;
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::removeAt (int idx)
{
	ListLink<T>* item = linkAt (idx);
	if(item)
	{
		removeLink (item);
		item->releaseLink ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Predicate>
int LinkedList<T>::removeIf (const Predicate& recognize)
{
	int removed = 0;
	ListIterator<T> iter (*this);
	while(!iter.done ())
		if(recognize (iter.next ()))
			if(remove (iter))
				removed++;

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int LinkedList<T>::removeIf (ContainerPredicateFunction recognize)
{
	int removed = 0;
	ListIterator<T> iter (*this);
	while(!iter.done ())
		if(recognize (&iter.next ()))
			if(remove (iter))
				removed++;

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::remove (ListIterator<T>& iter)
{
	ListLink<T>* del = nullptr;
	if(iter._next)
		del = iter._next->prev;
	else
		del = iter.list._tail;

	if(del)
	{
		removeLink (del);
		del->releaseLink ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::removeLink (ListLink<T>* item)
{
	if(item->prev)
		item->prev->next = item->next;
	else
	{
		ASSERT (item == _head)
		_head = item->next;
	}

	if(item->next)
		item->next->prev = item->prev;
	else
	{
		ASSERT (item == _tail)
		_tail = item->prev;
	}

	item->prev = item->next = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::removeAll ()
{
	ListLink<T>* item = _head;
	while(item)
	{
		ListLink<T>* next = item->next;
		item->releaseLink ();
		item = next;
	}

	removeAllLinks ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::removeAllLinks ()
{
	_head = nullptr;
	_tail = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T LinkedList<T>::removeFirst ()
{
	ListLink<T>* first = removeFirstLink ();
	if(first)
	{
		T data = first->data;
		first->releaseLink ();
		return data;
	}
	return T ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>* LinkedList<T>::removeFirstLink ()
{
	ListLink<T>* first = _head;
	if(first)
	{
		_head = _head->next;
		if(_head)
			_head->prev = nullptr;
		else
			_tail = nullptr; // list is empty now

		first->next = first->prev = nullptr; // unlink
	}
	return first;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T LinkedList<T>::removeLast ()
{
	ListLink<T>* last = removeLastLink ();
	if(last)
	{
		T data = last->data;
		last->releaseLink ();
		return data;
	}
	return T ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>* LinkedList<T>::removeLastLink ()
{
	ListLink<T>* last = _tail;
	if(last)
	{
		_tail = _tail->prev;
		if(_tail)
			_tail->next = nullptr;
		else
			_head = nullptr; // list is empty now

		last->next = last->prev = nullptr; // unlink
	}
	return last;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::isEmpty () const
{
	return _head == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::isMultiple () const
{
	return _head != _tail; // otherwise: both 0 (empty) or same element (1)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int LinkedList<T>::count () const
{
	int count = 0;
	for(ListLink<T>* item = _head; item; item = item->next)
		count++;
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void LinkedList<T>::sort ()
{
	struct Predicate
	{
		bool operator () (const T& t1, const T& t2)
		{
			return t1 > t2;
		}
	};
	sort (Predicate ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Predicate>
void LinkedList<T>::sort (Predicate greater)
{
	struct PredicateOperator
	{
		Predicate greater;

		PredicateOperator (Predicate greater)
		: greater (greater)
		{}

		bool operator () (const ListLink<T>* t1, const ListLink<T>* t2)
		{
			return greater (t1->getData (), t2->getData ());
		}
	};
	sortInternal (PredicateOperator (greater));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Predicate>
void LinkedList<T>::sortInternal (Predicate greater)
{
	int numLinks = count ();
	if(numLinks < 2)
		return;

	// Divide the list into two sublists
	int halfCount = numLinks / 2;
	LinkedList<T> sub1;
	sub1._head = _head;
	sub1._tail = linkAt (halfCount - 1);
	LinkedList<T> sub2;
	sub2._head = linkAt (halfCount);
	sub2._tail = _tail;

	// Make sure to cut the links between the two sublists
	sub1._tail->next = 0;
	sub2._head->prev = 0;

	sub1.sortInternal (greater);
	sub2.sortInternal (greater);

	// Merge the sublists back together
	ListLink<T>* subLink1 = sub1._head;
	ListLink<T>* subLink2 = sub2._head;
	if(greater (subLink1, subLink2))
	{
		_head = subLink2;
		subLink2 = subLink2->next;
	}
	else
	{
		_head = subLink1;
		subLink1 = subLink1->next;
	}

	ListLink<T>* currentLink = _head;
	while(subLink1 || subLink2)
	{
		if(subLink1 == 0 || (subLink2 && greater (subLink1, subLink2)))
		{
			currentLink->next = subLink2;
			subLink2->prev = currentLink;
			currentLink = subLink2;
			subLink2 = subLink2->next;
		}
		else
		{
			currentLink->next = subLink1;
			subLink1->prev = currentLink;
			currentLink = subLink1;
			subLink1 = subLink1->next;
		}
	}

	_tail = currentLink;
	_tail->next = 0;

	// Make sure to remove the remaining links in the sublist objects to prevent
	// releasing the links when the sublist destructors are called
	sub1._head = 0;
	sub1._tail = 0;
	sub2._head = 0;
	sub2._tail = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
const T& LinkedList<T>::at (int idx) const
{
	ListLink<T>* item = linkAt (idx);
	if(item)
		return item->data;
	return error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>* LinkedList<T>::linkAt (int idx) const
{
	int i = 0;
	for(ListLink<T>* item = _head; item; item = item->next, i++)
		if(i == idx)
			return item;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool LinkedList<T>::contains (const T& data) const
{
	for(ListLink<T>* item = _head; item; item = item->next)
		if(item->data == data)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
const T& LinkedList<T>::lookup (const T& data) const
{
	ListLink<T>* item = lookupLink (data);
	if(item)
		return item->data;
	return error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ListLink<T>* LinkedList<T>::lookupLink (const T& data) const
{
	for(ListLink<T>* item = _head; item; item = item->next)
		if(item->data == data)
			return item;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline const T& LinkedList<T>::getFirst () const
{
	return _head ? _head->data : error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline const T& LinkedList<T>::getLast () const
{
	ListLink<T>* last = _tail;
	return last ? last->data : error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Predicate>
const T& LinkedList<T>::findIf (const Predicate& recognize) const
{
	for(ListLink<T>* item = _head; item; item = item->next)
		if(recognize (item->data))
			return item->data;

	return error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
const T& LinkedList<T>::findIf (ContainerPredicateFunction recognize) const
{
	for(ListLink<T>* item = _head; item; item = item->next)
		if(recognize (&item->data))
			return item->data;

	return error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
RangeIterator<LinkedList<T>, ListIterator<T>, T&> LinkedList<T>::begin () const
{
	return RangeIterator<LinkedList<T>, ListIterator<T>, T&> (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
RangeIterator<LinkedList<T>, ListIterator<T>, T&> LinkedList<T>::end () const
{
	static LinkedList<T> dummy;
	return RangeIterator<LinkedList<T>, ListIterator<T>, T&> (dummy);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline T& LinkedList<T>::error ()
{
	static T e;
	return e;
}

} // namespace Core

#endif // _corelinkedlist_h
