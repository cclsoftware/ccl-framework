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
// Filename    : jscontext.quickjs.cpp
// Description : QuickJS JavaScript Context
//
//************************************************************************************************

#include "jscontext.quickjs.h"
#include "jsengine.quickjs.h"

#include "ccl/base/message.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/system/ilogger.h"

#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
#include <malloc/malloc.h>
#elif CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID || CCL_PLATFORM_WINDOWS
#include <malloc.h>
#endif

namespace JScript {
namespace QuickJS {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debug logging
//////////////////////////////////////////////////////////////////////////////////////////////////

static void* allocCalloc (void* /*opaque*/, size_t count, size_t size)
{
	LOG_JS_MALLOC ("calloc", count * size, nullptr)
	return calloc (count, size);
}

static void* allocMalloc (void* /*opaque*/, size_t size)
{
	LOG_JS_MALLOC ("malloc", size, nullptr)
	return malloc (size);
}

static void allocFree (void* /*opaque*/, void* ptr)
{
	LOG_JS_MALLOC ("free", 0, ptr)
	free (ptr);
}

static void* allocRealloc (void* /*opaque*/, void* ptr, size_t size)
{
	LOG_JS_MALLOC ("realloc", size, ptr)
	return realloc (ptr, size);
}

static size_t allocUsableSize (const void* ptr)
{
	#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
	return malloc_size (ptr);
	#elif CCL_PLATFORM_WINDOWS
	return _msize (const_cast<void*> (ptr));
	#elif CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID
	return malloc_usable_size (const_cast<void*> (ptr));
	#else
	return 0; // no portable way to query usable size on other platforms; quickjs-ng
			  // falls back to treating the requested size as the usable size in this case
	#endif
}

const JSMallocFunctions kAllocFunctions = {allocCalloc, allocMalloc, allocFree, allocRealloc, allocUsableSize};

} // namespace QuickJS
} // namespace JScript

using namespace CCL;
using namespace Scripting;
using namespace JScript::QuickJS;

//************************************************************************************************
// JScript::QuickJS::StringValue / PoolString
//************************************************************************************************

