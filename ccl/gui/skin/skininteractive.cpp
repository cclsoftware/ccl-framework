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
// Filename    : ccl/gui/skin/skininteractive.cpp
// Description : Interactive Skin Elements
//
//************************************************************************************************

#include "ccl/gui/skin/skininteractive.h"
#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skinwizard.h"

#include "ccl/gui/views/triggerview.h"
#include "ccl/gui/controls/linkview.h"
#include "ccl/gui/system/animation.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/skinxmldefs.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

namespace CCL {
namespace SkinElements {

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSkinInteractive () {} // force linkage of this file

//************************************************************************************************
// ParameterSetter
/** Sets the value of a parameter. */
//************************************************************************************************

class ParameterSetter: public TriggerAction
{
public:
	DECLARE_CLASS_ABSTRACT (ParameterSetter, TriggerAction)

	ParameterSetter ();

	PROPERTY_MUTABLE_CSTRING (paramPath, ParamPath)
	PROPERTY_OBJECT (Variant, value, Value)

	// TriggerAction
	void CCL_API execute (IObject* target) override;

private:
	static IUnknown* lookupController (IController* anchor, CStringRef path);
	IParameter* getParameter (IObject* target) const;
};

//************************************************************************************************
// AnchorElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (AnchorElement, Element, TAG_ANCHOR, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_URL, TYPE_STRING) ///< The target url of a LinkView 
END_SKIN_ELEMENT_WITH_MEMBERS (AnchorElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (AnchorElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
END_SKIN_ELEMENT_ATTRIBUTES (AnchorElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorElement::setAttributes (const SkinAttributes& a)
{
	url = a.getString (ATTR_URL);
	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_URL, url);
	return Element::getAttributes (a);
}

//************************************************************************************************
// LinkElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (LinkElement, ControlElement, TAG_LINK, DOC_GROUP_GENERAL, LinkView)
DEFINE_SKIN_ENUMERATION (TAG_LINK, ATTR_OPTIONS, LinkView::customStyles)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (style, ATTR_OPTIONS, LinkView::customStyles);
	return ControlElement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinkElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, style, LinkView::customStyles);
	return ControlElement::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* LinkElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		AnchorElement* anchor = (AnchorElement*)getParent (ccl_typeid<AnchorElement> ());
		if(anchor && !anchor->getUrl ().isEmpty ())
		{
			String urlString (anchor->getUrl ());
			if(urlString.contains (SkinVariable::prefix))
				urlString = args.wizard.resolveTitle (urlString);

			AutoPtr<Url> url = NEW Url (urlString, Url::kDetect);
			view = NEW LinkView (size, url, title);
		}
		else
		{
			IParameter* p = getParameter (args);
			view = NEW LinkView (size, p, title);
		}
		view->setStyle (style);
	}

