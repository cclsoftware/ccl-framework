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
// Filename    : ccl/base/collections/container.cpp
// Description : Container class
//
//************************************************************************************************

#include "ccl/base/collections/container.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/storage.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* CCL::createConcatenatedIterator (Iterator* iterator1, Iterator* iterator2)
{
	if(iterator1 && iterator2)
	{
		AutoPtr<ObjectArray> iterators (NEW ObjectArray (2));
		iterators->objectCleanup (true);
		iterators->add (iterator1);
		iterators->add (iterator2);
		return NEW HoldingIterator (iterators, makeCascadedIterator (iterators->newIterator (), [] (Object* obj) { return return_shared (static_cast<Iterator*> (obj)); }));
	}
	else
		return iterator1 ? iterator1 : iterator2;
}

//************************************************************************************************
// Iterator
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (Iterator, Object)
DEFINE_CLASS_NAMESPACE (Iterator, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Iterator::nextUnknown ()
{
	Object* obj = next ();
	return obj ? obj->asUnknown () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Iterator)
	DEFINE_METHOD_NAME ("done")
	DEFINE_METHOD_NAME ("first")
	DEFINE_METHOD_NAME ("last")
	DEFINE_METHOD_NAME ("next")
	DEFINE_METHOD_NAME ("previous")
END_METHOD_NAMES (Iterator)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Iterator::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "done")
	{
		returnValue = done ();
		return true;
	}
	else if(msg == "first")
	{
		first ();
		return true;
	}
	else if(msg == "last")
	{
		last ();
		return true;
	}
	else if(msg == "next")
	{
		returnValue = static_cast<IObject*> (next ());
		return true;
	}
	else if(msg== "previous")
	{
		returnValue = static_cast<IObject*> (previous ());
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Container
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Container, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Container::Container ()
: flags (0) 
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Container::Container (const Container& objects)
{
	// can not be used because class is abstract!
	ASSERT (0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Container::copyFrom (const Container& objects)
{
	add (objects, kClone);
	objectCleanup (objects.isObjectCleanup ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Container::objectCleanup (bool state)
{
	if(state)
		flags |= kCleanup;
	else
		flags &= ~kCleanup;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Container::isObjectCleanup () const
{
	return (flags & kCleanup) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Container::add (const Container& objects, CopyMode mode)
{
	ForEach (objects, Object, obj)
		if(mode == kClone)
		{
			Object* twin = obj->clone ();
			ASSERT (twin != nullptr)
			if(twin)
				add (twin);
		}
		else
		{
			add (obj);
			if(mode == kShare)
				obj->retain ();
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API Container::createIterator () const
{
	return newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Container::load (const Storage& storage)
{
	objectCleanup ();

	Attributes& attr = storage.getAttributes ();
	Object* obj = nullptr;
	while((obj = attr.unqueueObject ("items")) != nullptr)
		add (obj);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Container::save (const Storage& storage) const
{
	ASSERT (flags & kCleanup)

	Attributes& attr = storage.getAttributes ();

	attr.setAttribute ("count", count ());
	ForEach (*this, Object, obj)
		attr.queue ("items", obj);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Container)
	DEFINE_PROPERTY_NAME ("count")
END_PROPERTY_NAMES (Container)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Container::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "count")
	{
		var = count ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Container)
	DEFINE_METHOD_ARGR ("at", "index", "Object")
	DEFINE_METHOD_ARGR ("findEqual", "object_or_string", "Object")
	DEFINE_METHOD_ARGR ("newIterator", nullptr, "Iterator")
END_METHOD_NAMES (Container)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Container::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "at")
	{
		int index = msg[0].asInt ();
		returnValue = static_cast<IObject*> (at (index));
		return true;
	}
	else if(msg == "findEqual")
	{
		Object* keyObject = unknown_cast<Object> (msg[0].asUnknown ());
		if(keyObject)
			returnValue = static_cast<IObject*> (findEqual (*keyObject));
		else
		{
			String keyString (msg[0].asString ());
			if(!keyString.isEmpty ())
				ForEach (*this, Object, object)
					String string;
					if(object->toString (string) && string == keyString)
					{
						returnValue = static_cast<IObject*> (object);
						break;
					}
				EndFor
		}
		return true;
	}
	else if(msg == "newIterator")
	{
		AutoPtr<Iterator> iter;
		if(Iterator* it = newIterator ())
			iter = NEW HoldingIterator (this, it);
		returnValue.takeShared (ccl_as_unknown (iter));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
