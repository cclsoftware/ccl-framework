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
// Filename    : jscontextbase.h
// Description : JavaScript Context - shared backend base classes
//
//************************************************************************************************

#ifndef _jscontextbase_h
#define _jscontextbase_h

#include "jsenginebase.h"

#include "ccl/base/object.h"

#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/base/ibuffer.h"
#include "ccl/public/collections/hashmap.h"
#include "ccl/public/collections/stack.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/text/cstring.h"

namespace JScript {

//************************************************************************************************
// Debug logging
//************************************************************************************************

#if DEBUG
/** Thread flagged via setOption(kLogMemoryAllocations, true) - see ContextBase::setOption(). */
extern CCL::Threading::ThreadID theDebugThreadId;

/** Logs a JS engine heap allocation if it happened on the flagged thread. No-op outside DEBUG builds. */
#define LOG_JS_MALLOC(name, bytes, address) \
	if(CCL::System::GetThreadSelfID () == theDebugThreadId) \
		{ CCL::Debugger::printf ("JS Allocation %s %d %p\n", name, int(bytes), address); \
		 /*CCL::Debugger::debugBreak ("");*/ }
#else
#define LOG_JS_MALLOC(name, bytes, address)
#endif

//************************************************************************************************
// JScript::GlobalFunctionMap
//************************************************************************************************

class GlobalFunctionMap: public CCL::HashMap<CCL::CString, CCL::IObject*>
{
public:
	GlobalFunctionMap ()
	: CCL::HashMap<CCL::CString, CCL::IObject*> (10, &hashName)
	{}

	static int hashName (const CCL::CString& key, int size) { return key.getHashCode () % size; }
};

//************************************************************************************************
// JScript::ScriptClassBase
//************************************************************************************************

class ScriptClassBase
{
protected:
	/** Build a JS class name from typeInfo's module address and class name, replacing characters
		that are not valid in a JS identifier ('.', ':', ' ') with '_'. */
	static void getNativeClassName (CCL::MutableCString& result, const CCL::ITypeInfo& typeInfo);
};

//************************************************************************************************
// JScript::ScriptObjectBase
//************************************************************************************************

class ScriptObjectBase: public CCL::Object,
						public CCL::Scripting::IFunction,
						public CCL::IMutableArray,
						public CCL::IBuffer,
						public CCL::IInnerUnknown
{
public:
	DECLARE_CLASS_ABSTRACT (ScriptObjectBase, Object)

	/** Whether this wraps an array or typed array - stub objects are never created for these. */
	bool isArrayOrTypedArray () const { return type == kArray || type == kTypedArray; }

	// IFunction
	CCL::tbool CCL_API call (CCL::Variant& returnValue, CCL::IObject* This, const CCL::Variant args[], int argCount) override { return false; }

	// IArrayObject / IMutableArray
	int CCL_API getArrayLength () const override { return 0; }
	CCL::tbool CCL_API getArrayElement (CCL::Variant& var, int index) const override { return false; }
	CCL::tbool CCL_API addArrayElement (CCL::VariantRef var) override { return false; }
	CCL::tbool CCL_API removeArrayElement (int index) override { return false; }
	CCL::tbool CCL_API setArrayElement (int index, CCL::VariantRef var) override { return false; }

	// IBuffer
	void* CCL_API getBufferAddress () const override { return nullptr; }
	CCL::uint32 CCL_API getBufferSize () const override { return 0; }

	// IInnerUnknown
	void CCL_API setOuterUnknown (CCL::IUnknown* outerUnknown) override {}

	// IUnknown
	UNKNOWN_REFCOUNT
	CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override;

protected:
	ScriptObjectBase (): type (kUnknown) {}

	/** What kind of JSValue is wrapped, determined once at construction by each backend (once the
		wrapped value/object is available); controls which interfaces queryInterface() exposes and
		how the TypedArray-only IBuffer methods behave. */
	enum Type
	{
		kUnknown,
		kFunction,
		kArray,
		kTypedArray
	};

	Type type;
};

//************************************************************************************************
// JScript::StringValueBase
//************************************************************************************************

class StringValueBase: public CCL::Unknown,
					   public CCL::Scripting::IStringValue
{
public:
	// IStringValue
	CCL::UStringPtr CCL_API getUCharData () const override { return nullptr; }
	CCL::CStringPtr CCL_API getCharData () const override { return nullptr; }
	int CCL_API getLength () const override { return 0; }
	CCL::TextEncoding CCL_API getEncoding () const override { return CCL::Text::kUTF8; }

	CLASS_INTERFACE (IStringValue, Unknown)
};

//************************************************************************************************
// JScript::ContextBase
//************************************************************************************************

class ContextBase: public CCL::Object,
				   public CCL::Scripting::IContext
{
public:
	DECLARE_CLASS_ABSTRACT (ContextBase, Object)

	ContextBase (EngineBase& engine);

	/** Per-context setup that must run once, after construction (see each backend's own JSContext
		creation timing). Default is a no-op; every backend currently needs its own real setup
		(e.g. defining the global include_file() function), so overrides don't chain to this. */
	virtual void initialize () {}

	/** Ask the host to create a native stub object for scriptObject (see IEngineHost::createStubObject()). */
	CCL::IObject* createStub (CCL::IObject* scriptObject);

	/** The IScript currently executing on this Context (top of scriptStack), or nullptr if none. */
	const CCL::Scripting::IScript* peekScript () const { return scriptStack.peek (); }

	// Scripting::IContext
	CCL::Scripting::IEngine& CCL_API getEngine () const override { return engine; }
	CCL::tresult CCL_API setOption (CCL::StringID id, CCL::VariantRef value) override;
	void CCL_API attachModule (CCL::ModuleRef module) override {}
	void CCL_API detachModule (CCL::ModuleRef module) override {}
	CCL::tresult CCL_API setReporter (CCL::Alert::IReporter* reporter) override;
	CCL::tresult CCL_API registerObject (CCL::CStringRef name, CCL::IObject* nativeObject) override { return CCL::kResultNotImplemented; }
	CCL::IObject* CCL_API createObject (CCL::CStringRef className, const CCL::Variant args[], int argCount) override { return nullptr; }
	CCL::tresult CCL_API registerGlobalFunction (CCL::CStringRef methodName, CCL::IObject* nativeObject) override { return CCL::kResultNotImplemented; }
	CCL::tresult CCL_API executeScript (CCL::Variant& returnValue, const CCL::Scripting::IScript& script) override { return CCL::kResultNotImplemented; }
	CCL::IObject* CCL_API compileScript (const CCL::Scripting::IScript& script) override { return nullptr; }
	void CCL_API garbageCollect (CCL::tbool force) override {}
	CCL::tbool CCL_API removeReference (IUnknown* nativeObject) override { return false; }
	void CCL_API dump () override {}

	CLASS_INTERFACE (IContext, Object)

protected:
	EngineBase& engine;
	bool stubObjectsEnabled;
	bool hostStringsEnabled;
	CCL::Alert::IReporter* reporter;
	CCL::Stack<const CCL::Scripting::IScript*> scriptStack;
	GlobalFunctionMap globalFunctionMap;

	/** Build the display name used for a script's evaluation "file", prefixing the script's package
		ID (if any) so scripts from different packages sharing a file name remain distinguishable. */
	static CCL::String makeScriptFileName (const CCL::Scripting::IScript& script, CCL::StringRef fileName);

	CCL::tresult setBaseOption (CCL::StringID id, CCL::VariantRef value);
};

} // namespace JScript

#endif // _jscontextbase_h
