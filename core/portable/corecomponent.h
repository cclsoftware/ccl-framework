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
// Filename    : core/portable/corecomponent.h
// Description : Component class
//
//************************************************************************************************

#ifndef _corecomponent_h
#define _corecomponent_h

#include "core/portable/coreparams.h"
#include "core/portable/corecomponentflags.h"

#include "core/public/coreuid.h"

namespace Core {
namespace Portable {
	
class InputStorage;
class OutputStorage;
class IStorageFilter;
class Component;
	
//************************************************************************************************
// ComponentArray
//************************************************************************************************

class ComponentArray
{
public:
	ComponentArray (int total): total (total) {}	
	virtual ~ComponentArray () {}

	INLINE int count () const { return total; }
	virtual Component* at (int index) const = 0;

protected:
	int total;
};

//************************************************************************************************
// TComponentArray
//************************************************************************************************

template <class ComponentClass>
class TComponentArray: public ComponentArray
{
public:
	TComponentArray (int total)
	: ComponentArray (total)
	{
		components = NEW ComponentClass[total];
	}

	~TComponentArray ()
	{
		delete [] components;
	}

	// ComponentArray
	Component* at (int index) const override
	{
		return &components[index];
	}

protected:
	ComponentClass* components;
};

//************************************************************************************************
// Component
/** Basic component class for embedded systems. This design pattern structures program functionality
	into smaller and reusable pieces. Components and their childs make up the parameter tree attached to
	a graphical user interface or hardware front panel.
	\ingroup core_portable */
//************************************************************************************************

class Component: public TypedObject,
				 public IParamObserver
{
public:
	BEGIN_CORE_CLASS ('Comp', Component)
		ADD_CORE_CLASS_ (IParamObserver)
	END_CORE_CLASS (TypedObject)

	Component (CStringPtr name = "");
	~Component ();

	void addParameters (const ParamInfo infos[], int count, bool ownsInfo = false);
	virtual void construct (); ///< can be called for initialization after parameters have been added

	enum Flags
	{
		kMutable = ComponentFlags::kMutable,
		kSaveDisabled = ComponentFlags::kSaveDisabled,
		kOwnedByArray = ComponentFlags::kOwnedByArray
	};

	// Identity
	PROPERTY_CSTRING_BUFFER (ParamInfo::kMaxNameLength, name, Name)
	PROPERTY_FLAG (flags, kMutable, isMutable)
	PROPERTY_FLAG (flags, kSaveDisabled, isSaveDisabled)
	PROPERTY_READONLY_FLAG (flags, kOwnedByArray, isOwnedByArray)
	virtual UIDRef getClassID () const;
	static CStringPtr kClassIDAttr;

	// Nesting
	Component* getParent () const;
	Component* getRootComponent () const;
	void addChild (Component* c);
	void addChildren (ComponentArray* components);
	void removeChild (Component* c);
	bool insertChildAt (int index, Component* c);
	int countChildren () const;
	Component* getChild (int index) const;
	int getChildIndex (const Component* c) const;
	virtual Component* findChild (CStringPtr name) const; ///< override to make non-child components accessible
	Component* lookupChild (CStringPtr path) const;
	void getComponentPath (CString256& path) const;

	template <class T> T* getCoreComponent (CStringPtr name) const; ///< uses core_cast
	template <class T> T* getParentOfCoreType () const; ///< uses core_cast

	struct Visitor;
	void traverse (Visitor& visitor, bool deep);

	// Parameters
	int countParameters () const;
	Parameter* getParameterAt (int index) const;
	Parameter* getParameterByTag (int tag) const;
	virtual Parameter* findParameter (CStringPtr name) const; ///< override to make non-owned parameters accessible
	Parameter* lookupParameter (CStringPtr path) const;
	void getParameterPath (CString256& path, Parameter* p) const;
	virtual void resetToDefaults (bool storableOnly = true, bool edit = false, const IStorageFilter* filter = nullptr);
	void paramChanged (Parameter* p, int msg) override; // IParamObserver

	// Persistence
	virtual bool load (const InputStorage& storage);
	virtual bool save (OutputStorage& storage) const;
	bool hasSaveData () const;
	bool needsSave () const;
	void setNeedsSave (bool needsSave);
	PROPERTY_VARIABLE (int, storageTag, StorageTag)

	virtual void onIdle (); ///< delegate idle to children

	static abs_time getLastEditTime ();

protected:
	static abs_time lastEditTime;
	ParamList paramList;
	Component* parent;
	Vector<Component*> children;

	void updateLastEditTime ();

private:
	int flags;
	bool dirty;
	Vector<ComponentArray*> childArrays;
};

//************************************************************************************************
// Component::Visitor
//************************************************************************************************

struct Component::Visitor
{
	virtual void visit (Component* component) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Component inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Component* Component::getParent () const { return parent; }
inline int Component::countChildren () const { return children.count (); }
inline Component* Component::getChild (int index) const { return children.at (index); }
inline int Component::getChildIndex (const Component* c) const { return children.index (const_cast<Component*> (c)); }

template <class T> inline T* Component::getCoreComponent (CStringPtr name) const
{
	return core_cast<T> (findChild (name));
}

template <class T> inline T* Component::getParentOfCoreType () const
{
	for(Component* p = parent; p != 0; p = p->parent)
		if(T* result = core_cast<T> (p))
			return result;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _corecomponent_h
