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
// Filename    : ccl/system/plugins/stubclasses.cpp
// Description : Stub classes
//
//************************************************************************************************

#include "ccl/system/plugins/stubclasses.h"
#include "ccl/system/plugins/plugmanager.h"

#include "ccl/base/message.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/system/iatomtable.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (StubFactory, kFrameworkLevelFirst)
{
	REGISTER_STUB_CLASS (IClassFactory, ClassFactoryStub)
	REGISTER_STUB_CLASS (IComponent, ComponentStub)
	REGISTER_STUB_CLASS (IObjectNode, NestingStub)
	REGISTER_STUB_CLASS (IObserver, ObserverStub)
	REGISTER_STUB_CLASS (IPersistAttributes, PersistAttributesStub)
	return true;
}

//************************************************************************************************
// StubFactory
//************************************************************************************************

bool StubFactory::ClassEntry::equals (const Object& obj) const
{
	return iid == ((const ClassEntry&)obj).iid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int StubFactory::ClassEntry::getHashCode (int size) const
{
	return iid.getHashCode (size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StubFactory& StubFactory::instance ()
{
	static StubFactory theRegistry;
	return theRegistry;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StubFactory::StubFactory ()
{
	classes.objectCleanup (true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool StubFactory::addClass (UIDRef iid, StringID name, StubConstructor constructor, bool failIfExists)
{
	if(failIfExists)
	{
		StubConstructor existing = lookupClass (iid);
		if(existing)
			return false;
	}

	ClassEntry* entry = NEW ClassEntry (iid, name, constructor);
	classes.add (entry);
	classTable.add (entry);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool StubFactory::removeClass (UIDRef iid)
{
	ClassEntry* entry = lookupEntry (iid);
	if(entry)
	{
		classes.remove (entry);
		classTable.remove (entry);
		entry->release ();
	}
	return entry != nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StubFactory::ClassEntry* StubFactory::lookupEntry (UIDRef iid)
{
	ClassEntry temp (iid);
	return (ClassEntry*)classTable.lookup (temp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StubConstructor StubFactory::lookupClass (UIDRef iid)
{
	ClassEntry* entry = lookupEntry (iid);
	return entry ? entry->constructor : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::UID* StubFactory::lookupInterface (StringID name)
{
	ForEach (classes, ClassEntry, entry)
		if(entry->name == name)
			return &entry->iid;
	EndFor
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IStubObject* StubFactory::createStubInstance (UIDRef iid, IObject* object, IUnknown* outerUnknown)
{
	StubConstructor constructor = lookupClass (iid);
	ASSERT (constructor != nullptr)
	return constructor ? (*constructor) (iid, object, outerUnknown) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StubFactory::getPropertyNames (IPropertyCollector& collector) const
{
	ForEach (classes, ClassEntry, entry)
		collector.addPropertyName (entry->name);
	EndFor
	return true;
}

//************************************************************************************************
// GenericStub
//************************************************************************************************

void* GenericStub::InterfaceEntry::getInterfacePointer ()
{
	void* ptr = nullptr;
	if(innerUnknown->stubQueryInterface (iid, &ptr) == kResultOk)
		innerUnknown->stubRelease ();
	return ptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (GenericStub, Object)

////////////////////////////////////////////////////////////////////////////////////////////////////

GenericStub::GenericStub (IObject* object)
: object (object)
{
	ASSERT (object != nullptr)
	if(object)
	{
		object->retain ();

		UnknownPtr<IInnerUnknown> innerUnknown (object);
		if(innerUnknown)
			innerUnknown->setOuterUnknown (asUnknown ());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

GenericStub::~GenericStub ()
{
	ListForEach (interfaces, InterfaceEntry, e)
		e.innerUnknown->stubRelease ();
	EndFor

	{
		UnknownPtr<IInnerUnknown> innerUnknown (object);
		if(innerUnknown)
			innerUnknown->setOuterUnknown (nullptr);
	}

	if(object)
		object->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API GenericStub::getInnerUnknown ()
{
	return object;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GenericStub::InterfaceEntry GenericStub::addInterface (UIDRef iid, IStubObject* innerUnknown)
{
	InterfaceEntry entry (iid, innerUnknown);
	interfaces.append (entry);
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GenericStub::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IPluginInstance)
	QUERY_INTERFACE (IObject)
	QUERY_INTERFACE (IOuterUnknown)
	QUERY_UNKNOWN (IObject)

	if(iid.equals (ccl_iid<IArrayObject> ()))
		return object->queryInterface (iid, ptr);

	ListForEach (interfaces, InterfaceEntry, e)
		if(e.iid == iid)
		{
			*ptr = e.getInterfacePointer ();
			if(*ptr)
			{
				retain ();
				return kResultOk;
			}
			return kResultNoInterface;
		}
	EndFor

	bool hasInterface = false;

	// 1) try to get interface array property...
	Variant interfaces;
	object->getProperty (interfaces, "interfaces");
	UnknownPtr<IArrayObject> iArrayObject (interfaces.asUnknown ());
	if(iArrayObject)
	{
		for(int index = 0; ; index++)
		{
			Variant iidVar;
			if(!iArrayObject->getArrayElement (iidVar, index))
				break;

			Boxed::UID* interfaceUid = unknown_cast<Boxed::UID> (iidVar.asUnknown ());
			if(!interfaceUid)
				break;

			if(iid.equals (*interfaceUid))
			{
				hasInterface = true;
				break;
			}
		}
	}
	// 2) ...or call queryInterface method...
	else
	{
		AutoPtr<Boxed::UID> interfaceUid = NEW Boxed::UID (iid);
		Variant returnValue;
		object->invokeMethod (returnValue, Message ("queryInterface", static_cast<IObject*> (interfaceUid)));
		hasInterface = returnValue.asUnknown () != nullptr;
	}

	if(hasInterface)
	{
		IStubObject* innerUnknown = StubFactory::instance ().createStubInstance (iid, object, asUnknown ());
		if(innerUnknown)
		{
			InterfaceEntry e = addInterface (iid, innerUnknown);
			*ptr = e.getInterfacePointer ();
			if(*ptr)
			{
				retain ();
				return kResultOk;
			}
			return kResultNoInterface;
		}
	}

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo& CCL_API GenericStub::getTypeInfo () const
{
	ASSERT (object != nullptr)
	return object ? object->getTypeInfo () : getTypeInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GenericStub::getProperty (Variant& var, MemberID propertyId) const
{
	ASSERT (object != nullptr)
	return object ? object->getProperty (var, propertyId) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GenericStub::setProperty (MemberID propertyId, const Variant& var)
{
	ASSERT (object != nullptr)
	return object ? object->setProperty (propertyId, var) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GenericStub::invokeMethod (Variant& returnValue, MessageRef msg)
{
	ASSERT (object != nullptr)
	return object ? object->invokeMethod (returnValue, msg) : false;
}

//************************************************************************************************
// ClassFactoryStub
//************************************************************************************************

void CCL_API ClassFactoryStub::getVersion (VersionDesc& version) const
{
	Variant v;
	object->getProperty (v, "version");
	UnknownPtr<IObject> versionObject (v);
	if(versionObject)
	{
		v.clear ();
		versionObject->getProperty (v, "name");
		version.name = v;
		v.clear ();
		versionObject->getProperty (v, "version");
		version.version = v;
		v.clear ();
		versionObject->getProperty (v, "vendor");
		version.vendor = v;
		v.clear ();
		versionObject->getProperty (v, "copyright");
		version.copyright = v;
		v.clear ();
		versionObject->getProperty (v, "url");
		version.url = v;
	}
	else
	{
		AutoPtr<VersionDescription> description = NEW VersionDescription;
		Variant returnValue;
		invokeMethod (returnValue, Message ("getVersion", description->asUnknown ()));
		description->toVersionDesc (version);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ClassFactoryStub::getNumClasses () const
{
	Variant classes;
	object->getProperty (classes, "classes");
	UnknownPtr<IArrayObject> iArrayObject (classes.asUnknown ());
	if(iArrayObject)
	{
		return iArrayObject->getArrayLength ();
	}
	else
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("getNumClasses"));
		return returnValue.asInt ();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassFactoryStub::getClassDescription (ClassDesc& description, int index) const
{
	Variant classes;
	object->getProperty (classes, "classes");
	UnknownPtr<IArrayObject> iArrayObject (classes.asUnknown ());
	if(iArrayObject)
	{
		Variant iidVar;
		if(!iArrayObject->getArrayElement (iidVar, index))
			return false;

		UnknownPtr<IObject> classDesc = iidVar.asUnknown ();
		if(!classDesc)
			return false;

		Variant result;
		classDesc->getProperty (result, "category");
		description.category = result;
		result.clear ();
		classDesc->getProperty (result, "name");
		description.name = result;
		result.clear ();
		classDesc->getProperty (result, "classID");
		Boxed::UID* classUid = unknown_cast<Boxed::UID> (result.asUnknown ());
		if(classUid)
			description.classID = *classUid;
		return true;
	}
	else
	{
		AutoPtr<ClassDescription> descObject = NEW ClassDescription;
		Variant returnValue;
		invokeMethod (returnValue, Message ("getClassDescription", static_cast<IObject*> (descObject), index));
		if(returnValue.asBool ())
			descObject->toClassDesc (description);
		return returnValue.asBool ();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassFactoryStub::getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const
{
	//CCL_NOT_IMPL ("ClassFactoryStub::getClassAttributes not implemented!")
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ClassFactoryStub::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	AutoPtr<Boxed::UID> boxedClassID = NEW Boxed::UID (cid);

	Variant returnValue;
	invokeMethod (returnValue, Message ("createInstance", static_cast<IObject*> (boxedClassID)));

	IUnknown* unknown = returnValue.asUnknown ();
	if(unknown)
		return unknown->queryInterface (iid, obj);

	*obj = nullptr;
	return kResultClassNotFound;
}

//************************************************************************************************
// ComponentStub
//************************************************************************************************

tresult CCL_API ComponentStub::initialize (IUnknown* context)
{
	Variant returnValue;
	invokeMethod (returnValue, Message ("initialize", context));
	return returnValue.asResult ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ComponentStub::terminate ()
{
	Variant returnValue;
	invokeMethod (returnValue, Message ("terminate"));
	return returnValue.asResult ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ComponentStub::canTerminate () const
{
	//CCL_NOT_IMPL ("ComponentStub::canTerminate")
	return true;
}

//************************************************************************************************
// NestingStub
//************************************************************************************************

StringRef CCL_API NestingStub::getObjectID () const
{
	Variant var;
	getProperty (var, "name");
	objectID = var.asString ();
	return objectID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API NestingStub::getParent () const
{
	Variant var;
	getProperty (var, "parent");
	return UnknownPtr<IObjectNode> (var.asUnknown ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IArrayObject* NestingStub::accessChildren () const
{
	Variant var;
	getProperty (var, "children");
	UnknownPtr<IArrayObject> childArray (var.asUnknown ());
	return childArray.detach ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API NestingStub::findChild (StringRef id) const
{
	if(AutoPtr<IArrayObject> children = accessChildren ())
		for(int i = 0; i < children->getArrayLength (); i++)
		{
			Variant var;
			children->getArrayElement (var, i);
			UnknownPtr<IObjectNode> child (var.asUnknown ());
			if(child && child->getObjectID () == id)
				return child;
		}

	// second try as property
	Variant var;
	if(getProperty (var, MutableCString (id)))
		if(UnknownPtr<IObjectNode> node = var.asUnknown ())
			return node;

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NestingStub::getChildDelegates (IMutableArray& delegates) const
{
	if(AutoPtr<IArrayObject> children = accessChildren ())
	{
		for(int i = 0; i < children->getArrayLength (); i++)
		{
			Variant var;
			children->getArrayElement (var, i);
			UnknownPtr<IObjectNode> child (var.asUnknown ());
			if(child)
			{
				Variant name (child->getObjectID ());
				name.share ();
				delegates.addArrayElement (name);
			}
		}
		return true;
	}
	return false;
}

//************************************************************************************************
// ObserverStub
//************************************************************************************************

void CCL_API ObserverStub::notify (ISubject* subject, MessageRef msg)
{
	Variant returnValue;
	AutoGCObject<IMessage> msg2 (const_cast<IMessage&> (msg)); // take care of GC issues

	UnknownPtr<IAtom> atom (subject);
	if(atom)
	{
		// Note: IAtom can not be passed to scripts because cclsystem is not registered by module reference!
		String atomName (atom->getAtomName ());
		invokeMethod (returnValue, Message ("notify", atomName, static_cast<IMessage*> (msg2)));
	}
	else
		invokeMethod (returnValue, Message ("notify", subject, static_cast<IMessage*> (msg2)));
}

//************************************************************************************************
// PersistAttributesStub
//************************************************************************************************

tresult CCL_API PersistAttributesStub::storeValues (IAttributeList& values) const
{
	Variant returnValue;
	if(!invokeMethod (returnValue, Message ("storeValues", &values)))
		return kResultUnexpected;
	return returnValue.asResult ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PersistAttributesStub::restoreValues (const IAttributeList& values)
{
	Variant returnValue;
	if(!invokeMethod (returnValue, Message ("restoreValues", const_cast<IAttributeList*> (&values))))
		return kResultUnexpected;
	return returnValue.asResult ();
}