DEFINE_OBJECTPOOL_SIZE (PoolString, CCL::MemoryPool, 128)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringValue* StringValue::create (JSContext* cx, JSValueConst value)
{
	StringValue* v = PoolString::pool_new (cx, value);
	if(v == nullptr)
		v = NEW StringValue (cx, value);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringValue::StringValue (JSContext* cx, JSValueConst value)
: context (cx)
{
	size_t length = 0;
	charData = JS_ToCStringLen2 (cx, &length, value, false);
	charLength = charData ? length : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringValue::~StringValue ()
{
	if(charData)
		JS_FreeCString (context, charData);
}

//************************************************************************************************
// JScript::QuickJS::ScriptObject
//************************************************************************************************

ScriptObject* ScriptObject::create (JSContext* cx, JSValueConst value, Context* nativeContext)
{
	return NEW ScriptObject (cx, value, nativeContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptObject* ScriptObject::castUnknown (IUnknown* unknown)
{
	if(!unknown)
		return nullptr;

	UnknownPtr<IOuterUnknown> outer (unknown);
	if(outer)
		return unknown_cast<ScriptObject> (outer->getInnerUnknown ());
	return unknown_cast<ScriptObject> (unknown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ScriptObject, ScriptObjectBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptObject::ScriptObject (JSContext* cx, JSValueConst _value, Context* _nativeContext)
: nativeContext (_nativeContext),
  context (cx),
  value (JS_DupValue (cx, _value)),
  outerUnknown (nullptr)
{
	if(JS_IsFunction (cx, value))
		type = kFunction;
	else if(JS_GetTypedArrayType (value) >= 0)
		type = kTypedArray;
	else if(JS_IsArray (value))
		type = kArray;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptObject::~ScriptObject ()
{
	JS_FreeValue (context, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptObject::setOuterUnknown (IUnknown* _outerUnknown)
{
	outerUnknown = _outerUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::getProperty (Variant& var, MemberID propertyId) const
{
	ThreadScope threadScope (nativeContext);

	JSValue val = JS_GetPropertyStr (context, value, propertyId.str ());
	bool found = !JS_IsUndefined (val);
	if(found)
		Context::toVariant (var, context, val);
	JS_FreeValue (context, val);
	return found;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::setProperty (MemberID propertyId, const Variant& var)
{
	ThreadScope threadScope (nativeContext);

	JSValue val = Context::fromVariant (context, var); // ownership passed to JS_SetPropertyStr below
	return JS_SetPropertyStr (context, value, propertyId.str (), val) >= 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::invokeMethod (Variant& returnValue, MessageRef msg)
{
	ThreadScope threadScope (nativeContext);

	JSValue func = JS_GetPropertyStr (context, value, msg.getID ().str ());
	if(!JS_IsFunction (context, func))
	{
		JS_FreeValue (context, func);
		return false;
	}

	int argCount = ccl_min<int> (msg.getArgCount (), Message::kMaxMessageArgs);
	JSValue argv[Message::kMaxMessageArgs];
	for(int i = 0; i < argCount; i++)
		argv[i] = Context::fromVariant (context, msg[i]);

	JSValue result = JS_Call (context, func, value, argCount, argv);

	for(int i = 0; i < argCount; i++)
		JS_FreeValue (context, argv[i]);
	JS_FreeValue (context, func);

	bool ok = !JS_IsException (result);
	if(ok)
		Context::toVariant (returnValue, context, result);
	else
		nativeContext->reportError (JS_GetException (context));

	JS_FreeValue (context, result);
	return ok;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::call (Variant& returnValue, IObject* This, const Variant args[], int argCount)
{
	ThreadScope threadScope (nativeContext);

	JSValue jsThis = JS_UNDEFINED;
	if(This)
	{
		ScriptObject* scriptThis = ScriptObject::castUnknown (This);
		if(!scriptThis)
			return false;
		jsThis = scriptThis->getJSValue ();
	}

	argCount = ccl_min<int> (argCount, Message::kMaxMessageArgs);
	JSValue argv[Message::kMaxMessageArgs];
	for(int i = 0; i < argCount; i++)
		argv[i] = Context::fromVariant (context, args[i]);

	JSValue result = JS_Call (context, value, jsThis, argCount, argv);

	for(int i = 0; i < argCount; i++)
		JS_FreeValue (context, argv[i]);

	bool ok = !JS_IsException (result);
	if(ok)
		Context::toVariant (returnValue, context, result);
	else
		nativeContext->reportError (JS_GetException (context));

	JS_FreeValue (context, result);
	return ok;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScriptObject::getArrayLength () const
{
	ThreadScope threadScope (nativeContext);

	int64_t length = 0;
	JS_GetLength (context, value, &length);
	return int(length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::getArrayElement (Variant& var, int index) const
{
	ThreadScope threadScope (nativeContext);

	if(index < 0)
		return false;

	// Element access goes through ordinary indexed property get/set for both regular arrays and
	// typed arrays - both support standard [[Get]]/[[Set]] semantics per spec, so there is no need
	// to poke at the underlying ArrayBuffer memory directly the way IBuffer does for bulk access.
	JSValue val = JS_GetPropertyUint32 (context, value, uint32_t(index));
	bool ok = !JS_IsException (val);
	if(ok)
		Context::toVariant (var, context, val);
	JS_FreeValue (context, val);
	return ok;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::addArrayElement (VariantRef var)
{
	if(type == kTypedArray) // fixed size, cannot grow
		return false;

	ThreadScope threadScope (nativeContext);

	int64_t length = 0;
	JS_GetLength (context, value, &length);

	JSValue val = Context::fromVariant (context, var);
	return JS_SetPropertyInt64 (context, value, length, val) >= 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::removeArrayElement (int index)
{
	if(type == kTypedArray) // fixed size, cannot shrink
		return false;

	ThreadScope threadScope (nativeContext);

	JSAtom atom = JS_NewAtomUInt32 (context, uint32_t(index));
	int result = JS_DeleteProperty (context, value, atom, 0);
	JS_FreeAtom (context, atom);
	return result > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::setArrayElement (int index, VariantRef var)
{
	ThreadScope threadScope (nativeContext);

	if(index < 0)
		return false;

	JSValue val = Context::fromVariant (context, var);
	return JS_SetPropertyUint32 (context, value, uint32_t(index), val) >= 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API ScriptObject::getBufferAddress () const
{
	if(type != kTypedArray)
		return nullptr;

	ThreadScope threadScope (nativeContext);

	size_t byteOffset = 0, byteLength = 0, bytesPerElement = 0;
	JSValue buffer = JS_GetTypedArrayBuffer (context, value, &byteOffset, &byteLength, &bytesPerElement);
	if(JS_IsException (buffer))
		return nullptr;

	size_t bufferSize = 0;
	uint8_t* data = JS_GetArrayBuffer (context, &bufferSize, buffer);
	JS_FreeValue (context, buffer);

	return data ? data + byteOffset : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API ScriptObject::getBufferSize () const
{
	if(type != kTypedArray)
		return 0;

	ThreadScope threadScope (nativeContext);

	size_t byteOffset = 0, byteLength = 0, bytesPerElement = 0;
	JSValue buffer = JS_GetTypedArrayBuffer (context, value, &byteOffset, &byteLength, &bytesPerElement);
	if(JS_IsException (buffer))
		return 0;
	JS_FreeValue (context, buffer);

	return uint32(byteLength);
}

//************************************************************************************************
// JScript::QuickJS::ScriptClass
//************************************************************************************************

ScriptClass::ScriptClass (Context* _context, const ITypeInfo& _typeInfo)
: context (_context),
  typeInfo (_typeInfo),
  classId (0),
  prototype (JS_UNDEFINED)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass::~ScriptClass ()
{
	// prototype must already have been released by Context::~Context() (see releasePrototype()),
	// while the owning JSContext/JSRuntime were still alive.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScriptClass::releasePrototype ()
{
	JS_FreeValue (context->getJSContext (), prototype);
	prototype = JS_UNDEFINED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClass::initialize (JSValueConst parentPrototype)
{
	static const JSClassExoticMethods exoticMethods =
	{
		nullptr,			// get_own_property
		nullptr,			// get_own_property_names
		nullptr,			// delete_property
		nullptr,			// define_own_property
		nullptr,			// has_property
		&ScriptClass::getProperty,
		&ScriptClass::setProperty
	};

	JSContext* cx = context->getJSContext ();
	JSRuntime* rt = JS_GetRuntime (cx);

	classId = 0;
	JS_NewClassID (rt, &classId);

	MutableCString className;
	getNativeClassName (className, typeInfo);

	JSClassDef classDef = {};
	classDef.class_name = className.str ();
	classDef.finalizer = &ScriptClass::finalizer;
	classDef.exotic = const_cast<JSClassExoticMethods*> (&exoticMethods);

	if(JS_NewClass (rt, classId, &classDef) < 0)
		return false;

	prototype = JS_IsUndefined (parentPrototype) ? JS_NewObject (cx) : JS_NewObjectProto (cx, parentPrototype);
	if(JS_IsException (prototype))
		return false;

	defineMethods ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScriptClass::defineMethods ()
{
	JSContext* cx = context->getJSContext ();

	if(const ITypeInfo::MethodDefinition* methodNames = typeInfo.getMethodNames ())
		for(int i = 0; methodNames[i].name != nullptr; i++)
		{
			JSValue nameValue = JS_NewString (cx, methodNames[i].name);
			JSValueConst data[1] = { nameValue };

			JSValue func = JS_NewCFunctionData2 (cx, &ScriptClass::invokeMethod, methodNames[i].name, 0, 0, 1, data);
			JS_FreeValue (cx, nameValue);

			JS_DefinePropertyValueStr (cx, prototype, methodNames[i].name, func, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScriptClass::finalizer (JSRuntime* rt, JSValueConst val)
{
	JSClassID classId = JS_GetClassID (val);
	if(IObject* nativeObj = static_cast<IObject*> (JS_GetOpaque (val, classId)))
	{
		if(Context* context = Context::getNativeContext (rt))
			context->unregisterNativeObject (nativeObj);
		nativeObj->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSValue ScriptClass::invokeMethod (JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv, int /*magic*/, JSValueConst* funcData)
{
	// unlike the exotic get/set hooks (dispatched by quickjs-ng only for instances of the exact
	// class they were registered for), this is a plain function value sitting on the prototype -
	// script can extract it (`var f = obj.method`) and call it with an unrelated `this`, so the
	// opaque pointer must be validated against our own class registry before it's trusted.
	Context* context = Context::getNativeContext (ctx);
	JSClassID classId;
	void* opaque = JS_GetAnyOpaque (thisVal, &classId);
	IObject* nativeObj = (opaque && context && context->isNativeClassID (classId)) ? static_cast<IObject*> (opaque) : nullptr;
	if(!nativeObj)
		return JS_ThrowTypeError (ctx, "Native method called on invalid object");

	CStringPtr name = JS_ToCString (ctx, funcData[0]);
	if(!name)
		return JS_EXCEPTION;

	int argCount = ccl_min<int> (argc, Message::kMaxMessageArgs);
	Variant args[Message::kMaxMessageArgs];
	for(int i = 0; i < argCount; i++)
		Context::toVariant (args[i], ctx, argv[i]);

	Variant returnValue;
	tbool ok = nativeObj->invokeMethod (returnValue, Message (name, args, argCount));
	JS_FreeCString (ctx, name);

	if(!ok)
		return JS_ThrowTypeError (ctx, "Native method invocation failed");

	return Context::fromVariant (ctx, returnValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSValue ScriptClass::getProperty (JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst /*receiver*/)
{
	JSClassID classId;
	IObject* nativeObj = static_cast<IObject*> (JS_GetAnyOpaque (obj, &classId));
	if(!nativeObj)
		return JS_UNDEFINED;

	// methods (and anything else found on the prototype chain, e.g. Object.prototype.toString) win
	// over generic native property dispatch
	JSValue proto = JS_GetPrototype (ctx, obj);
	int hasProp = JS_HasProperty (ctx, proto, atom);
	if(hasProp != 0)
	{
		JSValue result = hasProp > 0 ? JS_GetProperty (ctx, proto, atom) : JS_EXCEPTION;
		JS_FreeValue (ctx, proto);
		return result;
	}
	JS_FreeValue (ctx, proto);

	CStringPtr name = JS_AtomToCString (ctx, atom);
	if(!name)
		return JS_EXCEPTION;

	Variant var;
	bool found = nativeObj->getProperty (var, name);
	JS_FreeCString (ctx, name);

	return found ? Context::fromVariant (ctx, var) : JS_UNDEFINED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScriptClass::setProperty (JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst value, JSValueConst /*receiver*/, int /*flags*/)
{
	JSClassID classId;
	IObject* nativeObj = static_cast<IObject*> (JS_GetAnyOpaque (obj, &classId));
	if(!nativeObj)
		return 0;

	CStringPtr name = JS_AtomToCString (ctx, atom);
	if(!name)
		return -1;

	Variant var;
	Context::toVariant (var, ctx, value);
	bool ok = nativeObj->setProperty (name, var);
	JS_FreeCString (ctx, name);

	return ok ? 1 : 0;
}

//************************************************************************************************
// JScript::QuickJS::Context
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (Context, ContextBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

Context::Context (Engine& _engine, ::JSContext* _context)
: ContextBase (_engine),
  context (_context)
{
	JS_SetContextOpaque (context, this);
	JS_SetRuntimeOpaque (JS_GetRuntime (context), this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Context::~Context ()
{
	// Release all JS-side resources owned by our ScriptClasses while the JSContext/JSRuntime are
	// still alive. ClassRegistry (a base of Context) only deletes the ScriptClass instances
	// themselves (see ~ClassRegistry()) after this destructor body returns, i.e. *after*
	// engine.onContextDestroyed() below has already freed the runtime.
	ListForEach (classList, ScriptClass*, scriptClass)
		scriptClass->releasePrototype ();
	EndFor

	static_cast<Engine&> (engine).onContextDestroyed (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::initialize ()
{
	// JS_NewContext() already installs the standard intrinsics and an implicit global object, so
	// the only additional per-context setup is the include_file() global script function.
	JSValue global = JS_GetGlobalObject (context);
	JSValue func = JS_NewCFunction (context, &Context::includeCallback, "include_file", 1);
	JS_DefinePropertyValueStr (context, global, "include_file", func, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
	JS_FreeValue (context, global);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::attachModule (ModuleRef module)
{
	ClassRegistry<ScriptClass>::addModule (module);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::detachModule (ModuleRef module)
{
	ClassRegistry<ScriptClass>::removeModule (module);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass* Context::resolveClass (const ITypeInfo& typeInfo)
{
	ScriptClass* scriptClass = ClassRegistry<ScriptClass>::lookupClass (typeInfo);
	if(!scriptClass)
		scriptClass = registerClass (typeInfo);
	return scriptClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::isNativeClassID (JSClassID classId) const
{
	ListForEach (classList, ScriptClass*, scriptClass)
		if(scriptClass->getClassID () == classId)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass* Context::registerClass (const ITypeInfo& typeInfo)
{
	JSValueConst parentPrototype = JS_UNDEFINED;
	if(const ITypeInfo* parentInfo = typeInfo.getParentType ())
	{
		ScriptClass* parentClass = resolveClass (*parentInfo);
		if(!parentClass)
			return nullptr;
		parentPrototype = parentClass->getPrototype ();
	}

	ScriptClass* scriptClass = NEW ScriptClass (this, typeInfo);
	if(!scriptClass->initialize (parentPrototype))
	{
		delete scriptClass;
		return nullptr;
	}

	if(!ClassRegistry<ScriptClass>::addClass (typeInfo, scriptClass))
	{
		delete scriptClass;
		return nullptr;
	}

	return scriptClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::registerNativeObject (IObject* nativeObject, JSValueConst obj)
{
	nativeObjects.add (nativeObject, JS_VALUE_GET_PTR (obj));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::unregisterNativeObject (IObject* nativeObject)
{
	return nativeObjects.remove (nativeObject);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::lookupNativeObject (IObject* nativeObject, JSValue& obj) const
{
	void* ptr = nullptr;
	if(!nativeObjects.get (ptr, nativeObject))
		return false;
	obj = JS_MKPTR (JS_TAG_OBJECT, ptr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSValue Context::resolveObject (IUnknown* unknown)
{
	if(!unknown)
		return JS_NULL;

	// if it is already one of our JS-bridged wrapper objects, reuse its JSValue directly to
	// preserve identity and the object's real semantics (e.g. being a genuine Array)
	if(ScriptObject* scriptObject = unknown_cast<ScriptObject> (unknown))
		return JS_DupValue (context, scriptObject->getJSValue ());

	UnknownPtr<IObject> nativeObject (unknown);
	if(!nativeObject)
		return JS_NULL;

	JSValue existing;
	if(lookupNativeObject (nativeObject, existing))
		return JS_DupValue (context, existing);

	ScriptClass* scriptClass = resolveClass (nativeObject->getTypeInfo ());
	if(!scriptClass)
		return JS_NULL;

	JSValue obj = JS_NewObjectProtoClass (context, scriptClass->getPrototype (), scriptClass->getClassID ());
	if(JS_IsException (obj))
		return JS_NULL;

	JS_SetOpaque (obj, static_cast<IObject*> (nativeObject));
	nativeObject->retain ();

	registerNativeObject (nativeObject, obj);

	return obj;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::registerObject (CStringRef name, IObject* nativeObject)
{
	ThreadScope threadScope (this);

	if(!nativeObject)
		return kResultFalse;

	ScriptClass* scriptClass = resolveClass (nativeObject->getTypeInfo ());
	if(!scriptClass)
		return kResultFalse;

	JSValue obj = JS_NewObjectProtoClass (context, scriptClass->getPrototype (), scriptClass->getClassID ());
	if(JS_IsException (obj))
		return kResultFalse;

	JS_SetOpaque (obj, nativeObject);
	nativeObject->retain ();

	registerNativeObject (nativeObject, obj);

	JSValue global = JS_GetGlobalObject (context);
	int result = JS_DefinePropertyValueStr (context, global, name.str (), obj, JS_PROP_ENUMERABLE);
	JS_FreeValue (context, global);

	return result >= 0 ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API Context::createObject (CStringRef className, const Variant[] /*args*/, int /*argCount*/)
{
	ThreadScope threadScope (this);

	if(className == "Object")
	{
		JSValue obj = JS_NewObject (context);
		return JS_IsException (obj) ? nullptr : ScriptObject::create (context, obj, this);
	}
	else if(className == "Array")
	{
		JSValue obj = JS_NewArray (context);
		return JS_IsException (obj) ? nullptr : ScriptObject::create (context, obj, this);
	}

	// look up an already-registered native class by name (see ClassRegistry::classList); classes
	// not yet touched by registerObject()/resolveObject() are not found here
	ListForEach (classList, ScriptClass*, scriptClass)
		if(CString (scriptClass->getTypeInfo ().getClassName ()) == className)
		{
			AutoPtr<IUnknown> instance (scriptClass->getTypeInfo ().createInstance ());
			UnknownPtr<IObject> nativeObject (instance);
			return nativeObject.detach ();
		}
	EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSValue Context::globalFunctionCallback (JSContext* ctx, JSValueConst /*thisVal*/, int argc, JSValueConst* argv, int /*magic*/, JSValueConst* funcData)
{
	Context* context = Context::getNativeContext (ctx);
	if(!context)
		return JS_ThrowTypeError (ctx, "No native context");

	CStringPtr name = JS_ToCString (ctx, funcData[0]);
	if(!name)
		return JS_EXCEPTION;

	IObject* nativeObject = nullptr;
	context->globalFunctionMap.get (nativeObject, CString (name));
	if(!nativeObject)
	{
		JS_FreeCString (ctx, name);
		return JS_ThrowTypeError (ctx, "Unknown global function");
	}

	int argCount = ccl_min<int> (argc, Message::kMaxMessageArgs);
	Variant args[Message::kMaxMessageArgs];
	for(int i = 0; i < argCount; i++)
		toVariant (args[i], ctx, argv[i]);

	Variant returnValue;
	tbool ok = nativeObject->invokeMethod (returnValue, Message (name, args, argCount));
	JS_FreeCString (ctx, name);

	if(!ok)
		return JS_ThrowTypeError (ctx, "Global function invocation failed");

	return fromVariant (ctx, returnValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSValue Context::includeCallback (JSContext* ctx, JSValueConst /*thisVal*/, int argc, JSValueConst* argv)
{
	Context* context = Context::getNativeContext (ctx);
	if(!context)
		return JS_NewBool (ctx, false);

	String includeFileName;
	if(argc >= 1)
	{
		Variant argValue;
		toVariant (argValue, ctx, argv[0]);
		if(argValue.getType () == Variant::kString)
			includeFileName = argValue.asString ();
		else if(UnknownPtr<IStringValue> stringValue = argValue.asUnknown ()) // host strings disabled
		{
			TextEncoding encoding = stringValue->getEncoding ();
			if(Text::isValidCStringEncoding (encoding))
				includeFileName.appendCString (encoding, stringValue->getCharData (), stringValue->getLength ());
			else if(Text::isUTF16Encoding (encoding))
				includeFileName.append (stringValue->getUCharData (), stringValue->getLength ());
		}
	}

	const IScript* currentScript = context->peekScript ();
	IEngineHost* host = context->engine.getHost ();
	AutoPtr<IScript> script = host ? host->resolveIncludeFile (includeFileName, currentScript) : nullptr;

	bool ok = false;
	CodePiece code;
	if(script && script->getCode (code))
	{
		String source;
		source.assign (code.code, code.length);
		MutableCString utf8Source (source, Text::kUTF8);
		MutableCString fileName (Context::makeScriptFileName (*script, code.fileName), Text::kUTF8);

		JSEvalOptions options {};
		options.version = JS_EVAL_OPTIONS_VERSION;
		options.eval_flags = JS_EVAL_TYPE_GLOBAL;
		options.filename = fileName.str ();
		options.line_num = code.lineNumber;

		context->scriptStack.push (script);
		JSValue result = JS_Eval2 (ctx, utf8Source.str (), size_t(utf8Source.length ()), &options);
		context->scriptStack.pop ();

		ok = !JS_IsException (result);
		if(!ok)
			context->reportError (JS_GetException (ctx));
		JS_FreeValue (ctx, result);
	}

	if(!ok)
	{
		String warning;
		warning.appendFormat ("Failed JavaScript include: \"%(1)\"", includeFileName);
		if(currentScript)
		{
			String parentFileName;
			currentScript->getPath ().getName (parentFileName);
			warning.appendFormat (" from \"%(1)/%(2)\"", currentScript->getPackageID (), parentFileName);
		}
		System::GetLogger ().reportEvent (Alert::Event (warning, Alert::kWarning));
		#if DEBUG
		Debugger::println (warning);
		#endif
	}

	return JS_NewBool (ctx, ok);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::registerGlobalFunction (CStringRef methodName, IObject* nativeObject)
{
	ThreadScope threadScope (this);

	if(!nativeObject)
		return kResultFalse;

	JSValue nameValue = JS_NewString (context, methodName.str ());
	JSValueConst data[1] = { nameValue };
	JSValue func = JS_NewCFunctionData2 (context, &Context::globalFunctionCallback, methodName.str (), 0, 0, 1, data);
	JS_FreeValue (context, nameValue);

	if(JS_IsException (func))
		return kResultFailed;

	JSValue global = JS_GetGlobalObject (context);
	int result = JS_DefinePropertyValueStr (context, global, methodName.str (), func, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
	JS_FreeValue (context, global);

	if(result < 0)
		return kResultFailed;

	globalFunctionMap.add (methodName, nativeObject);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::executeScript (Variant& returnValue, const IScript& script)
{
	return executeScriptInternal (returnValue, script);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Context::executeScriptInternal (Variant& returnValue, const IScript& script)
{
	ThreadScope threadScope (this);

	CodePiece code;
	if(!script.getCode (code))
		return kResultFailed;

	String source;
	source.assign (code.code, code.length);
	MutableCString utf8Source (source, Text::kUTF8);
	MutableCString fileName (Context::makeScriptFileName (script, code.fileName), Text::kUTF8);

	JSEvalOptions options {};
	options.version = JS_EVAL_OPTIONS_VERSION;
	options.eval_flags = JS_EVAL_TYPE_GLOBAL;
	options.filename = fileName.str ();
	options.line_num = code.lineNumber;

	scriptStack.push (&script);
	JSValue result = JS_Eval2 (context, utf8Source.str (), size_t(utf8Source.length ()), &options);
	scriptStack.pop ();

	tresult res = kResultOk;
	if(JS_IsException (result))
	{
		reportError (JS_GetException (context));
		res = kResultFalse;
	}
	else
	{
		toVariant (returnValue, context, result);
	}

	JS_FreeValue (context, result);
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API Context::compileScript (const IScript& script)
{
	ThreadScope threadScope (this);

	CodePiece code;
	if(!script.getCode (code))
		return nullptr;

	String source;
	source.assign (code.code, code.length);
	MutableCString utf8Source (source, Text::kUTF8);
	MutableCString fileName (Context::makeScriptFileName (script, code.fileName), Text::kUTF8);

	JSEvalOptions options {};
	options.version = JS_EVAL_OPTIONS_VERSION;
	options.eval_flags = JS_EVAL_TYPE_GLOBAL;
	options.filename = fileName.str ();
	options.line_num = code.lineNumber;

	// Note: QuickJS-ng has no public API for evaluating against a private lexical scope object,
	// so the script is evaluated directly against the real global object:
	// top-level "function"/"var" declarations become global properties.
	// This means multiple compiled scripts on the same Context currently share one namespace.
	scriptStack.push (&script);
	JSValue result = JS_Eval2 (context, utf8Source.str (), size_t(utf8Source.length ()), &options);
	scriptStack.pop ();
	if(JS_IsException (result))
	{
		reportError (JS_GetException (context));
		JS_FreeValue (context, result);
		return nullptr;
	}
	JS_FreeValue (context, result);

	JSValue global = JS_GetGlobalObject (context);
	ScriptObject* scriptObject = ScriptObject::create (context, global, this);
	JS_FreeValue (context, global); // ScriptObject duplicated its own reference in its constructor
	return scriptObject;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::garbageCollect (tbool force)
{
	ThreadScope threadScope (this);

	// QuickJS-ng has no public equivalent of "maybe" GC; only run a full collection
	// when explicitly forced, otherwise leave collection to the runtime's own GC threshold.
	if(force)
		JS_RunGC (JS_GetRuntime (context));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Context::removeReference (IUnknown* unknown)
{
	ThreadScope threadScope (this);

	UnknownPtr<IObject> nativeObject (unknown);
	JSValue obj;
	if(!nativeObject || !lookupNativeObject (nativeObject, obj))
		return false;

	unregisterNativeObject (nativeObject);
	nativeObject->release ();
	JS_SetOpaque (obj, nullptr);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::dump ()
{
	Debugger::println ("=== JavaScript Context Native Object Map ===");
	int counter = 0;
	NativeObjectMap::Iterator iter (nativeObjects);
	while(!iter.done ())
	{
		const NativeObjectMap::Association& assoc = iter.nextAssociation ();
		const IObject* nativeObject = static_cast<const IObject*> (assoc.key);
		CStringPtr className = nativeObject ? nativeObject->getTypeInfo ().getClassName () : nullptr;
		Debugger::printf ("%04d: Native object %p | Class \"%s\"\n", counter++, nativeObject, className);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::reportError (JSValue errorValue)
{
	#if !DEBUG
	if(reporter)
	#endif
	{
		JSValue messageValue = JS_IsObject (errorValue) ? JS_GetPropertyStr (context, errorValue, "message") : JS_UNDEFINED;
		JSValueConst textValue = JS_IsUndefined (messageValue) ? errorValue : messageValue;

		String message;
		if(CStringPtr text = JS_ToCString (context, textValue))
		{
			message = String (Text::kUTF8, text);
			JS_FreeCString (context, text);
		}
		JS_FreeValue (context, messageValue);

		Alert::Event event (message, Alert::kError);

		if(JS_IsObject (errorValue))
		{
			JSValue fileNameValue = JS_GetPropertyStr (context, errorValue, "fileName");
			if(JS_IsString (fileNameValue))
				if(CStringPtr fileNameStr = JS_ToCString (context, fileNameValue))
				{
					event.fileName = String (Text::kUTF8, fileNameStr);
					JS_FreeCString (context, fileNameStr);
				}
			JS_FreeValue (context, fileNameValue);

			JSValue lineNumberValue = JS_GetPropertyStr (context, errorValue, "lineNumber");
			int32_t lineNumber = 0;
			if(JS_IsNumber (lineNumberValue))
				JS_ToInt32 (context, &lineNumber, lineNumberValue);
			event.lineNumber = lineNumber;
			JS_FreeValue (context, lineNumberValue);
		}

		#if DEBUG
		MutableCString messageStr (message, Text::kUTF8);
		MutableCString fileNameStr (event.fileName, Text::kUTF8);
		Debugger::printf ("%s (%d) : %s\n", fileNameStr.str (), event.lineNumber, messageStr.str ());
		#endif

		if(reporter)
			reporter->reportEvent (event);
	}

	JS_FreeValue (context, errorValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::toVariant (Variant& var, JSContext* cx, JSValueConst val)
{
	var.clear ();

	switch(JS_VALUE_GET_NORM_TAG (val))
	{
	case JS_TAG_UNDEFINED :
	case JS_TAG_NULL :
	case JS_TAG_UNINITIALIZED :
		break; // leave var invalid/empty

	case JS_TAG_BOOL :
		var = (JS_VALUE_GET_BOOL (val) != 0);
		break;

	case JS_TAG_INT :
		var = JS_VALUE_GET_INT (val);
		break;

	case JS_TAG_FLOAT64 :
		var = JS_VALUE_GET_FLOAT64 (val);
		break;

	case JS_TAG_BIG_INT :
	{
		// CCL::Variant has no arbitrary-precision integer kind, so this is a lossy truncation to
		// int64 - matches the SpiderMonkey backend's ScriptArguments::toVariant() BigInt handling.
		int64_t bigIntValue = 0;
		if(JS_ToBigInt64 (cx, &bigIntValue, val) == 0)
			var = int64(bigIntValue);
		break;
	}

	case JS_TAG_STRING :
	case JS_TAG_STRING_ROPE :
	{
		Context* context = getNativeContext (cx);
		if(context && !context->isHostStringsEnabled ())
		{
			AutoPtr<IStringValue> stringValue (StringValue::create (cx, val));
			var.takeShared (stringValue);
		}
		else if(CStringPtr str = JS_ToCString (cx, val))
		{
			var = Variant (String (Text::kUTF8, str), true);
			JS_FreeCString (cx, str);
		}
		break;
	}

	case JS_TAG_OBJECT :
	{
		Context* context = getNativeContext (cx);

		// JS_GetAnyOpaque() returns whatever raw bits occupy the opaque union slot for *any*
		// object - including plain functions/arrays/typed arrays that are not one of our native
		// classes - so the returned classId must be checked against our own registry before the
		// opaque pointer can be trusted as an IObject*, or this reads garbage.
		JSClassID classId;
		void* opaque = JS_GetAnyOpaque (val, &classId);
		if(opaque && context && context->isNativeClassID (classId))
			var.takeShared (static_cast<IObject*> (opaque));
		else if(context)
		{
			AutoPtr<ScriptObject> scriptObject (ScriptObject::create (cx, val, context));
			if(context->isStubNeeded (*scriptObject))
			{
				IObject* stubObject = context->createStub (scriptObject);
				ASSERT (stubObject)
				if(stubObject)
				{
					var.takeShared (stubObject);
					stubObject->release ();
				}
			}
			else
				var.takeShared (scriptObject->asUnknown ());
		}
		break;
	}

	default :
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSValue Context::fromVariant (JSContext* cx, const Variant& var)
{
	switch(var.getType ())
	{
	case Variant::kInt :
		return JS_NewInt64 (cx, var.asLargeInt ());

	case Variant::kFloat :
		return JS_NewFloat64 (cx, var.asDouble ());

	case Variant::kString :
	{
		MutableCString utf8 (var.asString (), Text::kUTF8);
		return JS_NewStringLen (cx, utf8.str (), size_t(utf8.length ()));
	}

	case Variant::kObject :
	{
		Context* context = getNativeContext (cx);
		return context ? context->resolveObject (var.asUnknown ()) : JS_NULL;
	}

	default :
		return JS_UNDEFINED;
	}
}
