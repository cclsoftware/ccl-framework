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
// Filename    : ccl/app/componentfactory.h
// Description : Component Factory
//
//************************************************************************************************

#ifndef _ccl_componentfactory_h
#define _ccl_componentfactory_h

#include "ccl/app/component.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"

namespace CCL {

//************************************************************************************************
// IComponentExtender
/** Component extender interface. */
//************************************************************************************************

interface IComponentExtender: IUnknown
{
	/** Extend component. */
	virtual void extendComponent (Component* component) = 0;

	DECLARE_IID (IComponentExtender)
};

//************************************************************************************************
// ComponentExtender
/** Component extender base class. */
//************************************************************************************************

class ComponentExtender: public Object,
						 public IComponentExtender
{
public:
	DECLARE_CLASS_ABSTRACT (ComponentExtender, Object)

	CLASS_INTERFACE (IComponentExtender, Object)
};

//************************************************************************************************
// ComponentFactory
/** Light-weight factory for internal component classes. */
//************************************************************************************************

class ComponentFactory: public Object,
						public Singleton<ComponentFactory>
{
public:
	ComponentFactory ();
	~ComponentFactory ();

	/** Component creation function. */
	typedef Component* (*CreateFunc) (StringID name, Component* owner, Object* arg);

	/** Component extension function. */
	typedef void (*ExtendFunc) (Component* component);

	/** Register component class. */
	void registerComponentClass (StringID name, CreateFunc createFunc);

	/** Register component class (w/o name). */
	void registerComponentClass (CreateFunc createFunc);

	/** Unregister component class. */
	bool unregisterComponentClass (StringID name);

	/** Hide component class, i.e. register with null constructor. */
	void hideComponentClass (StringID name);
	bool isHiddenClass (StringID name) const;

	/** Check if class is registered. */
	bool isClassRegistered (StringID name) const;

	/** Create component instance. */
	Component* createComponent  (StringID name, Component* owner = nullptr, Object* arg = nullptr);
	
	/** Create component instance (template). */
	template <class T> T* createComponent (StringID name, Component* owner = nullptr, Object* arg = nullptr);

	/** Add component extender. */
	void addExtender (IComponentExtender* extender, bool owns = false);

	/** Add component extension function. */
	void addExtendFunction (ExtendFunc extendFunc);

	/** Remove component extender. */
	void removeExtender (IComponentExtender* extender);

	/** Extend component. */
	void extendComponent (Component* component);

protected:
	class ComponentClass: public Object
	{
	public:
		PROPERTY_MUTABLE_CSTRING (name, Name)
		PROPERTY_VARIABLE (CreateFunc, createFunc, CreateFunc)

		ComponentClass (StringID name = nullptr, 
						CreateFunc createFunc = nullptr)
		: name (name),
		  createFunc (createFunc)
		{}

		ComponentClass (CreateFunc createFunc)
		: createFunc (createFunc)
		{}
	};

	class FunctionExtender;
	class NullConstructor;

	ObjectList namedClasses;
	ObjectList unnamedClasses;
	CCL::LinkedList<IComponentExtender*> extenders;
	CCL::LinkedList<IComponentExtender*> ownedExtenders;

	ComponentClass* findNamedClass (StringID name) const;
};

//************************************************************************************************
// ComponentConstructor
/** Component constructor helper class. */
//************************************************************************************************

template <class T>
class ComponentConstructor
{
public:
	static Component* createInstance (StringID name, Component* owner, Object* arg)
	{
		return NEW T;
	}

	static void registerClass ()
	{
		ComponentFactory::instance ().registerComponentClass (ccl_typeid<T> ().getPersistentName (), createInstance);
	}

	static void registerClass (StringID name)
	{
		ComponentFactory::instance ().registerComponentClass (name, createInstance);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ComponentFactory inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T* ComponentFactory::createComponent (StringID name, Component* owner, Object* arg)
{
	Component* c = createComponent (name, owner, arg);
	T* c2 = ccl_cast<T> (c);
	if(c && !c2)
	{
		ASSERT (false) // ccl_cast<T> failed
		c->release ();
	}
	return c2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_componentfactory_h
