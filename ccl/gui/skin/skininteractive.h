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
// Filename    : ccl/gui/skin/skininteractive.h
// Description : Interactive Skin Elements
//
//************************************************************************************************

#ifndef _ccl_skininteractive_h
#define _ccl_skininteractive_h

#include "ccl/base/trigger.h"
#include "ccl/gui/skin/skincontrols.h"

namespace CCL {

class Animation;

namespace SkinElements {

//************************************************************************************************
// AnchorElement
/** An Anchor specifies the url for a child Link element. 
	\see LinkElement */
//************************************************************************************************

class AnchorElement: public Element
{
public:
	DECLARE_CLASS (AnchorElement, Element)
	DECLARE_TYPELIB_CLASS (AnchorElement)

	PROPERTY_STRING (url, Url)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// LinkElement
/** Displays a text link.
Displays a text link similar as in a web browser. It can be used with an url or a parameter.

An url for the link must be specified in a parent <Anchor> element. When a parameter ("name") is used, no Anchor is required. The Link then behaves like a button.
The title can be omitted, and child views can be used instead e.g. an ImageView.

\see AnchorElement

\code{.xml}
<!-- Example: link url specified in parent anchor element -->
<Anchor url = "https://ccl.dev">
	<Link title = "...">
</Anchor>
\endcode
 */
//************************************************************************************************

class LinkElement: public ControlElement
{
public:
	DECLARE_CLASS (LinkElement, ControlElement)
	DECLARE_TYPELIB_CLASS (LinkElement)

	PROPERTY_OBJECT (StyleFlags, style, Style)

	// ControlElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// TriggerViewElement
/** A view that sends messages on certain gui events.
These messages can be referred to in the "event" attribute of a <Trigger>. \see TriggerElement

Available events are "onAttached", "onRemoved", "onMouseDown", "onSingleClick" "onDoubleClick", "onSingleTap",
"onLongPress", "onSwipe", "onSwipeH", "onSwipeV". */
//************************************************************************************************

class TriggerViewElement: public ViewElement
{
public:
	DECLARE_CLASS (TriggerViewElement, ViewElement)
	DECLARE_TYPELIB_CLASS (TriggerViewElement)
	
	TriggerViewElement ();

	PROPERTY_VARIABLE (int, gesturePriority, GesturePriority)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& optionsString) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// TriggerElement
/** A trigger reacts to an event, signaled by a view.
The event can be any message signaled by a view (attribute "event"). \see TriggerViewElement

As a special case, a "propertyChanged" message can be handled, that only triggers when the property takes a given value. (attributes "property", "value").
When the "trigger" option of a view is set, it sends "propertyChanged" messages for its "mousestate", "value" and "visualState" (the latter two only for controls).

When a trigger receives the specified event or property change, it executes all of its actions, which are placed as child elements in the <Trigger>.
Actions are executed in order of their appearance.

Available actions are:

<Setter>: sets a property of a target object, or a parameter value \see SetterElement
<Invoker>: invokes a method of a target object \see InvokerElement
<StartAnimation>: starts an animation \see StartAnimationElement
<StopAnimation>: stops an animation \see StopAnimationElement

A Trigger can only appear in the <Triggers> list of a Visual Style. \see TriggerListElement

\code{.xml}
<Style name="MyStyle">
	<Triggers>
		<Trigger event="onDoubleClick">
			<StartAnimation>
				<Animation property="children[scrollView].vpos" from="0" to="1" duration="98"/>
			</StartAnimation>
		</Trigger>

		<Trigger property="value" value="1"> 
			<Setter property="phase" value="0"/>
			<Invoker target="window" name="popupContextMenu"/>
		</Trigger>
	</Triggers>
</Style>
\endcode
*/
//************************************************************************************************

class TriggerElement: public Element,
					  public ITriggerPrototype
{
public:
	DECLARE_CLASS (TriggerElement, Element)
	DECLARE_TYPELIB_CLASS (TriggerElement)

	PROPERTY_AUTO_POINTER (Trigger, prototype, Prototype)

	// ITriggerPrototype
	void CCL_API applyTrigger (IObject* target) override;

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

	CLASS_INTERFACE (ITriggerPrototype, Element)
};

//************************************************************************************************
// TriggerListElement
/** A List of <Trigger> elements in a <Style>. \see TriggerElement  */
//************************************************************************************************

class TriggerListElement: public Element,
						  public ITriggerPrototype
{
public:
	DECLARE_CLASS (TriggerListElement, Element)
	DECLARE_TYPELIB_CLASS (TriggerListElement)

	// ITriggerPrototype
	void CCL_API applyTrigger (IObject* target) override;

	CLASS_INTERFACE (ITriggerPrototype, Element)
};

//************************************************************************************************
// TriggerActionElement
/** Base class for trigger actions.
Not to be used directly, use derived classes <Setter> \see SetterElement, <Invoker> \see InvokerElement,
<StartAnimation> \see StartAnimationElement <StopAnimation> \see StopAnimationElement */
//************************************************************************************************

class TriggerActionElement: public Element
{
public:
	DECLARE_CLASS (TriggerActionElement, Element)

