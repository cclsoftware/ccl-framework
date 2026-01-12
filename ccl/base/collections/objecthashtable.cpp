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
// Filename    : ccl/base/collections/objecthashtable.cpp
// Description : Object Hash table
//
//************************************************************************************************

#include "ccl/base/collections/objecthashtable.h"

using namespace CCL;

//************************************************************************************************
// ObjectHashTable
//************************************************************************************************

int ObjectHashTable::hashObject (Object* const& obj, int size)
{
	return obj->getHashCode (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ObjectHashTable, Object)
DEFINE_CLASS_NAMESPACE (ObjectHashTable, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectHashTable::ObjectHashTable (int size)
: HashTable<Object*, ObjectList> (size, hashObject)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ObjectHashTable::lookup (const Object& obj) const
{
	int idx = obj.getHashCode (size);
	return table[idx].findEqual (obj);
}
