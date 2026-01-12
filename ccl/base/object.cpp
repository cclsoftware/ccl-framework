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
// Filename    : ccl/base/object.cpp
// Description : Object class
//
//************************************************************************************************

#include "ccl/base/kernel.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IObjectCast, 0x96314a11, 0x726a, 0x4d00, 0x82, 0xa9, 0xe7, 0x10, 0xc7, 0xdf, 0xf9, 0x28)

//************************************************************************************************
// Object
//************************************************************************************************

#if CCL_DEBUG_INTERNAL
Object::~Object ()
{
	if(debugFlags & kDebugFlagHasObserver)
	{
		ASSERT (System::GetSignalHandler ().hasObservers (this) == false)
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

const void* Object::getModuleAddress ()
{
	// Note: The address of a static variable is unique in the address space of a process.
	// We use it to differentiate between multiple modules based on the same code.
	static const int staticVariable = 0;
	return &staticVariable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Object::addGarbageCollected (Object* obj, bool globalScope)
{
	Kernel::instance ().addObject (obj, globalScope);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Object::deferDestruction (Object* obj)
{
	Kernel::instance ().deferDestruction (ccl_as_unknown (obj));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_BASE_CLASS (Object)
DEFINE_CLASS_NAMESPACE (Object, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Object::queryInterface (UIDRef iid, void** ptr)
{
	if(ccl_iid<IObjectCast> ().equals (iid))
	{
		*ptr = static_cast<IObjectCast*> (this); // we don't call retain here!
		return kResultOk;
	}

	QUERY_INTERFACE (ISubject)
	QUERY_INTERFACE (IObserver)
	QUERY_INTERFACE (IObject)
	QUERY_UNKNOWN (ISubject)
	*ptr = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API Object::retain ()
{
	return Unknown::retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API Object::release ()
{
	return Unknown::release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* CCL_API Object::revealObject (const void* moduleAddress)
{
	if(moduleAddress == getModuleAddress ())
		return this;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Object::addObserver (IObserver* observer)
{
	#if CCL_DEBUG_INTERNAL
	if(debugFlags & kDebugFlagObserver)
	{
		if(Object* obj = unknown_cast<Object> (observer))
		{
			Debugger::printf ("%saddObserver %s to %s\n", 
							  Debugger::getIndent (),
							  getTypeInfo ().getClassName (), 
							  obj->getTypeInfo ().getClassName ());
		}
		Debugger::debugBreak ("addObserver\n");
	}
	debugFlags |= kDebugFlagHasObserver;
	#endif
	
	System::GetSignalHandler ().advise (this, observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Object::removeObserver (IObserver* observer)
{
	#if CCL_DEBUG_INTERNAL
	if(debugFlags & kDebugFlagObserver)
	{
		if(Object* obj = unknown_cast<Object> (observer))
		{
			Debugger::printf ("%sremoveObserver %s to %s\n",
							  Debugger::getIndent (),
							  getTypeInfo ().getClassName (),
							  obj->getTypeInfo ().getClassName ());
		}
		Debugger::debugBreak ("removeObserver\n");
	}
	#endif
	
	System::GetSignalHandler ().unadvise (this, observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Object::signal (MessageRef msg)
{
	System::GetSignalHandler ().performSignal (this, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Object::deferSignal (IMessage* msg)
{
	System::GetSignalHandler ().queueSignal (this, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Object::deferChanged ()
{
	System::GetSignalHandler ().queueChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Object::notify (ISubject* subject, MessageRef msg)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Object::cancelSignals ()
{
	System::GetSignalHandler ().cancelSignals (this);
	System::GetSignalHandler ().cancelMessages (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo& CCL_API Object::getTypeInfo () const
{
	return myClass ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Object::getProperty (Variant& var, MemberID propertyId) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Object::setProperty (MemberID propertyId, const Variant& var)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Object::getPropertyNames (IPropertyCollector& collector) const
{
	bool result = false;
	const ITypeInfo* typeInfo = &myClass ();
	while(typeInfo)
	{
		if(const MetaClass::PropertyDefinition* propertyNames = typeInfo->getPropertyNames ())
		{
			for(int i = 0; propertyNames[i].name != nullptr; i++)
			{
				collector.addProperty (propertyNames[i]);
				result = true;
			}
		}

		typeInfo = typeInfo->getParentType ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Object::invokeMethod (Variant& returnValue, MessageRef msg)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Object::equals (const Object& obj) const
{
	return &obj == this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Object::compare (const Object& obj) const
{
	return this == &obj ? 0 : this > &obj ? 1 : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Object::load (const Storage& storage)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Object::save (const Storage& storage) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Object::save (const OutputStorage& storage) const
{
	CCL_NOT_IMPL ("Save to output storage not implemented\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Object::toString (String& string, int flags) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Object::getHashCode (int size) const
{
	return ccl_hash_pointer (this, size);
}

