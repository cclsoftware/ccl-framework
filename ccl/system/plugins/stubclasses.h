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
// Filename    : ccl/system/plugins/stubclasses.h
// Description : Stub classes
//
//************************************************************************************************

#ifndef _ccl_stubclasses_h
#define _ccl_stubclasses_h

#include "ccl/base/boxedtypes.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objecthashtable.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/plugins/icomponent.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/storage/ipersistattributes.h"

namespace CCL {

interface IArrayObject;

//************************************************************************************************
// StubFactory
//************************************************************************************************

class StubFactory: public Object
{
public:
	StubFactory ();

	static StubFactory& instance ();

	bool addClass (UIDRef iid, StringID name, StubConstructor constructor, bool failIfExists = true);
	bool removeClass (UIDRef iid);
	StubConstructor lookupClass (UIDRef iid);
	Boxed::UID* lookupInterface (StringID name);

	IStubObject* createStubInstance (UIDRef iid, IObject* object, IUnknown* outerUnknown);

	// IObject
	tbool CCL_API getPropertyNames (IPropertyCollector& collector) const override;

protected:
	class ClassEntry: public Object
	{
	public:
		Boxed::UID iid;
		MutableCString name;
		StubConstructor constructor;

		ClassEntry (UIDRef iid = kNullUID,
					StringID name = nullptr,
					StubConstructor constructor = nullptr)
		: iid (iid),
		  name (name),
		  constructor (constructor)
		{}

		// Object
		bool equals (const Object& obj) const override;
		int getHashCode (int size) const override;
	};

	ObjectArray classes;
	ObjectHashTable classTable;

	ClassEntry* lookupEntry (UIDRef iid);
};

//************************************************************************************************
// GenericStub
//************************************************************************************************

class GenericStub: public Object,
				   public IOuterUnknown,
				   public PluginInstance
{
public:
	DECLARE_CLASS_ABSTRACT (GenericStub, Object)

	GenericStub (IObject* object);
	~GenericStub ();

	// IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	UNKNOWN_REFCOUNT

	// IOuterUnknown
	IUnknown* CCL_API getInnerUnknown () override;

	// IObject
	const ITypeInfo& CCL_API getTypeInfo () const override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

protected:
	struct InterfaceEntry
	{
		UIDBytes iid;
		IStubObject* innerUnknown;

		InterfaceEntry (UIDRef iid = kNullUID, 
						IStubObject* innerUnknown = nullptr)
		: iid (iid),
		  innerUnknown (innerUnknown)
		{}

		void* getInterfacePointer ();
	};

	IObject* object;
	LinkedList<InterfaceEntry> interfaces;

	InterfaceEntry addInterface (UIDRef iid, IStubObject* innerUnknown);
};

//************************************************************************************************
// ClassFactoryStub
//************************************************************************************************

class ClassFactoryStub: public StubObject,
						public IClassFactory
{
public:
	DECLARE_STUB_METHODS (IClassFactory, ClassFactoryStub)

	// IClassFactory
	void CCL_API getVersion (VersionDesc& version) const override;
	int CCL_API getNumClasses () const override;
	tbool CCL_API getClassDescription (ClassDesc& description, int index) const override;
	tbool CCL_API getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const override;
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;
};

//************************************************************************************************
// ComponentStub
//************************************************************************************************

class ComponentStub: public StubObject,
					 public IComponent
{
public:
	DECLARE_STUB_METHODS (IComponent, ComponentStub)

	// IComponent
	tresult CCL_API initialize (IUnknown* context) override;
	tresult CCL_API terminate () override;
	tbool CCL_API canTerminate () const override;
};

//************************************************************************************************
// NestingStub
//************************************************************************************************

class NestingStub: public StubObject,
				   public AbstractNode
{
public:
	DECLARE_STUB_METHODS (IObjectNode, NestingStub)

	// IObjectNode
	StringRef CCL_API getObjectID () const override;
	IObjectNode* CCL_API getParent () const override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tbool CCL_API getChildDelegates (IMutableArray& delegates) const override;

protected:
	mutable String objectID;
	IArrayObject* accessChildren () const;
};

//************************************************************************************************
// ObserverStub
//************************************************************************************************

class ObserverStub: public StubObject,
					public IObserver
{
public:
	DECLARE_STUB_METHODS (IObserver, ObserverStub)

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// PersistAttributesStub
//************************************************************************************************

class PersistAttributesStub: public StubObject,
							 public IPersistAttributes
{
public:
	DECLARE_STUB_METHODS (IPersistAttributes, PersistAttributesStub)

	// IPersistAttributes
	tresult CCL_API storeValues (IAttributeList& values) const override;
	tresult CCL_API restoreValues (const IAttributeList& values) override;
};

} // namespace CCL

#endif // _ccl_stubclasses_h
