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
// Filename    : ccl/base/collections/objecthashtable.h
// Description : Object Hash table
//
//************************************************************************************************

#ifndef _ccl_objecthashtable_h
#define _ccl_objecthashtable_h

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/collections/hashtable.h"

namespace CCL {

//************************************************************************************************
// ObjectHashTable
/** Container class for hash table of objects. \ingroup base_collect */
//************************************************************************************************

class ObjectHashTable: public Object,
					   public HashTable<Object*, ObjectList>
{
public:
	DECLARE_CLASS (ObjectHashTable, Object)

	ObjectHashTable (int size = 512);

	Object* lookup (const Object& obj) const;

protected:
	static int hashObject (Object* const& obj, int size);
};

} // namespace CCL

#endif // _ccl_objecthashtable_h