	TriggerActionElement (TriggerAction* action = nullptr);

	template <class T>
	T* getAction () const { return (T*)(TriggerAction*)action; }

protected:
	AutoPtr<TriggerAction> action;
};

//************************************************************************************************
// SetterElement
/** Trigger action that sets a property of a target object, or a parameter value.
Must be placed inside a <Trigger>. \see TriggerElement

Sets the property of the given target "property" or "parameter" to the specified "value".

The property path can be an absolute path, or a path relative to the triggering view.
The value can be a constant literal, but when it starts with "@", it is interpreted as another property 
path, and that source property value gets assigned to the target property.

When a "parameter" path is specified instead of "property", the parameter is set to the (always constant) "value".

\code{.xml}
<Setter property="parent.parent.parent.value" value="@parent.name"/> 
<Setter parameter="://WindowManager/StartPage" value="1"/>
\endcode
*/
//************************************************************************************************

class SetterElement: public TriggerActionElement
{
public:
	DECLARE_CLASS (SetterElement, TriggerActionElement)
	DECLARE_TYPELIB_CLASS (SetterElement)

	SetterElement ();

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// InvokerElement
/** Trigger action that invokes a method of a target object.
Must be placed inside a <Trigger>. \see TriggerElement

The "target" path is evaluated as a property path relative to the triggering view to find the target object.
The method "name" of the target object is then called.

An <Invoker> action cannot pass any arguments to the method.

\code{.xml}
<Invoker target="window" name="popupContextMenu"/>
<Invoker target="parent.controller" name="select"/>
\endcode
*/
//************************************************************************************************

class InvokerElement: public TriggerActionElement
{
public:
	DECLARE_CLASS (InvokerElement, TriggerActionElement)
	DECLARE_TYPELIB_CLASS (InvokerElement)

	InvokerElement ();

	PROPERTY_MUTABLE_CSTRING (target, Target)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// StartAnimationElement
/** Trigger action that starts an animation.
Must be placed inside a <Trigger>. \see TriggerElement

Has no special attributes. The <Animation> has to be a child of this element.

\see AnimationElement \see StopAnimationElement 

\code{.xml}
<Triggers>
	<Trigger event="onAttached">
		<StartAnimation>
			<Animation property="children[scrollView].vpos" from="0" to="1" duration="60"/>
		</StartAnimation>
	</Trigger>
	<Trigger event="onRemoved">
		<StopAnimation property="children[scrollView].vpos"/>
	</Trigger>
</Triggers>
\endcode
*/
//************************************************************************************************

class StartAnimationElement: public TriggerActionElement
{
public:
	DECLARE_CLASS (StartAnimationElement, TriggerActionElement)
	DECLARE_TYPELIB_CLASS (StartAnimationElement)

	StartAnimationElement ();

	// TriggerActionElement
	void loadFinished () override;
};

//************************************************************************************************
// StopAnimationElement
/** Trigger action that stops an animation.
Must be placed inside a <Trigger>. \see TriggerElement

Looks for an animation for the given "property" (path relative to the triggering view) and stops it.

\see StartAnimationElement  */
//************************************************************************************************

class StopAnimationElement: public TriggerActionElement
{
public:
	DECLARE_CLASS (StopAnimationElement, TriggerActionElement)
	DECLARE_TYPELIB_CLASS (StopAnimationElement)

	StopAnimationElement ();

	// TriggerActionElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// AnimationElement
/** Describes an animation of a property.
Can be placed inside a <StartAnimation>. \see StartAnimationElement or be used as transition between <StyleCondition> scopes. \see StyleConditionElement

The "property" path is relative to the triggering view.
An Animation specifies that the property value should transition "from" a start value "to" an end value for some "duration".

Additionally, a "repeat" mode and a timing "function" can be specified.

Multiple animations can be in the same "group" to ensure synchronized execution. */
//************************************************************************************************

class AnimationElement: public Element
{
public:
	DECLARE_CLASS (AnimationElement, Element)
	DECLARE_TYPELIB_CLASS (AnimationElement)

	AnimationElement ();
	~AnimationElement ();

	DECLARE_STYLEDEF (animationOptions)
	DECLARE_STYLEDEF (timingTypes)
	DECLARE_STYLEDEF (resetModes)

	Animation* getAnimation ();

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Animation* animation;
};

} // namespace CCL
} // namespace SkinElements

#endif // _ccl_skininteractive_h
