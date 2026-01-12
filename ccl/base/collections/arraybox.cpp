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
// Filename    : ccl/base/collections/arraybox.cpp
// Description : Box for IArrayObject
//
//************************************************************************************************

#include "ccl/base/collections/arraybox.h"

using namespace CCL;

//************************************************************************************************
// ArrayBox
//************************************************************************************************

Container* ArrayBox::convert (IUnknown* unknown)
{
	if(auto c = unknown_cast<Container> (unknown))
		return return_shared (c);
	else
	{
		UnknownPtr<IArrayObject> items (unknown);
		return NEW ArrayBox (items); // items can be null
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ArrayBox, Container)

//////////////////////////////////////////////////////////////////////////////////////////////////

ArrayBox::ArrayBox (IArrayObject* items)
: items (items)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ArrayBox::newIterator () const
{
	return NEW ArrayBoxIterator (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArrayBox::isEmpty () const
{
	if(items == nullptr)
		return true;
	if(items->getArrayLength () == 0)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ArrayBox::count () const
{
	return items ? items->getArrayLength () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ArrayBox::at (int idx) const
{
	Variant var;
	if(items) items->getArrayElement (var, idx);
	return unknown_cast<Object> (var.asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ArrayBox::index (const Object& searchObj) const
{
	if(items) for(int i = 0, count = items->getArrayLength (); i < count; i++)
	{
		Variant var;
		items->getArrayElement (var, i);
		Object* obj = unknown_cast<Object> (var.asUnknown ());
		if(obj && obj->equals (searchObj))
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ArrayBox::index (const Object* searchObj) const
{
	if(items) for(int i = 0, count = items->getArrayLength (); i < count; i++)
	{
		Variant var;
		items->getArrayElement (var, i);
		Object* obj = unknown_cast<Object> (var.asUnknown ());
		if(obj && obj == searchObj)
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArrayBox::add (Object* obj)
{
#if 0 // unused yet
	UnknownPtr<IMutableArray> mutableItems (items);
	ASSERT (mutableItems != 0)
	if(mutableItems == 0)
		return false;

	Variant var (*obj);
	var.share ();
	return mutableItems->addArrayElement (var) != 0;
#else
	CCL_NOT_IMPL ("ArrayBox::add() not implemented!")
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArrayBox::remove (Object* obj)
{
	CCL_NOT_IMPL ("ArrayBox::remove() not implemented!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArrayBox::removeAll ()
{
	CCL_NOT_IMPL ("ArrayBox::removeAll() not implemented!")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ArrayBox::findEqual (const Object& obj) const
{
	int idx = index (obj);
	return idx != -1 ? at (idx) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArrayBox::addSorted (Object* obj)
{
	CCL_NOT_IMPL ("ArrayBox::addSorted() not implemented!")
	return false;
}

//************************************************************************************************
// ArrayBoxIterator
//************************************************************************************************

ArrayBoxIterator::ArrayBoxIterator (const ArrayBox& items)
: items (items),
  index (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ArrayBoxIterator::done () const
{
	return index < 0 || index >= items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArrayBoxIterator::first ()
{
	index = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArrayBoxIterator::last ()
{
	index = items.count () - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ArrayBoxIterator::next ()
{
	int idx = index++;
	if(idx < items.count ())
		return items.at (idx);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ArrayBoxIterator::previous ()
{
	int idx = index--;
	if(idx >= 0)
		return items.at (idx);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ArrayBoxIterator::peekNext () const
{
	if(index < items.count ())
		return items.at (index);
	return nullptr;
}
