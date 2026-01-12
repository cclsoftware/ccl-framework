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
// Filename    : ccl/base/collections/objectarray.cpp
// Description : Object Array
//
//************************************************************************************************

#include "ccl/base/collections/objectarray.h"

using namespace CCL;

//************************************************************************************************
// ObjectArray
//************************************************************************************************

int ObjectArray::compareObjects (const void* e1, const void* e2)
{
	Object* obj1 = *(Object**)e1;
	Object* obj2 = *(Object**)e2;
	return obj1->compare (*obj2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ObjectArray, Container)
DEFINE_CLASS_NAMESPACE (ObjectArray, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectArray::sort ()
{
	if(total > 0)
		::qsort (items, total, sizeof(Object*), compareObjects);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectArray::sort (CompareFunction function)
{
	if(total > 0)
		::qsort (items, total, sizeof(Object*), function);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ObjectArray::search (const Object& obj) const
{
	const Object* objPtr = &obj;
	Object** result = nullptr;
	if(total > 0)
		result = (Object**)::bsearch (&objPtr, items, total, sizeof(Object*), compareObjects);
	return result ? *result : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectArray::searchIndex (const Object& obj) const
{
	const Object* objPtr = &obj;
	Object** result = nullptr;
	if(total > 0)
		result = (Object**)::bsearch (&objPtr, items, total, sizeof(Object*), compareObjects);
	return result ? (int)(result - items) : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectArray::index (const Object& searchObj) const
{
	int idx = 0;
	VectorForEachFast (*this, Object*, obj)
		if(obj->equals (searchObj))
			return idx;
		idx++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectArray::index (const Object* searchObj) const
{
	int idx = 0;
	VectorForEachFast (*this, Object*, obj)
		if(obj == searchObj)
			return idx;
		idx++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectArray::replaceAt (int index, Object* obj)
{
	if(!isValidIndex (index))
		return false;

	if(isObjectCleanup () && items[index] != nullptr)
		items[index]->release ();

	items[index] = obj;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectArray::removeAll ()
{
	if(isObjectCleanup ())
	{
		VectorForEach (*this, Object*, obj)
			if(obj)
				obj->release ();
		EndFor
	}
	
	Vector<Object*>::resize (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ObjectArray::findEqual (const Object& searchObj) const
{
	VectorForEachFast (*this, Object*, obj)
		if(obj->equals (searchObj))
			return obj;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectArray::addSorted (Object* addObj)
{
	for(int i = 0; i < total; i++)
		if(items[i]->compare (*addObj) > 0)
			return insertAt (i, addObj);
	return add (addObj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectArray::addSorted (Object* addObj, CompareFunction function, bool reversed)
{
	for(int i = 0; i < total; i++)
	{
		int cmpResult = function (&items[i], &addObj);
		if(reversed)
			cmpResult *= -1;
		if(cmpResult > 0)
			return insertAt (i, addObj);
	}
	return add (addObj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectArray::getInsertIndex (const Object* obj, CompareFunction function) const
{
	int left = 0;
	int right = count () - 1;

	// before first
	if(isEmpty () || function (&obj, &items[0]) <= 0)
		return 0;
	
	// after last
	int cmpResult = function (&obj, &items[right]);
	if(cmpResult >= 0)
		return cmpResult == 0 ? right : right + 1;

	// binary search
	while(left <= right)
	{
		int mid = ((right - left) / 2) + left;
		int cmpResult = function (&items[mid], &obj);

		if(cmpResult < 0) // items[mid] < obj
			left = mid + 1;
		else if(cmpResult > 0)
			right = mid - 1; // items[mid] > obj
		else
			return mid;
	}
	return left;
}
