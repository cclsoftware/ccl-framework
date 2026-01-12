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
// Filename    : ccl/base/collections/objectarray.h
// Description : Object Array
//
//************************************************************************************************

#ifndef _ccl_objectarray_h
#define _ccl_objectarray_h

#include "ccl/base/collections/container.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

class ObjectArrayIterator;

#if !DOXYGEN

//************************************************************************************************
// ObjectArray macros
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
// ArrayForEach : iterate thru array (realtime safe, no memory allocation!)
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ArrayForEach(list, Class, var) VectorForEach (list, Class*, var)

//////////////////////////////////////////////////////////////////////////////////////////////////
// ArrayForEachFast : iterate thru array without checking index (uses pointer arithmetic, doesn't call at ())
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ArrayForEachFast(list, Class, var) \
{ Object** __ptr = (list).getItems (); for(int __iter = (list).count (); __iter != 0; __iter--) { \
    Class* var = (Class*)*__ptr++;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ArrayForEachReverse : iterate thru array in reverse order (realtime safe!)
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ArrayForEachReverse(list, Class, var) VectorForEachReverse (list, Class*, var)

//////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINE_ARRAY_COMPARE : define compare function for two objects in an array
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_ARRAY_COMPARE(FunctionName, Type, lhs, rhs) \
	DEFINE_VECTOR_COMPARE (FunctionName, Type, lhs, rhs)

//////////////////////////////////////////////////////////////////////////////////////////////////
// LAMBDA_ARRAY_COMPARE : define lambda compare function for two objects in an array
//////////////////////////////////////////////////////////////////////////////////////////////////

#define LAMBDA_ARRAY_COMPARE(Type, lhs, rhs) \
	LAMBDA_VECTOR_COMPARE (Type, lhs, rhs)

#endif

//************************************************************************************************
// ObjectArray
/** Container class for array of objects. \ingroup base_collect */
//************************************************************************************************

class ObjectArray:	public Container,
					public Vector<Object*>
{
public:
	DECLARE_CLASS (ObjectArray, Container)

	ObjectArray (int capacity = 0, int delta = 10);
	explicit ObjectArray (const InitializerList<Object*>& list);
	explicit ObjectArray (const Container&);
	explicit ObjectArray (const ObjectArray&);
	~ObjectArray ();

	void sort ();

	typedef Core::VectorCompareFunction CompareFunction;
	void sort (CompareFunction function); ///< use DEFINE_ARRAY_COMPARE for function
	bool addSorted (Object* obj, CompareFunction function, bool reversed = false);

	Object* search (const Object& obj) const;
	int searchIndex (const Object& obj) const;

	bool insertAt (int index, Object* obj);
	bool replaceAt (int index, Object* obj);

	/** Returns index for object to be inserted in a sorted array (binary search); item at index might be a "duplicate" of object (according to CompareFunction). */
	int getInsertIndex (const Object* object, CompareFunction function) const;
	int getInsertIndex (const Object* object) const;

	// Container
	using Container::add;
	using Container::addOnce;
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

	Object* operator[] (int idx) const;

	template<class Predicate> int removeIf (const Predicate& recognize);
	template<class Class, class Predicate> int removeIf (const Predicate& recognize);

	template<class Element = Object> RangeIterator<ObjectArray, ObjectArrayIterator, Element*> begin () const;
	template<class Element = Object> RangeIterator<ObjectArray, ObjectArrayIterator, Element*> end () const;
	using IteratorClass = ObjectArrayIterator;

protected:
	static int compareObjects (const void*, const void*);
};

//************************************************************************************************
// VectorIteratorAdapter
//************************************************************************************************

template<class T, class Element>
class VectorIteratorAdapter: public Iterator,
							 public VectorIterator<Element*>
{
public:
	VectorIteratorAdapter (const T& array);

	// Iterator
	tbool CCL_API done () const override;
	void first () override;
	void last () override;
	Object* next () override;
	Object* previous () override;
};

//************************************************************************************************
// ObjectArrayIterator
//************************************************************************************************

class ObjectArrayIterator: public VectorIteratorAdapter<ObjectArray, Object>
{
public:
	ObjectArrayIterator (const ObjectArray& array);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObjectArray inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ObjectArray::ObjectArray (int capacity, int delta)
: Vector<Object*> (capacity, delta) {}

inline ObjectArray::ObjectArray (const InitializerList<Object*>& list)
: Vector<Object*> (list) {}

inline ObjectArray::ObjectArray (const Container& objects)
{ copyFrom (objects); }

inline ObjectArray::ObjectArray (const ObjectArray& objects)
{ copyFrom (objects); }

inline ObjectArray::~ObjectArray ()
{ removeAll (); }

inline Iterator* ObjectArray::newIterator () const
{ return NEW ObjectArrayIterator (*this); }

inline bool ObjectArray::isEmpty () const
{ return Vector<Object*>::isEmpty (); }

inline int ObjectArray::count () const
{ return Vector<Object*>::count (); }

inline Object* ObjectArray::at (int idx) const
{ return (idx >= 0 && idx < total) ? items[idx] : nullptr ; }

inline Object* ObjectArray::operator[] (int idx) const
{ return items[idx]; }

inline bool ObjectArray::add (Object* obj)
{ Vector<Object*>::add (obj); return true; }

inline bool ObjectArray::insertAt (int index, Object* obj)
{ return Vector<Object*>::insertAt (index, obj); }

inline bool ObjectArray::remove (Object* obj)
{ return Vector<Object*>::remove (obj); }

inline int ObjectArray::getInsertIndex (const Object* object) const
{ return getInsertIndex (object, compareObjects); }

template<class Element>
inline RangeIterator<ObjectArray, ObjectArrayIterator, Element*> ObjectArray::begin () const
{ return RangeIterator<ObjectArray, ObjectArrayIterator, Element*> (*this); }

template<class Element>
inline RangeIterator<ObjectArray, ObjectArrayIterator, Element*> ObjectArray::end () const
{ static ObjectArray dummy; return RangeIterator<ObjectArray, ObjectArrayIterator, Element*> (dummy); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Predicate>
inline int ObjectArray::removeIf (const Predicate& recognize)
{
	int removed = 0;
	for(int i = count () - 1; i >= 0; i--)
	{
		Object* obj = at (i);
		if(obj && recognize (obj))
		{
			if(removeAt (i))
			{
				if(isObjectCleanup ())
					obj->release ();

				removed++;
			}
		}
	}

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Predicate>
inline int ObjectArray::removeIf (const Predicate& recognize)
{
	int removed = 0;
	for(int i = count () - 1; i >= 0; i--)
	{
		T* t = ccl_cast<T> (at (i));
		if(t && recognize (*t))
		{
			if(removeAt (i))
			{
				if(isObjectCleanup ())
					t->release ();

				removed++;
			}
		}
	}

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// VectorIteratorAdapter
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Element>
VectorIteratorAdapter<T, Element>::VectorIteratorAdapter (const T& array)
: VectorIterator<Element*> (array) {}

template<class T, class Element>
tbool CCL_API VectorIteratorAdapter<T, Element>::done () const
{ return VectorIterator<Element*>::done (); }

template<class T, class Element>
void VectorIteratorAdapter<T, Element>::first ()
{ VectorIterator<Element*>::first (); }

template<class T, class Element>
void VectorIteratorAdapter<T, Element>::last ()
{ VectorIterator<Element*>::last (); }

template<class T, class Element>
Object* VectorIteratorAdapter<T, Element>::next ()
{ int idx = this->_index++; return idx < this->items.count () ? this->items[idx] : nullptr; }

template<class T, class Element>
Object* VectorIteratorAdapter<T, Element>::previous ()
{ int idx = this->_index--; return idx >= 0 ? this->items[idx] : nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObjectArrayIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ObjectArrayIterator::ObjectArrayIterator (const ObjectArray& array)
: VectorIteratorAdapter<ObjectArray, Object> (array) {}

} // namespace CCL

#endif // _ccl_objectarray_h
