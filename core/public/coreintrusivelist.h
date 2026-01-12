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
// Filename    : core/public/coreintrusivelist.h
// Description : Intrusive Double-Linked List
//
//************************************************************************************************

#ifndef _coreintrusivelist_h
#define _coreintrusivelist_h

#include "core/public/corelinkedlist.h"

namespace Core {

template <class T> class IntrusiveLinkedList;
template <class T> class IntrusiveListIterator;

//************************************************************************************************
// IntrusiveList macros
//************************************************************************************************

/** Iterate trough list (realtime safe, no memory allocation). */
#define IntrusiveListForEach(list, T, var) \
{ Core::IntrusiveListIterator<T> __iter (list); \
  while(!__iter.done ()) { \
   T* var = __iter.next ();

/** Iterate trough list in reverse order (realtime safe, no memory allocations). */
#define IntrusiveListForEachReverse(list, T, var) \
{ Core::IntrusiveListIterator<T> __iter (list); \
  __iter.last (); \
  while(!__iter.done ()) { \
   T* var = __iter.previous ();

//************************************************************************************************
// Specialization of ListLink for IntrusiveLink. 
//************************************************************************************************

class IntrusiveLinkBase {};

template<>
class ListLink<IntrusiveLinkBase*>
{
public:
	ListLink (): next (nullptr), prev (nullptr) {}

protected:
	ListLink (const ListLink&): next (nullptr), prev (nullptr) {}
	
	void releaseLink () { ASSERT (0) } // must not be called!

	friend class LinkedList<IntrusiveLinkBase*>;
	friend class ListIterator<IntrusiveLinkBase*>;

	ListLink* next;
	ListLink* prev;
};

//************************************************************************************************
// IntrusiveLink
/**	Base class for elements in an intrusively double-linked list.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class T>
class IntrusiveLink: public IntrusiveLinkBase,
					 private ListLink<IntrusiveLinkBase*>
{
public:
	/** Get next element in list. */
	T* getNext () const;
	
	/** Get previous element in list. */
	T* getPrevious () const;

protected:
	friend class IntrusiveLinkedList<T>;
	friend class IntrusiveListIterator<T>;

	void setNext (T* next);
	void setPrevious (T* prev);
};

//************************************************************************************************
// IntrusiveLinkedList
/** Intrusively double-linked list container class, i.e. the link pointers are provided by the data itself.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class T>
class IntrusiveLinkedList: private LinkedList<IntrusiveLinkBase*>
{
public:
	/** Append list element. */
	void append (T* link);
	
	/** Prepend list element. */
	void prepend (T* link);
	
	/** Insert before existing element. */
	bool insertBefore (T* existingLink, T* newLink);
	
	/** Insert after existing element. */
	bool insertAfter (T* existingLink, T* newLink);
	
	/** Remove element. */
	bool remove (T* link);
	
	/** Remove all elements. */
	void removeAll ();
	
	/** Remove first element. */
	T* removeFirst ();
	
	/** Remove last element. */
	T* removeLast ();
	
	/** Sort elements in container. */
	void sort ();
	
	/** Sort elements in container with custom predicate. */
	template<class Predicate> void sort (Predicate greater);

	/** Check if container is empty. */
	bool isEmpty () const;
	
	/** Check if container holds more than one element. */
	bool isMultiple () const;
	
	/** Count elements in container. */
	int count () const;
	
	/** Get element at index. */
	T* at (int idx) const;
	
	/** Insert element at index. */
	bool insertAt (int idx, T* newLink);
	
	/** Get first element. */
	T* getFirst () const;

	/** Get last element. */
	T* getLast () const;

	/** Add element sorted, requires T::compare(const T&) method. */
	bool addSorted (T* newLink);

	/** Swap content with other list. */
	void swapContent (IntrusiveLinkedList<T>& other);

	RangeIterator<IntrusiveLinkedList<T>, IntrusiveListIterator<T>, T*> begin () const;
	RangeIterator<IntrusiveLinkedList<T>, IntrusiveListIterator<T>, T*> end () const;

protected:
	friend class IntrusiveListIterator<T>;
};

//************************************************************************************************
// IntrusiveListIterator
/** Intrusively double-linked list iterator.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T>
class IntrusiveListIterator: private ListIterator<IntrusiveLinkBase*>
{
public:
	IntrusiveListIterator (const IntrusiveLinkedList<T>& list);

	/** Check if iteration is done. */
	bool done () const;
	
	/** Seek to first element. */
	void first ();
	
	/** Seek to last element. */
	void last ();
	
	/** Seek and return next element. */
	T* next ();
	
	/** Seek and return previous element. */
	T* previous ();
	
	/** Peek at next element (but don't seek). */
	T* peekNext () const;

	bool operator == (const IntrusiveListIterator& other) const;
	bool operator != (const IntrusiveListIterator& other) const;

protected:
	void setNext (T* next);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// IntrusiveLink implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline T* IntrusiveLink<T>::getNext () const
{ return static_cast<T*> (next); }

template<class T>
inline T* IntrusiveLink<T>::getPrevious () const
{ return static_cast<T*> (prev); }

template<class T>
inline void IntrusiveLink<T>::setNext (T* next)
{ this->next = next; }

template<class T>
inline void IntrusiveLink<T>::setPrevious (T* prev)
{ this->prev = prev; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// IntrusiveLinkedList implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline void IntrusiveLinkedList<T>::append (T* link)
{ ASSERT (link->next == nullptr && link->prev == nullptr)
  LinkedList<IntrusiveLinkBase*>::appendLink (link); }

template<class T>
inline void IntrusiveLinkedList<T>::prepend (T* link)
{ ASSERT (link->next == nullptr && link->prev == nullptr)
  LinkedList<IntrusiveLinkBase*>::prependLink (link); }

template<class T>
inline bool IntrusiveLinkedList<T>::insertBefore (T* existingLink, T* newLink)
{ ASSERT (newLink->next == nullptr && newLink->prev == nullptr)
  LinkedList<IntrusiveLinkBase*>::insertBeforeLink (existingLink, newLink);
  return true; }

template<class T>
inline bool IntrusiveLinkedList<T>::insertAfter (T* existingLink, T* newLink)
{ ASSERT (newLink->next == nullptr && newLink->prev == nullptr)
  LinkedList<IntrusiveLinkBase*>::insertAfterLink (existingLink, newLink);
  return true; }

template<class T>
inline bool IntrusiveLinkedList<T>::remove (T* link)
{ LinkedList<IntrusiveLinkBase*>::removeLink (link);
  return true; }

template<class T>
inline void IntrusiveLinkedList<T>::removeAll ()
{ LinkedList<IntrusiveLinkBase*>::removeAllLinks (); }

template<class T>
inline T* IntrusiveLinkedList<T>::removeFirst ()
{ return static_cast<T*> (LinkedList<IntrusiveLinkBase*>::removeFirstLink ()); }

template<class T>
inline T* IntrusiveLinkedList<T>::removeLast ()
{ return static_cast<T*> (LinkedList<IntrusiveLinkBase*>::removeLastLink ()); }

template<class T>
inline void IntrusiveLinkedList<T>::sort ()
{
	struct Predicate
	{
		bool operator () (const T* t1, const T* t2)
		{
			return (*t1) > (*t2);
		}
	};
	sort (Predicate ());
}

template<class T>
template<class Predicate>
inline void IntrusiveLinkedList<T>::sort (Predicate greater)
{
	struct PredicateOperator
	{
		Predicate greater;

		PredicateOperator (Predicate greater)
		: greater (greater)
		{}

		bool operator () (const ListLink<IntrusiveLinkBase*>* l1, const ListLink<IntrusiveLinkBase*>* l2)
		{
			return greater (static_cast<const T*> (l1), static_cast<const T*> (l2));
		}
	};
	LinkedList::sortInternal (PredicateOperator (greater));
}

template<class T>
inline bool IntrusiveLinkedList<T>::isEmpty () const
{ return LinkedList<IntrusiveLinkBase*>::isEmpty (); }

template<class T>
inline bool IntrusiveLinkedList<T>::isMultiple () const
{ return LinkedList<IntrusiveLinkBase*>::isMultiple (); }

template<class T>
inline int IntrusiveLinkedList<T>::count () const
{ return LinkedList<IntrusiveLinkBase*>::count (); }

template<class T>
inline T* IntrusiveLinkedList<T>::at (int idx) const
{ return static_cast<T*> (LinkedList<IntrusiveLinkBase*>::linkAt (idx)); }

template<class T>
inline bool IntrusiveLinkedList<T>::insertAt (int idx, T* newLink)
{ ASSERT (newLink->next == nullptr && newLink->prev == nullptr)
  if(T* link = (idx >= 0) ? at (idx) : nullptr)
    LinkedList<IntrusiveLinkBase*>::insertBeforeLink (link, newLink);
  else
    LinkedList<IntrusiveLinkBase*>::appendLink (newLink);
  return true; }

template<class T>
inline T* IntrusiveLinkedList<T>::getFirst () const
{ return static_cast<T*> (_head); }

template<class T>
inline T* IntrusiveLinkedList<T>::getLast () const
{ return static_cast<T*> (_tail); }

template<class T>
inline bool IntrusiveLinkedList<T>::addSorted (T* newLink)
{ IntrusiveListForEach (*this, T, link)
    if(link->compare (*newLink) > 0)
	  return insertBefore (link, newLink);
  EndFor
  append (newLink);
  return true; }

template<class T>
inline void IntrusiveLinkedList<T>::swapContent (IntrusiveLinkedList<T>& other)
{ return LinkedList<IntrusiveLinkBase*>::swapContent (other); }

template<class T>
inline RangeIterator<IntrusiveLinkedList<T>, IntrusiveListIterator<T>, T*> IntrusiveLinkedList<T>::begin () const
{ return RangeIterator<IntrusiveLinkedList<T>, IntrusiveListIterator<T>, T*> (*this); }

template<class T>
inline RangeIterator<IntrusiveLinkedList<T>, IntrusiveListIterator<T>, T*> IntrusiveLinkedList<T>::end () const
{ static IntrusiveLinkedList<T> dummy; return RangeIterator<IntrusiveLinkedList<T>, IntrusiveListIterator<T>, T*> (dummy); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// IntrusiveListIterator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline IntrusiveListIterator<T>::IntrusiveListIterator (const IntrusiveLinkedList<T>& list)
: ListIterator<IntrusiveLinkBase*> (list)
{}

template <class T>
inline bool IntrusiveListIterator<T>::done () const
{ return ListIterator<IntrusiveLinkBase*>::done (); }

template <class T>
inline void IntrusiveListIterator<T>::first ()
{ return ListIterator<IntrusiveLinkBase*>::first (); }

template <class T>
inline void IntrusiveListIterator<T>::last ()
{ return ListIterator<IntrusiveLinkBase*>::last (); }

template <class T>
inline T* IntrusiveListIterator<T>::next ()
{ return static_cast<T*> (ListIterator<IntrusiveLinkBase*>::nextLink ()); }

template <class T>
inline T* IntrusiveListIterator<T>::previous ()
{ return static_cast<T*> (ListIterator<IntrusiveLinkBase*>::previousLink ()); }

template <class T>
inline void IntrusiveListIterator<T>::setNext (T* next)
{ _next = next; }

template <class T>
inline bool IntrusiveListIterator<T>::operator == (const IntrusiveListIterator<T>& other) const
{ return ListIterator<IntrusiveLinkBase*>::operator == (other); }

template <class T>
inline bool IntrusiveListIterator<T>::operator != (const IntrusiveListIterator<T>& other) const
{ return ListIterator<IntrusiveLinkBase*>::operator != (other); }

template <class T>
inline T* IntrusiveListIterator<T>::peekNext () const
{ return static_cast<T*> (_next); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coreintrusivelist_h
