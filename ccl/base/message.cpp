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
// Filename    : ccl/base/message.cpp
// Description : Message class
//
//************************************************************************************************

#include "ccl/base/message.h"

#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// MessageArgument
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MessageArgument, Object)

//************************************************************************************************
// Message
//************************************************************************************************

DEFINE_CLASS (Message, Object)
DEFINE_CLASS_NAMESPACE (Message, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (StringID id)
: id (id),
  argCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (StringID id, const Variant args[], int count)
: id (id),
  argCount (ccl_min<int> (count, kMaxMessageArgs))
{
	ASSERT (argCount == count) 
	for(int i = 0; i < argCount; i++)
		this->args[i] = args[i]; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (StringID id, VariantRef arg0)
: id (id),
  argCount (1)
{
	args[0] = arg0; args[0].share (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (StringID id, VariantRef arg0, VariantRef arg1)
: id (id),
  argCount (2)
{
	args[0] = arg0; args[0].share (); 
	args[1] = arg1; args[1].share (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (StringID id, VariantRef arg0, VariantRef arg1, VariantRef arg2)
: id (id),
  argCount (3)
{
	args[0] = arg0; args[0].share (); 
	args[1] = arg1; args[1].share (); 
	args[2] = arg2; args[2].share (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (StringID id, VariantRef arg0, VariantRef arg1, VariantRef arg2, VariantRef arg3)
: id (id),
  argCount (4)
{
	args[0] = arg0; args[0].share (); 
	args[1] = arg1; args[1].share (); 
	args[2] = arg2; args[2].share (); 
	args[3] = arg3; args[3].share (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (MessageRef other)
{
	id = other.getID ();
	ASSERT (other.getArgCount () < kMaxMessageArgs)
	argCount = ccl_min<int> (other.getArgCount (), kMaxMessageArgs);
	for(int i = 0; i < argCount; i++)
		args[i] = other.getArg (i);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::Message (const Message& other)
{
	id = other.getID ();
	ASSERT (other.getArgCount () < kMaxMessageArgs)
	argCount = ccl_min<int> (other.getArgCount (), kMaxMessageArgs);
	for(int i = 0; i < argCount; i++)
		args[i] = other.getArg (i);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message::~Message ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Message::post (IObserver* observer, int delay)
{
	System::GetSignalHandler ().postMessage (observer, this, delay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Message::postBlocking (IObserver* observer)
{
	System::GetSignalHandler ().postMessageBlocking (observer, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Message::setID (StringID id)
{
	this->id = id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Message::setArg (int index, VariantRef arg)
{
	ASSERT (index >= 0 && index < kMaxMessageArgs)
	if(index >= 0 && index < kMaxMessageArgs)
	{
		args[index] = arg;
		args[index].share ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Message::setArgCount (int count)
{
	ASSERT (count >= 0 && count <= kMaxMessageArgs)
	this->argCount = ccl_bound<int> (count, 0, kMaxMessageArgs);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Message::appendArg (VariantRef arg)
{
	if(argCount >= kMaxMessageArgs)
		return false;

	setArg (argCount++, arg);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Message& Message::operator << (VariantRef arg)
{
	appendArg (arg);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Message::getID () const
{
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Message::getArgCount () const
{
	return argCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantRef CCL_API Message::getArg (int index) const
{
	static Variant emptyVariant;
	ASSERT (index >= 0 && index < argCount)
	return index >= 0 && index < argCount ? args[index] : emptyVariant;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Message)
	DEFINE_PROPERTY_TYPE ("id", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("argCount", ITypeInfo::kInt | ITypeInfo::kReadOnly)
END_PROPERTY_NAMES (Message)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Message::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "id")
	{
		String idString (getID ());
		var = idString;
		var.share ();
		return true;
	}
	if(propertyId == "argCount")
	{
		var = getArgCount ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Message::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "id")
	{
		id.empty ();
		id.append (var.asString ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Message)
	DEFINE_METHOD_ARGR ("getArg", "index: int", "variant")
	DEFINE_METHOD_ARGR ("getArgCount", "", "int")
END_METHOD_NAMES (Message)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Message::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getArg")
	{
		int index = msg[0].asInt ();
		returnValue = getArg (index);
		return true;
	}
	else if(msg == "getArgCount")
	{
		returnValue = getArgCount ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
