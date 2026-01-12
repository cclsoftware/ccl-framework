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
// Filename    : ccl/system/plugins/objecttable.cpp
// Description : Object Table
//
//************************************************************************************************

#include "ccl/system/plugins/objecttable.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IObjectTable& CCL_API System::CCL_ISOLATED (GetObjectTable) ()
{
	return ObjectTable::instance ();
}

//************************************************************************************************
// ObjectTable
//************************************************************************************************

DEFINE_CLASS (ObjectTable, Object)
DEFINE_CLASS_NAMESPACE (ObjectTable, NAMESPACE_CCL)
DEFINE_SINGLETON (ObjectTable)

////////////////////////////////////////////////////////////////////////////////////////////////////

ObjectTable::ObjectTable ()
{
	entries.objectCleanup (true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ObjectTable::getHostApp () const
{
	ArrayForEach (entries, Entry, e)
		if(e->isHostApp ())
			return e->getObject ();
	EndFor
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ObjectTable::registerObject (IUnknown* obj, UIDRef id, StringID name, int flags)
{
	ASSERT (obj != nullptr)
	if(obj == nullptr)
		return kResultInvalidPointer;
	
	ASSERT (!name.isEmpty ())
	Entry* e = NEW Entry;
	e->setObject (obj);
	e->setID (id);
	e->setName (name);
	e->setFlags (flags);

	entries.add (e);

	signal (Message (kChanged));
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ObjectTable::unregisterObject (IUnknown* obj)
{
	ArrayForEach (entries, Entry, e)
		if(e->getObject () == obj)
		{
			entries.remove (e);
			e->release ();
			signal (Message (kChanged));
			return kResultOk;
		}
	EndFor

	ASSERT (0)
	return kResultFalse;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ObjectTable::countObjects () const
{
	return entries.count ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ObjectTable::getObjectName (int index) const
{
	Entry* e = (Entry*)entries.at (index);
	if(e)
		return e->getName ();
	return CString::kEmpty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ObjectTable::getObjectByIndex (int index) const
{
	Entry* e = (Entry*)entries.at (index);
	return e ? e->getObject () : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ObjectTable::getObjectByID (UIDRef id) const
{
	ArrayForEach (entries, Entry, e)
		if(e->getID () == id)
			return e->getObject ();
	EndFor
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ObjectTable::getObjectByName (StringID name) const
{
	ArrayForEach (entries, Entry, e)
		if(e->getName () == name)
			return e->getObject ();
	EndFor

	if(name.compare (kHostApp, false) == 0)
		return getHostApp ();
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ObjectTable::getObjectByUrl (UrlRef url) const
{
	MutableCString rootName (url.getHostName ());
	IUnknown* rootObject = getObjectByName (rootName);
	if(url.getPath ().isEmpty ())
		return rootObject;

	UnknownPtr<IObjectNode> iNode (rootObject);
	return iNode ? iNode->lookupChild (url.getPath ()) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTable::getProperty (Variant& var, MemberID propertyId) const
{
	// try to find object by name...
	IUnknown* object = getObjectByName (propertyId);
	if(object)
	{
		var = object;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTable::getPropertyNames (IPropertyCollector& collector) const
{
	// Keep it disabled for global object table!
	return SuperClass::getPropertyNames (collector);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectTable::getObjectNames (IPropertyCollector& collector)
{
	ArrayForEach (entries, Entry, e)
		ASSERT (!e->getName ().isEmpty ())

		ITypeInfo::PropertyDefinition prop { nullptr };
		prop.name = e->getName ();
		prop.type = CCL::ITypeInfo::kVoid | CCL::ITypeInfo::kReadOnly;	// all entries are read-only properties
		collector.addProperty (prop);
	EndFor
}

////////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ObjectTable)
	DEFINE_METHOD_ARGR ("registerObject", "obj, name", "tresult")
	DEFINE_METHOD_ARGR ("unregisterObject", "obj", "tresult")
	DEFINE_METHOD_ARGR ("getObjectByName", "name", "Object")
	DEFINE_METHOD_ARGR ("getObjectByID", "uidString", "Object")
	DEFINE_METHOD_ARGR ("getObjectByUrl", "url, optional", "Object")
END_METHOD_NAMES (ObjectTable)

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTable::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "registerObject")
	{
		returnValue = registerObject (msg[0], kNullUID, MutableCString (msg[1].asString ()));
		return true;
	}
	else if(msg == "unregisterObject")
	{
		returnValue = unregisterObject (msg[0]);
		return true;
	}
	else if(msg == "getObjectByName")
	{
		MutableCString objectName (msg[0].asString ());
		IUnknown* obj = getObjectByName (objectName);
		ASSERT (obj != nullptr)
		returnValue = obj;
		return true;
	}
	else if(msg == "getObjectByID")
	{
		UID uid;
		bool result = uid.fromString (msg[0].asString ());
		ASSERT (result == true)
		IUnknown* obj = result ? getObjectByID (uid) : nullptr;
		ASSERT (obj != nullptr)
		returnValue = obj;
		return true;
	}
	else if(msg == "getObjectByUrl")
	{
		Url url (msg[0].asString ());
		bool optional = false;
		if(msg.getArgCount () > 1)
			optional = msg[1].asBool ();
		IUnknown* obj = getObjectByUrl (url);
		ASSERT (optional || obj != nullptr)
		returnValue = obj;
		return true;		
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
