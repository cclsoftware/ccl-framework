//************************************************************************************************
//
// JavaScript Engine
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
// Filename    : jscontext.cpp
// Description : JavaScript Context
//
//************************************************************************************************

#define DEBUG_LOG 0
#define LOG_INVOKE (0 && DEBUG)
#define LOG_PROPERTIES (0 && DEBUG)

#include "jscontext.h"
#include "jsengine.h"

#include "ccl/public/base/ibuffer.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/systemservices.h"

namespace JScript {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
	CCL::MutableCString logArgument (const CCL::Variant* var)
	{
		using namespace CCL;
		if(var)
		{
			if(!var->isValid ())
				return CSTR ("[NULL]");
			else if(var->isObject ())
			{
				UnknownPtr<IObject> obj (var->asUnknown ());
				if(obj)
				{
					MutableCString s;
					s.appendFormat ("[%s]", obj->getTypeInfo ().getClassName ());
					return s;
				}
				return CSTR ("[Unknown]");
			}
			else
			{
				String s;
				var->toString (s);
				MutableCString cs;
				cs.appendFormat ("\"%s\"", MutableCString (s).str ());
				return cs;
			}
		}
		return CString::kEmpty;
	}

	static CCL::Threading::ThreadID theDebugThreadId = 0;

	#define LOG_JS_MALLOC(name, bytes, address) \
		if(CCL::System::GetThreadSelfID () == theDebugThreadId) \
			{ CCL::Debugger::printf ("JS Allocation %s %d %p\n", name, (int)bytes, address); \
			 /*CCL::Debugger::debugBreak ("");*/ }
#else
	#define LOG_JS_MALLOC(name, bytes, address)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

void logMalloc (size_t bytes)
{
	LOG_JS_MALLOC ("malloc", bytes, 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void logCalloc (size_t bytes)
{
	LOG_JS_MALLOC ("calloc", bytes, 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void logRealloc (void* p, size_t bytes)
{
	LOG_JS_MALLOC ("realloc", bytes, p)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void logFree (void* p)
{
	LOG_JS_MALLOC ("free", 0, p)
}

//************************************************************************************************
// JScript::PropertyCollector
//************************************************************************************************

class PropertyCollector: public CCL::Unknown,
						 public CCL::IPropertyCollector
{
public:
	PropertyCollector (JSContext* cx, JS::MutableHandleIdVector& props);

	// IPropertyCollector
	void CCL_API addProperty (const CCL::ITypeInfo::PropertyDefinition& propDef) override;
	void CCL_API addPropertyName (CCL::CStringPtr name) override;
	void CCL_API addPropertyNames (CCL::CStringPtr names[], int count) override;

	CLASS_INTERFACE (CCL::IPropertyCollector, CCL::Unknown)

protected:
	JSContext* context;
	JS::MutableHandleIdVector& ids;
};

//************************************************************************************************
// JScript::ScriptObject
//************************************************************************************************

class ScriptObject: public CCL::Object,
					public CCL::Scripting::IFunction,
					public CCL::IMutableArray,
					public CCL::IBuffer,
					public CCL::IInnerUnknown
{
public:
	DECLARE_CLASS_ABSTRACT (ScriptObject, Object)

	static ScriptObject* createInstance (JS::HandleObject obj, Context* context);
	static IUnknown* getInstance (JS::HandleObject obj, JSContext* context);
	static ScriptObject* castUnknown (IUnknown* unknown);

	JSObject* getJSObject () const { return obj; }

	// IInnerUnknown
	void CCL_API setOuterUnknown (IUnknown* outerUnknown) override;

	// IObject
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	CCL::tbool CCL_API setProperty (MemberID propertyId, const CCL::Variant& var) override;
	CCL::tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg) override;

	// IFunction
	CCL::tbool CCL_API call (CCL::Variant& returnValue, IObject* This, const CCL::Variant args[], int argCount) override;

	// IMutableArray
	int CCL_API getArrayLength () const override;
	CCL::tbool CCL_API getArrayElement (CCL::Variant& var, int index) const override;
	CCL::tbool CCL_API addArrayElement (CCL::VariantRef var) override;
	CCL::tbool CCL_API removeArrayElement (int index) override;
	CCL::tbool CCL_API setArrayElement (int index, CCL::VariantRef var) override;

	// IBuffer
	void* CCL_API getBufferAddress () const override;
	CCL::uint32 CCL_API getBufferSize () const override;

	// IUnknown
	UNKNOWN_REFCOUNT
	CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override;

protected:
	Context* nativeContext;
	JSContext* context;
	JS::Realm* realm;
	JS::PersistentRootedObject obj;

	enum Type
	{
		kUnknown,
		kFunction,
		kArray,
		kTypedArray
	};

	Type type;

	ScriptObject (JSObject* object, Context* context);

	const char* determineClassName ();
};

//************************************************************************************************
// JScript::ScriptObjectDebug
//************************************************************************************************

class ScriptObjectDebug: public ScriptObject
{
public:
	// ScriptObject
	const CCL::MetaClass& myClass () const override { return thisClass; }
	bool isClass (const CCL::MetaClass& mc) const override { return thisClass.isClass (mc); }
	bool canCast (const CCL::MetaClass& mc) const override { return thisClass.canCast (mc); }

	#if LOG_PROPERTIES
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const
	{
		CCL::tbool r = ScriptObject::getProperty (var, propertyId);
		CCL::MutableCString s (logArgument (&var));
		CCL::Debugger::printf ("[JS] %s::getProperty (%s): %s\n", thisClass.getClassName (), propertyId.str (), s.str ());
		return r;
	}
	CCL::tbool CCL_API setProperty (MemberID propertyId, const CCL::Variant& var)
	{
		CCL::MutableCString s (logArgument (&var));
		CCL::Debugger::printf ("[JS] %s::setProperty (%s): %s\n", thisClass.getClassName (), propertyId.str (), s.str ());
		return ScriptObject::setProperty (propertyId, var);
	}
	#endif
	#if LOG_INVOKE
	CCL::tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg)
	{
		CCL::tbool r = ScriptObject::invokeMethod (returnValue, msg);
		CCL::MutableCString s (logArgument (&returnValue));
		CCL::Debugger::printf ("[JS] %s::%s () returned %s\n", thisClass.getClassName (), msg.getID ().str (), s.str ());
		return r;
	}
	#endif

protected:
	class Class: public CCL::MetaClass
	{
	public:
		Class (JSObject* object);
		friend class ScriptObjectDebug;
	};

	Class thisClass;

	friend class ScriptObject;
	ScriptObjectDebug (JSObject* object, Context* context);
};

//************************************************************************************************
// JScript::ProxyHandler
//************************************************************************************************

class ProxyHandler: public js::BaseProxyHandler
{
public:
	ProxyHandler (ScriptClass* scriptClass);

	bool getOwnPropertyDescriptor (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc) const override;
	bool defineProperty (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult& result) const override;
	bool ownPropertyKeys (JSContext* cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const override;
	bool delete_ (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult& result) const override;

	bool hasOwn (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, bool* bp) const override;
	void finalize (JS::GCContext* gcx, JSObject* proxy) const override;
	bool finalizeInBackground (const JS::Value& priv) const override { return false; }

	/* Non-standard but conceptual kin to {g,s}etPrototype, so these live here. */
	bool getPrototypeIfOrdinary (JSContext* cx, JS::HandleObject proxy, bool* isOrdinary, JS::MutableHandleObject protop)  const override { return false; }
	bool preventExtensions (JSContext* cx, JS::HandleObject proxy, JS::ObjectOpResult& result) const override { return true; }
	bool isExtensible (JSContext* cx, JS::HandleObject proxy, bool* extensible) const override { return false; }

protected:
	ScriptClass* scriptClass;
};

//************************************************************************************************
// JScript::RealmScope
//************************************************************************************************

RealmScope::RealmScope (Context* _context, JSObject* target)
: threadScope (_context),
  context (_context),
  oldRealm (nullptr)
{
	ASSERT (context && target)
	if(context && target && ThreadScope::isCurrentContext (context))
		oldRealm = JS::EnterRealm (context->getJSContext (), target);
	else
		context = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RealmScope::RealmScope (Context* _context, JS::Realm* target)
: threadScope (_context),
  context (_context),
  oldRealm (nullptr)
{
	ASSERT (context && target)
	if(context && target && ThreadScope::isCurrentContext (context))
		oldRealm = JS::EnterRealm (context->getJSContext (), JS::GetRealmGlobalOrNull (target));
	else
		context = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RealmScope::~RealmScope ()
{
	if(context)
		JS::LeaveRealm (context->getJSContext (), oldRealm);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool isArrayOrTypedArray (JSContext* cx, JS::HandleObject obj)
{
	bool isArray = false;
	bool success = JS::IsArrayObject (cx, obj, &isArray);
	ASSERT (success)

	return isArray || JS_IsTypedArrayObject (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static CCL::String makeScriptFileName (const CCL::Scripting::IScript& script, CCL::StringRef fileName)
{
	CCL::String longName;
	if(!script.getPackageID ().isEmpty ())
	{
		longName << script.getPackageID ();
		longName << "#";
	}
	longName << fileName;
	return longName;
}

} // namespace JScript

using namespace CCL;
using namespace Scripting;
using namespace JScript;

//************************************************************************************************
// JScript::PropertyCollector
//************************************************************************************************

PropertyCollector::PropertyCollector (JSContext* cx, JS::MutableHandleIdVector& props)
: context (cx),
  ids (props)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertyCollector::addProperty (const ITypeInfo::PropertyDefinition& propDef)
{
	addPropertyName (propDef.name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertyCollector::addPropertyName (CStringPtr name)
{
	JS::RootedString propertyName (context, JS_NewStringCopyZ (context, name));
	JS::RootedId id (context);
	JS_StringToId (context, propertyName, &id);
	bool succeeded = ids.append (id);
	ASSERT (succeeded)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertyCollector::addPropertyNames (CStringPtr names[], int count)
{
	if(names == nullptr)
		return;

	if(count == -1)
		for(int i = 0; names[i] != nullptr; i++)
			addPropertyName (names[i]);
	else
		for(int i = 0; i < count; i++)
			addPropertyName (names[i]);
}

//************************************************************************************************
// JScript::Realm
//************************************************************************************************

Realm::Realm (Context* _context)
: context (_context)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Realm::registerNativeObject (IObject* nativeObject, JSObject* obj)
{
	nativeObjects.add (nativeObject, obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Realm::unregisterNativeObject (IObject* nativeObject)
{
	bool removed = nativeObjects.remove (nativeObject);
	ASSERT (removed == true)
	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSObject* Realm::lookupNativeObject (IObject* nativeObject) const
{
	return nativeObjects.lookup (nativeObject);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Realm::dumpNativeObjects () const
{
	Debugger::println ("=== JavaScript Context Native Object Map ===");
	NativeObjectMap::Iterator iter (nativeObjects);
	int counter = 0;
	while(!iter.done ())
	{
		const NativeObjectMap::Association& assoc = iter.nextAssociation ();
		const IObject* nativeObject = static_cast<const IObject*> (assoc.key);
		JSObject* jsObject = assoc.value;
		CStringPtr className = nativeObject ? nativeObject->getTypeInfo ().getClassName () : nullptr;

		Debugger::printf ("%04d: Native object %p | JS object %p | Class \"%s\"\n", counter++, nativeObject, jsObject, className);
	}
}

//************************************************************************************************
// JScript::Context
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (Context, Object)

const UserDataClass Context::userDataClass;

//////////////////////////////////////////////////////////////////////////////////////////////////

Context::Context (Engine& engine, ::JSContext* context)
: engine (engine),
  context (context),
  gcContext (js::gc::GetGCContext (context)),
  global (context),
  realm (NEW Realm (this)),
  scopeStack (nullptr),
  stubObjectsEnabled (false),
  hostStringsEnabled (false),
  inGarbageCollection (false),
  globalInitialized (false),
  reporter (nullptr),
  globalFunctionMap (10, &hashString),
  propertyAccessorMap (128, &hashIdentifier)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Context::~Context ()
{
	ThreadScope guard (this);

	cleanupPropertyAccessors ();

	global = nullptr;

	engine.onContextDestroyed (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::initialize ()
{
	if(globalInitialized)
	{
		ASSERT (false)
		return;
	}

	globalInitialized = true;

	static JSClass globalClass =
	{
		"global",
		JSCLASS_GLOBAL_FLAGS,
		&JS::DefaultGlobalClassOps
	};

	JS::RealmOptions options;
	global = JS_NewGlobalObject (context, &globalClass, nullptr, JS::FireOnNewGlobalHook, options);
	ASSERT (global)

	RealmScope guard (this, global);
	JS_DefineObject (context, global, "exports"); // allow loading modules
	JS::SetRealmPrivate (JS::GetObjectRealmOrNull (global), realm);
	JS::SetDestroyRealmCallback (context, &destroyRealmCallback);

	// define our userdata class
	JSObject* userDataPrototype = JS_InitClass (context, global, &userDataClass, nullptr, userDataClass.name, nullptr, 0, nullptr, nullptr, nullptr, nullptr);
	ASSERT (userDataPrototype)

	// define include function
	JSFunction* func = JS_DefineFunction (context, global, "include_file", &includeCallback, 1, 0);
	ASSERT (func)

	userDataID = JS::PropertyKey::fromPinnedString (JS_AtomizeAndPinString (context, "__userdata"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::destroyRealmCallback (JS::GCContext* gcx, JS::Realm* realm)
{
	delete static_cast<Realm*> (JS::GetRealmPrivate (realm));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::onGCFinished ()
{
	for(auto obj : pendingUserDataResets)
	{
		setUserData (JS::RootedObject (context, *obj), nullptr);
		delete obj;
	}
	pendingUserDataResets.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PropertyAccessor* Context::getPropertyAccessor (const Identifier& id)
{
	static const CString kPropertyAccessorName ("__ccl_accessNativeProperty");

	// define property accessor function
	bool functionDefined = false;
	JS_HasProperty (context, global, kPropertyAccessorName, &functionDefined);
	if(!functionDefined)
		JS_DefineFunction (context, global, kPropertyAccessorName, ScriptClass::getterSetter, 2, 0);

	// look up already defined accessors
	if(PropertyAccessor* accessor = propertyAccessorMap.lookup (id))
		return accessor;

	// define getter function
	JS::SourceText<char16_t> getterSource;
	String getterSourceString = String ("return ").appendASCII (kPropertyAccessorName).append (".apply (this, [\"").appendASCII (id).append ("\"]);");
	StringChars getterChars (getterSourceString);
	if(!getterSource.init (context, (const char16_t*) (const uchar*) getterChars, getterSourceString.length (), JS::SourceOwnership::Borrowed))
		return nullptr;

	// define setter function
	JS::SourceText<char16_t> setterSource;
	const char* setterArgs[1] = { "value" };
	String setterSourceString = String (kPropertyAccessorName).append (".apply (this, [\"").appendASCII (id).append ("\", value]);");
	StringChars setterChars (setterSourceString);
	if(!setterSource.init (context, (const char16_t*) (const uchar*) setterChars, setterSourceString.length (), JS::SourceOwnership::Borrowed))
		return nullptr;

	// compile functions and add to cache
	JS::CompileOptions compileOptions (context);
	JS::EnvironmentChain emptyScopeChain (context, JS::SupportUnscopables::No);

	PropertyAccessor* accessor = NEW PropertyAccessor;

	accessor->getter = JS_GetFunctionObject (JS::CompileFunction (context, emptyScopeChain, compileOptions, nullptr, 0, nullptr, getterSource));
	accessor->setter = JS_GetFunctionObject (JS::CompileFunction (context, emptyScopeChain, compileOptions, nullptr, 1, setterArgs, setterSource));

	propertyAccessorMap.add (id, accessor);
	return accessor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::cleanupPropertyAccessors ()
{
	for(PropertyAccessor* accessor : propertyAccessorMap)
		delete accessor;

	propertyAccessorMap.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::tracePropertyAccessors (JSTracer* tracer)
{
	for(PropertyAccessor* accessor : propertyAccessorMap)
	{
		JS::TraceEdge (tracer, &accessor->getter, "getter");
		JS::TraceEdge (tracer, &accessor->setter, "setter");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IEngine& CCL_API Context::getEngine () const
{
	return engine;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::setOption (StringID id, VariantRef value)
{
	if(id == kStubObjectsEnabled)
	{
		stubObjectsEnabled = value.asBool ();
		return kResultOk;
	}
	else if(id == kHostStringsEnabled)
	{
		hostStringsEnabled = value.asBool ();
		return kResultOk;
	}
#if DEBUG
	else if(id == kLogMemoryAllocations)
	{
		theDebugThreadId = value.asBool () ? System::GetThreadSelfID () : 0;
		return kResultOk;
	}
#endif

	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::attachModule (ModuleRef module)
{
	realm->addModule (module);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::detachModule (ModuleRef module)
{
	realm->removeModule (module);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::isStubNeeded (JS::HandleObject obj) const
{
	if(!ThreadScope::isCurrentContext (this))
		return false;

	return stubObjectsEnabled && !isArrayOrTypedArray (context, obj); // avoid creating stubs for arrays
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::setUserData (JS::HandleObject obj, IUnknown* userData)
{
	if(!ThreadScope::isCurrentContext (this))
		return false;

	if(JS::RuntimeHeapIsBusy ())
	{
		// during GC we can not modify any properties of obj; this should only be called from ~GenericStub () with setOuterUnknown (0)
		ASSERT (userData == nullptr)
		if(userData != nullptr)
			return false;

		pendingUserDataResets.append (NEW JS::PersistentRootedObject (context, obj));
		return true;
	}

	RealmScope guard (this, global);

	// check if property already exists...
	JS::RootedValue value (context);
	JS::RootedObject data (context);

	if(JS_GetPropertyById (context, obj, JS::RootedId (context, userDataID), &value))
	{
		if(JS_ValueToObject (context, value, &data) && data)
		{
			JS::SetReservedSlot (data, kPrivateDataSlot, JS::PrivateValue (userData));
			return true;
		}
	}

	data = JS_NewObject (context, &userDataClass);
	ASSERT (data)
	if(!data)
		return false;

	JS::SetReservedSlot (data, kPrivateDataSlot, JS::PrivateValue (userData));
	value = JS::ObjectOrNullValue (data);
	bool result = JS_SetPropertyById (context, obj, JS::RootedId (context, userDataID), value);
	ASSERT (result)
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Context::getUserData (JS::HandleObject obj) const
{
	if(!ThreadScope::isCurrentContext (this))
		return nullptr;

	IUnknown* userData = nullptr;
	JS::RootedValue value (context);
	if(JS_GetPropertyById (context, obj, JS::RootedId (context, userDataID), &value))
	{
		JS::RootedObject data (context);
		if(JS_ValueToObject (context, value, &data) && data)
			userData = static_cast<IUnknown*> (JS::GetReservedSlot (data, kPrivateDataSlot).toPrivate ());
	}
	return userData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass* Context::registerClass (const ITypeInfo& typeInfo)
{
	if(!ThreadScope::isCurrentContext (this))
		return nullptr;

	ScriptClass* scriptClass = NEW ScriptClass (realm, typeInfo);
	if(!realm->addClass (typeInfo, scriptClass))
	{
		delete scriptClass;
		return nullptr;
	}

	// find or create parent class first
	JS::RootedObject parentPrototype (context);
	const ITypeInfo* parentInfo = typeInfo.getParentType ();
	if(parentInfo)
	{
		ScriptClass* parentClass = resolveClass (*parentInfo);
		parentPrototype = parentClass ? parentClass->getPrototype () : nullptr;
		ASSERT (parentPrototype)
	}

	JS::RootedObject prototype (context);
	prototype = JS_InitClass (context, global, scriptClass, parentPrototype, scriptClass->name,
							  nullptr, 0, nullptr, nullptr, nullptr, nullptr);
	ASSERT (prototype)
	defineMethods (prototype, typeInfo);

	scriptClass->setPrototype (prototype);
	return scriptClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::defineMethods (JS::HandleObject prototype, const ITypeInfo& typeInfo)
{
	if(const ITypeInfo::MethodDefinition* methodNames = typeInfo.getMethodNames ())
		for(int i = 0; methodNames[i].name != nullptr; i++)
		{
			JSFunction* function = JS_DefineFunction (context, prototype, methodNames[i].name, ScriptClass::invokeNativeMethod, 0, 0);
			ASSERT (function)
			bool result = JS_SetProperty (context, prototype, methodNames[i].name, JS::RootedValue (context, JS::ObjectValue (*JS_GetFunctionObject (function))));
			ASSERT (result)
		}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::registerObject (CStringRef name, IObject* nativeObject)
{
	ASSERT (nativeObject)
	if(!nativeObject)
		return kResultFalse;

	RealmScope guard (this, global);
	if(!guard.isValid ())
		return kResultWrongThread;

	const ITypeInfo& typeInfo = nativeObject->getTypeInfo ();
	ScriptClass* scriptClass = resolveClass (typeInfo);
	ASSERT (scriptClass)
	if(!scriptClass)
		return kResultFalse;

	// generate accessors for registered properties ahead of time
	if(const ITypeInfo::PropertyDefinition* propertyNames = typeInfo.getPropertyNames ())
		for(int i = 0; propertyNames[i].name != nullptr; i++)
			getPropertyAccessor (Identifier (propertyNames[i].name));

	JS::Value target;
	target.setPrivate (nativeObject);
	JS::RootedValue targetValue (context, target);

	js::ProxyOptions options;
	JSObject* obj = js::NewProxyObject (context, scriptClass->getProxyHandler (), targetValue, scriptClass->getPrototype (), options);

	JS::RootedValue newObj (context, JS::ObjectValue (*obj));
	unsigned flags = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;
	JS_DefineProperty (context, global, name, newObj, flags);

	ASSERT (obj)
	if(!obj)
		return kResultFalse;

	nativeObject->retain ();

	// add to object map
	realm->registerNativeObject (nativeObject, obj);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::setReporter (Alert::IReporter* _reporter)
{
	reporter = _reporter;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass* Context::resolveClass (const ITypeInfo& typeInfo)
{
	ScriptClass* scriptClass = realm->lookupClass (typeInfo);
	if(!scriptClass)
	{
		scriptClass = registerClass (typeInfo);
		ASSERT (scriptClass)
	}
	return scriptClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSObject* Context::resolveObject (IUnknown* unknown)
{
	// check if it is already a JSObject, wrapped into a stub...
	ScriptObject* scriptObject = ScriptObject::castUnknown (unknown);
	if(scriptObject)
		return scriptObject->getJSObject ();

	UnknownPtr<IObject> nativeObject (unknown);
	if(!nativeObject)
		return nullptr;

	// first check if there is a JSObject in our map already...
	JSObject* obj = realm->lookupNativeObject (nativeObject);
	if(obj)
		return obj;

	ScriptClass* scriptClass = resolveClass (nativeObject->getTypeInfo ());
	ASSERT (scriptClass)

	JS::Value target;
	target.setPrivate (nativeObject);
	JS::RootedValue targetValue (context, target);
	js::ProxyOptions options;
	obj = scriptClass ? js::NewProxyObject (context, scriptClass->getProxyHandler (), targetValue, scriptClass->getPrototype (), options) : nullptr;

	if(obj)
	{
		nativeObject->retain ();

		// add to object map
		realm->registerNativeObject (nativeObject, obj);
	}
	return obj;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Context::removeReference (IUnknown* unknown)
{
	UnknownPtr<IObject> nativeObject (unknown);
	JSObject* obj = nativeObject ? realm->lookupNativeObject (nativeObject) : nullptr;
	if(obj)
	{
		// remove from object map
		realm->unregisterNativeObject (nativeObject);

		nativeObject->release ();

		js::SetProxyPrivate (obj, JS::NullValue ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::dump ()
{
	realm->dumpNativeObjects ();

	#if 0 && DEBUG
	Debugger::println ("=== JavaScript Heap ===");
	JS_DumpHeap (context, 0, 0, 0, 0, 100, 0);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* Context::createStub (IObject* scriptObject)
{
	return engine.getHost ()->createStubObject (scriptObject);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IScript* Context::peekScript () const
{
	return scriptStack.peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::createScopeStack ()
{
	ASSERT (scopeStack == nullptr)

	scopeStack = NEW JS::EnvironmentChain (context, JS::SupportUnscopables::No);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::deleteScopeStack ()
{
	ASSERT (scopeStack != nullptr)

	delete scopeStack;
	scopeStack = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::executeScript (Variant& returnValue, const IScript& script)
{
	RealmScope guard (this, global);
	if(!guard.isValid ())
		return kResultWrongThread;

	return executeScriptInternal (returnValue, script);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Context::executeScriptInternal (Variant& returnValue, const IScript& script)
{
	CodePiece code;
	if(!script.getCode (code))
		return kResultFailed;

	JS::SourceText<char16_t> sourceCode;
	if(!sourceCode.init (context, reinterpret_cast<const char16_t*> (code.code), code.length, JS::SourceOwnership::Borrowed))
		return kResultFailed;

	MutableCString fileName;
	fileName.append (makeScriptFileName (script, code.fileName), Text::kISOLatin1);
	JS::CompileOptions options (context);
	options.setFileAndLine (fileName, code.lineNumber);

	if(scriptStack.isEmpty ())
		createScopeStack ();

	scriptStack.push (&script);

	JS::RootedValue retVal (context);
	bool result = JS::Evaluate (context, options, sourceCode, &retVal);

	scriptStack.pop ();

	if(scriptStack.isEmpty ())
		deleteScopeStack ();

	ASSERT (result)
	if(!result)
		return kResultFalse;

	result = ScriptArguments::toVariant (returnValue, retVal, context);
	if(!result)
		return kResultFalse;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API Context::compileScript (const IScript& script)
{
	RealmScope guard (this, global);
	if(!guard.isValid ())
		return nullptr;

	CodePiece code;
	if(!script.getCode (code))
		return nullptr;

	JS::RootedObject obj (context, JS_NewPlainObject (context));

	JS::SourceText<char16_t> sourceCode;
	if(!sourceCode.init (context, reinterpret_cast<const char16_t*> (code.code), code.length, JS::SourceOwnership::Borrowed))
		return nullptr;

	MutableCString fileName;
	fileName.append (makeScriptFileName (script, code.fileName), Text::kISOLatin1);
	JS::CompileOptions options (context);
	options.setFileAndLine (fileName, code.lineNumber);

	if(scriptStack.isEmpty ())
		createScopeStack ();

	bool succeeded = scopeStack->append (obj);
	ASSERT (succeeded)
	scriptStack.push (&script);

	JS::RootedValue retVal (context);
	bool result = JS::Evaluate (context, *scopeStack, options, sourceCode, &retVal);

	scriptStack.pop ();
	scopeStack->chain ().popBack ();

	if(scriptStack.isEmpty ())
		deleteScopeStack ();

	ASSERT (result)
	if(!result)
		return nullptr;

	return ScriptObject::createInstance (obj, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API Context::createObject (CStringRef className, const Variant args[], int argCount)
{
	RealmScope guard (this, global);
	if(!guard.isValid ())
		return nullptr;

	auto typedArrayConstruct = [&] (js::Scalar::Type type, uint32_t numElements) -> JSObject*
	{
		switch(type)
		{
		case js::Scalar::Int8 :			return JS_NewInt8Array (context, numElements);
		case js::Scalar::Uint8 :		return JS_NewUint8Array (context, numElements);
		case js::Scalar::Uint8Clamped :	return JS_NewUint8ClampedArray (context, numElements);
		case js::Scalar::Int16 :		return JS_NewInt16Array (context, numElements);
		case js::Scalar::Uint16 :		return JS_NewUint16Array (context, numElements);
		case js::Scalar::Int32 :		return JS_NewInt32Array (context, numElements);
		case js::Scalar::Uint32 :		return JS_NewUint32Array (context, numElements);
		case js::Scalar::Float32 :		return JS_NewFloat32Array (context, numElements);
		case js::Scalar::Float64 :		return JS_NewFloat64Array (context, numElements);
		};
		return nullptr;
	};

	JS::RootedObject obj (context);
	if(className == "Object")
		obj.set (JS_NewObject (context, nullptr));
	else if(className == "Array")
		obj.set (JS::NewArrayObject (context, 0));
	else
	{
		static const struct { js::Scalar::Type type; CStringPtr name; } typedArrayClasses[] =
		{
			{js::Scalar::Int8,			"Int8Array"},
			{js::Scalar::Uint8,			"Uint8Array"},
			{js::Scalar::Uint8Clamped,	"Uint8ClampedArray"},
			{js::Scalar::Int16,			"Int16Array"},
			{js::Scalar::Uint16,		"Uint16Array"},
			{js::Scalar::Int32,			"Int32Array"},
			{js::Scalar::Uint32,		"Uint32Array"},
			{js::Scalar::Float32,		"Float32Array"},
			{js::Scalar::Float64,		"Float64Array"}
		};

		for(int i = 0; i < ARRAY_COUNT (typedArrayClasses); i++)
			if(className == typedArrayClasses[i].name)
			{
				ASSERT (argCount > 0)
				uint32_t numElements = argCount > 0 ? args[0].asInt () : 0;
				obj.set (typedArrayConstruct (typedArrayClasses[i].type, numElements));
				break;
			}
	}

	if(obj != nullptr)
		return ScriptObject::createInstance (obj, this);
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Context::registerGlobalFunction (CStringRef methodName, IObject* nativeObject)
{
	RealmScope guard (this, global);
	if(!guard.isValid ())
		return kResultWrongThread;

	JSFunction* func = JS_DefineFunction (context, global, methodName, &globalCallback, 0, 0);
	ASSERT (func != nullptr)
	if(func == nullptr)
		return kResultFailed;

	globalFunctionMap.add (methodName, nativeObject);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Context::garbageCollect (tbool force)
{
	if(inGarbageCollection)
		return;

	ThreadScope guard (this);
	ScopedVar<bool> scope (inGarbageCollection, true);
	if(force)
		JS_GC (context);
	else
		JS_MaybeGC (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::includeCallback (JSContext* cx, unsigned argc, JS::Value* vp)
{
	bool result = false;
	JS::CallArgs args = JS::CallArgsFromVp (argc, vp);

	Context* This = Context::getNativeContext (cx);
	IEngineHost* host = This->engine.getHost ();
	ASSERT (host != nullptr)

	Variant argValue;
	if(argc >= 1)
		ScriptArguments::toVariant (argValue, args.get (0), cx);

	String includeFileName;
	if(argValue.isString ())
		includeFileName = argValue.asString ();
	else
	{
		// host string conversion might be disabled for this context
		if(UnknownPtr<IStringValue> stringValue = argValue.asUnknown ())
		{
			TextEncoding encoding = stringValue->getEncoding ();
			if(Text::isValidCStringEncoding (encoding))
				includeFileName.appendCString (encoding, stringValue->getCharData (), stringValue->getLength ());
			else if(Text::isUTF16Encoding (encoding))
				includeFileName.append (stringValue->getUCharData (), stringValue->getLength ());
		}
	}

	CodePiece code;
	const IScript* currentScript = This->scriptStack.peek ();
	ASSERT (currentScript != nullptr)

	AutoPtr<IScript> script = host ? host->resolveIncludeFile (includeFileName, currentScript) : nullptr;
	if(script && script->getCode (code))
	{
		JS::SourceText<char16_t> sourceCode;
		if(!sourceCode.init (cx, (const char16_t*)code.code, code.length, JS::SourceOwnership::Borrowed))
			return false;

		MutableCString scriptFileName;
		scriptFileName.append (makeScriptFileName (*script, code.fileName), Text::kISOLatin1);
		JS::CompileOptions options (cx);
		options.setFileAndLine (scriptFileName, code.lineNumber);

		This->scriptStack.push (script);

		JS::RootedValue retVal (cx);
		result = JS::Evaluate (cx, *This->scopeStack, options, sourceCode, &retVal);

		This->scriptStack.pop ();
	}

	if(!result)
	{
		if(JS_IsExceptionPending (cx))
		{
			JS::RootedValue exn (cx);
			JS_GetPendingException (cx, &exn);
			JS_ClearPendingException (cx);
			// TODO: report error
		}

		String warning;
		warning.appendFormat ("Failed JavaScript include: \"%(1)\"", includeFileName);
		if(const IScript* parentScript = This->scriptStack.peek ())
		{
			String parentFileName;
			parentScript->getPath ().getName (parentFileName);
			warning.appendFormat (" from \"%(1)/%s(2)\"", parentScript->getPackageID (), parentFileName);
		}
		System::GetLogger ().reportEvent (Alert::Event (warning, Alert::kWarning));
		#if DEBUG
		Debugger::println (warning);
		ASSERT (0)
		#endif
	}

	args.rval ().setBoolean (result);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::getStringArgument (String& result, JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp (argc, vp);
	Variant argValue;
	if(argc >= 1)
		ScriptArguments::toVariant (argValue, args.get (0), cx);

	if(argValue.isString ())
		result = argValue.asString ();
	else
	{
		// host string conversion might be disabled for this context
		if(UnknownPtr<IStringValue> stringValue = argValue.asUnknown ())
		{
			TextEncoding encoding = stringValue->getEncoding ();
			if(Text::isValidCStringEncoding (encoding))
				result.appendCString (encoding, stringValue->getCharData (), stringValue->getLength ());
			else if(Text::isUTF16Encoding (encoding))
				result.append (stringValue->getUCharData (), stringValue->getLength ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Context::globalCallback (JSContext* cx, unsigned argc, JS::Value* vp)
{
	Context* This = Context::getNativeContext (cx);
	JS::CallArgs args = JS::CallArgsFromVp (argc, vp);
	JS::RootedFunction fn (cx, JS_ValueToFunction (cx, args.calleev ()));
	ASSERT (fn)

	JS::RootedString fnId (cx);
	if(!JS_GetFunctionId (cx, fn, &fnId))
		return false;

	Identifier methodId (cx, fnId);

	IObject* object = nullptr;
	This->globalFunctionMap.get (object, methodId.getText ());
	ASSERT (object)
	if(!object)
		return false;

	Variant returnValue;
	ScriptArguments list (args, cx);
	tbool result = object->invokeMethod (returnValue, Message (methodId, list.getArgs (), list.getCount ()));
	ASSERT (result)
	if(!result)
		return false;

	CCL_PRINTF ("ScriptClass::%s returned %s\n", methodId.getText (), logArgument (&returnValue).str ())
	return ScriptArguments::fromVariant (args.rval (), returnValue, cx);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Context::reportError (JS::HandleValue errorValue)
{
	if(!reporter)
		return;

	JS::RootedObject error (context);
	if(!JS_ValueToObject (context, errorValue, &error))
		return;

	JSErrorReport* report = JS_ErrorFromException (context, error);
	if(!report)
		return;
		
	String fileName;
	fileName.appendCString (Text::kISOLatin1, report->filename.c_str ());
	String errorMessage;
	errorMessage.appendCString (Text::kUTF8, report->message ().c_str ());
	String offendingCode;
	offendingCode.append ((const uchar*)report->linebuf (), int(report->linebufLength ()));
	
	int type = Alert::kError;
	if(report->isWarning ())
		type = Alert::kWarning;
	
	#if DEBUG
	MutableCString errorStr (errorMessage);
	MutableCString codeStr (offendingCode);
	Debugger::printf ("%s (%d) : %s: %s\n", report->filename, report->lineno + 1, errorStr.str (), codeStr.str ());
	#endif

	Alert::Event nativeReport (errorMessage, type);
	nativeReport.fileName = fileName;
	nativeReport.lineNumber = report->lineno + 1;
	reporter->reportEvent (nativeReport);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Context::hashString (const CString& key, int size)
{
	return key.getHashCode () % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Context::hashIdentifier (const Identifier& key, int size)
{
	return hashString (key, size);
}

//************************************************************************************************
// JScript::ScriptObject
//************************************************************************************************

ScriptObject* ScriptObject::createInstance (JS::HandleObject object, Context* context)
{
	#if ((LOG_INVOKE || LOG_PROPERTIES) && DEBUG)
	return NEW ScriptObjectDebug (object, context);
	#else
	return NEW ScriptObject (object, context);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ScriptObject::getInstance (JS::HandleObject obj, JSContext* context)
{
	Context* c = Context::getNativeContext (context);
	return static_cast<IUnknown*> (c->getUserData (obj));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptObject* ScriptObject::castUnknown (IUnknown* unknown)
{
	ScriptObject* scriptObject = nullptr;
	UnknownPtr<IOuterUnknown> outerUnknown (unknown);
	if(outerUnknown)
		scriptObject = unknown_cast<ScriptObject> (outerUnknown->getInnerUnknown ());
	else
		scriptObject = unknown_cast<ScriptObject> (unknown);
	return scriptObject;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ScriptObject, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptObject::ScriptObject (JSObject* object, Context* _context)
: nativeContext (_context),
  context (_context->getJSContext ()),
  realm (JS::GetCurrentRealmOrNull (context)),
  obj (context, object),
  type (kUnknown)
{
	if(JS_ObjectIsFunction (obj))
		type = kFunction;
	else if(JS_IsTypedArrayObject (obj))
		type = kTypedArray;
	else if(isArrayOrTypedArray (context, obj))
		type = kArray;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScriptObject::queryInterface (UIDRef iid, void** ptr)
{
	if(iid.equals (ccl_iid<IFunction> ()))
	{
		if(type == kFunction)
		{
			*ptr = static_cast<IFunction*> (this);
			retain ();
			return kResultOk;
		}
	}
	else if(iid.equals (ccl_iid<IBuffer> ()))
	{
		if(type == kTypedArray)
		{
			*ptr = static_cast<IBuffer*> (this);
			retain ();
			return kResultOk;
		}
	}
	else if(iid.equals (ccl_iid<IArrayObject> ()) || iid.equals (ccl_iid<IMutableArray> ()))
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

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptObject::setOuterUnknown (IUnknown* outerUnknown)
{
	ThreadScope guard (nativeContext);

	Context* c = Realm::getNativeRealm (realm)->getContext ();
	c->setUserData (obj, outerUnknown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::getProperty (Variant& var, MemberID propertyId) const
{
	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	JS::RootedValue val (context);
	bool result = JS_GetProperty (context, obj, propertyId, &val);
	ASSERT (result)
	if(!val.isUndefined ())
		ScriptArguments::toVariant (var, val, context);
	return !val.isUndefined ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::setProperty (MemberID propertyId, const Variant& var)
{
	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	JS::RootedValue val (context);
	ScriptArguments::fromVariant (&val, var, context);
	return JS_SetProperty (context, obj, propertyId, val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::invokeMethod (Variant& returnValue, MessageRef msg)
{
	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	JS::RootedValueArray<Message::kMaxMessageArgs> argArray (context);
	for(int i = 0; i < msg.getArgCount (); i++)
		ScriptArguments::fromVariant (argArray[i], msg[i], context);

	JS::RootedValue retval (context);
	bool result = JS_CallFunctionName (context, obj, msg.getID (), argArray, &retval);

	#if DEBUG
	if(!result)
		CCL_DEBUGGER ("JS_CallFunctionName() failed!")
	#endif
	if(!result)
		return false;

	return ScriptArguments::toVariant (returnValue, retval, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::call (Variant& returnValue, IObject* _This, const Variant args[], int argCount)
{
	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	JS::RootedObject jsThis (context);
	if(_This)
	{
		ScriptObject* This = ScriptObject::castUnknown (_This);
		jsThis = This ? This->getJSObject () : nullptr;
		if(!jsThis)
			return false;
	}

	JS::RootedFunction function (context, JS_GetObjectFunction (obj));
	ASSERT(function)
	if(!function)
		return false;

	JS::RootedValueArray<Message::kMaxMessageArgs> argArray (context);
	ASSERT (argCount <= Message::kMaxMessageArgs)
	ccl_upper_limit<int> (argCount, Message::kMaxMessageArgs);
	for(int i = 0; i < argCount; i++)
		ScriptArguments::fromVariant (argArray[i], args[i], context);

	JS::RootedValue retval (context);
	bool result = JS_CallFunction (context, jsThis, function, argArray, &retval);

	#if DEBUG
	if(!result)
		CCL_DEBUGGER ("JS_CallFunctionValue() failed!")
	#endif
	if(!result)
	{
		if(JS_IsExceptionPending (context))
		{
			JS::RootedValue exn (context);
			JS_GetPendingException (context, &exn);
			JS_ClearPendingException (context);
			// TODO: report error
			return false;
		}
	}

	return ScriptArguments::toVariant (returnValue, retval, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScriptObject::getArrayLength () const
{
	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return 0;

	if(type == kTypedArray)
		return int(JS_GetTypedArrayLength (obj));

	uint32_t length = 0;
	bool success = JS::GetArrayLength (context, obj, &length);
	ASSERT (success)
	return int(length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::getArrayElement (Variant& var, int index) const
{
	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	if(type == kTypedArray)
	{
		if(index < 0 || index >= JS_GetTypedArrayLength (obj))
			return false;

		bool isSharedMemory = false;
		JS::AutoAssertNoGC noGC;

		if(JS_IsInt8Array (obj))
			var = JS_GetInt8ArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsUint8Array (obj))
			var = JS_GetUint8ArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsUint8ClampedArray (obj))
			var = JS_GetUint8ClampedArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsInt16Array (obj))
			var = JS_GetInt16ArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsUint16Array (obj))
			var = JS_GetUint16ArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsInt32Array (obj))
			var = JS_GetInt32ArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsUint32Array (obj))
			var = JS_GetUint32ArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsFloat32Array (obj))
			var = JS_GetFloat32ArrayData (obj, &isSharedMemory, noGC)[index];
		else if(JS_IsFloat64Array (obj))
			var = JS_GetFloat64ArrayData (obj, &isSharedMemory, noGC)[index];
		else
		{
			CCL_DEBUGGER ("Unknown type!\n")
			return false;
		}
		return true;
	}
	else
	{
		JS::RootedValue val (context);
		bool result = JS_GetElement (context, obj, index, &val);
		if(!result)
			return false;

		return ScriptArguments::toVariant (var, val, context);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::addArrayElement (VariantRef var)
{
	if(type == kTypedArray)
	{
		CCL_DEBUGGER ("Not supported!\n")
		return false;
	}

	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	unsigned length = 0;
	JS::GetArrayLength (context, obj, &length);
	JS::RootedValue val (context);
	ScriptArguments::fromVariant (&val, var, context);
	return JS_SetElement (context, obj, length, val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::removeArrayElement (int index)
{
	if(type == kTypedArray)
	{
		CCL_DEBUGGER ("Not supported!\n")
		return false;
	}

	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	JS::ObjectOpResult result;
	JS_DeleteElement (context, obj, index, result);
	return result.ok ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptObject::setArrayElement (int index, VariantRef var)
{
	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return false;

	if(type == kTypedArray)
	{
		if(index < 0 || index >= JS_GetTypedArrayLength (obj))
			return false;

		bool isSharedMemory = false;
		JS::AutoAssertNoGC noGC;

		if(JS_IsInt8Array (obj))
			JS_GetInt8ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsUint8Array (obj))
			JS_GetUint8ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsUint8ClampedArray (obj))
			JS_GetUint8ClampedArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsInt16Array (obj))
			JS_GetInt16ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsUint16Array (obj))
			JS_GetUint16ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsInt32Array (obj))
			JS_GetInt32ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsUint32Array (obj))
			JS_GetUint32ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsFloat32Array (obj))
			JS_GetFloat32ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else if(JS_IsFloat64Array (obj))
			JS_GetFloat64ArrayData (obj, &isSharedMemory, noGC)[index] = var;
		else
		{
			CCL_DEBUGGER ("Unknown type!\n")
			return false;
		}
		return true;
	}

	JS::RootedValue val (context);
	ScriptArguments::fromVariant (&val, var, context);
	return JS_SetElement (context, obj, index, val);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API ScriptObject::getBufferAddress () const
{
	if(type != kTypedArray)
		return nullptr;

	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return nullptr;

	bool isSharedMemory = false;
	JS::AutoAssertNoGC noGC;
	return JS_GetArrayBufferViewData (obj, &isSharedMemory, noGC);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::uint32 CCL_API ScriptObject::getBufferSize () const
{
	if(type != kTypedArray)
		return 0;

	RealmScope guard (nativeContext, realm);
	if(!guard.isValid ())
		return 0;

	return CCL::uint32(JS_GetTypedArrayByteLength (obj));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* ScriptObject::determineClassName ()
{
	// obj.constructor.toString () gives the js code of the class
	JS::RootedValue val (context);
	bool result = JS_GetProperty (context, obj, "constructor", &val);
	if(result)
	{
		JS::RootedObject contructorObj (context);
		result = JS_ValueToObject (context, val, &contructorObj);
		if(result && contructorObj)
		{
			JS::RootedValue retval (context);
			JS::RootedValueArray<Message::kMaxMessageArgs> argArray (context);
			result = JS_CallFunctionName (context, contructorObj, "toString", argArray, &retval);
			if(result)
			{
				Variant var;
				ScriptArguments::toVariant (var, retval, context);
				String constructorCode (var.asString ());
				int index = constructorCode.index ("function");
				if(index >= 0)
				{
					constructorCode.remove (0, index + 9);
					ForEachStringToken(constructorCode, " (", s)
						MutableCString cstr (s);
						return CCL::System::GetConstantCString (cstr);
					EndFor
				}
			}
		}
	}
	return nullptr;
}

//************************************************************************************************
// JScript::ScriptObjectDebug
//************************************************************************************************

ScriptObjectDebug::Class::Class (JSObject* object)
: MetaClass (&ccl_typeid<ScriptObject> (), "ScriptObjectDebug", nullptr)
{
	const JSClass* c = JS::GetClass (object);
	className = c->name; // always 'Object' or 'Array' :-(
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptObjectDebug::ScriptObjectDebug (JSObject* object, Context* context)
: ScriptObject (object, context),
  thisClass (object)
{
	if(const char* className = determineClassName ())
		thisClass.className = className;
}

//************************************************************************************************
// JScript::ScriptClass
//************************************************************************************************

const ScriptClass* ScriptClass::getClassSafe (JS::HandleObject obj)
{
	const JSClass* c = JS::GetClass (obj);
	return c && c->isProxyObject () ? static_cast<const ScriptClass*> (c) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass::ScriptClass (Realm* _realm, const ITypeInfo& typeInfo)
: realm (_realm),
  proxyHandler (nullptr),
  prototype (_realm->getContext ()->getJSContext ())
{
	memset ((JSClass*)this, 0, sizeof(JSClass));

	className = "Native";
	className.appendFormat ("_%p_", typeInfo.getModuleReference ());
	className += typeInfo.getClassName ();
	className.replace (':', '_');
	className.replace ('.', '_');
	className.replace (' ', '_');

	this->name = className;
	proxyHandler = NEW ProxyHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass::~ScriptClass ()
{
	delete proxyHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JSObject* ScriptClass::getPrototype () const
{
	return prototype;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScriptClass::setPrototype (JSObject* object)
{
	prototype.set (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScriptClass::nativeDestructor (JS::GCContext* gcx, JSObject* obj)
{
	const JS::Value& target = js::GetProxyPrivate (obj);

	if(target.isNull () == false)
		if(IObject* nativeObj = static_cast<IObject*> (target.toPrivate ()))
		{
			#if DEBUG_LOG
			nativeObj->retain ();
			int refCount = nativeObj->release ();
			CCL_PRINTF ("ScriptClass::nativeDestructor (%x) %s%s\n", obj, nativeObj->getTypeInfo ().getClassName (), refCount == 1 ? " (DESTROY)" : "")
			#endif

			// remove from object map
			realm->unregisterNativeObject (nativeObj);

			nativeObj->release ();

			js::SetProxyPrivate (obj, JS::NullValue ());
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClass::getNativeProperty (JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Value* vp)
{
	const JS::Value& target = js::GetProxyPrivate (obj);
	IObject* nativeObj = static_cast<IObject*> (target.toPrivate ());

	ASSERT (nativeObj != nullptr)
	if(!nativeObj)
		return false;
	if(!id.isString ())
		return false;

	Identifier propertyId (cx, id);

	Variant var;
	if(!nativeObj->getProperty (var, propertyId))
		return false;

	if(vp)
	{
		JS::RootedValue rootedValue (cx);
		ScriptArguments::fromVariant (&rootedValue, var, cx);
		*vp = rootedValue;

		CCL_PRINTF ("getNativeProperty: \t%s.%s returned %s\n", MutableCString (nativeObj->getTypeInfo ().getClassName ()).str (), propertyId.getText (), logArgument (&var).str ())
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClass::getterSetter (JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp (argc, vp);
	JS::HandleValue thisv = args.thisv ();
	JS::RootedObject obj (cx);
	if(!JS_ValueToObject (cx, thisv, &obj))
		return false;
	JS::MutableHandleValue result = args.rval ();

	IObject* nativeObj = nullptr;
	const JS::Value& target = js::GetProxyPrivate (obj);
	nativeObj = static_cast<IObject*> (target.toPrivate ());

	Variant var;
	Identifier propertyId (cx, args.get (0).toString ());
	if(argc == 1)
	{
		nativeObj->getProperty (var, propertyId);
		ScriptArguments::fromVariant (result, var, cx);
	}
	else if(argc == 2)
	{
		ScriptArguments::toVariant (var, args.get (1), cx);
		nativeObj->setProperty (propertyId, var);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClass::invokeNativeMethod (JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp (argc, vp);
	JS::HandleValue thisv = args.thisv ();
	JS::RootedObject obj (cx);
	if(!JS_ValueToObject (cx, thisv, &obj))
		return false;

	IObject* nativeObj = nullptr;
	const JS::Value& target = js::GetProxyPrivate (obj);
	nativeObj = static_cast<IObject*> (target.toPrivate ());

	ASSERT (nativeObj)
	if(!nativeObj)
		return false;

	JS::RootedFunction fn (cx, JS_ValueToFunction (cx, args.calleev ()));
	ASSERT (fn)

	JS::RootedString fnId (cx);
	if(!JS_GetFunctionId (cx, fn, &fnId))
		return false;

	Identifier methodId (cx, fnId);

	Variant returnValue;
	ScriptArguments list (args, cx);
	tbool result = nativeObj->invokeMethod (returnValue, Message (methodId, list.getArgs (), list.getCount ()));
	ASSERT (result)
	if(!result)
		return false;

	CCL_PRINTF ("invokeNativeMethod: \t%s.%s returned %s\n", MutableCString (nativeObj->getTypeInfo ().getClassName ()).str (), methodId.getText (), logArgument (&returnValue).str ())

	return ScriptArguments::fromVariant (args.rval (), returnValue, cx);
}

//************************************************************************************************
// JScript::UserDataClass
//************************************************************************************************

UserDataClass::UserDataClass ()
{
	memset ((JSClass*)this, 0, sizeof(JSClass));
	name = "NativeUserDataClass";
	flags = JSCLASS_HAS_RESERVED_SLOTS(1);
}

//************************************************************************************************
// JScript::Identifier
//************************************************************************************************

Identifier::Identifier ()
{
	buffer[0] = 0;
	plainCString.text = buffer;
	plainCString.theString = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Identifier::Identifier (CStringRef string)
{
	buffer[0] = 0;
	plainCString.text = buffer;
	plainCString.theString = this;

	construct (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Identifier::Identifier (JSContext* cx, JSString* string)
{
	buffer[0] = 0;
	plainCString.text = buffer;
	plainCString.theString = this;

	ASSERT (string)
	construct (cx, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Identifier::Identifier (JSContext* cx, JS::PropertyKey id)
{
	buffer[0] = 0;
	plainCString.text = buffer;
	plainCString.theString = this;
	construct (cx, id.toString ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Identifier::construct (CStringRef string)
{
	string.copyTo (buffer, sizeof(buffer));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Identifier::construct (JSContext* cx, JSString* string)
{
	JS::AutoAssertNoGC noGC;
	size_t length = 0;
	if(const JS::Latin1Char* ptr = JS_GetLatin1StringCharsAndLength (cx, noGC, string, &length))
	{
		if(length > kMaxLen - 1)
			length = kMaxLen - 1;
		for(size_t i = 0; i < length; i++)
			buffer[i] = (char)ptr[i];
		buffer[length] = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Identifier::resize (int newLength)
{
	CCL_NOT_IMPL ("Must not get here!!!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

char* CCL_API Identifier::getText ()
{
	return buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICString* CCL_API Identifier::cloneString () const
{
	CCL_NOT_IMPL ("Must not get here!!!\n")
	return nullptr;
}

//************************************************************************************************
// JScript::StringValue / PoolString
//************************************************************************************************

DEFINE_OBJECTPOOL_SIZE (PoolString, CCL::MemoryPool, 128)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringValue* StringValue::create (JSContext* cx, JSString* string)
{
	StringValue* v = PoolString::pool_new (cx, string);
	if(v == nullptr)
		v = NEW StringValue (cx, string);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringValue::StringValue (JSContext* cx, JSString* _string)
: context (cx),
  string (cx, _string)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uchar* CCL_API StringValue::getUCharData () const
{
	if(JS::StringHasLatin1Chars (string))
	{
		ASSERT (0)
		return nullptr;
	}
	else
	{
		JS::AutoAssertNoGC noGC;
		size_t length = 0;
		return reinterpret_cast<const uchar*> (JS_GetTwoByteStringCharsAndLength (context, noGC, string, &length));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* CCL_API StringValue::getCharData () const
{
	if(JS::StringHasLatin1Chars (string))
	{
		JS::AutoAssertNoGC noGC;
		size_t length = 0;
		return reinterpret_cast<const char*> (JS_GetLatin1StringCharsAndLength (context, noGC, string, &length));
	}
	else
	{
		ASSERT (0)
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::TextEncoding CCL_API StringValue::getEncoding () const
{
	if(JS::StringHasLatin1Chars (string))
		return CCL::Text::kISOLatin1;
	else
		return CCL::Text::kUTF16;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StringValue::getLength () const
{
	JS::AutoAssertNoGC noGC;
	size_t length = 0;
	if(JS::StringHasLatin1Chars (string))
		JS_GetLatin1StringCharsAndLength (context, noGC, string, &length);
	else
		JS_GetTwoByteStringCharsAndLength (context, noGC, string, &length);
	return int(length);
}

//************************************************************************************************
// JScript::ScriptArguments
//************************************************************************************************

ScriptArguments::ScriptArguments (const JS::CallArgs& callArgs, JSContext* cx)
: count (ccl_min<int> (callArgs.length (), Message::kMaxMessageArgs))
{
	for(int i = 0; i < count; i++)
		toVariant (args[i], callArgs.get (i), cx);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptArguments::toVariant (Variant& var, JS::HandleValue val, JSContext* cx)
{
	var.clear ();

	if(val.isInt32 ())
		var = val.toInt32 ();
	else if(val.isBigInt ())
		var = int64(ToBigInt64 (val.toBigInt ()));
	else if(val.isDouble ())
		var = val.toDouble ();
	else if(val.isBoolean ())
		var = val.toBoolean ();
	else if(val.isString ())
	{
		JSString* string = val.toString ();
		if(string)
		{
			Context* context = Context::getNativeContext (cx);
			if(context->isHostStringsEnabled ())
			{
				JSLinearString* lstr = JS_EnsureLinearString (cx, string);
				ASSERT (lstr)
				if(lstr == nullptr) // this should never happen, except in out of memory situations
					return false;

				size_t length = JS::GetLinearStringLength (lstr);
				String temp;
				if(length > 0)
				{
					JS::AutoAssertNoGC noGC;
					if(JS::LinearStringHasLatin1Chars (lstr))
					{
						const char* ptr = reinterpret_cast<const char*> (JS::GetLatin1LinearStringChars (noGC, lstr));
						temp.appendCString (Text::kISOLatin1, ptr, int(length));
					}
					else
					{
						const uchar* ptr = reinterpret_cast<const uchar*> (JS::GetTwoByteLinearStringChars (noGC, lstr));
						temp.assign (ptr, int(length));
					}
				}
				var = temp;
				var.share ();
			}
			else
				var.takeShared (AutoPtr<IStringValue> (StringValue::create (cx, string)));
		}
	}
	else if(val.isObject ())
	{
		JS::RootedObject obj (cx, val.toObjectOrNull ());
		const ScriptClass* c = ScriptClass::getClassSafe (obj);
		if(c) // it's a native object
		{
			IObject* nativeObj = nullptr;
			const JS::Value& target = js::GetProxyPrivate (obj);
			nativeObj = static_cast<IObject*> (target.toPrivate ());
			ASSERT (nativeObj)
			var.takeShared (nativeObj);
		}
		else
		{
			// check if a native object exists already for this JSObject,
			// i.e. when native code holds a reference, we have to preserve object identity...
			IUnknown* unknown = ScriptObject::getInstance (obj, cx);
			if(unknown)
				var.takeShared (unknown);
			else
			{
				Context* context = Context::getNativeContext (cx);
				AutoPtr<ScriptObject> scriptObject = ScriptObject::createInstance (obj, context);
				if(context->isStubNeeded (JS::RootedObject (cx, scriptObject->getJSObject ())) == false)
					var.takeShared (scriptObject->asUnknown ());
				else
				{
					IObject* stubObject = context->createStub (scriptObject);
					ASSERT (stubObject)
					var.takeShared (stubObject);
					stubObject->release ();
				}
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptArguments::fromVariant (JS::MutableHandleValue val, const Variant& var, JSContext* cx)
{
	val.setUndefined ();
	short type = var.getType ();

	if(type == Variant::kInt)
	{
		int64 intValue = var;
		ASSERT (intValue >= NumericLimits::kMinInt32 && intValue <= NumericLimits::kMaxInt32)
		// LATER TODO: add support for BigInt? val.setBigInt (JS::detail::BigIntFromInt64 (cx, intValue));
		val.setInt32 (int32(intValue));
	}
	else if(type == Variant::kFloat)
	{
		val.setDouble (var.fValue);
	}
	else if(type == Variant::kString)
	{
		String s = var;
		StringChars chars (s);
		JSString* jsString = JS_NewUCStringCopyZ (cx, reinterpret_cast<const char16_t*> ((const uchar*)chars));
		val.setString (jsString);
	}
	else if(type == Variant::kObject)
	{
		if(Context* context = Context::getNativeContext (cx))
			if(JSObject* obj = context->resolveObject (var))
				val.setObject (*obj);
			else
				val.setNull ();
	}

	return true;
}

//************************************************************************************************
// JScript::ProxyHandler
//************************************************************************************************

ProxyHandler::ProxyHandler (ScriptClass* _scriptClass)
: js::BaseProxyHandler (reinterpret_cast<void*> (1), true),
  scriptClass (_scriptClass)
{
	ASSERT (_scriptClass)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProxyHandler::hasOwn (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, bool* result) const
{
	bool found = false;
	bool success = JS_HasOwnPropertyById (cx, JS::RootedObject (cx, scriptClass->getPrototype ()), id, &found);
	ASSERT (success)
	if(found)
	{
		*result = found;
		return true;
	}

	*result = scriptClass->getNativeProperty (cx, proxy, id, nullptr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProxyHandler::getOwnPropertyDescriptor (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc) const
{
	bool success = JS_GetOwnPropertyDescriptorById (cx, JS::RootedObject (cx, scriptClass->getPrototype ()), id, desc);
	ASSERT (success)
	if(desc.isSome ())
		return true;

	// ensure accessors are defined
	Context* context = Context::getNativeContext (cx);
	Identifier propertyId (cx, id);
	if(PropertyAccessor* accessor = context->getPropertyAccessor (propertyId))
	{
		// create descriptor
		JS::PropertyDescriptor proxyDesc = JS::PropertyDescriptor::Accessor (accessor->getter, accessor->setter);
		desc.set (mozilla::Some (proxyDesc));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProxyHandler::finalize (JS::GCContext* gcx, JSObject* proxy) const
{
	scriptClass->nativeDestructor (gcx, proxy);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProxyHandler::defineProperty (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult& result) const
{
	ASSERT (false)
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProxyHandler::ownPropertyKeys (JSContext* cx, JS::HandleObject proxy, JS::MutableHandleIdVector props) const
{
	const JS::Value& target = js::GetProxyPrivate (proxy);
	if(IObject* nativeObj = static_cast<IObject*> (target.toPrivate ()))
	{
		PropertyCollector collector (cx, props);
		nativeObj->getPropertyNames (collector);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProxyHandler::delete_ (JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult& result) const
{
	ASSERT (false)
	return true;
}