	return ViewElement::createView (args, view);
}

//************************************************************************************************
// TriggerViewElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (TriggerViewElement, ViewElement, TAG_TRIGGERVIEW, DOC_GROUP_ANIMATION, TriggerView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_GESTURE_PRIO, TYPE_ENUM)	///< priority in touch gesture handling
END_SKIN_ELEMENT_WITH_MEMBERS (TriggerViewElement)
DEFINE_SKIN_ENUMERATION (TAG_TRIGGERVIEW, ATTR_OPTIONS, TriggerView::customStyles)
DEFINE_SKIN_ENUMERATION (TAG_TRIGGERVIEW, ATTR_GESTURE_PRIO, TriggerView::gesturePriorities)

//////////////////////////////////////////////////////////////////////////////////////////////////

TriggerViewElement::TriggerViewElement ()
: gesturePriority (GestureEvent::kPriorityNormal)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerViewElement::setAttributes (const SkinAttributes& a)
{
	gesturePriority = a.getOptions (ATTR_GESTURE_PRIO, TriggerView::gesturePriorities, true, GestureEvent::kPriorityNormal);
	a.getOptions (options, ATTR_OPTIONS, TriggerView::customStyles);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerViewElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_GESTURE_PRIO, gesturePriority, TriggerView::gesturePriorities, true);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerViewElement::appendOptions (String& optionsString) const
{
	SkinAttributes::makeOptionsString (optionsString, options.custom, TriggerView::customStyles);
	return SuperClass::appendOptions (optionsString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TriggerViewElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		view = NEW TriggerView (args.controller, size, options, title);
		((TriggerView*)view)->setGesturePriority (gesturePriority);
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// TriggerElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (TriggerElement, Element, TAG_TRIGGER, DOC_GROUP_ANIMATION, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROPERTY, TYPE_STRING)	///< name of a property, the trigger fires when the property changes to the given "value"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_STRING)		///< valued of the property that fires the trigger
	ADD_SKIN_ELEMENT_MEMBER (ATTR_EVENT, TYPE_STRING)		///< name of messsage signaled by a view, that fires the trigger
END_SKIN_ELEMENT_WITH_MEMBERS (TriggerElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (TriggerElement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_TRIGGERCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (TriggerElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerElement::setAttributes (const SkinAttributes& a)
{
	if(a.exists (ATTR_PROPERTY))
	{
		PropertyTrigger* pt = NEW PropertyTrigger;

		MutableCString propertyId (a.getString (ATTR_PROPERTY));
		pt->setPropertyId (propertyId);
		
		Variant value;
		String string (a.getString (ATTR_VALUE));
		if(string.startsWith ("@"))
		{
			MutableCString propertyPath (string.subString (1));
			value = Property (propertyPath).get ();
			value.share ();
		}
		else
			value.fromString (string);

		pt->setValue (value);
		prototype = pt;
	}
	else if(a.exists (ATTR_EVENT))
	{
		EventTrigger* et = NEW EventTrigger;

		ForEachCStringToken (MutableCString (a.getString (ATTR_EVENT)), " ", eventId)
			et->addEventID (eventId);
		EndFor

		prototype = et;
	}
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriggerElement::getAttributes (SkinAttributes& a) const
{
	if(a.isVerbose ())
	{
		a.setString (ATTR_PROPERTY, String::kEmpty);
		a.setString (ATTR_VALUE, String::kEmpty);
		a.setString (ATTR_EVENT, String::kEmpty);
	}
	
	PropertyTrigger* pt = ccl_cast<PropertyTrigger> (prototype);
	if(pt)
	{
		a.setString (ATTR_PROPERTY, String (pt->getPropertyId ()));
		
		// TODO: unresolved value???
		String string;
		pt->getValue ().toString (string);
		a.setString (ATTR_VALUE, string);
	}
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TriggerElement::applyTrigger (IObject* target)
{
	if(prototype)
	{
		Trigger* t = (Trigger*)prototype->clone ();

		// add actions (shared between all trigger clones!)
		ArrayForEach (*this, Element, element)
			TriggerActionElement* ae = ccl_cast<TriggerActionElement> (element);
			TriggerAction* action = ae ? ae->getAction<TriggerAction> () : nullptr;
			if(action)
			{
				action->retain ();
				t->addAction (action);
			}
		EndFor

		t->activate (target);
	}
}

//************************************************************************************************
// TriggerListElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (TriggerListElement, Element, TAG_TRIGGERLIST, DOC_GROUP_ANIMATION, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (TriggerListElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_TRIGGER)
END_SKIN_ELEMENT_ATTRIBUTES (TriggerListElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TriggerListElement::applyTrigger (IObject* target)
{
	ArrayForEach (*this, Element, element)
		TriggerElement* te = ccl_cast<TriggerElement> (element);
		if(te)
			te->applyTrigger (target);
	EndFor
}

//************************************************************************************************
// TriggerActionElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TriggerActionElement, Element)

//////////////////////////////////////////////////////////////////////////////////////////////////

TriggerActionElement::TriggerActionElement (TriggerAction* action)
: action (action)
{}

//************************************************************************************************
// ParameterSetter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ParameterSetter, TriggerAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterSetter::ParameterSetter ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ParameterSetter::lookupController (IController* anchor, CStringRef path)
{
	if(path.contains ("://")) // lookup from root
	{
		String _path (path);
		Url objectUrl (_path);
		return System::GetObjectTable ().getObjectByUrl (objectUrl);					
	}
	else
	{
		// lookup relative to current controller
		UnknownPtr<IObjectNode> iNode (anchor);
		return iNode ? iNode->lookupChild (String (path)) : nullptr;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParameterSetter::getParameter (IObject* target) const
{
	IParameter* parameter = nullptr;

	// anchor controller from target
	UnknownPtr<IView> view (target);
	UnknownPtr<IController> anchor (view ? view->getController () : target);

	UnknownPtr<IController> controller;
		
	// try to interpret the name as "controllerPath/paramName"
	int pos = paramPath.lastIndex ('/');
	if(pos >= 0)
	{
		MutableCString controllerPath (paramPath.subString (0, pos));
		MutableCString paramName (paramPath.subString (pos + 1));
		controller = lookupController (anchor, controllerPath);
		if(controller)
			parameter = controller->findParameter (paramName);
	}
	else
	{
		controller = anchor;
		if(controller)
			parameter = controller->findParameter (paramPath);
	}

	if(parameter)
		return parameter;

	if(controller == nullptr)
		CCL_WARN ("Controller not found for Parameter: '%s'", paramPath.str ())
	else
		CCL_WARN ("Parameter not found: '%s'", paramPath.str ())
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ParameterSetter::execute (IObject* target)
{
	ASSERT (target != nullptr)
	IParameter* param = getParameter (target);
	if(param)
		param->setValue (value, true);
}

//************************************************************************************************
// SetterElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (SetterElement, TriggerActionElement, TAG_SETTER, DOC_GROUP_ANIMATION, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PARAMETER, TYPE_STRING)	///< path to a parameter to be set
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROPERTY, TYPE_STRING)	///< path to a property to be set
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_STRING)		///< value to be assigned to the property or parameter
END_SKIN_ELEMENT_WITH_MEMBERS (SetterElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (SetterElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TRIGGERCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (SetterElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

SetterElement::SetterElement ()
: TriggerActionElement ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SetterElement::setAttributes (const SkinAttributes& a)
{
	String valueString (a.getString (ATTR_VALUE));
	Variant value;

	MutableCString propertyId (a.getString (ATTR_PROPERTY));
	if(!propertyId.isEmpty ())
	{
		PropertySetter* ps = NEW PropertySetter;
		action = ps;
		ps->setPropertyId (propertyId);
	
		if(valueString.startsWith ("@"))
		{
			String propertyPath (valueString.subString (1));
			value = propertyPath;
			value.share ();
			ps->setConstant (false);
			ps->setValue (value);
		}
		else
		{
			value.fromString (valueString);
			ps->setConstant (true);
			ps->setValue (value);
		}
	}
	else
	{
		ParameterSetter* paramSetter = NEW ParameterSetter;
		action = paramSetter;
		paramSetter->setParamPath (MutableCString  (a.getString (ATTR_PARAMETER)));

		value.fromString (valueString);
		paramSetter->setValue (value);
	}
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SetterElement::getAttributes (SkinAttributes& a) const
{
	if(PropertySetter* ps = ccl_cast<PropertySetter> (action))
	{
		a.setString (ATTR_PROPERTY, String (ps->getPropertyId ()));

		// TODO: unresolved value???
		String string;
		ps->getValue ().toString (string);
		a.setString (ATTR_VALUE, string);
	}
	else if(ParameterSetter* paramSetter = ccl_cast<ParameterSetter> (action))
	{
		a.setString (ATTR_PARAMETER, String (paramSetter->getParamPath ()));

		String string;
		paramSetter->getValue ().toString (string);
		a.setString (ATTR_VALUE, string);
	}
	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// InvokerElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (InvokerElement, TriggerActionElement, TAG_INVOKER, DOC_GROUP_ANIMATION, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TARGET, TYPE_STRING)	///< property path to a target object whose method will be invoked
	ADD_SKIN_ELEMENT_MEMBER (ATTR_NAME, TYPE_STRING)	///< Method name to be invoked in the target
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ARGUMENT1, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ARGUMENT2, TYPE_STRING)
END_SKIN_ELEMENT_WITH_MEMBERS (InvokerElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (InvokerElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TRIGGERCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (InvokerElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

InvokerElement::InvokerElement ()
: TriggerActionElement (NEW MethodInvoker)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InvokerElement::setAttributes (const SkinAttributes& a)
{
	MethodInvoker* invoker = getAction<MethodInvoker> ();
	
	MutableCString targetName (a.getString (ATTR_TARGET));
	invoker->setTargetPath (targetName);

	MutableCString methodName (a.getString (ATTR_NAME));
	invoker->setMethodName (methodName);

	int argCount = 0;
	if(a.exists (ATTR_ARGUMENT1))
		argCount++;
	if(a.exists (ATTR_ARGUMENT2))
		argCount++;

	if(argCount > 0)
	{
		Variant arg1, arg2;
		arg1.fromString (a.getString (ATTR_ARGUMENT1));
		arg2.fromString (a.getString (ATTR_ARGUMENT2));
		invoker->setArgumentCount (argCount);
		invoker->setArgument1 (arg1);
		invoker->setArgument2 (arg2);
	}

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InvokerElement::getAttributes (SkinAttributes& a) const
{
	MethodInvoker* invoker = getAction<MethodInvoker> ();
	a.setString (ATTR_TARGET, invoker->getTargetPath ());

	if(invoker->getArgumentCount () > 0 || a.isVerbose ())
	{
		a.setString (ATTR_ARGUMENT1, VariantString (invoker->getArgument1 ()));
		a.setString (ATTR_ARGUMENT2, VariantString (invoker->getArgument2 ()));
	}

	// Note: name is handled by superclass!
	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// StartAnimationElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (StartAnimationElement, TriggerActionElement, TAG_STARTANIMATION, DOC_GROUP_ANIMATION, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (StartAnimationElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TRIGGERCHILDREN)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_ANIMATION)
END_SKIN_ELEMENT_ATTRIBUTES (StartAnimationElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

StartAnimationElement::StartAnimationElement ()
: TriggerActionElement (NEW StartAnimationAction)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StartAnimationElement::loadFinished ()
{
	AnimationElement* ae = findElement<AnimationElement> ();
	ASSERT (ae != nullptr)
	if(ae)
	{
		getAction<StartAnimationAction> ()->setPrototype (ae->getAnimation ());
	}
}

//************************************************************************************************
// StopAnimationElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (StopAnimationElement, TriggerActionElement, TAG_STOPANIMATION, DOC_GROUP_ANIMATION, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROPERTY, TYPE_STRING)	///< path to the property whose animation should stop
END_SKIN_ELEMENT_WITH_MEMBERS (StopAnimationElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (StopAnimationElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TRIGGERCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (StopAnimationElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

StopAnimationElement::StopAnimationElement ()
: TriggerActionElement (NEW StopAnimationAction)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StopAnimationElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	MutableCString propertyId (a.getString (ATTR_PROPERTY));
	getAction<StopAnimationAction> ()->setPropertyID (propertyId);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StopAnimationElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);

	a.setString (ATTR_PROPERTY, getAction<StopAnimationAction> ()->getPropertyID ());
	return true;
}

//************************************************************************************************
// AnimationElement
//************************************************************************************************

BEGIN_STYLEDEF (AnimationElement::animationOptions)
	{"autoreverse",	IAnimation::kAutoReverse},
END_STYLEDEF

BEGIN_STYLEDEF (AnimationElement::timingTypes)
	{"linear",		kTimingLinear},
	{"toggle",		kTimingToggle},
	{"ease-in",		kTimingEaseIn},
	{"ease-out",	kTimingEaseOut},
	{"ease-in-out",	kTimingEaseInOut},
	// TODO: add support for cubic bezier with custom control points!
END_STYLEDEF

BEGIN_STYLEDEF (AnimationElement::resetModes)
	{"backwards",	IAnimation::kResetBackwards},
	{"forwards",	IAnimation::kResetForwards},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (AnimationElement, Element, TAG_ANIMATION, DOC_GROUP_ANIMATION, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROPERTY, TYPE_STRING)		///< Target property name
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FROM, TYPE_FLOAT)				///< Start value of target property
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TO, TYPE_FLOAT)				///< End value of target property
	ADD_SKIN_ELEMENT_MEMBER (ATTR_DURATION, TYPE_FLOAT)			///< Animation duration in seconds. An animation duration of one and a half second can either be expressed as "1500 ms" or "1.5".
	ADD_SKIN_ELEMENT_MEMBER (ATTR_REPEAT, TYPE_INT)			    ///< Number of repeats - or "forever" 
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM)			///< animation options like "autoreverse"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FUNCTION, TYPE_ENUM)			///< specifies the timing function of the animation: linear, ease-in-out, ...
	ADD_SKIN_ELEMENT_MEMBER (ATTR_RESET, TYPE_ENUM)				///< animations are reset by default (reset="backwards") - using reset="forwards" preserves the end value of the target property
	ADD_SKIN_ELEMENT_MEMBER (ATTR_GROUP, TYPE_STRING)			///< Animation group name (optional). Can be used to synchronize the timing of multiple animations.
END_SKIN_ELEMENT_WITH_MEMBERS (AnimationElement)
DEFINE_SKIN_ENUMERATION (TAG_ANIMATION, ATTR_OPTIONS, AnimationElement::animationOptions)
DEFINE_SKIN_ENUMERATION (TAG_ANIMATION, ATTR_FUNCTION, AnimationElement::timingTypes)
DEFINE_SKIN_ENUMERATION (TAG_ANIMATION, ATTR_RESET, AnimationElement::resetModes)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationElement::AnimationElement ()
: animation (NEW BasicAnimation)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationElement::~AnimationElement ()
{
	if(animation)
		animation->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Animation* AnimationElement::getAnimation ()
{
	return animation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnimationElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	MutableCString propertyId (a.getString (ATTR_PROPERTY));
	animation->setTargetProperty (Property (nullptr, propertyId));

	if(BasicAnimation* basicAnimation = ccl_cast<BasicAnimation> (animation))
	{
		basicAnimation->setStartValue (a.getFloat (ATTR_FROM));
		basicAnimation->setEndValue (a.getFloat (ATTR_TO, 1.f));
	}

	double duration = ImageElement::parseDuration (a.getString (ATTR_DURATION));
	animation->setDuration (duration);

	if(a.getString (ATTR_REPEAT).contains ("forever", false))
		animation->setRepeatCount (Animation::kRepeatForever);
	else
		animation->setRepeatCount (ccl_max (a.getInt (ATTR_REPEAT, Animation::kRepeatForever), 1));

	animation->setOptions (a.getOptions (ATTR_OPTIONS, animationOptions));
	animation->setTimingType (a.getOptions (ATTR_FUNCTION, timingTypes, true, kTimingLinear));
	animation->setResetMode (a.getOptions (ATTR_RESET, resetModes, true, Animation::kResetBackwards));
	
	MutableCString groupName (a.getString (ATTR_GROUP));
	if(!groupName.isEmpty ())
		animation->setClock (AnimationManager::instance ().getSharedClock (groupName));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnimationElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);

	a.setString (ATTR_PROPERTY, animation->getTargetProperty ().getID ());

	if(BasicAnimation* basicAnimation = ccl_cast<BasicAnimation> (animation))
	{
		a.setFloat (ATTR_FROM, (float)basicAnimation->getStartValue ());
		a.setFloat (ATTR_TO, (float)basicAnimation->getEndValue ());
	}

	a.setFloat (ATTR_DURATION, (float)animation->getDuration ());
	a.setOptions (ATTR_OPTIONS, animation->getOptions (), animationOptions);
	a.setOptions (ATTR_FUNCTION, animation->getTimingType (), timingTypes, true);
	a.setOptions (ATTR_RESET, animation->getResetMode (), resetModes, true);
	
	MutableCString groupName;
	if(animation->getClock ())
		groupName = animation->getClock ()->getName ();
	a.setString (ATTR_GROUP, groupName);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace SkinElements
} // CCL
