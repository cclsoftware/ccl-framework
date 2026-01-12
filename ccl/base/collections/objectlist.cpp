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
// Filename    : ccl/base/collections/objectlist.cpp
// Description : Object List
//
//************************************************************************************************

#include "ccl/base/collections/objectstack.h"

using namespace CCL;

//************************************************************************************************
// ObjectStack (included here to save a source file)
//************************************************************************************************

DEFINE_CLASS (ObjectStack, ObjectList)
DEFINE_CLASS_NAMESPACE (ObjectStack, NAMESPACE_CCL)

//************************************************************************************************
// ObjectList
//************************************************************************************************

DEFINE_CLASS (ObjectList, Container)
DEFINE_CLASS_NAMESPACE (ObjectList, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectList::index (const Object& searchObj) const
{
	int idx = 0;
	ListForEach (*this, Object*, obj)
		if(obj->equals (searchObj))
			return idx;
		idx++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectList::index (const Object* searchObj) const
{
	int idx = 0;
	ListForEach (*this, Object*, obj)
		if(obj == searchObj)
			return idx;
		idx++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectList::removeAll ()
{
	if(isObjectCleanup ())
	{
		ListForEach (*this, Object*, obj)
			if(obj)
				obj->release ();
		EndFor
	}
	
	if(iterator)
		iterator->removeAll ();

	LinkedList<Object*>::removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ObjectList::findEqual (const Object& searchObj) const
{
	ListForEach (*this, Object*, obj)
		if(obj->equals (searchObj))
			return obj;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectList::addSorted (Object* addObj)
{
	if(addObj == nullptr)
		return false;

	ListForEach (*this, Object*, obj)
		if(obj->compare (*addObj) > 0)
			return insertBefore (obj, addObj);
	EndFor
	return add (addObj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectList::addDuringIteration (Object* obj)
{
	bool added = add (obj);
	
	// adjust an iterator that is already beyond the last element
	if(iterator && iterator->done ())
		iterator->last ();
	
	return added;
}

//************************************************************************************************
// ObjectListIterator
//************************************************************************************************

ObjectListIterator::ObjectListIterator (const ObjectList& list)
: ListIterator<Object*> (list)
{
	if(list.iterator == nullptr)
		const_cast<ObjectList&> (list).iterator = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectListIterator::~ObjectListIterator ()
{
	if(static_cast<const ObjectList&> (list).iterator == this)
		const_cast<ObjectList&> (static_cast<const ObjectList&> (list)).iterator = nullptr;
}
