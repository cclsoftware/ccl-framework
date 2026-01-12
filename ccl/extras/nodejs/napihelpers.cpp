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
// Filename    : ccl/extras/nodejs/napihelpers.cpp
// Description : Helper classes for N-API
//
//************************************************************************************************

#include "ccl/extras/nodejs/napihelpers.h"

#include "ccl/base/message.h"

#include "ccl/public/system/threadsync.h"

#include <memory>

using namespace Core;
using namespace CCL;
using namespace NodeJS;

namespace CCL {
namespace NodeJS {

//************************************************************************************************
// NAPIBlockingCall
//************************************************************************************************

class NAPIBlockingCall
{
public:
	NAPIBlockingCall (IObject* thisArg, const Variant* args, int argCount);

	void invoke (NAPIValue function);
	void wait ();

	const Variant& getResult () const { return result; }

private:
	IObject* thisArg;
	const Variant* args;
	int argCount;
	Variant result;
	CCL::Threading::Signal signal;
};

} // namespace NodeJS
} // namespace CCL

//*************************************************************************************************
// NAPIValue
//*************************************************************************************************

NAPIValue::NAPIValue (napi_env environment, napi_value value)
: environment (environment),
  value (value)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_valuetype NAPIValue::getType () const
{
	napi_valuetype result;
	napi_status status;

	status = napi_typeof (environment, value, &result);
	assert (status == napi_ok);

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_env NAPIValue::getEnvironment () const
{
	return environment;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::call () const
{
	napi_status status;

	napi_value global;
	status = napi_get_global (environment, &global);
	assert (status == napi_ok);

	napi_value result;
	status = napi_call_function (environment, global, value, 0, nullptr, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue::operator napi_value () const
{
	return value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_value* NAPIValue::operator & ()
{
	return &value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIValue::setProperty (const char* name, const NAPIValue& property)
{
	napi_status status = napi_set_named_property (environment, value, name, property);
	assert (status == napi_ok);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Variant NAPIValue::toVariant () const
{
	napi_status status;
	napi_valuetype type;

	status = napi_typeof (environment, value, &type);
	assert (status == napi_ok);

	switch(type)
	{
	case napi_undefined:
		return Variant ();

	case napi_null:
		return Variant ();

	case napi_boolean:
		{
			bool result;
			status = napi_get_value_bool (environment, value, &result);
			assert (status == napi_ok);
			return result;
		}

	case napi_number:
	{
		//int32 result;
		//status = napi_get_value_int32 (environment, value, &result);

		double result;
		status = napi_get_value_double (environment, value, &result);
		assert (status == napi_ok);
		return result;
	}

	case napi_string:
	{
		size_t length = 0;

		status = napi_get_value_string_utf16 (environment, value, nullptr, 0, &length);
		assert (status == napi_ok);

		char16_t* buffer = new char16_t[length + 1];
		status = napi_get_value_string_utf16 (environment, value, buffer, length + 1, &length);
		assert (status == napi_ok);

		String string(reinterpret_cast<const uchar *>(buffer));
		delete[] buffer;

		return Variant(string, true);
	}

	case napi_symbol:
		return Variant ();

	case napi_object:
		{
			IUnknown* object = nullptr;
			status = napi_unwrap (environment, value, reinterpret_cast<void**>(&object));
			assert (status == napi_ok);

			return Variant (object, true);
		}

	case napi_function:
		return Variant (static_cast<CCL::Scripting::IFunction*>(new NAPIFunction (environment, value)));

	case napi_external:
		return Variant ();

	case napi_bigint:
		return Variant ();

	default:
		return Variant ();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::wrap (napi_env environment, void* object, napi_finalize finalizeCallback)
{
	napi_status status;
	napi_value result;

	status = napi_create_object (environment, &result);
	assert (status == napi_ok);

	status = napi_wrap (environment, result, object, finalizeCallback, nullptr, nullptr);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::getNull (napi_env environment)
{
	napi_status status;
	napi_value result;

	status = napi_get_null(environment, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::getUndefined (napi_env environment)
{
	napi_status status;
	napi_value result;

	status = napi_get_undefined (environment, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromString (napi_env environment, Core::CStringPtr string)
{
	napi_status status;
	napi_value result;

	status = napi_create_string_utf8 (environment, string, NAPI_AUTO_LENGTH, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromString (napi_env environment, StringRef string)
{
	napi_status status;
	napi_value result;

	MutableCString utf8string (string, Text::kUTF8);
	status = napi_create_string_utf8 (environment, utf8string.str(), utf8string.length(), &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromInt32 (napi_env environment, Core::int32 value)
{
	napi_status status;
	napi_value result;

	status = napi_create_int32 (environment, value, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromInt64 (napi_env environment, Core::int64 value)
{
	napi_status status;
	napi_value result;

	status = napi_create_int64 (environment, value, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromFloat64 (napi_env environment, Core::float64 value)
{
	napi_status status;
	napi_value result;

	status = napi_create_double (environment, value, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromFunction (napi_env environment, napi_callback callback)
{
	napi_status status;
	napi_value result;

	status = napi_create_function (environment, nullptr, 0, callback, nullptr, &result);
	assert (status == napi_ok);

	return NAPIValue (environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromVariant (napi_env environment, const Variant& variant)
{
	switch(variant.getType ())
	{
	case Variant::kInt:
		return NAPIValue::fromInt64 (environment, variant);

	case Variant::kFloat:
		return NAPIValue::fromFloat64 (environment, variant);

	case Variant::kString:
		return NAPIValue::fromString (environment, variant);

	case Variant::kObject:
	{
		UnknownPtr<IObject> nativeObject (variant);
		return NAPIValue::fromObject (environment, nativeObject);
	}

	default:
		return NAPIValue::getUndefined (environment);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIValue::fromObject (napi_env environment, IObject* object)
{
	if(object == nullptr)
		return getNull (environment);

	Vector<napi_property_descriptor> properties;
	addObjectProperties (properties, object->getTypeInfo ());

	napi_status status;
	napi_value result;

	status = napi_create_object (environment, &result);
	assert (status == napi_ok);

	status = napi_wrap (environment, result, object, cclObjectFinalize, nullptr, nullptr);
	assert (status == napi_ok);
	
	if(!properties.isEmpty ())
	{
		status = napi_define_properties (environment, result, properties.count(), properties.getItems ());
		assert (status == napi_ok);
	}

	object->retain (); // result takes a pointer to object
	return NAPIValue(environment, result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIValue::addObjectProperties (Core::Vector<napi_property_descriptor>& properties, const CCL::ITypeInfo& typeInfo)
{
	// Recursively add properties of base classes
	const ITypeInfo* parentTypeInfo = typeInfo.getParentType ();
	if(parentTypeInfo != nullptr)
		addObjectProperties (properties, *parentTypeInfo);

	const ITypeInfo::MethodDefinition* methodDef = typeInfo.getMethodNames ();
	if(methodDef != nullptr)
	{
		size_t methodIndex = 0;

		while(methodDef->name != nullptr)
		{
			napi_property_descriptor descriptor;

			descriptor.utf8name   = methodDef->name;
			descriptor.name       = nullptr;
			descriptor.method     = cclObjectInvokeMethod;
			descriptor.getter     = nullptr;
			descriptor.setter     = nullptr;
			descriptor.value      = nullptr;
			descriptor.attributes = napi_default;
			descriptor.data       = const_cast<char*>(methodDef->name);

			properties.add (descriptor);
			methodDef++;
		}
	}

	const ITypeInfo::PropertyDefinition* propertyDef = typeInfo.getPropertyNames ();
	if(propertyDef != nullptr)
	{
		size_t propertyIndex = 0;
		while(propertyDef->name != nullptr)
		{
			napi_property_descriptor descriptor;

			descriptor.utf8name   = propertyDef->name;
			descriptor.name       = nullptr;
			descriptor.method     = nullptr;
			descriptor.getter     = cclObjectGetProperty;
			descriptor.setter     = cclObjectSetProperty;
			descriptor.value      = nullptr;
			descriptor.attributes = napi_default;
			descriptor.data       = const_cast<char*>(propertyDef->name);

			properties.add (descriptor);
			propertyDef++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_value NAPIValue::cclObjectInvokeMethod (napi_env environment, napi_callback_info info)
{
	napi_status status;
	size_t      argc    = 0;
	napi_value  argv[8] = { nullptr };
	napi_value  thisArg = nullptr;
	void*       data    = nullptr;
	IObject*    object  = nullptr;

	argc = ARRAY_COUNT (argv);
	CHECK_NAPI_STATUS (napi_get_cb_info (environment, info, &argc, argv, &thisArg, &data));
	CHECK_NAPI_STATUS (napi_unwrap (environment, thisArg, reinterpret_cast<void**>(&object)));

	{
		Message message;
		message.setID (reinterpret_cast<const char*>(data));
		message.setArgCount (static_cast<int>(argc));

		for(size_t i = 0; i < argc; i++)
			message.setArg (static_cast<int>(i), NAPIValue (environment, argv[i]).toVariant ());

		Variant value;
		object->invokeMethod (value, message);

		return NAPIValue::fromVariant (environment, value);
	}

error:
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_value NAPIValue::cclObjectGetProperty (napi_env environment, napi_callback_info info)
{
	napi_status status;
	size_t      argc    = 0;
	napi_value  argv    = nullptr;
	napi_value  thisArg = nullptr;
	void*       data    = nullptr;
	IObject*    object  = nullptr;

	CHECK_NAPI_STATUS (napi_get_cb_info (environment, info, &argc, &argv, &thisArg, &data));
	CHECK_NAPI_STATUS (napi_unwrap (environment, thisArg, reinterpret_cast<void**>(&object)));

	{
		Variant value;
		if(object->getProperty (value, CString (reinterpret_cast<const char*>(data))))
			return NAPIValue::fromVariant (environment, value);
		else
			return NAPIValue::getUndefined (environment);
	}

error:
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_value NAPIValue::cclObjectSetProperty (napi_env environment, napi_callback_info info)
{
	napi_status status;
	size_t      argc    = 1;
	napi_value  argv    = nullptr;
	napi_value  thisArg = nullptr;
	void*       data    = nullptr;
	IObject*    object  = nullptr;

	CHECK_NAPI_STATUS (napi_get_cb_info (environment, info, &argc, &argv, &thisArg, &data));
	CHECK_NAPI_STATUS (napi_unwrap (environment, thisArg, reinterpret_cast<void**>(&object)));
	assert (argc == 1);

	{
		Variant value = NAPIValue (environment, argv).toVariant ();
		if(!object->setProperty (reinterpret_cast<const char*>(data), value))
			napi_throw_error (environment, nullptr, "cannot set property value");
	}

error:
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIValue::cclObjectFinalize (napi_env env, void* finalize_data, void* finalize_hint)
{
	IObject* object = reinterpret_cast<IObject*>(finalize_data);
	object->release ();
}

//*************************************************************************************************
// NAPIReference
//*************************************************************************************************

NAPIReference::NAPIReference ()
: environment (nullptr),
  reference (nullptr)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference::NAPIReference (napi_env environment, napi_ref reference) :
	environment (environment),
	reference (reference)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference::NAPIReference (const NAPIReference& other) :
	environment (other.environment),
	reference (other.reference)
{
	if(reference != nullptr)
	{
		napi_status status = napi_reference_ref (environment, reference, nullptr);
		assert (status == napi_ok);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference::NAPIReference (NAPIReference&& other) :
	environment (other.environment),
	reference (other.reference)
{
	other.environment = nullptr;
	other.reference = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference::~NAPIReference ()
{
	release ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_env NAPIReference::getEnvironment () const
{
	return environment;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPIReference::getValue () const
{
	NAPIValue value (environment, nullptr);

	if(reference != nullptr)
	{
		napi_status status = napi_get_reference_value (environment, reference, &value);
		assert (status == napi_ok);
	}

	return value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIReference::release ()
{
	if(reference != nullptr)
	{
		napi_status status = napi_delete_reference (environment, reference);
		assert (status == napi_ok);

		reference = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference& NAPIReference::operator = (const NAPIReference& rhs)
{
	if(this != std::addressof(rhs))
	{
		napi_status status;

		if(reference != nullptr)
		{
			status = napi_reference_unref (environment, reference, nullptr);
			assert (status == napi_ok);

			environment = nullptr;
			reference = nullptr;
		}

		status = napi_reference_ref (rhs.environment, rhs.reference, nullptr);
		assert (status == napi_ok);

		environment = rhs.environment;
		reference = rhs.reference;
	}

	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference& NAPIReference::operator = (NAPIReference&& rhs)
{
	if(this != std::addressof(rhs))
	{
		napi_status status;

		if(reference != nullptr)
		{
			status = napi_reference_unref (environment, reference, nullptr);
			assert (status == napi_ok);

			environment = nullptr;
			reference = nullptr;
		}

		environment = rhs.environment;
		reference = rhs.reference;
		rhs.environment = nullptr;
		rhs.reference = nullptr;
	}

	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool NAPIReference::operator == (std::nullptr_t) const
{
	return reference == nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool NAPIReference::operator != (std::nullptr_t) const
{
	return reference != nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference::operator napi_ref () const
{
	return reference;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

napi_ref* NAPIReference::operator & ()
{
	return &reference;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIReference NAPIReference::fromValue (const NAPIValue& value)
{
	napi_status status;
	napi_ref result;
	
	status = napi_create_reference (value.getEnvironment (), value, 1, &result);
	assert (status == napi_ok);

	return NAPIReference (value.getEnvironment (), result);
}

//*************************************************************************************************
// NAPICallbackInfo
//*************************************************************************************************

NAPICallbackInfo::NAPICallbackInfo (napi_env environment, napi_callback_info info)
: environment(environment),
  argCount (ARRAY_COUNT (argVector.array)),
  argVector { 0 },
  argThis (nullptr)
{
	napi_status status;
	
	status = napi_get_cb_info (environment, info, &argCount, argVector.array, &argThis, nullptr);
	assert (status == napi_ok);

	if(argCount > ARRAY_COUNT (argVector.array))
	{
		argVector.pointer = new napi_value[argCount];

		status = napi_get_cb_info (environment, info, &argCount, argVector.pointer, &argThis, nullptr);
		assert (status == napi_ok);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPICallbackInfo::getThisArg () const
{
	return NAPIValue (environment, argThis);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

size_t NAPICallbackInfo::getArgCount () const
{
	return argCount;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIValue NAPICallbackInfo::getArgAt (size_t index) const
{
	assert (index < argCount);

	if(argCount < ARRAY_COUNT (argVector.array))
		return NAPIValue(environment, argVector.array[index]);
	else
		return NAPIValue(environment, argVector.pointer[index]);
}

//*************************************************************************************************
// NAPICallbackInfo
//*************************************************************************************************

NAPIHandleScope::NAPIHandleScope (napi_env environment) 
: environment (environment),
  scope(nullptr)
{
	napi_status status = napi_open_handle_scope (environment, &scope);
	assert (status == napi_ok);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIHandleScope::~NAPIHandleScope()
{
	if(scope != nullptr)
	{
		napi_status status = napi_close_handle_scope (environment, scope);
		assert (status == napi_ok);
	}
}

//*************************************************************************************************
// NAPIFunction
//*************************************************************************************************

NAPIFunction::NAPIFunction (napi_env environment, napi_value value)
: environment (environment),
  reference (nullptr)
{
	napi_status status = napi_create_reference (environment, value, 1, &reference);
	assert (status == napi_ok);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIFunction::~NAPIFunction ()
{
	napi_status status = napi_delete_reference (environment, reference);
	assert (status == napi_ok);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NAPIFunction::call (Variant& returnValue, IObject* This /* = 0 */, const Variant args[] /* = 0 */, int argCount /* = 0 */)
{
	NAPIHandleScope scope(environment);

	NAPIValue recv = This ? NAPIValue::fromObject (environment, This) : NAPIValue::getUndefined (environment);

	Vector<napi_value> argv;
	for(int i = 0; i < argCount; i++)
		argv.add (NAPIValue::fromVariant (environment, args[i]));

	napi_value callback;
	napi_status status = napi_get_reference_value (environment, reference, &callback);
	if(status != napi_ok)
		return false;

	napi_value result;
	status = napi_call_function (environment, recv, callback, argCount, argv.getItems (), &result);

	if(status != napi_ok)
		return false;

	returnValue = NAPIValue (environment, result).toVariant ();
	return true;
}

//*************************************************************************************************
// NAPICallbackInfo
//*************************************************************************************************

NAPIThreadsafeFunction::NAPIThreadsafeFunction ()
: environment (nullptr),
  callback (nullptr),
  function (nullptr),
  callData (nullptr)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NAPIThreadsafeFunction::~NAPIThreadsafeFunction ()
{
	delete[] callData;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIThreadsafeFunction::create (NAPIValue functionValue, CStringPtr functionName)
{
	assert (function == nullptr);
	assert (functionValue.getType () == napi_function);

	environment = functionValue.getEnvironment ();
	NAPIValue resourceName = NAPIValue::fromString (environment, functionName);

	napi_status status = napi_create_threadsafe_function (
		environment, functionValue, nullptr, resourceName, 0, 1, nullptr, nullptr, this, invokeCallbackFromJS, &function);

	assert (status == napi_ok);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIThreadsafeFunction::call (IMessage &message)
{
	napi_status status = napi_call_threadsafe_function (function, &message, napi_tsfn_blocking);
	assert (status == napi_ok);

	message.retain ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

tresult NAPIThreadsafeFunction::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IUnknown> ())
	{
		*ptr = static_cast<Unknown*>(this);
		return kResultOk;
	}
	else if(iid == ccl_iid<IFunction> ())
	{
		*ptr = static_cast<IFunction*>(this);
		return kResultOk;
	}
	else
	{
		*ptr = nullptr;
		return kResultClassNotFound;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int NAPIThreadsafeFunction::retain ()
{
	napi_status status = napi_acquire_threadsafe_function (function);
	assert (status == napi_ok);

	return Unknown::retain ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int NAPIThreadsafeFunction::release ()
{
	napi_status status = napi_release_threadsafe_function (function, napi_tsfn_release);
	assert (status == napi_ok);

	return Unknown::release ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NAPIThreadsafeFunction::call (Variant& returnValue, IObject* This /* = 0 */, const Variant args[] /* = 0 */, int argCount /* = 0 */)
{
	NAPIBlockingCall call (This, args, argCount);

	napi_status status = napi_call_threadsafe_function (function, &call, napi_tsfn_blocking);
	if(status != napi_ok)
		return false;

	call.wait ();
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIThreadsafeFunction::invokeCallbackFromJS (napi_env environment, napi_value callback, void* context, void* data)
{
	NAPIBlockingCall* call = reinterpret_cast<NAPIBlockingCall*>(data);

	if(environment != nullptr && callback != nullptr)
	{
		napi_value recv;
		napi_status status;

		status = napi_get_undefined (environment, &recv);
		if(status != napi_ok)
		{
			napi_throw_error (environment, "ERR_NAPI_TSFN_GET_UNDEFINED", "Failed to retrieve undefined value");
			return;
		}

		call->invoke (NAPIValue (environment, callback));
	}
}

//*************************************************************************************************
// NAPIBlockingCall
//*************************************************************************************************

NAPIBlockingCall::NAPIBlockingCall (IObject* thisArg, const Variant* args, int argCount)
: thisArg (thisArg),
  args (args),
  argCount (argCount)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIBlockingCall::invoke (NAPIValue function)
{
	napi_env environment = function.getEnvironment ();
	NAPIValue recv = NAPIValue::fromObject (environment, thisArg);

	Vector<napi_value> argv;
	for(int i = 0; i < argCount; i++)
		argv.add (NAPIValue::fromVariant (environment, args[i]));

	napi_value resultValue;
	napi_status status = napi_call_function (environment, recv, function, argCount, argv.getItems (), &resultValue);

	if(status == napi_ok)
	{
		result = NAPIValue (environment, resultValue).toVariant ();
		signal.signal ();
	}
	if(status != napi_ok && status != napi_pending_exception)
	{
		napi_throw_error (environment, "ERR_NAPI_TSFN_CALL_JS", "Failed to call JS callback");
		signal.signal ();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NAPIBlockingCall::wait ()
{
	signal.wait (Core::Threads::kWaitForever);
}

//*************************************************************************************************
// N-API error handling
//*************************************************************************************************

void CCL::NodeJS::handleNAPIError (napi_env env, napi_status status)
{
	const napi_extended_error_info* errorInfo = nullptr;
	napi_get_last_error_info (env, &errorInfo);

	bool isPending;
	napi_is_exception_pending (env, &isPending);

	if(!isPending)
	{
		const char* message = errorInfo->error_message ? errorInfo->error_message : "empty message";
		napi_throw_error (env, nullptr, message);
	}
}
