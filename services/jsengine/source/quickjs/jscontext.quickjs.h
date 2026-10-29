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
// Filename    : jscontext.quickjs.h
// Description : QuickJS JavaScript Context
//
//************************************************************************************************

#ifndef _jscontext_quickjs_h
#define _jscontext_quickjs_h

#include "jscrossthread.quickjs.h"

#include "../jsclassregistry.h"
#include "../jscontextbase.h"

#include "ccl/base/memorypool.h"

namespace JScript {
namespace QuickJS {

class Context;
class Engine;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debug logging
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Custom allocator installed on every JSRuntime via JS_NewRuntime2(). Always delegates to the plain
	C allocator; only logs when a thread has been flagged via setOption(kLogMemoryAllocations, true). */
extern const ::JSMallocFunctions kAllocFunctions;

//************************************************************************************************
// JScript::QuickJS::ScriptClass
/** Bridges a native C++ type (described by a CCL::ITypeInfo) to a QuickJS-ng JSClassID. Instances of
	the native class are represented as JSValues of this class, carrying the native CCL::IObject* as
	their opaque pointer (see JS_SetOpaque()/JS_GetOpaque()/JS_GetAnyOpaque()).

	Property/method access is bridged generically: the prototype only ever carries the declared
	native methods (see defineMethods()) as real JS function properties; arbitrary property get/set
	(e.g. "gTest.width") is dispatched dynamically to CCL::IObject::getProperty()/setProperty() via
	the class' JSClassExoticMethods (see getProperty()/setProperty() below), so no property list has
	to be known ahead of time (mirrors how ITypeInfo::getPropertyNames() is optional/informational
	only - CCL::Object subclasses are free to accept arbitrary property names in getProperty()/
	setProperty(), as JScript::TestClass does for "width"). */
//************************************************************************************************

class ScriptClass: public ScriptClassBase
{
public:
	ScriptClass (Context* context, const CCL::ITypeInfo& typeInfo);
	~ScriptClass ();

	/** Register the QuickJS-ng class and build its prototype object (chained to parentPrototype, or
		to the default Object.prototype if parentPrototype is JS_UNDEFINED). */
	bool initialize (JSValueConst parentPrototype);

	const CCL::ITypeInfo& getTypeInfo () const { return typeInfo; }
	JSClassID getClassID () const { return classId; }
	JSValueConst getPrototype () const { return prototype; }

	/** Release the owned prototype JSValue. Must be called (once, by Context::~Context()) while the
		owning JSContext/JSRuntime are still alive, i.e. *before* Engine::onContextDestroyed() frees
		them - ClassRegistry (a base of Context) only deletes ScriptClass instances (see ~ScriptClass())
		after Context's own destructor body (which frees the runtime) has already returned. */
	void releasePrototype ();

protected:
	Context* context;
	const CCL::ITypeInfo& typeInfo;
	JSClassID classId;
	JSValue prototype;

	void defineMethods ();

	static void finalizer (JSRuntime* rt, JSValueConst val);

	static JSValue invokeMethod (JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv, int magic, JSValueConst* funcData);

	// JSClassExoticMethods
	static JSValue getProperty (JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst receiver);
	static int setProperty (JSContext* ctx, JSValueConst obj, JSAtom atom, JSValueConst value, JSValueConst receiver, int flags);
};

//************************************************************************************************
// JScript::QuickJS::NativeObjectMap
/** Identity map from native CCL::IObject* to its (non-owning) JS wrapper object, so that repeatedly
	crossing the same native pointer into script (e.g. as a method return value) yields the same
	JSValue rather than a new wrapper every time. Entries are removed by ScriptClass::finalizer()
	when the wrapper is garbage-collected.

	Stores the raw heap pointer (JS_VALUE_GET_PTR()) rather than a full JSValue: all values kept here
	are always JS_TAG_OBJECT (reconstructed via JS_MKPTR(JS_TAG_OBJECT, ptr)), and unlike JSValue,
	void* has a built-in operator==, which CCL::HashMap's Vector-based bucket implementation requires
	for TValue. */
//************************************************************************************************

class NativeObjectMap: public CCL::PointerHashMap<void*>
{
public:
	NativeObjectMap ()
	: CCL::PointerHashMap<void*> (512)
	{}
};

//************************************************************************************************
// JScript::QuickJS::StringValue
/** Lightweight, non-owning-feeling wrapper exposing a JS string value as IStringValue instead of
	converting it into a full CCL::String, used when kHostStringsEnabled is off (the default) - see
	Context::toVariant()'s JS_TAG_STRING case. Always reports kUTF8/getCharData() as the valid
	representation (getUCharData() returns nullptr): QuickJS-ng's public API
	does not expose whether a string is internally narrow or wide, so there is no cheap zero-copy
	representation to prefer - UTF8 is simply this codebase's usual default single encoding choice
	(see e.g. ccl/platform/wasm/gui/systemevent.wasm.cpp) when one has to be picked. */
//************************************************************************************************

class StringValue: public StringValueBase
{
public:
	static StringValue* create (JSContext* cx, JSValueConst value);
	~StringValue ();

