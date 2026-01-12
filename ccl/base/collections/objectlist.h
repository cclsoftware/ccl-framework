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
// Filename    : ccl/base/collections/objectlist.h
// Description : Object List
//
//************************************************************************************************

#ifndef _ccl_objectlist_h
#define _ccl_objectlist_h

#include "ccl/base/collections/container.h"

#include "ccl/public/collections/linkedlist.h"

namespace CCL {

class ObjectListIterator;

#if !DOXYGEN

//************************************************************************************************
// ObjectList macros
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListForEachObject : iterate thru list (realtime safe, no memory allocation!)
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ListForEachObject(list, Class, var) \
{ CCL::ObjectListIterator __iter (list); \
  while(Class* var = (Class*)__iter.next ()) {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListForEachObjectReverse : iterate thru list in reverse order (realtime safe!)
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ListForEachObjectReverse(list, Class, var) \
{ CCL::ObjectListIterator __iter (list); \
  __iter.last (); \
  while(Class* var = (Class*)__iter.previous ()) {


#endif
//************************************************************************************************
// ObjectList
/** Double-linked object list \ingroup base_collect  */
//************************************************************************************************

class ObjectList: public Container,
				  public LinkedList<Object*>
{
public:
	DECLARE_CLASS (ObjectList, Container)

	ObjectList ();
	explicit ObjectList (const Container&);
	explicit ObjectList (const ObjectList&);
	~ObjectList ();

	// Container
	using Container::add;
	using Container::contains;
	using Container::findIf;
	using Container::removeIf;
	Iterator* newIterator () const override;
	bool isEmpty () const override;
	int count () const override;
	Object* at (int idx) const override;
	int index (const Object& obj) const override;
	int index (const Object* obj) const override;
	bool add (Object* obj) override;
	bool remove (Object* obj) override;
	void removeAll () override;
	Object* findEqual (const Object& obj) const override;
	bool addSorted (Object* obj) override;

	bool insertAt (int idx, Object* const& obj);
	bool remove (ListIterator<Object*>& iter); // remove previous

	template<class Predicate> int removeIf (const Predicate& recognize);
	template<class Class, class Predicate> int removeIf (const Predicate& recognize);

	bool addDuringIteration (Object* obj); // add object while list is being iterated, so that it can participate in the iteration

	template<class Element = Object> RangeIterator<ObjectList, ObjectListIterator, Element*> begin () const;
	template<class Element = Object> RangeIterator<ObjectList, ObjectListIterator, Element*> end () const;
	using IteratorClass = ObjectListIterator;

protected:
	friend class ObjectListIterator;
	ObjectListIterator* iterator;
};

//************************************************************************************************
// ObjectListIterator
/** Object list iterator */
//************************************************************************************************

class ObjectListIterator: public Iterator,
						  public ListIterator<Object*>
{
public:
	ObjectListIterator (const ObjectList& list);
	~ObjectListIterator ();

	void remove (Object* obj);
	void removeAll ();

	// Iterator
	tbool CCL_API done () const override;
	void first () override;
	void last () override;
	Object* next () override;
	Object* previous () override;

protected:
	friend class ObjectList;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObjectList inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ObjectList::ObjectList ()
: iterator (nullptr) {}

inline ObjectList::ObjectList (const Container& objects)
: iterator (nullptr) { copyFrom (objects); }

inline ObjectList::ObjectList (const ObjectList& objects)
: iterator (nullptr) { copyFrom (objects); }

inline ObjectList::~ObjectList () { /*ASSERT (iterator == 0)*/ removeAll (); }

inline Iterator* ObjectList::newIterator () const
{ return NEW ObjectListIterator (*this); }

inline bool ObjectList::isEmpty () const
{ return LinkedList<Object*>::isEmpty (); }

inline int ObjectList::count () const
{ return LinkedList<Object*>::count (); }

inline Object* ObjectList::at (int idx) const
{ return LinkedList<Object*>::at (idx); }

inline bool ObjectList::add (Object* obj)
{ LinkedList<Object*>::append (obj); return true; }

inline bool ObjectList::insertAt (int idx, Object* const& obj)
{ return LinkedList<Object*>::insertAt (idx, obj); }

inline bool ObjectList::remove (Object* obj)
{ if(iterator) iterator->remove (obj); return LinkedList<Object*>::remove (obj); }

inline bool ObjectList::remove (ListIterator<Object*>& iter)
{ return LinkedList<Object*>::remove (iter); }

template<class Element>
inline RangeIterator<ObjectList, ObjectListIterator, Element*> ObjectList::begin () const
{ return RangeIterator<ObjectList, ObjectListIterator, Element*> (*this); }

template<class Element>
inline RangeIterator<ObjectList, ObjectListIterator, Element*> ObjectList::end () const
{ static ObjectList dummy; return RangeIterator<ObjectList, ObjectListIterator, Element*> (dummy); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Predicate>
inline int ObjectList::removeIf (const Predicate& recognize)
{
	int removed = 0;
	ObjectListIterator iter (*this);
	while(!iter.done ())
	{
		Object* obj = iter.next ();
		if(recognize (obj))
			if(remove (iter))
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
inline int ObjectList::removeIf (const Predicate& recognize)
{
	int removed = 0;
	ObjectListIterator iter (*this);
	while(!iter.done ())
	{
		T* t = ccl_cast<T> (iter.next ());
		if(t && recognize (*t))
			if(remove (iter))
			{
				if(isObjectCleanup ())
					t->release ();

				removed++;
			}
	}

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObjectListIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API ObjectListIterator::done () const
{ return ListIterator<Object*>::done (); }

inline void ObjectListIterator::first ()
{ ListIterator<Object*>::first (); }

inline void ObjectListIterator::last ()
{ ListIterator<Object*>::last (); }

inline Object* ObjectListIterator::next ()
{ return ListIterator<Object*>::next (); }

inline Object* ObjectListIterator::previous ()
{ return ListIterator<Object*>::previous (); }

inline void ObjectListIterator::remove (Object* obj)
{ if(_next && obj == _next->getData ()) next (); }

inline void ObjectListIterator::removeAll ()
{ _next = nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_objectlist_h
