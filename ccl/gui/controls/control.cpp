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
// Filename    : ccl/gui/controls/control.cpp
// Description : Control class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/control.h"

#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/tooltip.h"
#include "ccl/gui/system/accessibility.h"
#include "ccl/gui/gui.h"

#include "ccl/app/params.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/controlsignals.h"

using namespace CCL;

//************************************************************************************************
// Control
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Control, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

Control::Control (const Rect& size, IParameter* _param, StyleRef style, StringRef title)
: View (size, style, title),
  param (nullptr),
  renderer (nullptr),
  wheelEnabled (true),
  contextMenuEnabled (true)
{
	#if DEBUG
	if(style.isDirectUpdate () && style.isTransparent () && !style.isComposited ())
	{
		CCL_PRINTLN (title)
		CCL_PRINTLN ("Conflict between transparent and directupdate option")
	}
	#endif

	if(_param)
		setParameter (_param);
	else
	{
		setParameter (NEW Parameter);
		param->release (); // shared by setParameter
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Control::~Control ()
{
	setParameter (nullptr);
	if(renderer)
		renderer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API Control::getParameter () const 
{ 
	return param; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Control::setParameter (IParameter* p)
{
	if(param == p)
		return;

	if(param)
	{
		ISubject::removeObserver (param, this);
		param->release ();
	}

	param = p;

	if(param)
	{
		param->retain ();
		ISubject::addObserver (param, this);
		helpId = String (param->getName ());
	}

	enable (param && param->isEnabled ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::connect (IParamObserver* owner, int tag)
{
	if(param)
		param->connect (owner, tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Control::getVisualState () const
{
	return param ? param->getVisualState () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Control::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		UnknownPtr<IParameter> p (subject);
		if(p && p == param)
			paramChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::setHelpIdentifier (StringRef id)
{
	helpId = id;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef Control::getHelpIdentifier () const
{
	if(!helpId.isEmpty () && !getStyle ().isCommonStyle (Styles::kNoHelpId))
		return helpId;
	return SuperClass::getHelpIdentifier ();
}	

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::paramChanged ()
{
	ASSERT (param != nullptr)
	enable (param && param->isEnabled () ? true : false);
	
	propertyChanged ("value");
	propertyChanged ("visualState");

	if(accessibilityProvider)
		accessibilityProvider->sendEvent (AccessibilityEvent::kValueChanged);

	updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Control::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "value")
	{
		ASSERT (param != nullptr)
		if(param)
			var = param->getValue ();
		var.share ();
		return true;
	}
	else if(propertyId == "visualState")
	{
		var = getVisualState ();
		return true;
	}
	else if(propertyId == "paramController")
	{
		ASSERT (param != nullptr)
		if(param)
			var = param->getController ();
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Control::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "value")
	{
		ASSERT (param != nullptr)
		if(param)
			param->setValue (var, true);
		return true;
	}
	else if(propertyId == "visualState")
	{
		ASSERT (param != nullptr)
		if(param)
			param->setVisualState (var);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& CCL_API Control::getVisualStyle () const
{
	if(visualStyle)
		return *visualStyle;

	// need this for calculating auto-size with correct visual style
	if(ThemeRenderer* r = const_cast<Control*> (this)->getRenderer ())
		if(VisualStyle* rvs = r->getVisualStyle ())
			return *rvs;

	return SuperClass::getVisualStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::onVisualStyleChanged ()
{
	// discard renderer when visual style changes
	safe_release (renderer);

	SuperClass::onVisualStyleChanged ();

	// trigger initial value state
	if(style.isTrigger ())
		propertyChanged ("value");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	// discard renderer when visual style changes
	if(!visualStyle || visualStyle->hasReferences (event.scheme))
		safe_release (renderer);

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Control::getRenderer ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::setRenderer (ThemeRenderer* _renderer)
{
	safe_release (renderer);
	renderer = _renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::draw (const UpdateRgn& updateRgn)
{
	ThemeRenderer* renderer = getRenderer ();
	if(renderer)
		renderer->draw (this, updateRgn);

	View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::updateClient ()
{
	if(!hasBeenDrawn ())
		return;

	WindowUpdateInfo windowInfo;
	Window* window = getWindowForUpdate (windowInfo);
	if(window == nullptr)
		return;

	if(windowInfo.collectUpdates || window->hasBeenDrawn () == false) 
	{
		invalidate ();
		return;
	}

	ThemeRenderer* renderer = getRenderer ();
	if(renderer)
		renderer->update (this, ThemeRenderer::UpdateInfo (&windowInfo));
	else
		View::updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::onFocus (const FocusEvent& event)
{
	if(event.eventType == FocusEvent::kSetFocus)
	{
		if(isFocused () == false)
		{
			isFocused (true);
			invalidate ();
		}
	}
	else
	{
		if(isFocused () == true)
		{
			isFocused (false);
			invalidate ();
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::onMouseWheel (const MouseWheelEvent& event)
{
	if(View::onMouseWheel (event))
		return true;

	if(isWheelEnabled ())
		return tryWheelParam (event);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::onContextMenu (const ContextMenuEvent& event)
{
	if(isContextMenuEnabled ())
		return contextMenuForParam (event, param);
	
	return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::contextMenuForParam (const ContextMenuEvent& event, IParameter* param)
{
	UnknownPtr<IContextMenuHandler> paramMenuHandler (param);
	if(paramMenuHandler)
		if(paramMenuHandler->appendContextMenu (event.contextMenu) == kResultOk)
			return true;
	
	AutoPtr<IUnknown> identity = param ? param->createIdentity () : nullptr;
	if(identity)
	{
		Boxed::Variant result;
		UIDString classString (myClass ().getClassID ());
		Message msg (Signals::kControlContextMenu, &event.contextMenu, static_cast<IUnknown*> (identity), static_cast<IVariant*> (&result), classString);
		SignalSource (Signals::kControls).signal (msg);

		// give controller a chance
		UnknownPtr<IObserver> controllerObs (param->getController ());
		if(controllerObs)
			controllerObs->notify (this, msg);

		return static_cast<VariantRef> (result).asBool ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// static 
bool Control::handleMouseWheel (const MouseWheelEvent& event, IParameter* param, bool inverse)
{
	if(event.isVertical ())
	{
		if(param && param->isEnabled () && param->canIncrement ())
		{
			if(param->isReverse ())
				inverse = !inverse;
			
			auto editParam = [&]()
			{
				param->beginEdit ();
				
				if(inverse != (event.getOriginalDirection () == MouseWheelEvent::kWheelUp))
					param->increment ();
				else
					param->decrement ();
				
				param->endEdit ();
			};

			if(event.isContinuous ())
			{
				static float accumulatedDelta = 0.f;
				accumulatedDelta += ccl_abs (event.delta);
				
				if(accumulatedDelta > 50.f)
				{
					editParam ();
					accumulatedDelta = 0.f;
				}
			}
			else
				editParam ();

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::tryWheelParam (const MouseWheelEvent& event, bool inverse)
{
	SharedPtr<View> holder (this);
	if(handleMouseWheel (event, param, inverse))
	{
		if(getTooltip ().contains (CCLSTR ("@value[]")))
			GUI.retriggerTooltip (this);
		else
			GUI.hideTooltip ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::tryRecognizeParam (const DragEvent& event)
{
	UnknownPtr<IRecognizer> recognizer (event.session.getItems ().getFirst ());
	if(recognizer)
	{
		AutoPtr<IUnknown> identity = param ? param->createIdentity () : nullptr;
		if(UnknownPtr<IUnknownList> paramList = &*identity)
			identity = paramList->getFirst ();

		if(identity)
		{
			if(recognizer->recognize (identity))
			{
				event.session.setResult (IDragSession::kDropCopyReal);
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::onDragEnter (const DragEvent& event)
{
	if(tryRecognizeParam (event))
		return true;

	return View::onDragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::onDrop (const DragEvent& event)
{
	if(tryRecognizeParam (event))
		return true;

	return View::onDrop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::onGesture (const GestureEvent& event)
{
	if(event.getType () == GestureEvent::kDoubleTap
	   && !(event.getState () & GestureEvent::kPossible)
	   && canHandleDoubleTap ())
	{
		performReset ();
		return true;
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ bool Control::isResetClick (const MouseEvent& event)
{
	if(event.eventType == MouseEvent::kMouseDown) // Command + Click, ignore Option
		return (event.keys &~ KeyState::kOption) == (KeyState::kLButton|KeyState::kCommand);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Control::canHandleDoubleTap () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Control::performReset ()
{
	if(param)
	{
		param->beginEdit ();
		param->setValue (param->getDefaultValue (), true);
		param->endEdit ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Control::makeEditTooltip ()
{
	if(getTooltip ().contains (CCLSTR ("value[]")))
		return ComposedTooltip (this);	// value string is included in tooltip pattern
	else
	{
		// only the value string
		String text;
		if(param)
			param->toString (text);
		return text;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParamPreviewHandler* Control::getPreviewHandler () const
{
	IParameter* originalParam = param ? param->getOriginal () : nullptr;
	return UnknownPtr<IParamPreviewHandler> (originalParam ? originalParam->getController () : nullptr);
}