	// StringValueBase
	CCL::CStringPtr CCL_API getCharData () const override { return charData; }
	int CCL_API getLength () const override { return int(charLength); }

protected:
	StringValue (JSContext* cx, JSValueConst value);

	JSContext* context;
	CCL::CStringPtr charData;
	size_t charLength;
};

//************************************************************************************************
// JScript::QuickJS::PoolString
/** Memory-pooled StringValue: since string marshaling (see Context::toVariant()) happens on every
	property access - including from real time threads - StringValue::create() allocates from this
	fixed-capacity, lock-free pool instead of the heap, falling back to a plain heap-allocated
	StringValue only once the pool is exhausted. */
//************************************************************************************************

class PoolString: public StringValue,
				  public CCL::PooledObject<PoolString, CCL::MemoryPool>
{
public:
	PoolString (JSContext* cx, JSValueConst value)
	: StringValue (cx, value)
	{}
};

//************************************************************************************************
// JScript::QuickJS::ScriptObject
/** Wraps a JSValue (typically the global object after compileScript() ran the script, or a plain
	Object/Array created via Context::createObject()) as an IObject, so that top-level script
	functions/properties can be accessed repeatedly via getProperty()/setProperty()/invokeMethod().
	Note: since this stage does not yet implement per-script scoping (see Context::compileScript),
	all compiled scripts share the same global object.

	Also implements IFunction, IMutableArray and IBuffer, exposed conditionally via queryInterface()
	depending on what kind of JSValue is actually wrapped (see Type): a plain object only ever
	answers to IObject/IInnerUnknown, a function additionally to IFunction, an Array additionally to
	IArrayObject/IMutableArray, and a TypedArray additionally to IArrayObject/IMutableArray/IBuffer. */
//************************************************************************************************

class ScriptObject: public ScriptObjectBase
{
public:
	DECLARE_CLASS_ABSTRACT (ScriptObject, ScriptObjectBase)

	static ScriptObject* create (JSContext* cx, JSValueConst value, Context* nativeContext);

	/** Resolve the ScriptObject wrapping the given native IUnknown, following IOuterUnknown
		aggregation (see setOuterUnknown()) if present. */
	static ScriptObject* castUnknown (CCL::IUnknown* unknown);

	~ScriptObject ();

	JSValueConst getJSValue () const { return value; }

	// ScriptObjectBase
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	CCL::tbool CCL_API setProperty (MemberID propertyId, const CCL::Variant& var) override;
	CCL::tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg) override;
	void CCL_API setOuterUnknown (CCL::IUnknown* outerUnknown) override;
	CCL::tbool CCL_API call (CCL::Variant& returnValue, CCL::IObject* This, const CCL::Variant args[], int argCount) override;
	int CCL_API getArrayLength () const override;
	CCL::tbool CCL_API getArrayElement (CCL::Variant& var, int index) const override;
	CCL::tbool CCL_API addArrayElement (CCL::VariantRef var) override;
	CCL::tbool CCL_API removeArrayElement (int index) override;
	CCL::tbool CCL_API setArrayElement (int index, CCL::VariantRef var) override;
	void* CCL_API getBufferAddress () const override;
	CCL::uint32 CCL_API getBufferSize () const override;

protected:
	ScriptObject (JSContext* cx, JSValueConst value, Context* nativeContext);

