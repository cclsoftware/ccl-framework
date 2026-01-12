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
// Filename    : ccl/app/componentfactory.cpp
// Description : Component Factory
//
//************************************************************************************************

#include "ccl/app/componentfactory.h"

namespace CCL {

//************************************************************************************************
// ComponentFactory::FunctionExtender
//************************************************************************************************

class ComponentFactory::FunctionExtender: public ComponentExtender
{
public:
	FunctionExtender (ExtendFunc extendFunc)
	: extendFunc (extendFunc)
	{}
	 
	PROPERTY_VARIABLE (ExtendFunc, extendFunc, ExtendFunc)

	void extendComponent (Component* component) override
	{
		(*extendFunc) (component);
	}
};

//************************************************************************************************
// ComponentFactory::NullConstructor
//************************************************************************************************

class ComponentFactory::NullConstructor
{
public:
	static Component* createInstance (StringID name, Component* owner, Object* arg)
	{
		return nullptr;
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IComponentExtender, 0xde609c41, 0x5cd7, 0x4613, 0xb4, 0xc0, 0x3c, 0x12, 0x7e, 0xd8, 0xc5, 0x6)

//************************************************************************************************
// ComponentExtender
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ComponentExtender, Object)

//************************************************************************************************
// ComponentFactory
//************************************************************************************************

DEFINE_SINGLETON (ComponentFactory)

//////////////////////////////////////////////////////////////////////////////////////////////////

ComponentFactory::ComponentFactory ()
{
	namedClasses.objectCleanup (true);
	unnamedClasses.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ComponentFactory::~ComponentFactory ()
{
	ListForEach (ownedExtenders, IComponentExtender*, extender)
		extenders.remove (extender);
		extender->release ();
	EndFor
	
	ASSERT (extenders.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentFactory::registerComponentClass (StringID name, CreateFunc createFunc)
{
	ASSERT (name.isEmpty () == false && createFunc != nullptr)
	ASSERT (isClassRegistered (name) == false)
	namedClasses.add (NEW ComponentClass (name, createFunc));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ComponentFactory::ComponentClass* ComponentFactory::findNamedClass (StringID name) const
{
	ForEach (namedClasses, ComponentClass, c)
		if(c->getName () == name)
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentFactory::registerComponentClass (CreateFunc createFunc)
{
	ASSERT (createFunc != nullptr)
	unnamedClasses.add (NEW ComponentClass (createFunc));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComponentFactory::unregisterComponentClass (StringID name)
{
	if(ComponentClass* c = findNamedClass (name))
	{
		namedClasses.remove (c);
		c->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentFactory::hideComponentClass (StringID name)
{
	if(isHiddenClass (name) == false)
		registerComponentClass (name, NullConstructor::createInstance);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComponentFactory::isHiddenClass (StringID name) const
{
	if(ComponentClass* c = findNamedClass (name))
		if(c->getCreateFunc () == NullConstructor::createInstance)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComponentFactory::isClassRegistered (StringID name) const
{
	return findNamedClass (name) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* ComponentFactory::createComponent (StringID name, Component* owner, Object* arg)
{
	// try named classes...
	if(!name.isEmpty ())
	{
		if(ComponentClass* c = findNamedClass (name))
			return c->getCreateFunc () (name, owner, arg);
	}

	// try unnamed classes...
	ForEach (unnamedClasses, ComponentClass, c)
		Component* result = c->getCreateFunc () (name, owner, arg);
		if(result)
			return result;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentFactory::addExtender (IComponentExtender* extender, bool owns)
{
	extenders.append (extender);
	if(owns)
		ownedExtenders.append (extender);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentFactory::addExtendFunction (ExtendFunc extendFunc)
{
	addExtender (NEW FunctionExtender (extendFunc), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentFactory::removeExtender (IComponentExtender* extender)
{
	extenders.remove (extender);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComponentFactory::extendComponent (Component* component)
{
	ListForEach (extenders, IComponentExtender*, extender)
		extender->extendComponent (component);
	EndFor
}
