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
// Filename    : core/public/corecontainer.h
// Description : Common utilities for container classes
//
//************************************************************************************************

#ifndef _corecontainer_h
#define _corecontainer_h

#if __cpp_initializer_lists
#include <initializer_list>
#endif

namespace Core {

//************************************************************************************************
// ContainerPredicateFunction
/** Function for performing a condition on a container element.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

typedef bool (ContainerPredicateFunction) (const void*);

/** Define predicate function for element in a container that is a pointer to an object. */
#define DEFINE_CONTAINER_PREDICATE(FunctionName, Type, var) \
bool FunctionName (const void* __var) \
{ Type* var = *(Type**)__var;

/** Define predicate for element in a container that is an object or build-in type. */
#define DEFINE_CONTAINER_PREDICATE_OBJECT(FunctionName, Type, var) \
bool FunctionName (const void* __var) \
{ Type* var = (Type*)__var;

//************************************************************************************************
// InitializerList
/** C++ 11 initializer list.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

#if __cpp_initializer_lists
template<class T> using InitializerList = std::initializer_list<T>;
#else
template<class T> class InitializerList // dummy implementation to allow compilation as < C++11
{
public:
	const T* begin () const { return 0; }
	const T* end () const { return 0; }
	int size () const { return 0; }
};
#endif

//************************************************************************************************
// RangeIterator
/** Helper template for using a container in an STL-like context.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class Container, class Iterator, class Element>
class RangeIterator
{
public:
	RangeIterator (const Container& container)
	: iterator (container)
	{}

	RangeIterator& operator ++ () { iterator.next (); return *this; }
	Element operator * () { return static_cast<Element> (iterator.peekNext ()); }
	bool operator != (RangeIterator& other) { return iterator != other.iterator; }

private:
	Iterator iterator;
};

} // namespace Core

#endif // _corecontainer_h
