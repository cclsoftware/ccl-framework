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
// Filename    : core/portable/corecomponent.cpp
// Description : Component class
//
//************************************************************************************************

#include "corecomponent.h"
#include "corecontrollershared.h"
#include "corestorage.h"

#include "core/system/coredebug.h"
#include "core/system/coretime.h"
#include "core/system/corethread.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// Component
//************************************************************************************************

CStringPtr Component::kClassIDAttr = "__classid"; ///< mutable component class identifier (keep in sync with CCL)
abs_time Component::lastEditTime = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////

Component::Component (CStringPtr name)
: name (name),
  parent (nullptr),
  dirty (false),
  flags (0),
  storageTag (0)
{
	paramList.setController (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component::~Component ()
{
	VectorForEach (children, Component*, c)
		if(!c->isOwnedByArray ())
			delete c;
	EndFor

	if(!childArrays.isEmpty ())
		VectorForEach (childArrays, ComponentArray*, components)
			delete components;
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::addParameters (const ParamInfo infos[], int count, bool ownsInfo)
{
	paramList.add (infos, count, ownsInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::construct ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef Component::getClassID () const
{
	return kNullUID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* Component::getRootComponent () const
{
	const Component* c = this;
	while(true)
	{
		if(c->parent == nullptr)
			return const_cast<Component*> (c);
		c = c->parent;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::addChild (Component* c)
{
	ASSERT (c && !c->parent)
	c->parent = this;
	children.add (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::addChildren (ComponentArray* components)
{
	children.resize (children.count () + components->count ());
	for(int i = 0; i < components->count (); i++)
	{
		Component* c = components->at (i);
		c->flags |= kOwnedByArray;
		c->parent = this;
		children.add (c);
	}
	childArrays.add (components);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::removeChild (Component* c)
{
	ASSERT (c && c->parent == this)
	//ASSERT (!c->isOwnedByArray ()) - caller must not release component owned by array!
	if(children.remove (c))
		c->parent = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::insertChildAt (int index, Component* c)
{
	ASSERT (c && !c->parent)
	if(!children.insertAt (index, c))
		return false;
	c->parent = this;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* Component::findChild (CStringPtr _name) const
{
	ConstString name (_name);

	VectorForEach (children, Component*, c)
		if(c->getName () == name)
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* Component::lookupChild (CStringPtr path) const
{
	return TControllerFinder<Component>::lookup (this, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::getComponentPath (CString256& path) const
{
	path.empty ();
	const Component* current = this;
	while(current)
	{
		ConstString name (current->getName ());
		if(name.isEmpty ())
			break;

		if(!path.isEmpty ())
			path.insert (0, "/");

		path.insert (0, name);
		current = current->getParent ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::traverse (Visitor& visitor, bool deep)
{
	VectorForEach (children, Component*, c)
		visitor.visit (c);
		if(deep == true)
			c->traverse (visitor, true);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Component::countParameters () const
{
	return paramList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* Component::getParameterAt (int index) const
{
	return paramList.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* Component::getParameterByTag (int tag) const
{
	return paramList.byTag (tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* Component::findParameter (CStringPtr name) const
{
	return paramList.find (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* Component::lookupParameter (CStringPtr _path) const
{
	Parameter* p = nullptr;

	ParamPath64 path (_path);
	if(!path.childName.isEmpty ())
	{
		// use optimized tokenizer for faster lookup
		Component* child = TControllerFinder<Component>::lookupInplace (this, path.childName.getBuffer ());
		if(child)
			p = child->findParameter (path.paramName);
	}
	else
		p = findParameter (path.paramName);

	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::getParameterPath (CString256& path, Parameter* p) const
{
	getComponentPath (path);
	if(!path.isEmpty ())
		path += "/";
	path += p->getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Storage Analysis

#define SIMULATE_LONG_STORAGE (0 && CORE_PLATFORM_WINDOWS && DEBUG)
#define LONG_STORAGE_DELAY 10

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::load (const InputStorage& storage)
{
	//if(storage.isCanceled ()) not used
	//	return false;
	
#if SIMULATE_LONG_STORAGE
	abs_time now = SystemClock::getMilliseconds ();
	while(SystemClock::getMilliseconds () - now < LONG_STORAGE_DELAY)
		;
#endif	

	if(paramList.hasStorableParams ())
		paramList.restoreValues (storage, true);

	const Attributes& a = storage.getAttributes ();
	const IStorageFilter* filter = storage.getFilter ();

	VectorForEachFast (children, Component*, c)
		if(filter && !filter->shouldLoad (Component::kTypeID, c->getName (), c))
			continue;

		if(Attributes* a2 = a.getAttributes (c->getName ()))
		{
			InputStorage childStorage (*a2, storage);
			childStorage.setParentAttributes (&a);
			if(!c->load (childStorage))
				return false;
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::save (OutputStorage& storage) const
{
	if(storage.isCanceled ())
		return false;
		
#if SIMULATE_LONG_STORAGE
	abs_time now = SystemClock::getMilliseconds ();
	while (SystemClock::getMilliseconds () - now < LONG_STORAGE_DELAY)
		;
#endif	

	if(paramList.hasStorableParams ())
		paramList.storeValues (storage);

	AttributeHandler& writer = storage.getWriter ();
	const IStorageFilter* filter = storage.getFilter ();

	// save class identifier of mutable components
	if(isMutable () == true && getClassID () != kNullUID)
	{
		CString128 cidString;
		UIDBytes cid = getClassID ();
		cid.toCString (cidString.getBuffer (), cidString.getSize ());
		writer.setValue (kClassIDAttr, cidString, Attribute::kShareID);
	}

	Threads::ScopedLock lock (storage.getLock ()); // protect children when storage lock is set

	VectorForEachFast (children, Component*, c)
		if(c->isSaveDisabled () || !c->hasSaveData ()) // avoid recursion if no data expected
			continue;

		if(filter && !filter->shouldSave (Component::kTypeID, c->getName (), c))
			continue;

		writer.startObject (c->getName ());
		bool saved = c->save (storage);
		writer.endObject (c->getName ());
		
		if(!saved)
			return false;
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::hasSaveData () const
{
	return paramList.hasStorableParams () || !children.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::needsSave () const
{
	if(dirty)
		return true;
	
	VectorForEachFast (children, Component*, c)
		if(c->needsSave ())
			return true;
	EndFor
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::setNeedsSave (bool needsSave)
{
	dirty = needsSave;
	
	if(!needsSave) // only do this if we're CLEARING the dirty...
	{
		VectorForEachFast (children, Component*, c)
			c->setNeedsSave (needsSave);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::paramChanged (Parameter* p, int msg)
{
	if(msg == Parameter::kChanged)
	{
		if(p->isStorable ())
		{
			dirty = true;
			updateLastEditTime ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION void Component::onIdle ()
{
	VectorForEachFast (children, Component*, c)
		c->onIdle ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::resetToDefaults (bool storableOnly, bool edit, const IStorageFilter* filter)
{
	VectorForEach (paramList, Parameter*, p)
		if(storableOnly && !p->isStorable ())
			continue;
		if(!filter || filter->shouldLoad (Parameter::kTypeID, p->getName (), p))
			p->resetValue (edit);
	EndFor
	
	VectorForEachFast (children, Component*, c)
		if(!filter || filter->shouldLoad (Component::kTypeID, c->getName (), c))
			c->resetToDefaults (storableOnly, edit, filter);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

abs_time Component::getLastEditTime ()
{
	return lastEditTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::updateLastEditTime ()
{
	lastEditTime = SystemClock::getMilliseconds ();
}
