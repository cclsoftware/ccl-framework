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
// Filename    : ccl/system/plugins/objecttable.h
// Description : Object Table
//
//************************************************************************************************

#ifndef _ccl_objecttable_h
#define _ccl_objecttable_h

#include "ccl/base/singleton.h"

#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// ObjectTable
//************************************************************************************************

class ObjectTable: public Object,
				   public IObjectTable,
				   public Singleton<ObjectTable>
{
public:
	DECLARE_CLASS (ObjectTable, Object)
	DECLARE_METHOD_NAMES (ObjectTable)

	ObjectTable ();

	void getObjectNames (IPropertyCollector& collector);

	// IObjectTable
	tresult CCL_API registerObject (IUnknown* obj, UIDRef id, StringID name, int flags = 0) override;
	tresult CCL_API unregisterObject (IUnknown* obj) override;
	int CCL_API countObjects () const override;
	StringID CCL_API getObjectName (int index) const override;
	IUnknown* CCL_API getObjectByIndex (int index) const override;
	IUnknown* CCL_API getObjectByID (UIDRef id) const override;
	IUnknown* CCL_API getObjectByName (StringID name) const override;
	IUnknown* CCL_API getObjectByUrl (UrlRef url) const override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API getPropertyNames (IPropertyCollector& collector) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	CLASS_INTERFACE (IObjectTable, Object)

protected:
	class Entry: public Object
	{
	public:
		Entry ()
		: flags (0)
		{}

		PROPERTY_VARIABLE (int, flags, Flags)
		PROPERTY_SHARED_AUTO (IUnknown, object, Object)
		PROPERTY_OBJECT (UID, id, ID)
		PROPERTY_MUTABLE_CSTRING (name, Name)

		PROPERTY_FLAG (flags, kIsHostApp, isHostApp)
	};

	ObjectArray entries;

	IUnknown* getHostApp () const;
};

} // namespace CCL

#endif // _ccl_objecttable_h
