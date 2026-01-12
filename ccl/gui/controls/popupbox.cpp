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
// Filename    : ccl/gui/controls/popupbox.cpp
// Description : Popup Box
//
//************************************************************************************************

#include "ccl/gui/controls/popupbox.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchcollection.h"
#include "ccl/gui/gui.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

void setMouseStateDeep (View* parent, int state)
{
	if(parent)
		ForEachViewFast (*parent, child)
			child->setMouseState (state);
			setMouseStateDeep (child, state);
		EndFor
}

} // namespace

//************************************************************************************************
// PopupBox::ClientTouchHandler
/** Delegates touch events to a touch handler created by a IPopupSelectorClient.
 	Used for remote controlling a popup slider from a touch sequence originating on the PopupBox. */
//************************************************************************************************

class PopupBox::ClientTouchHandler: public RemotePopupTouchHandler
{
public:
	ClientTouchHandler (PopupBox* popupBox = nullptr, bool overridePosition = false)
	: RemotePopupTouchHandler (popupBox, overridePosition),
	  popupBox (popupBox)
	{
		openPopupImmediately (true);
	}

	// RemotePopupTouchHandler
	void openPopup () override
	{
		popupBox->showPopup ();
	}

	PopupSelector* getPopupSelector () const override
	{
		return popupBox->popupSelector;
	}

	ITouchHandler* createTouchHandlerInPopup (const TouchEvent& event, Window& popupWindow) override
	{
		return popupBox->client ? popupBox->client->createTouchHandler (event, &popupWindow) : nullptr;
	}

	void onBegin (const TouchEvent& event) override
	{
		// the popup should appear under the finger
		if(const TouchInfo* info = event.touches.getTouchInfoByID (event.touchID))
		{
			Point where (info->where);
			popupBox->getWindow ()->clientToScreen (where);
			if(IPopupSelectorClient* client = popupBox->getClient ())
			{
				client->setCursorPosition (where);
				if(UnknownPtr<IObject> obj = client)
					obj->setProperty ("forceTouch", true);
			}
		}

		RemotePopupTouchHandler::onBegin (event);
	}

private:
	PopupBox* popupBox;
};

//************************************************************************************************
// PopupBox
//************************************************************************************************

BEGIN_STYLEDEF (PopupBox::customStyles)
	{"slider", Styles::kPopupBoxBehaviorSlider},
	{"overrideposition", Styles::kPopupBoxBehaviorOverridePosition},
	{"wantsfocus", Styles::kPopupBoxBehaviorWantsFocus},
	{"keepmousepos", Styles::kPopupBoxBehaviorKeepMousePos},
	{"doubleclick", Styles::kPopupBoxBehaviorWantsDoubleClick},
	{"nowheel", Styles::kPopupBoxBehaviorNoWheel},
	{"wantsmouseview", Styles::kPopupBoxBehaviorWantsMouseView},
	{"hasparameter", Styles::kPopupBoxBehaviorHasTriggerParameter},
	{"mouseinside", Styles::kPopupBoxBehaviorWantsMouseInside},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PopupBox, View)
