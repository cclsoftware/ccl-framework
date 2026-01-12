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
// Filename    : ccl/system/persistence/objectcache.h
// Description : Object cache
//
//************************************************************************************************

#ifndef _ccl_objectcache_h
#define _ccl_objectcache_h

#include "ccl/public/system/ipersistentstore.h"

#include "ccl/public/collections/vector.h"
#include "ccl/base/object.h"

namespace CCL {
namespace Persistence {

//************************************************************************************************
// ObjectCache
//************************************************************************************************

class ObjectCache: public Object
{
public:
	ObjectCache ();
	~ObjectCache ();

	void addObject (IPersistentObject* object);
	void removeObject (IPersistentObject* object);

	IPersistentObject* lookup (ObjectID oid);

private:
	Vector<IPersistentObject*> entries;
};

} // namespace Persistence
} // namespace CCL

#endif // _ccl_objectcache_h
