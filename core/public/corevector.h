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
// Filename    : core/public/corevector.h
// Description : Vector class
//
//************************************************************************************************

#ifndef _corevector_h
#define _corevector_h

#include "core/public/coreplatform.h"
#include "core/public/corecontainer.h"

#ifndef CORE_VECTOR_EXPONENTIAL_GROWTH
#define CORE_VECTOR_EXPONENTIAL_GROWTH !CORE_PLATFORM_RTOS
#endif

namespace Core {

template <class T> class VectorIterator;

//************************************************************************************************
// Vector macros
//************************************************************************************************

/** Iterate trough vector (realtime safe, no memory allocation). */
#define VectorForEach(items, T, var) \
{ for(int __iter = 0, __count = (items).count (); __iter < __count; __iter++) { \
    T var = (T)(items).at (__iter);

/** Iterate trough vector without checking index (uses pointer arithmetic, doesn't call at ()). */
#define VectorForEachFast(items, T, var) \
{ T* __p = (items).getItems (); for(int __iter = (items).count (); __iter != 0; __iter--) { \
    T var = (T)*__p++;

/** Iterate trough vector in reverse order (realtime safe). */
#define VectorForEachReverse(items, T, var) \
{ for(int __iter = (items).count ()-1; __iter >= 0; __iter--) { \
    T var = (T)(items).at (__iter);

//************************************************************************************************
// VectorCompareFunction
/** Function for sorting elements in a vector.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

typedef int (VectorCompareFunction) (const void*, const void*);

/** Define compare function for two elements in a vector that are pointers to object. */
#define DEFINE_VECTOR_COMPARE(FunctionName, Type, lhs, rhs) \
int FunctionName (const void* __lhs, const void* __rhs) \
{ Type* lhs = *(Type**)__lhs; \
  Type* rhs = *(Type**)__rhs;

/** Define compare function for two elements in a vector that are objects or build-in types. */
#define DEFINE_VECTOR_COMPARE_OBJECT(FunctionName, Type, lhs, rhs) \
int FunctionName (const void* __lhs, const void* __rhs) \
{ Type* lhs = (Type*)__lhs; \
  Type* rhs = (Type*)__rhs;

//************************************************************************************************
// ConstVector
/**	Immutable one-dimensional array container class. External memory is passed as C array.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T>
class ConstVector
{
public:
	/** [LIGHT] Construct with C array (no copying). */
	ConstVector (const T* items, int numItems);

	/** Check if container is empty. */
	bool isEmpty () const;
	
	/** Get number of elements in container. */
	INLINE int count () const { return total; }
	
	/** Check if index is valid for accessing element. */
	INLINE bool isValidIndex (int index) const { return index >= 0 && index < total; }
	
	/** Get element at index. */
	T& at (int idx) const;
	
	/** Get first element. */
	T& first () const;
	
	/** Get last element. */
	T& last () const;
	
	/** Check if other vecotor is equal. */
	bool isEqual (const ConstVector<T>& other) const;
	
	bool operator == (const ConstVector& other) const;
	bool operator != (const ConstVector& other) const;

	/** Get index of given element in container (by reference). */
	int index (const T& data) const;
	
	/** Get index of given element in container (by pointer). */
	int index (const T* item) const;
	
	/** Check if container holds given element. */
	bool contains (const T& data) const;
	
	/** Check if container holds any other other container. */
	bool containsAnyOf (const ConstVector<T>& other) const;
	
	/**	Search for given element and return its address.
		Uses binary search, vector needs to be sorted. */
	T* search (const T& data) const;
	
	/**	Find element based on predicate.
		predicate: (T&) -> bool */
	template <class Predicate> T* findIf (const Predicate& recognize) const;
	
	/**	Find element based on predicate.
		Use DEFINE_CONTAINER_PREDICATE. */
	T* findIf (ContainerPredicateFunction recognize) const;

	/** Get start address of array. */
	INLINE T* getItems () const { return items; }
	INLINE operator T* () const { return items; }

	RangeIterator<ConstVector<T>, VectorIterator<T>, T&> begin () const;
	RangeIterator<ConstVector<T>, VectorIterator<T>, T&> end () const;

	/** Access error element. */
	static T& getError () { return error; }

protected:
	T* items;
	int total;
	static T error;

	static int compare (const void*, const void*);
};

//************************************************************************************************
// MutableVector
/** Base class for mutable vectors. Resizing is handled in derived classes.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T, class ResizePolicy>
class MutableVector: public ConstVector<T>
{
public:
	/** Fill internal storage with zeros. */
	void zeroFill ();
	
	/** Fill internal storage with given data. */
	void fill (const T& data);

	/** Add element. */
	bool add (const T& data);
	
	/** Add element once, i.e. if not contained already. */
	bool addOnce (const T& data);
	
	/** Add all elements from other vector. */
	void addAll (const ConstVector<T>& other);
	
	/** Add all elements from other vector once (if not contained already). */
	void addAllOnce (const ConstVector<T>& other);
	
	/** Remove equal element (if contained). */
	bool remove (const T& data);
	
	/** Remove element at index. */
	bool removeAt (int idx);
	
	/** Remove elements if condition is met.
		predicate: (T&) -> bool */
	template <class Predicate> int removeIf (const Predicate& recognize);
	
	/** Remove elements if condition is met.
		Use DEFINE_CONTAINER_PREDICATE. */
	int removeIf (ContainerPredicateFunction recognize);
	
	/** Remove first element. */
	bool removeFirst ();
	
	/** Remove last element. */
	bool removeLast ();
	
	/** Insert element at given index. */
	bool insertAt (int index, const T& data);
	
	/** Remove all elements. */
	void removeAll ();

	/** Set element count directly, only use if you know what you're doing. */
	void setCount (int _count);
	
	/** Reset element count to zero, but keep capacity. */
	void empty ();

	/** Sort elements. */
	void sort ();
	
	/** Sort elements. 
		Use DEFINE_VECTOR_COMPARE or DEFINE_VECTOR_COMPARE_OBJECT for function. */
	void sort (VectorCompareFunction function);
	
	/** Reverse elements. */
	void reverse ();
	
	/** Add element sorted. */
	bool addSorted (const T& data);
	
	/** Add element sorted. */
	bool addSorted (const T& data, VectorCompareFunction function, bool reversed = false);
	
	/** Swap elements. */
	bool swap (const T& t1, const T& t2);
	
	/** Swap elements at index. */
	bool swapAt (int index1, int index2);
	
	/** Add element (via operator). */
	MutableVector& operator << (const T& data);

protected:
	MutableVector (const T* items, int numItems)
	: ConstVector<T> (items, numItems)
	{}
};

//************************************************************************************************
// Vector
/** One-dimensional array that grows dynamically.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T>
class Vector: public MutableVector<T, Vector<T> >
{
public:
	/** Construct with initial capacity and delta. */
	Vector (int capacity = 0, int delta = 5);
	
	/** Copy constructor. */
	Vector (const Vector& other);
	
	#if __cpp_rvalue_references
	/** Move constructor. */
	Vector (Vector&& other);
	#endif
	
	#if __cpp_initializer_lists
	/** Construct with initializer list. */
	Vector (const InitializerList<T>& list);
	#endif
	
	virtual ~Vector ();

	/** Copy from other vector. */
	void copyVector (const Vector& other);
	
	/** Copy from other vector. */
	void copyVector (const T vector[], int count);
	
	/** Take over memory from other vector. */
	void takeVector (Vector& other);
	
	Vector& operator = (const Vector& other);
	#if __cpp_rvalue_references
	Vector& operator = (Vector&& other);
	#endif

	/** Resizes internal memory to \a capacity, rounded up to the next multiple of \a delta. */
	bool resize (int capacity);
	
	/** Set delta applied on resize. */
	void setDelta (int delta);
	
	/** Get configured delta. */
	int getDelta () const;
	
	/** Get current capacity. */
	int getCapacity () const;

protected:
	friend class MutableVector<T, Vector<T> >;

	int capacity;
	int delta;

	bool reserve (int capacity); ///< Reserves at least \a capacity bytes of memory. Grows exponentially, never shrinks.
}; 

//************************************************************************************************
// FixedSizeVector
/** One-dimensional array with a fixed capacity passed as template argument.
    \ingroup core_collect
	\ingroup base_collect  */
//************************************************************************************************

template <class T, int maxElements>
class FixedSizeVector: public MutableVector<T, FixedSizeVector<T, maxElements> >
{
public:
	FixedSizeVector ()
	: MutableVector<T, FixedSizeVector<T, maxElements> > (memory, 0)
	{}

	FixedSizeVector (const FixedSizeVector& other)
	: MutableVector<T, FixedSizeVector<T, maxElements> > (memory, 0)
	{
		for(int i = 0; i < other.count (); i++)
			this->add (other[i]);
	}

	FixedSizeVector& operator = (const FixedSizeVector& other)
	{
		this->removeAll ();
		for(int i = 0; i < other.count (); i++)
			this->add (other[i]);
		return *this;
	}

	/** Check if fixed size exceeded. */
	bool isFull () const
	{
		return this->total == maxElements;
	}

	CORE_STATIC_TEMPLATE_MEMBER bool resize (int capacity)	{ return false; }
	CORE_STATIC_TEMPLATE_MEMBER int getCapacity ()			{ return maxElements; }

protected:
	friend class MutableVector<T, FixedSizeVector<T, maxElements> >;

	T memory[maxElements];

	CORE_STATIC_TEMPLATE_MEMBER bool reserve (int capacity) { return false; }
};

//************************************************************************************************
// VectorIterator
/** Vector iterator
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <class T>
class VectorIterator
{
public:
	VectorIterator (const ConstVector<T>& items);

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

	bool operator == (const VectorIterator& other) const;
	bool operator != (const VectorIterator& other) const;

protected:
	const ConstVector<T>& items;
	int _index;
};

//************************************************************************************************
// VectorSelector
/** Abstraction for selecting between FixedSizeVector and Vector. */
//************************************************************************************************

#ifdef __cpp_alias_templates
namespace
{
	template <class T, int _maxElements>
	struct VectorHelper
	{
		typedef FixedSizeVector<T, _maxElements> vectortype;
	};

	template <class T>
	struct VectorHelper<T, 0>
	{
		typedef Vector<T> vectortype;
	};

	/* if significant savings can be achieved without arrays of 1
	template <class T>
	struct VectorHelper<T,1>
	{
		typedef T vectortype;
	};
	*/
}

template <class T, int _maxElements>
using VectorSelector = typename VectorHelper<T, _maxElements>::vectortype; 
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// ConstVector implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> T ConstVector<T>::error;

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ConstVector<T>::ConstVector (const T* items, int numItems)
: items (const_cast<T*> (items)),
  total (numItems)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int ConstVector<T>::index (const T& data) const
{
	for(int i = 0; i < total; i++)
		if(items[i] == data)
			return i;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int ConstVector<T>::index (const T* item) const
{ 
	if(total > 0 && item >= &items[0] && item <= &items[total-1])
		return int(item - items);
	else
		return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ConstVector<T>::contains (const T& data) const
{
	return index (data) != -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ConstVector<T>::containsAnyOf (const ConstVector<T>& other) const
{
	for(int i = 0; i < other.total; i++)
		if(contains (other.items[i]))
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ConstVector<T>::isEmpty () const
{
	return total == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& ConstVector<T>::at (int idx) const
{
	ASSERT (idx >= 0 && idx < total)

	if(idx >= 0 && idx < total)
		return items[idx];

	return getError ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& ConstVector<T>::first () const
{
	if(total > 0)
		return items[0];
	return getError ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& ConstVector<T>::last () const
{
	if(total > 0)
		return items[total - 1];
	return getError ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ConstVector<T>::isEqual (const ConstVector<T>& other) const
{
	if(this->count () != other.count ())
		return false;
	for(int i = 0; i < this->count (); i++)
		if((at (i) == other.at (i)) == false)
			return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ConstVector<T>::operator == (const ConstVector& other) const
{ return this->isEqual (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ConstVector<T>::operator != (const ConstVector& other) const
{ return this->isEqual (other) == false; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int ConstVector<T>::compare (const void* e1, const void* e2)
{
	T* data1 = (T*)e1;
	T* data2 = (T*)e2;
	return *data1 == *data2 ? 0 : *data1 > *data2 ? 1 : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class Predicate>
T* ConstVector<T>::findIf (const Predicate& recognize) const
{
	for(int i = 0; i < count (); i++)
	{
		T& data = this->items[i];
		if(recognize (data))
			return &data;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T* ConstVector<T>::findIf (ContainerPredicateFunction recognize) const
{
	for(int i = 0; i < count (); i++)
	{
		T& data = this->items[i];
		if(recognize (&data))
			return &data;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T* ConstVector<T>::search (const T& data) const
{
	if(total > 0)
		return (T*)::bsearch (&data, items, total, sizeof(T), Vector<T>::compare);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
RangeIterator<ConstVector<T>, VectorIterator<T>, T&> ConstVector<T>::begin () const
{
	return RangeIterator<ConstVector<T>, VectorIterator<T>, T&> (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
RangeIterator<ConstVector<T>, VectorIterator<T>, T&> ConstVector<T>::end () const
{
	static ConstVector<T> dummy (nullptr, 0);
	return RangeIterator<ConstVector<T>, VectorIterator<T>, T&> (dummy);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// MutableVector implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::zeroFill ()
{
	for(int i = 0; i < static_cast<ResizePolicy*> (this)->getCapacity (); i++)
		this->items[i] = T (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::fill (const T& data)
{
	for(int i = 0; i < static_cast<ResizePolicy*> (this)->getCapacity (); i++)
		this->items[i] = data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::add (const T& data)
{
	if(this->total + 1 > static_cast<ResizePolicy*> (this)->getCapacity ())
		if(!static_cast<ResizePolicy*> (this)->reserve (this->total + 1))
			return false;

	this->items[this->total++] = data;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::addOnce (const T& data)
{
	if(this->contains (data) == false)
		return add (data);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::addAll (const ConstVector<T>& other)
{
	for(int i = 0; i < other.count (); i++)
		add (other[i]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::addAllOnce (const ConstVector<T>& other)
{
	for(int i = 0; i < other.count (); i++)
		addOnce (other[i]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::remove (const T& data)
{
	int idx = this->index (data);
	if(idx == -1)
		return false;

	for(int i = idx; i < this->total - 1; i++)
		this->items[i] = this->items[i + 1];

	if(this->total > 0)
		this->items[this->total - 1] = this->getError ();

	this->total--;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::removeAt (int idx)
{
	if(idx >= this->total || idx < 0)
		return false;

	for(int i = idx; i < this->total - 1; i++)
		this->items[i] = this->items[i + 1];

	if(this->total > 0)
		this->items[this->total - 1] = this->getError ();

	this->total--;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
template <class Predicate>
int MutableVector<T, ResizePolicy>::removeIf (const Predicate& recognize)
{
	int removed = 0;
	for(int i = this->total - 1; i >= 0; i--)
	{
		if(recognize (this->items[i]))
			if(removeAt (i))
				removed++;
	}

	return removed;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
int MutableVector<T, ResizePolicy>::removeIf (ContainerPredicateFunction recognize)
{
	int removed = 0;
	for(int i = this->total - 1; i >= 0; i--)
	{
		if(recognize (&this->items[i]))
			if(removeAt (i))
				removed++;
	}

	return removed;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::removeFirst ()
{
	return removeAt (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::removeLast ()
{
	return removeAt (this->total - 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::insertAt (int idx, const T& data)
{
	if(idx < 0 || idx > this->total)
		return false;

	if(this->total + 1 > static_cast<ResizePolicy*> (this)->getCapacity ())
		if(!static_cast<ResizePolicy*> (this)->reserve (this->total + 1))
			return false;

	for(int i = this->total; i > idx; i--)
		this->items[i] = this->items[i-1];
	this->items[idx] = data;
	this->total++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::addSorted (const T& data)
{
	for(int i = 0; i < this->total; i++)
		if(this->items[i] > data)
			return insertAt (i, data);
	return add (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::addSorted (const T& data, VectorCompareFunction function, bool reversed)
{
	for(int i = 0; i < this->total; i++)
	{
		int cmpResult = function (&this->items[i], &data);
		if(reversed)
			cmpResult *= -1;
		if(cmpResult > 0)
			return insertAt (i, data);
	}
	return add (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::swap (const T& t1, const T& t2)
{
	int idx1 = this->index (t1);
	int idx2 = this->index (t2);
	if(idx1 >= 0 && idx2 >= 0 && idx1 != idx2)
	{
		this->items[idx1] = t2;
		this->items[idx2] = t1;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
bool MutableVector<T, ResizePolicy>::swapAt (int index1, int index2)
{	
	if(this->isValidIndex (index1) && this->isValidIndex (index2) && index1 != index2)
	{
		T tmp = this->items[index1];
		this->items[index1] = this->items[index2];
		this->items[index2] = tmp;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::reverse ()
{
	for(int i = 0; i < this->total/2; i++)
	{
		int ir = this->total-1-i;
		T tmp = this->items[i];
		this->items[i] = this->items[ir];
		this->items[ir] = tmp;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::sort ()
{
	::qsort (this->items, this->total, sizeof(T), ConstVector<T>::compare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::sort (VectorCompareFunction function)
{
	::qsort (this->items, this->total, sizeof(T), function);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
MutableVector<T, ResizePolicy>& MutableVector<T, ResizePolicy>::operator << (const T& data)
{
	add (data);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::removeAll ()
{
	static_cast<ResizePolicy*> (this)->resize (0);
	this->total = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::setCount (int _count)
{
	if(static_cast<ResizePolicy*> (this)->getCapacity () < _count)
		if(!static_cast<ResizePolicy*> (this)->resize (_count))
			return;

	this->total = _count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class ResizePolicy>
void MutableVector<T, ResizePolicy>::empty ()
{
	setCount (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Vector implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Vector<T>::Vector (int _capacity, int _delta)
: MutableVector<T, Vector<T> > (nullptr, 0),
  capacity (0),
  delta (0)
{
	delta = _delta;
	resize (_capacity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Vector<T>::Vector (const Vector& other)
: MutableVector<T, Vector<T> > (nullptr, 0),
  capacity (0),
  delta (0)
{
	setDelta (other.delta);
	copyVector (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __cpp_rvalue_references
template <class T>
Vector<T>::Vector (Vector&& other)
: MutableVector<T, Vector<T> > (other.items, other.total),
  capacity (other.capacity),
  delta (other.delta)
{
	other.capacity = 0;
	other.items = nullptr;
	other.total = 0;
}
#endif // __cpp_rvalue_references

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __cpp_initializer_lists
template <class T>
Vector<T>::Vector (const InitializerList<T>& list)
: MutableVector<T, Vector<T> > (nullptr, 0),
  capacity (0),
  delta (5)
{
	copyVector (list.begin (), (int)list.size ());
}
#endif // __cpp_initializer_lists

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Vector<T>::~Vector ()
{
	resize (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void Vector<T>::copyVector (const Vector& other)
{
	resize (other.getCapacity ());

	delta = other.delta;
	this->total = other.total;

	int _total = this->total;

	const T* src = other.items;
	T* dst = this->items;
	for(int i = 0; i < _total; i++)
		*(dst++) = *(src++);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void Vector<T>::copyVector (const T vector[], int count)
{
	resize (count);
	this->total = count;

	const T* src = vector;
	T* dst = this->items;
	for(int i = 0; i < count; i++)
		*(dst++) = *(src++);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void Vector<T>::takeVector (Vector<T>& other)
{
	if(&other != this)
	{
		if(this->items)
			delete [] this->items;
		this->items = other.items;
		this->total = other.total;
		capacity = other.capacity;
		delta = other.delta;

		other.items = nullptr;
		other.total = 0;
		other.capacity = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Vector<T>& Vector<T>::operator = (const Vector<T>& other)
{
	if(&other != this)
		copyVector (other);

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __cpp_rvalue_references
template <class T>
Vector<T>& Vector<T>::operator = (Vector<T>&& other)
{
	takeVector (other);
	return *this;
}
#endif // __cpp_rvalue_references

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Vector<T>::resize (int _capacity)
{
	if(capacity == _capacity)
		return true;

	if(_capacity <= 0)
	{
		if(this->items)
			delete [] this->items;
		this->items = nullptr;
		capacity = 0;
		this->total = 0; // no more items
		return true;
	}

	int newCapacity = ((_capacity - 1) / delta + 1) * delta;

	if(capacity == newCapacity)
		return true;

	T* newItems = NEW T[newCapacity];
	if(newItems)
	{
		// copy old items
		if(this->total > newCapacity)
			this->total = newCapacity;
		for(int i = 0; i < this->total; i++)
		{
			#if __cpp_rvalue_references
			newItems[i] = static_cast<T&&> (this->items[i]);
			#else
			newItems[i] = this->items[i];
			#endif
		}

		if(this->items)
			delete [] this->items;

		this->items = newItems;
		capacity = newCapacity;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Vector<T>::reserve (int _capacity)
{
	#if CORE_VECTOR_EXPONENTIAL_GROWTH

	if(_capacity <= capacity)
		return true;

	// Grow by a factor of 1.5 if this is enough to hold the new capacity.
	int exponentialGrowthCapacity = capacity + capacity / 2;
	if(_capacity <= exponentialGrowthCapacity)
		_capacity = exponentialGrowthCapacity;

	#endif // CORE_VECTOR_EXPONENTIAL_GROWTH

	return resize (_capacity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void Vector<T>::setDelta (int d)
{
	delta = d;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int Vector<T>::getDelta () const
{
	return delta;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int Vector<T>::getCapacity () const
{
	return capacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// VectorIterator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
VectorIterator<T>::VectorIterator (const ConstVector<T>& items)
: items (items),
  _index (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void VectorIterator<T>::first ()
{
	_index = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void VectorIterator<T>::last ()
{
	_index = items.count () - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& VectorIterator<T>::next ()
{
	int idx = _index++;
	if(idx < items.count ())
		return items[idx];

	return items.getError ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& VectorIterator<T>::previous ()
{
	int idx = _index--;
	if(idx >= 0)
		return items[idx];

	return items.getError ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T& VectorIterator<T>::peekNext () const
{
	if(done ())
		return items.getError ();

	return items[_index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool VectorIterator<T>::done () const
{
	return _index < 0 || _index >= items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool VectorIterator<T>::operator == (const VectorIterator<T>& other) const
{
	if(other.done ())
		return done ();
	else if(done ())
		return false;
	else
		return items.getItems () + _index == other.items.getItems () + other._index; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool VectorIterator<T>::operator != (const VectorIterator<T>& other) const
{ return !(*this == other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _corevector_h
