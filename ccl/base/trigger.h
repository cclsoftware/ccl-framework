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
// Filename    : ccl/base/trigger.h
// Description : Trigger
//
//************************************************************************************************

#ifndef _ccl_trigger_h
#define _ccl_trigger_h

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/itrigger.h"

#include "ccl/base/collections/objectlist.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

class TriggerAction;

//************************************************************************************************
// Trigger
/** A trigger performs actions conditionally. */
//************************************************************************************************

class Trigger: public Object
{
public:
	DECLARE_CLASS (Trigger, Object)

	Trigger ();
	Trigger (const Trigger&);
	~Trigger ();

	void addAction (TriggerAction* action);
	
	void activate (IObject* target);
	void deactivate ();

	void trigger ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	IObject* target;
	ISubject* subject;
	ObjectList actions;
};

//************************************************************************************************
// TriggerAction
/** Action to be performed for a trigger. */
//************************************************************************************************

class TriggerAction: public Object,
					 public ITriggerAction
{
public:
	DECLARE_CLASS_ABSTRACT (TriggerAction, Object)

	// ITriggerAction remains pure virtual here!
	
	CLASS_INTERFACE (ITriggerAction, Object)

	template <typename T> 
	static AutoPtr<ITriggerAction> make (const T& lambda);
};

//************************************************************************************************
// DeferredTrigger
/** Performs given action deferred. Usage: NEW DeferredTrigger (myAction). */
//************************************************************************************************

class DeferredTrigger: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (DeferredTrigger, Object)

	DeferredTrigger (ITriggerAction* action, IObject* target = nullptr);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// Property
/** Access object property via path (e.g. "object.child1.property1"). */
//************************************************************************************************

class Property
{
public:
	Property (IObject* anchor, StringID propertyPath);
	Property (StringID propertyPath);

	StringID getID () const;
	IObject* getHolder () const;
	
	Variant get () const;
	bool get (Variant& value) const;
	bool set (VariantRef value);

	operator Variant () const;
	Property& operator = (VariantRef value);

protected:
	IObject* propertyHolder;
	MutableCString propertyId;
	
	void resolve (IObject* anchor, StringID propertyPath);
};

//************************************************************************************************
// PropertyTrigger
/** Trigger condition is a property change. */
//************************************************************************************************

class PropertyTrigger: public Trigger
{
public:
	DECLARE_CLASS (PropertyTrigger, Trigger)

	PROPERTY_MUTABLE_CSTRING (propertyId, PropertyId)
	PROPERTY_OBJECT (Variant, value, Value)

	// Trigger
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// EventTrigger
/** Trigger condition is a message. */
//************************************************************************************************

class EventTrigger: public Trigger
{
public:
	DECLARE_CLASS (EventTrigger, Trigger)
	
	EventTrigger ();
	EventTrigger (const EventTrigger&);

	bool hasEventID (StringID eventId) const;
	void addEventID (StringID eventId);

	// Trigger
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	Vector<MutableCString> eventIds;
};

//************************************************************************************************
// PropertySetter
/** Apply property to target object. */
//************************************************************************************************

class PropertySetter: public TriggerAction
{
public:
	DECLARE_CLASS_ABSTRACT (PropertySetter, TriggerAction)

	PropertySetter ();

	PROPERTY_MUTABLE_CSTRING (propertyId, PropertyId)
	PROPERTY_OBJECT (Variant, value, Value)
	PROPERTY_BOOL (constant, Constant)

	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

//************************************************************************************************
// MethodInvoker
/** Call method of target object. */
//************************************************************************************************

class MethodInvoker: public TriggerAction
{
public:
	DECLARE_CLASS_ABSTRACT (MethodInvoker, TriggerAction)

	MethodInvoker ();

	PROPERTY_MUTABLE_CSTRING (targetPath, TargetPath)	///< resolved as property path starting from target object (e.g. "parent.parent")
	PROPERTY_MUTABLE_CSTRING (methodName, MethodName)

	static const int kMaxArgCount = 2;
	PROPERTY_VARIABLE (int, argumentCount, ArgumentCount)
	PROPERTY_VARIABLE (Variant, argument1, Argument1)
	PROPERTY_VARIABLE (Variant, argument2, Argument2)

	void setArgument (int index, VariantRef value);
	Variant getArgument (int index) const;

	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

//************************************************************************************************
// LambdaTriggerAction
//************************************************************************************************

template <typename T>
class LambdaTriggerAction: public TriggerAction
{
public:
	LambdaTriggerAction (const T& lambda)
	: lambda (lambda)
	{}

	// TriggerAction
	void CCL_API execute (IObject* target)
	{
		return lambda (target);
	}

protected:
	T lambda; ///< lambda instance is copied here!
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant Property::get () const
{ Variant v; get (v); return v; }

inline Property::operator Variant () const
{ return get (); }

inline Property& Property::operator = (VariantRef value)
{ set (value); return *this; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> 
AutoPtr<ITriggerAction> TriggerAction::make (const T& lambda)
{
	return AutoPtr<ITriggerAction> (NEW LambdaTriggerAction<T> (lambda));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_trigger_h
