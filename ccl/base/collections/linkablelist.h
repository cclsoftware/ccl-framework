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
// Filename    : ccl/base/collections/linkablelist.h
// Description : Linkable Object List
//
//************************************************************************************************

#ifndef _ccl_linkablelist_h
#define _ccl_linkablelist_h

#include "ccl/base/collections/container.h"

#include "ccl/public/collections/intrusivelist.h"

namespace CCL {

class LinkableListIterator;

//************************************************************************************************
// LinkableList macros
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListForEachLinkable : iterate thru list (realtime safe, no memory allocation!)
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ListForEachLinkable(list, Class, var) \
{ CCL::LinkableListIterator __iter (list); \
  while(Class* var = (Class*)__iter.next ()) {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListForEachLinkableReverse : iterate thru list in reverse order (realtime safe!)
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ListForEachLinkableReverse(list, Class, var) \
{ CCL::LinkableListIterator __iter (list); \
  __iter.last (); \
  while(Class* var = (Class*)__iter.previous ()) {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListForEachLinkableFast : iterate thru list (realtime safe, no memory allocation!)
// - faster than ListForEachLinkable
// - but does not allow to remove the current link during iteration!
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ListForEachLinkableFast(list, Class, var) \
{ CCL::FastLinkableListIterator __iter (list); \
  while(Class* var = (Class*)__iter.next ()) {

#define ListForEachLinkableFastReverse(list, Class, var) \
{ CCL::FastLinkableListIterator __iter (list); \
  while(Class* var = (Class*)__iter.previous ()) {

//************************************************************************************************
// Linkable
/** Directly linkable object. */
//************************************************************************************************

class Linkable: public Object,
				public IntrusiveLink<Linkable>
{
public:
	DECLARE_CLASS (Linkable, Object)

protected:
	friend class LinkableList;
	friend class LinkableListIterator;
	friend class FastLinkableListIterator;
};

//************************************************************************************************
// LinkableList
/** Container class for directly linkable objects. \ingroup base_collect */
//************************************************************************************************

class LinkableList: public Container,
					public IntrusiveLinkedList<Linkable>
{
public:
	LinkableList ();
	explicit LinkableList (const Container&);
	explicit LinkableList (const LinkableList&);
	~LinkableList ();

	bool mightContain (const Linkable& link) const;
	///< a fast check: false: link is definitely not in list; true: it might be in list (not sure)

	// Container
	using Container::add;
	using Container::findIf;
	using Container::removeIf;
	using Container::contains;
	void swapContent (LinkableList& other);

	Iterator* newIterator () const override;
	bool isEmpty () const override;
	int count () const override;
	Object* at (int idx) const override;
	int index (const Object& obj) const override;
	int index (const Object* obj) const override;
	bool add (Object* obj) override;
	bool remove (Object* obj) override;
	template<class Predicate> int removeIf (const Predicate& recognize);
	template<class Class, class Predicate> int removeIf (const Predicate& recognize);
	void removeAll () override;
	Object* findEqual (const Object& obj) const override;
	bool addSorted (Object* obj) override;

	template<class Element = Object> RangeIterator<LinkableList, LinkableListIterator, Element*> begin () const;
	template<class Element = Object> RangeIterator<LinkableList, LinkableListIterator, Element*> end () const;
	using IteratorClass = LinkableListIterator;

protected:
	friend class LinkableListIterator;
	friend class FastLinkableListIterator;
};

//************************************************************************************************
// LinkableListIterator
//************************************************************************************************

class LinkableListIterator: public Iterator,
							public IntrusiveListIterator<Linkable>
{
public:
	LinkableListIterator (const LinkableList& list);

	tbool CCL_API done () const override;
	void first () override;
	void last () override;
	Object* next () override;
	Object* previous () override;
};

//************************************************************************************************
// FastLinkableListIterator
/**
	- Faster than LinkableListIterator, but not compatible to the Iterator interface.
	- Does not allow to remove objects during iteration!!!
	- Usage: call next () or previous () until it returns 0.
*/
//************************************************************************************************

class FastLinkableListIterator
{
public:
	FastLinkableListIterator (const LinkableList& list);
	FastLinkableListIterator (Linkable& startLinkable);	///< start iteration with this linkable

	void first ();

	Object* next ();
	Object* previous ();

private:
	Linkable _beyond;
	Linkable* _current;

	Linkable* forwardLinkable;
	Linkable* backwardLinkable;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// LinkableList implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline LinkableList::LinkableList () {}

inline LinkableList::LinkableList (const Container& objects)
{ copyFrom (objects); }

inline LinkableList::LinkableList (const LinkableList& objects)
{ copyFrom (objects); }

inline LinkableList::~LinkableList ()
{ removeAll (); }

inline Iterator* LinkableList::newIterator () const
{ return NEW LinkableListIterator (*this); }

inline bool LinkableList::isEmpty () const
{ return IntrusiveLinkedList<Linkable>::isEmpty (); }

inline int LinkableList::count () const
{ return IntrusiveLinkedList<Linkable>::count (); }

inline Object* LinkableList::at (int idx) const
{ return IntrusiveLinkedList<Linkable>::at (idx); }

inline bool LinkableList::mightContain (const Linkable& link) const
{ return link.getPrevious () != nullptr || link.getNext () != nullptr || getFirst () == &link; } // link has a prev or next or is the only element

inline void LinkableList::swapContent (LinkableList& other)
{ IntrusiveLinkedList<Linkable>::swapContent (other); }

template<class Element>
inline RangeIterator<LinkableList, LinkableListIterator, Element*> LinkableList::begin () const
{ return RangeIterator<LinkableList, LinkableListIterator, Element*> (*this); }

template<class Element>
inline RangeIterator<LinkableList, LinkableListIterator, Element*> LinkableList::end () const
{ static LinkableList dummy; return RangeIterator<LinkableList, LinkableListIterator, Element*> (dummy); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Predicate>
inline int LinkableList::removeIf (const Predicate& recognize)
{
	int removed = 0;
	LinkableListIterator iter (*this);
	while(!iter.done ())
	{
		Object* obj = iter.next ();
		if(recognize (obj))
			if(remove (obj))
			{
				if(isObjectCleanup ())
					obj->release ();

				removed++;
			}
	}

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Predicate>
inline int LinkableList::removeIf (const Predicate& recognize)
{
	int removed = 0;
	LinkableListIterator iter (*this);
	while(!iter.done ())
	{
		T* t = ccl_cast<T> (iter.next ());
		if(t && recognize (*t))
			if(remove (t))
			{
				if(isObjectCleanup ())
					t->release ();

				removed++;
			}
	}

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// LinkableListIterator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline LinkableListIterator::LinkableListIterator (const LinkableList& list)
: IntrusiveListIterator<Linkable> (list) {}

inline tbool CCL_API LinkableListIterator::done () const
{ return IntrusiveListIterator<Linkable>::done (); }

inline void LinkableListIterator::first ()
{ IntrusiveListIterator<Linkable>::first (); }

inline void LinkableListIterator::last ()
{ IntrusiveListIterator<Linkable>::last (); }

inline Object* LinkableListIterator::next ()
{ return IntrusiveListIterator<Linkable>::next (); }

inline Object* LinkableListIterator::previous ()
{ return IntrusiveListIterator<Linkable>::previous (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// FastLinkableListIterator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline FastLinkableListIterator::FastLinkableListIterator (const LinkableList& list)
: forwardLinkable (list.getFirst ()),
  backwardLinkable (list.getLast ())
{
	first ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FastLinkableListIterator::FastLinkableListIterator (Linkable& startLinkable)
: forwardLinkable (&startLinkable),
  backwardLinkable (&startLinkable)
{
	first ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void FastLinkableListIterator::first ()
{
	// setup a dummy Linkable that will lead to the first or last element
	_beyond.setNext (forwardLinkable);		// for going forward
	_beyond.setPrevious (backwardLinkable);	// for going backwards
	_current = &_beyond;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Object* FastLinkableListIterator::next ()
{
	if(_current)
		_current = _current->getNext ();
	return _current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Object* FastLinkableListIterator::previous ()
{
	if(_current)
		_current = _current->getPrevious ();
	return _current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_linkablelist_h
