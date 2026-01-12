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
// Filename    : ccl/base/collections/arraybox.h
// Description : Box for IArrayObject
//
//************************************************************************************************

#ifndef _ccl_arraybox_h
#define _ccl_arraybox_h

#include "ccl/public/base/iarrayobject.h"

#include "ccl/base/collections/container.h"

namespace CCL {

class ArrayBoxIterator;

//************************************************************************************************
// ArrayBox
//************************************************************************************************

class ArrayBox: public Container
{
public:
	DECLARE_CLASS (ArrayBox, Container)

	ArrayBox (IArrayObject* items = nullptr);

	static Container* convert (IUnknown* unknown);

	// Container
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

	template<class Element = Object> RangeIterator<ArrayBox, ArrayBoxIterator, Element*> begin () const;
	template<class Element = Object> RangeIterator<ArrayBox, ArrayBoxIterator, Element*> end () const;
	using IteratorClass = ArrayBoxIterator;

protected:
	SharedPtr<IArrayObject> items;
};

//************************************************************************************************
// ArrayBoxIterator
//************************************************************************************************

class ArrayBoxIterator:	public Iterator
{
public:
	ArrayBoxIterator (const ArrayBox& items);

	// Iterator
	tbool CCL_API done () const override;
	void first () override;
	void last () override;
	Object* next () override;
	Object* previous () override;

	Object* peekNext () const;

	bool operator== (const ArrayBoxIterator& other) const;
	bool operator!= (const ArrayBoxIterator& other) const;

protected:
	const ArrayBox& items;
	int index;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ArrayBox inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Element>
inline RangeIterator<ArrayBox, ArrayBoxIterator, Element*> ArrayBox::begin () const
{ return RangeIterator<ArrayBox, ArrayBoxIterator, Element*> (*this); }

template<class Element>
inline RangeIterator<ArrayBox, ArrayBoxIterator, Element*> ArrayBox::end () const
{ static ArrayBox dummy; return RangeIterator<ArrayBox, ArrayBoxIterator, Element*> (dummy); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// ArrayBoxIterator inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ArrayBoxIterator::operator== (const ArrayBoxIterator& other) const
{ return done () == other.done (); }

inline bool ArrayBoxIterator::operator!= (const ArrayBoxIterator& other) const
{ return !(*this == other); }

} // namespace CCL

#endif // _ccl_arraybox_h