DEFINE_CLASS_UID (PopupBox, 0xECACA482, 0xD7F3, 0x416E, 0xBB, 0x73, 0x79, 0x66, 0x28, 0xDF, 0x8E, 0xD8)

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupBox::PopupBox (IPopupSelectorClient* client, StringID formName, const Rect& size, IParameter* param, StyleRef style)
: Control (size, param, style),
  popupSelector (NEW PopupSelector),
  client (nullptr),
  formName (formName),
  popupOptions (PopupSizeInfo::kLeft|PopupSizeInfo::kBottom)
{
	take_shared (this->client, client);
	
	suppressesChildTouch (true);
	wantsFocus (style.isCustomStyle (Styles::kPopupBoxBehaviorWantsFocus));
	wantsDoubleClick (style.isCustomStyle (Styles::kPopupBoxBehaviorWantsDoubleClick));
	setWheelEnabled (style.isCustomStyle (Styles::kPopupBoxBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupBox::~PopupBox ()
{
	if(client)
		client->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef PopupBox::getHelpIdentifier () const
{
	if(!name.isEmpty () && !getStyle ().isCommonStyle (Styles::kNoHelpId))
		return name;

	return SuperClass::getHelpIdentifier ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupBox::showPopup ()
{
	if(!popupSelector->isOpen () && client)
	{
		setMouseStateDeep (this, kMouseDown);

		if(getStyle ().isCustomStyle (Styles::kPopupBoxBehaviorHasTriggerParameter))
			if(IParameter* param = getParameter ())
				param->setValue (true, true);

		Point offset;
		Coord minWidth = getWidth ();
		if(const IVisualStyle* vs = popupSelector->getVisualStyle ())
		{
			offset (vs->getMetric<Coord> ("popup.offset.x", 0), vs->getMetric<Coord> ("popup.offset.y", 0));
			minWidth = vs->getMetric<Coord> ("popup.minwidth", minWidth);
		}
		
		PopupSizeInfo sizeInfo (this, popupOptions, offset);
		sizeInfo.canFlipParentEdge (true);
		sizeInfo.sizeLimits.minWidth = minWidth;

		popupSelector->setTheme (getTheme ());

		// set decorform name, when no explicit popupStyle/visualStyle is set.
		if(popupSelector->getVisualStyle () == nullptr)
			popupSelector->setDecorNameFromStyle (getVisualStyle ());

		IAsyncOperation* operation = nullptr;
		IView* view = nullptr;

		SharedPtr<Control> popupBox (this);

		if(!formName.isEmpty ())
		{
			// try to create a skin view
			view = getTheme ().createView (formName, client, &formVariables);
			if(view)
			{
				Rect size (view->getSize ());
				sizeInfo.sizeLimits.makeValid (size);
				view->setSize (size);

				operation = popupSelector->popupAsync (view, client, sizeInfo);
			}
		}

		// let client create the view
		if(!view)
			operation = popupSelector->popupAsync (client, sizeInfo);

		Promise (operation).then ([popupBox] (IAsyncOperation& operation)
		{
			setMouseStateDeep (popupBox, kMouseNone);

			if(popupBox->getStyle ().isCustomStyle (Styles::kPopupBoxBehaviorHasTriggerParameter))
				if(IParameter* param = popupBox->getParameter ())
					param->setValue (false, true);
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBox::onMouseDown (const MouseEvent& event)
{
	if(wantsDoubleClick () && !detectDoubleClick (event))
		return false;

	Point mousePos;
	GUI.getMousePosition (mousePos);
	if(client)
	{
		client->setCursorPosition (mousePos);
		if(UnknownPtr<IObject> obj = client)
			obj->setProperty ("forceTouch", false);
	}
	
	if(Control::isResetClick (event))
	{
		if(client->setToDefault ())
			return true;
		// let resetclick fall through
		return false;
	}

	if(event.eventType == MouseEvent::kMouseDown && event.keys == (KeyState::kOption|KeyState::kLButton))
		return false; // let option click fall through

	showPopup ();
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* PopupBox::enterMouse (const MouseEvent& event, View* currentMouseView)
{
	if(getStyle ().isCustomStyle (Styles::kPopupBoxBehaviorWantsMouseView))
	{
		if(this == currentMouseView)
		{
			MouseEvent e2 (event);
			e2.eventType = MouseEvent::kMouseMove;
			onMouseMove (e2);
		}
		else
			onMouseEnter (event);

		return this;
	}
	return SuperClass::enterMouse (event, currentMouseView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBox::onMouseEnter (const MouseEvent& event)
{
	setMouseStateDeep (this, popupSelector->isOpen () ? kMouseDown : kMouseOver);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBox::onMouseMove (const MouseEvent& event)
{
	setMouseStateDeep (this, popupSelector->isOpen () ? kMouseDown : kMouseOver);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBox::onMouseLeave (const MouseEvent& event)
{
	setMouseStateDeep (this, popupSelector->isOpen () ? kMouseDown : kMouseNone);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBox::onMouseWheel (const MouseWheelEvent& event)
{
	if(View::onMouseWheel (event))
		return true;

	if(client && isWheelEnabled ())
		return client->mouseWheelOnSource (event, this) != 0;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBox::onGesture (const GestureEvent& event)
{
	if(event.getType () == GestureEvent::kSingleTap)
	{
		showPopup ();
		return true;
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* PopupBox::createTouchHandler (const TouchEvent& event)
{
	if(style.isCustomStyle (Styles::kPopupBoxBehaviorSlider))
	{
		auto handler = NEW ClientTouchHandler (this, style.isCustomStyle (Styles::kPopupBoxBehaviorOverridePosition));
		
		if(getStyle ().isHorizontal ())
			handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, GestureEvent::kPriorityHigh);
		else if(getStyle ().isVertical ())
			handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, GestureEvent::kPriorityHigh);
		else
			handler->addRequiredGesture (GestureEvent::kSwipe, GestureEvent::kPriorityHigh);

		handler->addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh);
		
		return handler;
	}
	else
		return NEW GestureHandler (this, GestureEvent::kSingleTap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupBox::setPopupVisualStyle (VisualStyle* visualStyle)
{
	popupSelector->setVisualStyle (visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupBox::attached (View* parent)
{
	SuperClass::attached (parent);

	if(style.isCustomStyle (Styles::kPopupBoxBehaviorWantsMouseInside))
		GUI.addIdleTask (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupBox::removed (View* parent)
{
	if(style.isCustomStyle (Styles::kPopupBoxBehaviorWantsMouseInside))
	{
		GUI.removeIdleTask (this);
		popupSelector->close ();
	}

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupBox::onTimer (ITimer* timer)
{
	if(isAttached ())
	{
		ASSERT (style.isCustomStyle (Styles::kPopupBoxBehaviorWantsMouseInside))

		Point p;
		GUI.getMousePosition (p);
		screenToClient (p);

		Rect client;
		getVisibleClient (client);

		if(client.pointInside (p))
			showPopup ();
		else
			popupSelector->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kRequestFocus)
	{
		if(param && param->getValue ().asBool () == true)
			return;

		if(isAttached ())
			showPopup ();
	}
	else if(msg == IParameter::kReleaseFocus)
	{
		if(param && param->getValue ().asBool () == false)
			return;

		popupSelector->close ();
	}
	else
		SuperClass::notify (subject, msg);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PopupBox)
	DEFINE_METHOD_NAME ("showPopup")
	DEFINE_METHOD_NAME ("closePopup")
END_METHOD_NAMES (PopupBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupBox::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "showPopup")
	{
		showPopup ();
		return true;
	}
	else if(msg == "closePopup")
	{
		popupSelector->close ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