	Context* nativeContext;
	JSContext* context;
	JSValue value;
	CCL::IUnknown* outerUnknown;
};

//************************************************************************************************
// JScript::QuickJS::Context
//************************************************************************************************

class Context: public ContextBase,
			   protected ClassRegistry<ScriptClass>
{
public:
	DECLARE_CLASS_ABSTRACT (Context, ContextBase)

	Context (Engine& engine, ::JSContext* context);
	~Context ();

	/** Get the Context owning the given JSContext (set as its opaque pointer). */
	static Context* getNativeContext (JSContext* cx)
	{
		return static_cast<Context*> (JS_GetContextOpaque (cx));
	}

	/** Get the Context owning the given JSRuntime (set as its opaque pointer). */
	static Context* getNativeContext (JSRuntime* rt)
	{
		return static_cast<Context*> (JS_GetRuntimeOpaque (rt));
	}

	void initialize () override;

	JSContext* getJSContext () const { return context; }

	bool isHostStringsEnabled () const { return hostStringsEnabled; }

	/** Whether a plain script object crossing into native code (see toVariant()) should be handed
		to the host as a stub (via createStub()) instead of wrapped as a plain ScriptObject - see
		setOption(kStubObjectsEnabled). Never true for arrays/typed arrays. */
	bool isStubNeeded (const ScriptObject& scriptObject) const { return stubObjectsEnabled && !scriptObject.isArrayOrTypedArray (); }

	ScriptClass* resolveClass (const CCL::ITypeInfo& typeInfo); ///< add class if not registered

	/** Whether the given JSClassID belongs to one of this Context's registered native classes (see
		ScriptClass). JS_GetAnyOpaque() returns whatever raw bits occupy an object's internal opaque
		union slot for *any* JSValue, including plain functions/arrays/typed arrays that were never
		one of our native classes - reading that value as an IObject* without this check first is
		undefined behavior. */
	bool isNativeClassID (JSClassID classId) const;

	/** Wrap an arbitrary native IUnknown as a (owned) JSValue, preserving identity for objects that
		already crossed into script before. Returns JS_NULL if unknown is null or does not implement
		IObject. */
	JSValue resolveObject (CCL::IUnknown* unknown);

	void registerNativeObject (CCL::IObject* nativeObject, JSValueConst obj);
	bool unregisterNativeObject (CCL::IObject* nativeObject);
	bool lookupNativeObject (CCL::IObject* nativeObject, JSValue& obj) const;

	/** Convert a (borrowed) JSValue to a Variant. Supports undefined, null, bool, int, double,
		string and native/script objects. */
	static bool toVariant (CCL::Variant& var, JSContext* cx, JSValueConst val);

	/** Convert a Variant to a newly created (owned) JSValue, caller must JS_FreeValue() it. */
	static JSValue fromVariant (JSContext* cx, const CCL::Variant& var);

	/** Report a (owned) JSValue exception to the registered Alert::IReporter, if any, and free it. */
	void reportError (JSValue errorValue);

	// ContextBase
	void CCL_API attachModule (CCL::ModuleRef module) override;
	void CCL_API detachModule (CCL::ModuleRef module) override;
	CCL::tresult CCL_API registerObject (CCL::CStringRef name, CCL::IObject* nativeObject) override;
	CCL::IObject* CCL_API createObject (CCL::CStringRef className, const CCL::Variant args[], int argCount) override;
	CCL::tresult CCL_API registerGlobalFunction (CCL::CStringRef methodName, CCL::IObject* nativeObject) override;
	CCL::tresult CCL_API executeScript (CCL::Variant& returnValue, const CCL::Scripting::IScript& script) override;
	CCL::IObject* CCL_API compileScript (const CCL::Scripting::IScript& script) override;
	void CCL_API garbageCollect (CCL::tbool force) override;
	CCL::tbool CCL_API removeReference (IUnknown* nativeObject) override;
	void CCL_API dump () override;

protected:
	JSContext* context;

	NativeObjectMap nativeObjects;

	ScriptClass* registerClass (const CCL::ITypeInfo& typeInfo);

	CCL::tresult executeScriptInternal (CCL::Variant& returnValue, const CCL::Scripting::IScript& script);

	static JSValue globalFunctionCallback (JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv, int magic, JSValueConst* funcData);

	/** Implements the global include_file(name) script function, resolving `name` relative to
		peekScript() via IEngineHost::resolveIncludeFile() and evaluating it against this Context's
		global object (there is no separate lexical scope to include into, same simplification as
		compileScript()). */
	static JSValue includeCallback (JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
};

} // namespace QuickJS
} // namespace JScript

#endif // _jscontext_quickjs_h
