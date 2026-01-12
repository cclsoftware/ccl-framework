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
// Filename    : ccl/base/message.h
// Description : Message class
//
//************************************************************************************************

#ifndef _ccl_message_h
#define _ccl_message_h

#include "ccl/base/object.h"

#include "ccl/public/base/imessage.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// Message
/** Message class.
	\ingroup ccl_base */
//************************************************************************************************

class Message: public Object,
			   public IMessage
{
public:
	DECLARE_CLASS (Message, Object)
	DECLARE_METHOD_NAMES (Message)
	DECLARE_PROPERTY_NAMES (Message)

	enum Limits { kMaxMessageArgs = 8 };

	Message (StringID id = nullptr);
	Message (StringID id, VariantRef arg0);
	Message (StringID id, VariantRef arg0, VariantRef arg1);
	Message (StringID id, VariantRef arg0, VariantRef arg1, VariantRef arg2);
	Message (StringID id, VariantRef arg0, VariantRef arg1, VariantRef arg2, VariantRef arg3);
	Message (StringID id, const Variant args[], int count);

	Message (MessageRef other);
	Message (const Message& other);
	~Message ();

	void post (IObserver* observer, int delay = 0);
	void postBlocking (IObserver* observer);

	void setID (StringID id);
	void setArg (int index, VariantRef arg);
	void setArgCount (int count);
	bool appendArg (VariantRef arg);
	Message& operator << (VariantRef arg);

	// IMessage
	StringID CCL_API getID () const override;
	int CCL_API getArgCount () const override;
	VariantRef CCL_API getArg (int index) const override;

	CLASS_INTERFACE (IMessage, Object)

protected:
	MutableCString id;
	int argCount;
	Variant args[kMaxMessageArgs];

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};
	
//************************************************************************************************
// MessageArgument
/** Base class for typed message argument, do not use directly.
	\ingroup ccl_base */
//************************************************************************************************

class MessageArgument: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (MessageArgument, Object)

	MessageArgument (const void* typeAddress)
	: typeAddress (typeAddress) 
	{}

	PROPERTY_VARIABLE (const void*, typeAddress, TypeAddress)
};

//************************************************************************************************
// TypedMessageArgument
/** Template for wrapping plain data types into message arguments.
	
	Usage example:
	
	MyType payload;
	Message msg ("test", TypedMessageArgument<MyType>::make (payload));

	auto arg = TypedMessageArgument<MyTyp>::cast (msg[0]);
	arg->getPayload ();

	\ingroup ccl_base */
//************************************************************************************************

template <typename T>
class TypedMessageArgument: public MessageArgument
{
public:
	static AutoPtr<IObject> make (const T& payload)
	{
		return NEW TypedMessageArgument (payload, TypedMessageArgument<T>::getTypeAddress ());
	}

	static TypedMessageArgument* cast (VariantRef arg)
	{
		if(auto messageArgument = unknown_cast<MessageArgument> (arg.asUnknown ()))
			if(messageArgument->getTypeAddress () == TypedMessageArgument<T>::getTypeAddress ())
				return static_cast<TypedMessageArgument*> (messageArgument);
		return nullptr;
	}

	PROPERTY_OBJECT (T, payload, Payload)

private:
	static const void* getTypeAddress ()
	{
		// Note: The address of a static variable is unique in the address space of a process.
		static const int staticInt = 0;
		return &staticInt;
	}

	TypedMessageArgument (const T& payload, const void* typeAddress)
	: MessageArgument (typeAddress),
	  payload (payload)
	{}
};

} // namespace CCL

#endif // _ccl_message_h
