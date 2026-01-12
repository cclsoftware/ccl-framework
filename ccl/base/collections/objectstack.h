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
// Filename    : ccl/base/collections/objectstack.h
// Description : Object Stack
//
//************************************************************************************************

#ifndef _ccl_objectstack_h
#define _ccl_objectstack_h

#include "ccl/base/collections/objectlist.h"

namespace CCL {

//************************************************************************************************
// ObjectStack
//************************************************************************************************

class ObjectStack: public ObjectList
{
public:
	DECLARE_CLASS (ObjectStack, ObjectList)

	void push (Object* obj)	{ prepend (obj); }
	Object* pop ()			{ return removeFirst (); }
	Object* peek () const	{ return getFirst (); }
};

} // namespace CCL

#endif // _ccl_objectstack_h
