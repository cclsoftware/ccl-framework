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
// Filename    : ccl/extras/nodejs/napihelpers.h
// Description : Helper classes for N-API
//
//************************************************************************************************

#ifndef _ccl_napihelpers_h
#define _ccl_napihelpers_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/iobject.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/plugins/iscriptengine.h"
#include "ccl/public/collections/vector.h"

#include "nodejs/include/node_api.h"

namespace CCL {
namespace NodeJS {

//*************************************************************************************************
// NAPIValue
//*************************************************************************************************

class NAPIValue
{
public:
	NAPIValue (napi_env environment, napi_value value);

	napi_valuetype getType () const;
	napi_env getEnvironment () const;
	NAPIValue call () const;
	
	operator napi_value () const;
	napi_value* operator & ();
	
	void setProperty (const char* name, const NAPIValue& property);
	Variant toVariant () const;

	static NAPIValue wrap (napi_env environment, void *object, napi_finalize finalizeCallback);
	template<class Type> Type* unwrap () const;

	static NAPIValue getNull (napi_env environment);
	static NAPIValue getUndefined (napi_env environment);
	static NAPIValue fromString (napi_env environment, CStringPtr string);
	static NAPIValue fromString (napi_env environment, StringRef string);
	static NAPIValue fromInt32 (napi_env environment, int32 value);
	static NAPIValue fromInt64 (napi_env environment, int64 value);
	static NAPIValue fromFloat64 (napi_env environment, float64 value);
	static NAPIValue fromFunction(napi_env environment, napi_callback callback);
	static NAPIValue fromVariant (napi_env environment, const Variant& variant);
	static NAPIValue fromObject (napi_env environment, IObject *object);

private:
	napi_env environment;
	napi_value value;

	static void addObjectProperties (Vector<napi_property_descriptor>& properties, const ITypeInfo& typeInfo);
	static napi_value cclObjectInvokeMethod (napi_env env, napi_callback_info info);
	static napi_value cclObjectGetProperty (napi_env env, napi_callback_info info);
	static napi_value cclObjectSetProperty (napi_env env, napi_callback_info info);
	static void cclObjectFinalize (napi_env env, void* finalize_data, void* finalize_hint);
};

///////////////////////////////////////////////////////////////////////////////////////////////////

template<class Type>
Type* NAPIValue::unwrap () const
{
	napi_status status;
	void* result;

	status = napi_unwrap (environment, value, &result);
	assert (status == napi_ok);

	return reinterpret_cast<Type*>(result);
}

//*************************************************************************************************
// NAPIReference
//*************************************************************************************************

class NAPIReference
{
public:
	NAPIReference ();
	NAPIReference (napi_env environment, napi_ref reference);
	NAPIReference (const NAPIReference& other);
	NAPIReference (NAPIReference&& other);
	~NAPIReference ();

	napi_env getEnvironment () const;
	NAPIValue getValue () const;
	void release ();
	
	NAPIReference& operator = (const NAPIReference& rhs);
	NAPIReference& operator = (NAPIReference&& rhs);
	bool operator == (std::nullptr_t) const;
	bool operator != (std::nullptr_t) const;
	operator napi_ref () const;
	napi_ref* operator & ();

	static NAPIReference fromValue (const NAPIValue& value);

private:
	napi_env environment;
	napi_ref reference;
};

//*************************************************************************************************
// NAPICallbackInfo
//*************************************************************************************************

class NAPICallbackInfo
{
public:
	NAPICallbackInfo (napi_env environment, napi_callback_info info);
	NAPICallbackInfo (const NAPICallbackInfo& other) = delete;
	NAPICallbackInfo operator = (const NAPICallbackInfo& rhs) = delete;
	
	NAPIValue getThisArg () const;
	size_t getArgCount () const;
	NAPIValue getArgAt (size_t index) const;

private:
	napi_env environment;
	napi_value argThis;
	size_t argCount;

	union
	{
		napi_value* pointer;
		napi_value array[6];
	} argVector;
};

//*************************************************************************************************
// NAPIHandleScope
//*************************************************************************************************

class NAPIHandleScope
{
public:
	NAPIHandleScope (napi_env environment);
	~NAPIHandleScope ();

private:
	napi_env environment;
	napi_handle_scope scope;
};

//*************************************************************************************************
// NAPIFunction
//*************************************************************************************************

class NAPIFunction: public Unknown, 
					public Scripting::IFunction
{
public:
	NAPIFunction (napi_env environment, napi_value value);
	~NAPIFunction ();

	// IFunction
	tbool CCL_API call (Variant& returnValue, IObject* This = 0, const Variant args[] = 0, int argCount = 0) override;

	CLASS_INTERFACE (Scripting::IFunction, Unknown);

private:
	napi_env environment;
	napi_ref reference;
};

//*************************************************************************************************
// NAPIThreadsafeFunction
//*************************************************************************************************

class NAPIThreadsafeFunction: public Unknown, 
							  public Scripting::IFunction
{
public:
	NAPIThreadsafeFunction ();
	~NAPIThreadsafeFunction ();

	void create (NAPIValue functionValue, CStringPtr functionName);
	void call (IMessage &message);

	// IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	unsigned int CCL_API retain () override;
	unsigned int CCL_API release () override;

	// IFunction
	tbool CCL_API call (Variant& returnValue, IObject* This = 0, const Variant args[] = 0, int argCount = 0) override;

private:
	napi_env environment;
	napi_value callback;
	napi_threadsafe_function function;
	uint8* callData;

	static void invokeCallbackFromJS (napi_env environment, napi_value callback, void* context, void* data);
};

//*************************************************************************************************
// Utility functions
//*************************************************************************************************

void handleNAPIError (napi_env environment, napi_status status);

#define CHECK_NAPI_STATUS(expr)                                 \
	do                                                          \
	{                                                           \
	    status = (expr);                                        \
	    if(status != napi_ok)                                   \
	    {                                                       \
	        CCL::NodeJS::handleNAPIError (environment, status); \
	        goto error;                                         \
	    }                                                       \
	}                                                           \
	while(0)

#define DECLARE_NAPI_ACCESSOR(name, getter, setter) \
	{ name, 0, nullptr, getter, setter, nullptr, napi_default, nullptr }
#define DECLARE_NAPI_METHOD(name, func) \
	{ name, 0, func, 0, 0, 0, napi_default, 0 }

} // namespace NodeJS
} // namespace CCL

#endif // _ccl_napihelpers_h
