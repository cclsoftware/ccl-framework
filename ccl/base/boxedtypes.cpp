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
// Filename    : ccl/base/boxedtypes.cpp
// Description : Basic "boxed" types
//
//************************************************************************************************

#include "ccl/base/boxedtypes.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/system/formatter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Method definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_PERSISTENT (Boxed::UID, Object, "UID")
DEFINE_CLASS_NAMESPACE (Boxed::UID, NAMESPACE_CCL)

DEFINE_CLASS_PERSISTENT (Boxed::Formatter, Object, "Formatter")
DEFINE_CLASS_NAMESPACE (Boxed::Formatter, NAMESPACE_CCL)

namespace CCL
{ 
	namespace Boxed
	{
		BEGIN_METHOD_NAMES (UID)
			DEFINE_METHOD_ARGR ("equals", "id: UID | string", "bool")
		END_METHOD_NAMES (UID)

		BEGIN_METHOD_NAMES (Formatter)
			DEFINE_METHOD_ARGR ("print", "value: variant", "string")
			DEFINE_METHOD_ARGR ("scan", "value: string", "variant")
		END_METHOD_NAMES (Formatter)
	}
}

//************************************************************************************************
// IBoxedUID
//************************************************************************************************

DEFINE_IID_ (IBoxedUID, 0xce2d68ea, 0x63f6, 0x4587, 0xa6, 0x80, 0x5f, 0x9e, 0xba, 0x2e, 0xf0, 0xb4)

//************************************************************************************************
// Boxed::UID
//************************************************************************************************

