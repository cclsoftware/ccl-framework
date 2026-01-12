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
// Filename    : ccl/base/trigger.cpp
// Description : Trigger
//
//************************************************************************************************

#include "ccl/base/trigger.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// Trigger
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Trigger, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Trigger::Trigger ()
: target (nullptr),
  subject (nullptr)
{
	actions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Trigger::Trigger (const Trigger& t)
: target (nullptr),
  subject (nullptr),
  actions (t.actions)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Trigger::~Trigger ()
{
	ASSERT (target == nullptr)
	ASSERT (subject == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Trigger::addAction (TriggerAction* action)
{
	actions.add (action);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Trigger::activate (IObject* target)
{
	this->target = target;
	subject = UnknownPtr<ISubject> (target);
	ASSERT (subject != nullptr)
	subject->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Trigger::deactivate ()
{
	ASSERT (subject != nullptr)
	subject->removeObserver (this);
	subject = nullptr;
	target = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Trigger::trigger ()
{
	ASSERT (target != nullptr)
	ForEach (actions, TriggerAction, action)
		action->execute (target);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Trigger::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
	{
		deactivate ();
		release ();
	}
}

//************************************************************************************************
// TriggerAction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TriggerAction, Object)

//************************************************************************************************
// DeferredTrigger
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DeferredTrigger, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DeferredTrigger::DeferredTrigger (ITriggerAction* action, IObject* target)
{
	(NEW Message ("trigger", action, target))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DeferredTrigger::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "trigger")
	{
		UnknownPtr<ITriggerAction> action (msg[0]);
		UnknownPtr<IObject> target (msg[1]);
		ASSERT (action)
		action->execute (target);

		release ();
	}
}

//************************************************************************************************
// Property
//************************************************************************************************

Property::Property (IObject* anchor, StringID propertyPath)
: propertyHolder (nullptr)
{
	if(anchor)
	{
		// ignore given anchor when propertyPath is absolute url
		if(propertyPath.startsWith ("://"))
		{
			int index = propertyPath.index ('.');
			if(index > -1)
			{
				Url anchorUrl (String (propertyPath.subString (0, index)));
				MutableCString remainingPropertyPath = propertyPath.subString (index + 1);

				UnknownPtr<IObject> anchor (System::GetObjectTable ().getObjectByUrl (anchorUrl));
				resolve (anchor, remainingPropertyPath);
			}
		}
		else
			resolve (anchor, propertyPath);
	}
	else
		propertyId = propertyPath; // keep it for later use
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Property::Property (StringID propertyPath)
: propertyHolder (nullptr)
{
	if(propertyPath.contains ("/"))
	{
		int index = propertyPath.index ('.');
		if(index > -1)
		{
			Url anchorUrl (String (propertyPath.subString (0, index)));
			MutableCString remainingPropertyPath = propertyPath.subString (index + 1);

			UnknownPtr<IObject> anchor (System::GetObjectTable ().getObjectByUrl (anchorUrl));
			resolve (anchor, remainingPropertyPath);
		}
	}
	else
	{
		UnknownPtr<IObject> anchor (&System::GetObjectTable ());
		resolve (anchor, propertyPath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Property::resolve (IObject* anchor, StringID propertyPath)
{
	// split propertyPath into sections (using '.' as delimiter like "node.member"),
	// but text inside brackets (like "hasParam[Parent.Child.paramName]") must be kept together

	IObject* currentPropHolder = anchor;

	int pathPartStartIdx = 0;
	int pathEndIdx = 0;
	bool inBrackets = false;
	int pathLength = propertyPath.length ();

	while(pathEndIdx < pathLength && currentPropHolder != nullptr)
	{
		char c = propertyPath[pathEndIdx];
		if(c == '[')
			inBrackets = true;
		else if(c == ']')
			inBrackets = false;
		else if(c == '.')
		{
			if(inBrackets == false)
			{
				MutableCString objectPropId (propertyPath.subString (pathPartStartIdx, pathEndIdx - pathPartStartIdx));
				pathPartStartIdx = pathEndIdx + 1;

				Variant v;
				if(currentPropHolder->getProperty (v, objectPropId))
					currentPropHolder = UnknownPtr<IObject> (v.asUnknown ());
				else
					currentPropHolder = nullptr;
			}
		}
		pathEndIdx++;
	}

	propertyHolder = currentPropHolder;
	if(propertyHolder)
	{
		if(pathPartStartIdx == 0)
			propertyId = propertyPath;
		else
			propertyId = propertyPath.subString (pathPartStartIdx, pathEndIdx - pathPartStartIdx);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID Property::getID () const
{
	return propertyId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* Property::getHolder () const
{
	return propertyHolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Property::get (Variant& value) const
{
	return propertyHolder && propertyHolder->getProperty (value, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Property::set (VariantRef value)
{
	return propertyHolder && propertyHolder->setProperty (propertyId, value);
}

//************************************************************************************************
// PropertyTrigger
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PropertyTrigger, Trigger)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertyTrigger::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged && msg[0] == String (propertyId))
	{
		Variant current;
		tbool result = target->getProperty (current, propertyId);
		ASSERT (result)
		if(result && current == value)
			trigger ();
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// EventTrigger
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EventTrigger, Trigger)

//////////////////////////////////////////////////////////////////////////////////////////////////

EventTrigger::EventTrigger ()
: eventIds (1, 2)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventTrigger::EventTrigger (const EventTrigger& other)
: eventIds (other.eventIds)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EventTrigger::hasEventID (StringID eventId) const
{
	return eventIds.contains (eventId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EventTrigger::addEventID (StringID eventId)
{
	eventIds.add (eventId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EventTrigger::notify (ISubject* subject, MessageRef msg)
{
	if(hasEventID (msg.getID ()))
		trigger ();
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// PropertySetter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PropertySetter, TriggerAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

PropertySetter::PropertySetter ()
: constant (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertySetter::execute (IObject* target)
{
	ASSERT (target != nullptr)
	Property setter (target, propertyId);
	if(isConstant ())
		setter.set (value);
	else
	{
		ASSERT (value.isString ())
		MutableCString valueId (value.asString ());
		Property resolvedValue (target, valueId);
		setter.set (resolvedValue.get ());
	}
}

//************************************************************************************************
// MethodInvoker
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MethodInvoker, TriggerAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

MethodInvoker::MethodInvoker ()
: argumentCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MethodInvoker::setArgument (int index, VariantRef value)
{
	switch(index)
	{
	case 0 : setArgument1 (value); break;
	default : setArgument2 (value); break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant MethodInvoker::getArgument (int index) const
{
	switch(index)
	{
	case 0 : return getArgument1 ();
	default : return getArgument2 ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MethodInvoker::execute (IObject* target)
{
	ASSERT (target != nullptr)
	IObject* initialTarget = target;

	if(!targetPath.isEmpty ())
	{
		if(targetPath.startsWith ("object://")) // object url
		{
			String _path (targetPath);
			Url objectUrl (_path);
			target = UnknownPtr<IObject> (System::GetObjectTable ().getObjectByUrl (objectUrl));		
		}
		else
		{
			Property targetObject (target, targetPath);
			target = UnknownPtr<IObject> (targetObject.get ().asUnknown ());
		}
		if(!target)
			return;
	}

	Variant returnValue;
	if(argumentCount > 0)
	{
		Message m (methodName);
		m.setArgCount (argumentCount);
		for(int i = 0; i < argumentCount; i++)
			m.setArg (i, getArgument (i));
		target->invokeMethod (returnValue, m);
	}
	else
	{
		// default behavior: pass initial target
		target->invokeMethod (returnValue, Message (methodName, initialTarget));
	}
}
