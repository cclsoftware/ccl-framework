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
// Filename    : jscontext.h
// Description : JavaScript Context
//
//************************************************************************************************

#ifndef _jscontext_h
#define _jscontext_h

#include "ccl/base/object.h"
#include "ccl/base/memorypool.h"
#include "ccl/base/message.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/stack.h"
#include "ccl/public/plugins/iscriptengine.h"
#include "ccl/public/plugins/idebugservice.h"

#include "jsinclude.h"
#include "jsclassregistry.h"
#include "jscrossthread.h"

namespace JScript {

class Context;
class Engine;
class Realm;
class ProxyHandler;

//************************************************************************************************
// JScript::ScriptClass
//************************************************************************************************

class ScriptClass: public JSClass
{
public:
	ScriptClass (Realm* realm, const CCL::ITypeInfo& typeInfo);
	~ScriptClass ();

	static const ScriptClass* getClassSafe (JS::HandleObject obj);
	static bool getNativeProperty (JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Value* vp);
	static bool getterSetter (JSContext* cx, unsigned argc, JS::Value* vp);
	static bool invokeNativeMethod (JSContext* cx, unsigned argc, JS::Value* vp);

	void nativeDestructor (JS::GCContext* gcx, JSObject* obj);

	JSObject* getPrototype () const;
	void setPrototype (JSObject* object);

	const ProxyHandler* getProxyHandler () const { return proxyHandler; }

protected:
	CCL::MutableCString className;
	Realm* realm;
	JS::PersistentRootedObject prototype;
	ProxyHandler* proxyHandler;
};

//************************************************************************************************
// JScript::Identifier
/** Can be used as CCL::StringID (e.g. in a Message object) without additional memory allocation. */
//************************************************************************************************

class Identifier: public CCL::Unknown,
				  public CCL::ICString
{
public:
	Identifier ();
	Identifier (CCL::CStringRef string);
	Identifier (JSContext* cx, JSString* string);
	Identifier (JSContext* cx, JS::PropertyKey id);

	operator CCL::StringID () const
	{
		return reinterpret_cast<CCL::StringID> (plainCString);
	}

	operator const char* () const
	{
		return buffer;
	}

	bool operator == (const Identifier& other) const
	{
		return CCL::CString (buffer).compare (other.buffer) == 0;
	}

	// ICString
	CCL::tbool CCL_API resize (int newLength) override;
	char* CCL_API getText () override;
	CCL::ICString* CCL_API cloneString () const override;

	CLASS_INTERFACE (ICString, Unknown)

protected:
	static const int kMaxLen = 128;
	char buffer[kMaxLen];
	CCL::PlainCString plainCString;

	void construct (CCL::CStringRef string);
	void construct (JSContext* cx, JSString* string);
};

//************************************************************************************************
// JScript::StringValue
//************************************************************************************************

class StringValue: public CCL::Unknown,
				   public CCL::Scripting::IStringValue
{
public:
	StringValue (JSContext* cx, JSString* string);

	static StringValue* create (JSContext* cx, JSString* string);

	// IStringValue
	const CCL::uchar* CCL_API getUCharData () const override;
	const char* CCL_API getCharData () const override;
	int CCL_API getLength () const override;
	CCL::TextEncoding CCL_API getEncoding () const override;

	CLASS_INTERFACE (IStringValue, Unknown)

protected:
	JSContext* context;
	JS::RootedString string;
};

//************************************************************************************************
// JScript::PoolString
//************************************************************************************************

class PoolString: public StringValue,
				  public CCL::PooledObject<PoolString, CCL::MemoryPool>
{
public:
	PoolString (JSContext* cx, JSString* string)
	: StringValue (cx, string)
	{}
};

//************************************************************************************************
// JScript::UserDataClass
//************************************************************************************************

struct UserDataClass: JSClass
{
	UserDataClass ();
};

//************************************************************************************************
// JScript::NativeObjectMap
//************************************************************************************************

class NativeObjectMap: public CCL::PointerHashMap<JSObject*>
{
public:
	NativeObjectMap ()
	: CCL::PointerHashMap<JSObject*> (512)
	{}
};

//************************************************************************************************
// JScript::PropertyAccessor
//************************************************************************************************

struct PropertyAccessor
{
	JS::Heap<JSObject*> getter;
	JS::Heap<JSObject*> setter;
};

//************************************************************************************************
// JScript::Realm
//************************************************************************************************

class Realm: public ClassRegistry
{
public:
	Realm (Context* context);

	static Realm* getNativeRealm (JS::Realm* realm)
	{
		return static_cast<Realm*> (JS::GetRealmPrivate (realm));
	}

	Context* getContext () const { return context; }