UIDBytes Boxed::UID::fromVariant (CCL::VariantRef var)
{
	CCL::UID result;
	if(var.isString ())
		result.fromString (var.asString ());
	else if(UnknownPtr<IBoxedUID> uid = var.asUnknown ())
		uid->copyTo (result);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::UID::UID (CCL::UIDRef uid)
: CCL::UID (uid)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::UID::assign (UIDRef uid)
{
	UIDBytes::assign (uid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::UID::copyTo (UIDBytes& uid) const
{
	uid.assign (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Boxed::UID::asString () const
{
	String string;
	CCL::UID::toString (string);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Boxed::UID::asCString () const
{
	MutableCString string;
	CCL::UID::toCString (string);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Boxed::UID::getHashCode (int size) const
{
	return (int)(hash () & 0x7FFFFFFF) % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::UID::equals (const Object& obj) const
{
	const Boxed::UID* uid = ccl_cast<Boxed::UID> (&obj);
	if(uid)
		return *this == *uid;
	return Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::UID::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	CCL::String string;
	a.get (string, "uid");
	bool result = fromString (string);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::UID::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	a.set ("uid", asString ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::UID::toString (CCL::String& string, int flags) const
{
	CCL::UID::toString (string);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::UID::invokeMethod (CCL::Variant& returnValue, MessageRef msg)
{
	if(msg == "equals")
	{
		const CCL::Variant& arg = msg.getArg (0);
		if(arg.isObject ())	
		{
			Object* obj = unknown_cast<Object> (arg);
			returnValue = (obj != nullptr && equals (*obj));
			return true;
		}
		else if(arg.isString ())
		{
			Boxed::UID uid;
			returnValue = (uid.fromString (arg.asString ()) && equals (uid));
			return true;
		}
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Boxed::Variant
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Boxed::Variant, Object, "Variant")
DEFINE_CLASS_NAMESPACE (Boxed::Variant, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::Variant::Variant (CCL::VariantRef v)
: v (v)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::Variant::assign (CCL::VariantRef variant)
{
	v = variant;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::Variant::copyTo (CCL::Variant& variant) const
{
	variant = v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Variant::equals (const Object& obj) const
{
	const Boxed::Variant* vObj = ccl_cast<Boxed::Variant> (&obj);
	if(vObj)
	{
		if(v.isObject () && vObj->v.isObject ())
		{
			Object* obj1 = unknown_cast<Object> (v);
			Object* obj2 = unknown_cast<Object> (vObj->v);
			if(obj1 && obj2)
				return obj1->equals (*obj2);
		}
		
		return v == vObj->v;
	}
	return Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Boxed::Variant::compare (const Object& obj) const
{
	const Boxed::Variant* vObj = ccl_cast<Boxed::Variant> (&obj);
	if(vObj)
	{
		if(v == vObj->v)
			return 0;
		else if(v > vObj->v)
			return 1;
		else
			return -1;
	}
	return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::Variant::toString (CCL::String& string, int flags) const
{
	if(v.isObject ())
	{
		Object* obj = unknown_cast<Object> (v);
		if(obj)
			return obj->toString (string, flags);
	}
	return v.toString (string);
}

//************************************************************************************************
// Boxed::VariantWithName
//************************************************************************************************

Boxed::VariantWithName::VariantWithName (VariantRef value, CCL::StringRef name)
: Boxed::Variant (value), name (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::VariantWithName::toString (CCL::String& string, int flags) const
{
	string = getName ();
	return true;
}

//************************************************************************************************
// Boxed::String
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Boxed::String, Object, "String")
DEFINE_CLASS_NAMESPACE (Boxed::String, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::String::String (CCL::StringRef string)
: CCL::String (string)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::String::equals (const Object& obj) const
{
	if(const Boxed::String* strObj = ccl_cast<Boxed::String> (&obj))
		return *this == *strObj;

	if(const Boxed::Variant* varObj = ccl_cast<Boxed::Variant> (&obj))
		return *this == varObj->asVariant ().asString ();	

	return Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Boxed::String::compare (const Object& obj) const
{
	if(const Boxed::String* strObj = ccl_cast<Boxed::String> (&obj))
		return CCL::String::compare (*strObj);

	if(const Boxed::Variant* varObj = ccl_cast<Boxed::Variant> (&obj))
		return CCL::String::compare (varObj->asVariant ().asString ());	

	return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::String::load (const Storage& storage)
{
	CCL::String& s = *this;
	s = storage.getAttributes ().getString ("text");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::String::save (const Storage& storage) const
{
	CCL::StringRef s = *this;
	return storage.getAttributes ().set ("text", s);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::String::toString (CCL::String& string, int flags) const
{
	string = *this;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Boxed::String::getHashCode (int size) const
{
	unsigned int hashCode = CCL::String::getHashCode ();
	return (hashCode & 0x7FFFFFFF) % size;
}

//************************************************************************************************
// Boxed::DateTime
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Boxed::DateTime, Object, "DateTime")
DEFINE_CLASS_NAMESPACE (Boxed::DateTime, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::DateTime::DateTime (const CCL::DateTime& dt)
: CCL::DateTime (dt)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::DateTime::equals (const Object& obj) const
{
	const Boxed::DateTime* dt = ccl_cast<Boxed::DateTime> (&obj);
	return dt ? *this == *dt : Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Boxed::DateTime::compare (const Object& obj) const
{
	const Boxed::DateTime* dt = ccl_cast<Boxed::DateTime> (&obj);
	if(dt)
	{
		if(*this == *dt)
			return 0;
		else if(*this > *dt)
			return 1;
		else
			return -1;
	}
	return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::DateTime::assign (const CCL::DateTime& dateTime)
{
	*this = dateTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::DateTime::copyTo (CCL::DateTime& dateTime) const
{
	dateTime = *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::DateTime::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	CCL::DateTime temp;
	CCL::String string = a.getString ("time");
	Format::PortableDateTime::scan (temp, string);

	date = temp.getDate ();
	time = temp.getTime ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::DateTime::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	CCL::String string = Format::PortableDateTime::print (*this);

	a.set ("time", string);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::DateTime::getProperty (CCL::Variant& var, MemberID propertyId) const
{
	#define RETURN_PROPERTY(id, member) \
	if(propertyId == id) { var = member; return true; }

	RETURN_PROPERTY ("year", date.getYear ())
	RETURN_PROPERTY ("month", date.getMonth ())
	RETURN_PROPERTY ("day", date.getDay ())
	RETURN_PROPERTY ("hour", time.getHour ())
	RETURN_PROPERTY ("minute", time.getMinute ())
	RETURN_PROPERTY ("second", time.getSecond ())
	RETURN_PROPERTY ("ms", time.getMilliseconds ())

	#undef RETURN_PROPERTY
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::DateTime::setProperty (MemberID propertyId, const CCL::Variant& var)
{
	#define ASSIGN_PROPERTY(id, expr) \
	if(propertyId == id) { expr; return true; }

	ASSIGN_PROPERTY ("year", date.setYear (var))
	ASSIGN_PROPERTY ("month", date.setMonth (var))
	ASSIGN_PROPERTY ("day", date.setDay (var))
	ASSIGN_PROPERTY ("hour", time.setHour (var))
	ASSIGN_PROPERTY ("minute", time.setMinute (var))
	ASSIGN_PROPERTY ("second", time.setSecond (var))
	ASSIGN_PROPERTY ("ms", time.setMilliseconds (var))

	#undef ASSIGN_PROPERTY
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Boxed::DateTime)
	DEFINE_METHOD_NAME ("toOrdinal")
END_METHOD_NAMES (Boxed::DateTime)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::DateTime::invokeMethod (CCL::Variant& returnValue, MessageRef msg)
{
	if(msg == "toOrdinal")
	{
		returnValue = toOrdinal ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Boxed::Formatter
//************************************************************************************************

Boxed::Formatter::Formatter (IFormatter* formatter)
: formatter (formatter)
{
	ASSERT (formatter != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Boxed::Formatter::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IFormatter)

	if(SuperClass::queryInterface (iid, ptr) == kResultOk)
		return kResultOk;

	if(formatter && formatter->queryInterface (iid, ptr) == kResultOk && *ptr != nullptr)
		return kResultOk;

	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Boxed::Formatter::getFlags () const
{
	return formatter->getFlags ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Formatter::printString (CCL::String& string, CCL::VariantRef value) const
{
	return formatter->printString (string, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Formatter::scanString (CCL::Variant& value, CCL::StringRef string) const
{
	return formatter->scanString (value, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API Boxed::Formatter::getFactoryName () const
{
	return formatter->getFactoryName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::Formatter::invokeMethod (CCL::Variant& returnValue, MessageRef msg)
{
	if(msg == "print")
	{
		CCL::String string;
		printString (string, msg[0]);
		returnValue = string;
		returnValue.share ();
		return true;
	}
	else if(msg == "scan")
	{
		CCL::Variant v;
		scanString (v, msg[0].asString ());
		returnValue = v;
		returnValue.share ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
