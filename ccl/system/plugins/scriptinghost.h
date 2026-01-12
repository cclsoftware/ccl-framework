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
// Filename    : ccl/system/plugins/scriptinghost.h
// Description : Scripting Host
//
//************************************************************************************************

#ifndef _ccl_scriptinghost_h
#define _ccl_scriptinghost_h

#include "ccl/base/singleton.h"

#include "ccl/public/plugins/iscriptingmanager.h"

namespace CCL {

class ObjectTable;

//************************************************************************************************
// ScriptingHost
/** Object accessed from script applications via "Host". */
//************************************************************************************************

class ScriptingHost: public Object,
					 public IScriptingHost,
					 public Singleton<ScriptingHost>
{
public:
	DECLARE_CLASS (ScriptingHost, Object)
	DECLARE_METHOD_NAMES (ScriptingHost)
	DECLARE_PROPERTY_NAMES (ScriptingHost)

	ScriptingHost ();
	~ScriptingHost ();

	class InterfaceList: public Object
	{
	public:
		DECLARE_CLASS (InterfaceList, Object)
		DECLARE_PROPERTY_NAMES (InterfaceList)

		// IObject
		tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
		tbool CCL_API getPropertyNames (IPropertyCollector& collector) const override;
	};

	class ResultsList: public Object
	{
	public:
		DECLARE_CLASS (ResultsList, Object)

		struct ResultDef { const char* name; tresult result; };
		static const ResultDef resultList[];

		// IObject
		tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
		tbool CCL_API getPropertyNames (IPropertyCollector& collector) const override;
	};

	class Console: public Object
	{
	public:
		DECLARE_CLASS (Console, Object)
		DECLARE_METHOD_NAMES (Console)

		// IObject
		tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	};

	class Signals: public Object
	{
	public:
		DECLARE_CLASS (Signals, Object)
		DECLARE_METHOD_NAMES (Signals)

		ISubject* resolve (VariantRef var);

		// IObject
		tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	};

	class ScriptableIO: public Object
	{
	public:
		DECLARE_CLASS (ScriptableIO, Object)
		DECLARE_METHOD_NAMES (ScriptableIO)

		// IObject
		tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	};

	// IScriptingHost
	void CCL_API registerObject (StringID name, IObject& object) override;
	void CCL_API unregisterObject (IObject& object) override;
	IObject* CCL_API getObject (StringID name) const override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getPropertyNames (IPropertyCollector& collector) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	CLASS_INTERFACE (IScriptingHost, Object)

protected:
	InterfaceList* interfaceList;
	ResultsList* resultsList;
	Console* console;
	Signals* signals;
	ScriptableIO* scriptableIO;
	ObjectTable* children;
};

} // namespace CCL

#endif // _ccl_scriptinghost_h
