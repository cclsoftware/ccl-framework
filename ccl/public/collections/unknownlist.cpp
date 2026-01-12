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
// Filename    : ccl/public/collections/unknownlist.cpp
// Description : IUnknown List
//
//************************************************************************************************

#include "ccl/public/collections/unknownlist.h"

#include "ccl/public/base/iarrayobject.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IContainer, 0x703469C0, 0x5C71, 0x4488, 0x9C, 0x9F, 0x93, 0xDB, 0x43, 0x5E, 0x72, 0xE8)
DEFINE_IID_ (IUnknownList, 0x462f2bf1, 0x256f, 0x402f, 0xa5, 0x75, 0xf6, 0x2c, 0x55, 0x61, 0x5f, 0x71) 
DEFINE_IID_ (IUnknownIterator, 0xcec32585, 0x7e3f, 0x44a2, 0xbd, 0x59, 0xd1, 0xd8, 0xcc, 0xb2, 0x58, 0x7d) 

//************************************************************************************************
// UnknownList
//************************************************************************************************

IUnknownList* UnknownList::convert (IUnknown* unknown)
{
	if(unknown == nullptr)
		return nullptr;

	UnknownPtr<IUnknownList> unknownList (unknown);
	if(unknownList)
	{
		unknownList->retain ();
		return unknownList;
	}

	UnknownPtr<IArrayObject> arrayObject (unknown);
	if(arrayObject)
	{
		UnknownList* list = NEW UnknownList;
		int length = arrayObject->getArrayLength ();
		for(int i = 0; i < length; i++)
		{
			Variant v;
			arrayObject->getArrayElement (v, i);
			IUnknown* element = v.asUnknown ();
			ASSERT (element != nullptr)
			if(element)
				list->add (element, true);
		}
		return list;
	}

	UnknownList* list = NEW UnknownList;
	list->add (unknown, true);
	return list;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnknownList::~UnknownList ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API UnknownList::createIterator () const
{
	return NEW UnknownIterator (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnknownList::isEmpty () const
{
	return list.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API UnknownList::getFirst () const
{
	return list.getFirst ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API UnknownList::getLast () const
{
	return list.getLast ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnknownList::contains (IUnknown* object) const
{
	ListForEach (list, IUnknown*, obj)
		if(isEqualUnknown (obj, object))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnknownList::add (IUnknown* object, tbool share)
{
	ASSERT (object != nullptr)
	list.append (object);
	if(share)
		object->retain ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnknownList::remove (IUnknown* object)
{
	return list.remove (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnknownList::removeAll ()
{
	ListForEach (list, IUnknown*, object)
		object->release ();
	EndFor
	list.removeAll ();
}

//************************************************************************************************
// UnknownIterator
//************************************************************************************************

UnknownIterator::UnknownIterator (const UnknownList& list)
: ListIterator (list.list)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnknownIterator::done () const
{
	return ListIterator::done ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API UnknownIterator::nextUnknown ()
{
	return next ();
}