	void registerNativeObject (CCL::IObject* nativeObject, JSObject* obj);
	bool unregisterNativeObject (CCL::IObject* nativeObject);
	JSObject* lookupNativeObject (CCL::IObject* nativeObject) const;
	void dumpNativeObjects () const;

protected:
	Context* context;
	NativeObjectMap nativeObjects;
};

//************************************************************************************************
// JScript::Context
//************************************************************************************************

class Context: public CCL::Object,
			   public CCL::Scripting::IContext
{
public:
	DECLARE_CLASS_ABSTRACT (Context, Object)

	static constexpr int kPrivateDataSlot = 0;

	Context (Engine& engine, ::JSContext* context);
	~Context ();

	static Context* getNativeContext (JSContext* cx)
	{
		return Realm::getNativeRealm (JS::GetCurrentRealmOrNull (cx))->getContext ();
	}

	virtual void initialize ();

	JSContext* getJSContext () const { return context; }
	JS::GCContext* getGCContext () const { return gcContext; }

	ScriptClass* resolveClass (const CCL::ITypeInfo& typeInfo); ///< add class if not registered
	JSObject* resolveObject (IUnknown* nativeObject);
	
	bool isStubNeeded (JS::HandleObject obj) const;
	bool isHostStringsEnabled () const { return hostStringsEnabled; }
	bool setUserData (JS::HandleObject obj, IUnknown* userData);
	IUnknown* getUserData (JS::HandleObject obj) const;

	IObject* createStub (IObject* scriptObject);
	const CCL::Scripting::IScript* peekScript () const;
	void onGCFinished ();

	PropertyAccessor* getPropertyAccessor (const Identifier& id);
	void tracePropertyAccessors (JSTracer* tracer);

	void reportError (JS::HandleValue error);

	// Scripting::IContext
	CCL::Scripting::IEngine& CCL_API getEngine () const override;
	CCL::tresult CCL_API setOption (CCL::StringID id, CCL::VariantRef value) override;
	void CCL_API attachModule (CCL::ModuleRef module) override;
	void CCL_API detachModule (CCL::ModuleRef module) override;
	CCL::tresult CCL_API registerObject (CCL::CStringRef name, CCL::IObject* nativeObject) override;
	CCL::IObject* CCL_API createObject (CCL::CStringRef className, const CCL::Variant args[], int argCount) override;
	CCL::tresult CCL_API registerGlobalFunction (CCL::CStringRef methodName, CCL::IObject* nativeObject) override;
	CCL::tresult CCL_API setReporter (CCL::Alert::IReporter* reporter) override;
	CCL::tresult CCL_API executeScript (CCL::Variant& returnValue, const CCL::Scripting::IScript& script) override;
	CCL::IObject* CCL_API compileScript (const CCL::Scripting::IScript& script) override;
	void CCL_API garbageCollect (CCL::tbool force) override;
	CCL::tbool CCL_API removeReference (IUnknown* nativeObject) override;
	void CCL_API dump () override;

	CLASS_INTERFACE (IContext, Object)

protected:
	static const UserDataClass userDataClass;

	Engine& engine;
	JSContext* context;
	JS::GCContext* gcContext;

	JS::PersistentRootedObject global;
	JS::PropertyKey userDataID;
	CCL::LinkedList<JS::PersistentRootedObject*> pendingUserDataResets;

	Realm* realm;

	bool stubObjectsEnabled;
	bool hostStringsEnabled;
	bool inGarbageCollection;
	bool globalInitialized;

	CCL::Alert::IReporter* reporter;
	CCL::HashMap<CCL::CString, CCL::IObject*> globalFunctionMap;
	CCL::HashMap<Identifier, PropertyAccessor*> propertyAccessorMap;
	JS::EnvironmentChain* scopeStack;
	CCL::Stack<const CCL::Scripting::IScript*> scriptStack;

	ScriptClass* registerClass (const CCL::ITypeInfo& typeInfo);
	bool defineMethods (JS::HandleObject prototype, const CCL::ITypeInfo& typeInfo);

	void createScopeStack ();
	void deleteScopeStack ();

	void cleanupPropertyAccessors ();
	CCL::tresult executeScriptInternal (CCL::Variant& returnValue, const CCL::Scripting::IScript& script);

	static void destroyRealmCallback (JS::GCContext* gcx, JS::Realm* realm);

	static bool includeCallback (JSContext* cx, unsigned argc, JS::Value* vp);
	static bool globalCallback (JSContext* cx, unsigned argc, JS::Value* vp);

	static void getStringArgument (CCL::String& result, JSContext* cx, unsigned argc, JS::Value* vp);

	static int hashString (const CCL::CString& key, int size);
	static int hashIdentifier (const Identifier& key, int size);
};

//************************************************************************************************
// JScript::RealmScope
//************************************************************************************************

class RealmScope
{
public:
	RealmScope (Context* _context, JSObject* target);
	RealmScope (Context* _context, JS::Realm* target);
	~RealmScope ();

	bool isValid () const { return context != nullptr; }

private:
	ThreadScope threadScope;
	Context* context;
	JS::Realm* oldRealm;
};

//************************************************************************************************
// JScript::ScriptArguments
//************************************************************************************************

class ScriptArguments
{
public:
	ScriptArguments (const JS::CallArgs& callArgs, JSContext* cx);

	const CCL::Variant* getArgs () const { return args; }
	int getCount () const { return count; }

	static bool toVariant (CCL::Variant& var, JS::HandleValue val, JSContext* cx);
	static bool fromVariant (JS::MutableHandleValue val, const CCL::Variant& var, JSContext* cx);

protected:
	CCL::Variant args[CCL::Message::kMaxMessageArgs];
	int count;
};

} // namespace JScript

#endif // _jscontext_h
