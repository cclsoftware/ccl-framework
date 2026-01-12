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
// Filename    : ccl/system/persistence/objectcache.cpp
// Description : Object cache
//
//************************************************************************************************

#include "ccl/system/persistence/objectcache.h"

using namespace CCL;
using namespace Persistence;

//************************************************************************************************
// ObjectCache
//************************************************************************************************

ObjectCache::ObjectCache ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectCache::~ObjectCache ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectCache::addObject (IPersistentObject* object)
{
	entries.add (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectCache::removeObject (IPersistentObject* object)
{
	entries.remove (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPersistentObject* ObjectCache::lookup (ObjectID oid)
{
	VectorForEachFast (entries, IPersistentObject*, object)
		ASSERT (object)
		if(object == nullptr)
			break; // should not happen, but we had a crash with unclear cause here

		if(object->getObjectID () == oid)
			return object;
	EndFor
	return nullptr;
}
