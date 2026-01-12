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
// Filename    : ccl/base/collections/linkablelist.cpp
// Description : Linkable Object List
//
//************************************************************************************************

#include "ccl/base/collections/linkablelist.h"

using namespace CCL;

//************************************************************************************************
// Linkable
//************************************************************************************************

DEFINE_CLASS (Linkable, Object)
DEFINE_CLASS_NAMESPACE (Linkable, NAMESPACE_CCL)
	
//************************************************************************************************
// LinkableList
//************************************************************************************************

int LinkableList::index (const Object& searchObj) const
{
	int idx = 0;
	ListForEachLinkableFast (*this, Linkable, obj)
		if(obj->equals (searchObj))
			return idx;
		idx++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LinkableList::index (const Object* searchObj) const
{
	const Linkable* link = ccl_cast<Linkable> (searchObj);
	if(!link)
		return -1;

	// test if not linked 
	if(link->getPrevious () == nullptr)
		return getFirst () == link ? 0 : -1;

	int idx = 0;
	ListForEachLinkableFast (*this, Linkable, obj)
		if(obj == searchObj)
			return idx;
		idx++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkableList::add (Object* obj)
{
	Linkable* link = ccl_cast<Linkable> (obj);
	ASSERT (link != nullptr)
	if(link)
	{
		IntrusiveLinkedList<Linkable>::append (link);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkableList::remove (Object* obj)
{
	Linkable* link = ccl_cast<Linkable> (obj);
	ASSERT (link != nullptr)
	return link ? IntrusiveLinkedList<Linkable>::remove (link) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinkableList::removeAll ()
{
	if(isObjectCleanup ())
	{
		ListForEachLinkable (*this, Linkable, obj)
			unsigned int refCount = obj->release ();
			if(refCount)
			{
				obj->setNext (nullptr);
				obj->setPrevious (nullptr);
			}
		EndFor
	}
	
	IntrusiveLinkedList<Linkable>::removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* LinkableList::findEqual (const Object& searchObj) const
{
	ListForEachLinkableFast (*this, Linkable, obj)
		if(obj->equals (searchObj))
			return obj;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkableList::addSorted (Object* addObj)
{
	Linkable* linkToAdd = ccl_cast<Linkable> (addObj);
	ASSERT (linkToAdd != nullptr)
	if(!linkToAdd)
		return false;

	return IntrusiveLinkedList<Linkable>::addSorted (linkToAdd);
}
