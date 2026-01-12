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
// Filename    : ccl/public/collections/hashmap.h
// Description : Hash map
//
//************************************************************************************************

#ifndef _ccl_hashmap_h
#define _ccl_hashmap_h

#include "core/public/corehashmap.h"

#include "ccl/public/base/primitives.h"

namespace CCL {

using Core::HashMap;
using Core::HashMapIterator;

//************************************************************************************************
// PointerHashMap
//************************************************************************************************

template <class TValue>
class PointerHashMap: public HashMap<const void*, TValue>
{
public:
	static int hashFunction (const void* const& key, int size)
	{
		return ccl_hash_pointer (key, size);
	}

	using typename HashMap<const void*, TValue>::HashFunc;

	typedef HashMapIterator<const void*, TValue> Iterator;
	typedef typename HashMap<const void*, TValue>::TAssociation Association;

	PointerHashMap (int size = 512, HashFunc hashFunc = hashFunction)
	: HashMap<const void*, TValue> (size, hashFunc)
	{}
};

} // namespace CCL

#endif // _ccl_hashmap_h
