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
// Filename    : ccl/base/collections/container.h
// Description : Container class
//
//************************************************************************************************

#ifndef _ccl_container_h
#define _ccl_container_h

#include "ccl/base/object.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/collections/iunknownlist.h"

#include "core/public/corecontainer.h"

namespace CCL {

using Core::RangeIterator;
using Core::InitializerList;

/** \addtogroup base_collect 
@{ */

//************************************************************************************************
// Container macros
//************************************************************************************************

#if !DOXYGEN

//////////////////////////////////////////////////////////////////////////////////////////////////
// ForEach : iterate thru container
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEach(cont, Class, var) \
{ CCL::AutoPtr<CCL::Iterator> __iter = (cont).newIterator (); \
  if(__iter) while(!__iter->done ()) { \
    Class* var = (Class*)__iter->next ();

//////////////////////////////////////////////////////////////////////////////////////////////////
// ForEachReverse : iterate thru container in reverse order
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachReverse(cont, Class, var) \
{ CCL::AutoPtr<CCL::Iterator> __iter = (cont).newIterator (); \
  if(__iter) __iter->last (); \
  if(__iter) while(!__iter->done ()) { \
    Class* var = (Class*)__iter->previous ();

//////////////////////////////////////////////////////////////////////////////////////////////////
// IterForEach : loop thru iterator
//////////////////////////////////////////////////////////////////////////////////////////////////

#define IterForEach(createIter, Class, var) \
{ CCL::AutoPtr<CCL::Iterator> __iter = createIter; \
  if(__iter) while(!__iter->done ()) { \
    Class* var = (Class*)__iter->next ();

//////////////////////////////////////////////////////////////////////////////////////////////////
// IterForEachReverse : loop thru iterator in reverse order
//////////////////////////////////////////////////////////////////////////////////////////////////

#define IterForEachReverse(createIter, Class, var) \
{ CCL::AutoPtr<CCL::Iterator> __iter = createIter; \
  if(__iter) __iter->last (); \
  if(__iter) while(!__iter->done ()) { \
    Class* var = (Class*)__iter->previous ();

#endif

//************************************************************************************************
// Iterator
/** Abstract iterator base class. */
//************************************************************************************************

class Iterator: public Object,
				public IUnknownIterator
{
public:
	DECLARE_CLASS_ABSTRACT (Iterator, Object)
	DECLARE_METHOD_NAMES (Iterator)

	/** Move to first. */
	virtual void first () = 0;

	/** Move to last. */
	virtual void last () = 0;

	/** Get next object. */
	virtual Object* next () = 0;

	/** Get previous object. */
	virtual Object* previous () = 0;

	#if __cpp_return_type_deduction
	template<class Element = Object> auto begin () const;
	template<class Element = Object> auto end () const;
	#endif

	CLASS_INTERFACE (IUnknownIterator, Object)

protected:
	// IUnknownIterator
	IUnknown* CCL_API nextUnknown () override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// NullIterator
/** Iterator for the nothingness. */
//************************************************************************************************

class NullIterator: public Iterator
{
public:
	// Iterator
	tbool CCL_API done () const override { return true; }
	void first () override {}
	void last () override {}
	Object* next () override { return nullptr; }
	Object* previous () override { return nullptr; }
};

//************************************************************************************************
// Container
/** Abstract container base class. */
//************************************************************************************************

class Container: public Object,
				 public IContainer
{
public:
	DECLARE_CLASS_ABSTRACT (Container, Object)
	DECLARE_METHOD_NAMES (Container)
	DECLARE_PROPERTY_NAMES (Container)

	Container ();

	/** Set object ownership. */
	void objectCleanup (bool state = true);

	/** Get object ownership. */
	bool isObjectCleanup () const;

	/** Create new iterator. */
	virtual Iterator* newIterator () const = 0;

	/** Check if container is empty. */
	virtual bool isEmpty () const = 0;

	/** Count number of items in container. */
	virtual int count () const = 0;

	/** Get object at given index. */
	virtual Object* at (int idx) const = 0;

	/** Get index of object, compares via Object::equals. */
	virtual int index (const Object& obj) const = 0;

	/** Get index of object, compares object address. */
	virtual int index (const Object* obj) const = 0;

	/** Add object. */
	virtual bool add (Object* obj) = 0;

	/** Remove object, ownership is transfered to caller. */
	virtual bool remove (Object* obj) = 0;

	/**
	 * Delete all objects satisfying lambda function, releases object optionally.
	 * Predicate: (Object*) -> bool.
	 * @returns number of removed elements
	 */
	template<class Predicate>
	int removeIf (const Predicate& recognize);

	/**
	 * Delete all objects satisfying lambda function; performs ccl_cast to
	 * check type, releases object optionally.
	 * Predicate: (Class&) -> bool.
	 * @returns number of removed elements
	 */
	template<class Class, class Predicate>
	int removeIf (const Predicate& recognize);

	/** Remove (and optionally release) all object. */
	virtual void removeAll () = 0;

	/** Find equal object. */
	virtual Object* findEqual (const Object& obj) const = 0;

	/** Add object sorted using Object::compare. */
	virtual bool addSorted (Object* obj) = 0;

	/** Check if object is contained, compares via Object::equals. */
	bool contains (const Object& obj) const;

	/** Check if object is contained, compares object address. */
	bool contains (const Object* obj) const;

	/**
	 * Check if object is contained using lamda function.
	 * Predicate: (Object*) -> bool.
	 */
	template<class Predicate>
	Object* findIf (const Predicate& recognize) const;

	/**
	 * Check if object is contained using lamda function; performs ccl_cast to check type.
	 * Predicate: (Class&) -> bool.
	 */
	template<class Class, class Predicate>
	Class* findIf (const Predicate& recognize) const;

	/** Copy mode. */
	enum CopyMode
	{
		kNormal,	///< leave reference count unchanged
		kShare,		///< share objects
		kClone		///< clone objects
	};

	/** Add from other container using given copy mode. */
	void add (const Container& objects, CopyMode mode = kNormal);

	/** Add object if not already in container. */
	bool addOnce (Object* object);

	/** Add a copy of object if not already in container. */
	bool addOnce (const Object& object);

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	template<class Element = Object> RangeIterator<CCL::Container, CCL::Iterator, Element*> begin () const;
	template<class Element = Object> RangeIterator<CCL::Container, CCL::Iterator, Element*> end () const;
	using IteratorClass = Iterator;

	CLASS_INTERFACE (IContainer, Object)

protected:
	enum Flags { kCleanup = 1<<0 };
	int flags;

	explicit Container (const Container&);
	void copyFrom (const Container&);

	// IContainer
	IUnknownIterator* CCL_API createIterator () const override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// IteratorDelegate
/** Passes all Iterator calls to another iterator. */
//************************************************************************************************

class IteratorDelegate: public Iterator
{
public:
	IteratorDelegate (Iterator* iterator); ///< takes ownership

	// Iterator
	tbool CCL_API done () const override	{ return iterator->done (); }
	void first () override					{ iterator->first (); }
	void last () override					{ iterator->last (); }
	Object* next () override				{ return iterator->next (); }
	Object* previous () override			{ return iterator->previous (); }

protected:
	AutoPtr<Iterator> iterator;
};

//************************************************************************************************
// PrefetchingIteratorDelegate
/** Prefetches the next / previous item, used as base class for Resolving / FilteringIterator. */
//************************************************************************************************

class PrefetchingIteratorDelegate: public IteratorDelegate
{
public:
	// Iterator
	tbool CCL_API done () const override;
	void first () override;
	void last () override;
	Object* next () override;
	Object* previous () override;

protected:
	Object* _next;

	PrefetchingIteratorDelegate (Iterator* iterator); // derived class ctor must call prefetchNext!

	virtual void prefetchNext () = 0;
	virtual void prefetchPrevious () = 0;
};

//************************************************************************************************
// ResolvingIterator
/** Passes the objects returned by another iterator through a Resolver function.
	The Resolver class must have a static function resolveObject (). */
//************************************************************************************************

template<typename Resolver>
class ResolvingIterator: public PrefetchingIteratorDelegate
{
public:
	ResolvingIterator (Iterator* iterator); ///< takes ownership

protected:
	// PrefetchingIteratorDelegate (skip items that are resolved to 0)
	void prefetchNext () override		{ while((_next = iterator->next ())     && !(_next = Resolver::resolveObject (_next))); }
	void prefetchPrevious () override	{ while((_next = iterator->previous ()) && !(_next = Resolver::resolveObject (_next))); }
};

//************************************************************************************************
// LambdaResolvingIterator
/** Passes the objects returned by another iterator through a Resolver lambda function. */
//************************************************************************************************

template<typename T>
class LambdaResolvingIterator: public PrefetchingIteratorDelegate
{
public:
	LambdaResolvingIterator (Iterator* iterator, const T& resolve); ///< takes ownership of iterator

protected:
	T resolve;

	// PrefetchingIteratorDelegate (skip items that are resolved to 0)
	void prefetchNext () override		{ while((_next = iterator->next ())     && !(_next = resolve (_next))); }
	void prefetchPrevious () override	{ while((_next = iterator->previous ()) && !(_next = resolve (_next))); }
};

//************************************************************************************************
// FilteringIterator
/** Delivers objects returned by another iterator only if they match the filter. */
//************************************************************************************************

class FilteringIterator: public PrefetchingIteratorDelegate
{
public:
	FilteringIterator (Iterator* iterator, IObjectFilter* filter); ///< takes ownership of iterator & filter

private:
	AutoPtr<IObjectFilter> filter;

	// PrefetchingIteratorDelegate
	void prefetchNext () override		{ while((_next = iterator->next ()) && !filter->matches (ccl_as_unknown (_next))); }
	void prefetchPrevious () override	{ while((_next = iterator->previous ()) && !filter->matches (ccl_as_unknown (_next))); }
};

//************************************************************************************************
// CascadedIterator
/** Creates an iterator from each object delivered by outerIterator, using the given
	Lambda function createInnerIterator. Delivers all objects of all these inner iterators. */
//************************************************************************************************

template<class CreateIter>
class CascadedIterator: public PrefetchingIteratorDelegate
{
public:
	CascadedIterator (Iterator* outerIterator, CreateIter createInnerIterator);

private:
	CreateIter createInnerIterator;
	AutoPtr<Iterator> innerIterator;

	// PrefetchingIteratorDelegate
	void first () override;
	void last () override;
	void prefetchNext () override;
	void prefetchPrevious () override;
};

//************************************************************************************************
// createConcatenatedIterator
/** Creates an iterator that delivers all objects from iterator1, followed by all objects from iterator2.
	Owns both input interators. */
//************************************************************************************************

Iterator* createConcatenatedIterator (Iterator* iterator1, Iterator* iterator2);

//************************************************************************************************
// HoldingIterator
/** Holds the container during the iterators lifetime (to avoid issues with late garbage collection). */
//************************************************************************************************

class HoldingIterator: public IteratorDelegate
{
public:
	HoldingIterator (Object* container, Iterator* iterator);
	~HoldingIterator ();

protected:
	SharedPtr<Object> container;
};

//************************************************************************************************
// ReverseIterator
/** Iterates objects in reverse order of another iterator.
	Use e.g. with IterForEach to avoid 2 separate loops for both directions. */
//************************************************************************************************

class ReverseIterator: public IteratorDelegate
{
public:
	ReverseIterator (Iterator* iterator); ///< takes ownership

	// Iterator
	void first () override			{ iterator->last (); }
	void last () override			{ iterator->first (); }
	Object* next () override		{ return iterator->previous (); }
	Object* previous () override	{ return iterator->next (); }
};

//************************************************************************************************
// Helper for creating a range-based iterator that casts the returned objects to a given class
// e.g.
//	ObjectArray a;
//	for(auto : iterate_as<Url> (a)) ...
//************************************************************************************************

template<class T, class Container>
class CastingRangeIterator
{
public:
	CastingRangeIterator (const Container& container);

	#if __cpp_return_type_deduction
	auto begin () const;
	auto end () const;
	#endif

private:
	const Container& container;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Container>
CastingRangeIterator<T, Container> iterate_as (const Container& c);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Container inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Container::contains (const Object& obj) const
{ return index (obj) != -1; }

inline bool Container::contains (const Object* obj) const
{ return index (obj) != -1; }

inline bool Container::addOnce (Object* object)
{ if(contains (object)) return false; add (object); return true; }

inline bool Container::addOnce (const Object& object)
{ if(contains (object)) return false; add (object.clone ()); return true; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Predicate>
inline Object* Container::findIf (const Predicate& recognize) const
{
	ForEach (*this, Object, obj)
		if(recognize (obj))
			return obj;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Predicate>
inline T* Container::findIf (const Predicate& recognize) const
{
	ForEach (*this, Object, obj)
		T* t = ccl_cast<T> (obj);
		if(t && recognize (*t))
			return t;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Predicate>
inline int Container::removeIf (const Predicate& recognize)
{
	CCL_NOT_IMPL ("Container::removeIf() not implemented!")
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Predicate>
inline int Container::removeIf (const Predicate& recognize)
{
	CCL_NOT_IMPL ("Container::removeIf() not implemented!")
	return 0;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Element>
RangeIterator<Container, Iterator, Element*> Container::begin () const
{
	return RangeIterator<Container, Iterator, Element*> (*this);
}

template<class Element>
RangeIterator<Container, Iterator, Element*> Container::end () const
{
	return RangeIterator<Container, Iterator, Element*> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IteratorDelegate inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline IteratorDelegate::IteratorDelegate (Iterator* iterator)
: iterator (iterator) { ASSERT (iterator) }

//////////////////////////////////////////////////////////////////////////////////////////////////
// PrefetchingIteratorDelegate inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline PrefetchingIteratorDelegate::PrefetchingIteratorDelegate (Iterator* iterator)
: IteratorDelegate (iterator), _next (nullptr) {}

inline tbool CCL_API PrefetchingIteratorDelegate::done () const
{ return _next == nullptr; }

inline void PrefetchingIteratorDelegate::first ()
{ IteratorDelegate::first (); prefetchNext (); }

inline void PrefetchingIteratorDelegate::last ()
{ IteratorDelegate::last (); prefetchPrevious ();}

inline Object* PrefetchingIteratorDelegate::next ()
{ Object* obj = _next; prefetchNext (); return obj; }

inline Object* PrefetchingIteratorDelegate::previous ()
{ Object* obj = _next; prefetchPrevious (); return obj; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// ResolvingIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Resolver>
inline ResolvingIterator<Resolver>::ResolvingIterator (Iterator* iterator)
: PrefetchingIteratorDelegate (iterator) { prefetchNext (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// LambdaResolvingIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Resolve>
inline LambdaResolvingIterator<Resolve>::LambdaResolvingIterator (Iterator* iterator, const Resolve& resolve)
: PrefetchingIteratorDelegate (iterator), resolve (resolve) {  prefetchNext (); }

template <typename Resolve>
Iterator* makeResolvingIterator (Iterator* iterator, const Resolve& resolve)
{ return iterator ? NEW LambdaResolvingIterator<Resolve> (iterator, resolve) : nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// FilteringIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline FilteringIterator::FilteringIterator (Iterator* iterator, IObjectFilter* filter)
: PrefetchingIteratorDelegate (iterator), filter (filter)
{ prefetchNext (); }

template <typename Filter>
CCL::Iterator* makeFilteringIterator (CCL::Iterator* iterator, const Filter& matches)
{ return iterator ? NEW FilteringIterator (iterator, ObjectFilter::create<Filter> (matches)) : nullptr; }

//************************************************************************************************
// CascadedIterator inline
//************************************************************************************************

template <typename CreateIter>
Iterator* makeCascadedIterator (Iterator* iterator, const CreateIter& createInnerIterator)
{ return iterator ? NEW CascadedIterator<CreateIter> (iterator, createInnerIterator) : nullptr; }

template<class CreateIter>
CascadedIterator<CreateIter>::CascadedIterator (Iterator* outerIterator, CreateIter createInnerIterator)
: PrefetchingIteratorDelegate (outerIterator), createInnerIterator (createInnerIterator)
{ prefetchNext (); }

template<class CreateIter>
void CascadedIterator<CreateIter>::first ()	{ innerIterator = nullptr; PrefetchingIteratorDelegate::first (); }

template<class CreateIter>
void CascadedIterator<CreateIter>::last ()	{ innerIterator = nullptr; PrefetchingIteratorDelegate::last (); }

template<class CreateIter>
void CascadedIterator<CreateIter>::prefetchNext ()
{
	_next = innerIterator ? innerIterator->next () : nullptr;

	if(!_next)
		while(Object* nextOuter = iterator->next ())
			if(Iterator* i = createInnerIterator (nextOuter))
			{
				innerIterator = i; // don't set back to 0!
				if(_next = innerIterator->next ())
					return;
			}
}

template<class CreateIter>
void CascadedIterator<CreateIter>::prefetchPrevious ()
{
	_next = innerIterator ? innerIterator->previous () : nullptr;

	if(!_next)
		while(Object* nextOuter = iterator->previous ())
			if(Iterator* i = createInnerIterator (nextOuter))
			{
				innerIterator = i; // don't set back to 0!
				if(_next = innerIterator->previous ())
					return;
			}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// HoldingIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline HoldingIterator::HoldingIterator (Object* container, Iterator* iterator)
: IteratorDelegate (iterator), container (container) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline HoldingIterator::~HoldingIterator ()
{
	iterator.release (); // make sure iterator is released before container
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ReverseIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ReverseIterator::ReverseIterator (Iterator* iterator)
: IteratorDelegate (iterator) { iterator->last (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// CastingRangeIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Container>
inline CastingRangeIterator<T, Container>::CastingRangeIterator (const Container& container)
: container (container) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if __cpp_return_type_deduction
template<class T, class Container>
inline auto CastingRangeIterator<T, Container>::begin () const
{ return container.template begin<T> (); }

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Container>
inline auto CastingRangeIterator<T, Container>::end () const
{ return container.template end<T> (); }

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Element>
auto Iterator::begin () const
{ return RangeIterator<Container, Iterator, Element*> (return_shared (const_cast<Iterator*> (this))); }

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class Element>
auto Iterator::end () const
{ return RangeIterator<Container, Iterator, Element*> (); }

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class Container>
inline CastingRangeIterator<T, Container> iterate_as (const Container& c)
{ return CastingRangeIterator<T, Container> (c); }

//////////////////////////////////////////////////////////////////////////////////////////////////

/** @} */ // ingroup
} // namespace CCL

namespace Core {

//************************************************************************************************
// RangeIterator
/** Partial specialization of Core::RangeIterator<> for iterating a container given as an (abstract) reference.
	Creates iterator on heap via Container::newIterator (). */
//************************************************************************************************

template<class Element>
class RangeIterator<CCL::Container, CCL::Iterator, Element>
{
public:
	RangeIterator (const CCL::Container& container)
	: iterator (container.newIterator ()),
	  current (iterator ? static_cast<Element> (iterator->next ()) : nullptr)
	{}

	RangeIterator (CCL::Iterator* iterator)
	: iterator (iterator),
	  current (iterator ? static_cast<Element> (iterator->next ()) : nullptr)
	{}

	RangeIterator ()
	: iterator (nullptr),
	  current (nullptr)
	{}

	RangeIterator& operator++ ()			{ current = static_cast<Element> (iterator->next ()); return *this; }
	Element operator* ()					{ return current; }
	bool operator!= (RangeIterator& other)	{ return (current == nullptr) != (other.current == nullptr); }

private:
	CCL::AutoPtr<CCL::Iterator> iterator;
	Element current;
};

} // namespace Core

#endif // _ccl_container_h
