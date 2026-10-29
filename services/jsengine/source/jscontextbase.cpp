//************************************************************************************************
//
// JavaScript Engine
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : jscontextbase.cpp
// Description : JavaScript Context - shared backend base classes
//
//************************************************************************************************

#include "jscontextbase.h"

#include "ccl/public/base/variant.h"

using namespace CCL;
using namespace Scripting;
using namespace JScript;

//************************************************************************************************
// Debug logging
//************************************************************************************************

#if DEBUG
Threading::ThreadID JScript::theDebugThreadId = 0;
#endif

//************************************************************************************************
// JScript::ScriptClassBase
//************************************************************************************************

void ScriptClassBase::getNativeClassName (MutableCString& result, const ITypeInfo& typeInfo)
{
	result = "Native";
	result.appendFormat ("_%p_", typeInfo.getModuleReference ());
	result += typeInfo.getClassName ();
	result.replace (':', '_');
	result.replace ('.', '_');
	result.replace (' ', '_');
}

//************************************************************************************************
// JScript::ScriptObjectBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (ScriptObjectBase, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScriptObjectBase::queryInterface (UIDRef iid, void** ptr)
{
	if(ccl_iid<IFunction> ().equals (iid))
	{
		if(type == kFunction)
		{
			*ptr = static_cast<IFunction*> (this);
			retain ();
			return kResultOk;
		}
	}
	else if(ccl_iid<IBuffer> ().equals (iid))
	{
		if(type == kTypedArray)
		{
			*ptr = static_cast<IBuffer*> (this);
			retain ();
			return kResultOk;
		}
	}
	else if(ccl_iid<IArrayObject> ().equals (iid) || ccl_iid<IMutableArray> ().equals (iid))
	{
		if(type == kArray || type == kTypedArray)
		{
			QUERY_INTERFACE (IArrayObject)
			QUERY_INTERFACE (IMutableArray)
		}
	}

	QUERY_INTERFACE (IInnerUnknown)
	return SuperClass::queryInterface (iid, ptr);
}

//************************************************************************************************
// JScript::ContextBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (ContextBase, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

String ContextBase::makeScriptFileName (const IScript& script, StringRef fileName)
{
	String longName;
	if(!script.getPackageID ().isEmpty ())
	{
		longName << script.getPackageID ();
		longName << "#";
	}
	longName << fileName;
	return longName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContextBase::ContextBase (EngineBase& _engine)
: engine (_engine),
  stubObjectsEnabled (false),
  hostStringsEnabled (false),
  reporter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* ContextBase::createStub (IObject* scriptObject)
{
	return engine.getHost ()->createStubObject (scriptObject);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextBase::setReporter (Alert::IReporter* _reporter)
{
	reporter = _reporter;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextBase::setOption (StringID id, VariantRef value)
{
	#if DEBUG
	if(id == IContext::kLogMemoryAllocations)
	{
		theDebugThreadId = value.asBool () ? System::GetThreadSelfID () : 0;
		return kResultOk;
	}
	#endif

	return setBaseOption (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ContextBase::setBaseOption (StringID id, VariantRef value)
{
	if(id == IContext::kStubObjectsEnabled)
	{
		stubObjectsEnabled = value.asBool ();
		return kResultOk;
	}
	else if(id == IContext::kHostStringsEnabled)
	{
		hostStringsEnabled = value.asBool ();
		return kResultOk;
	}

	return kResultInvalidArgument;
}
