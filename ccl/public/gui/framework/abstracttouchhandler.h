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
// Filename    : ccl/public/gui/framework/abstracttouchhandler.h
// Description : Abstract touch handler base class
//
//************************************************************************************************

#ifndef _ccl_abstracttouchhandler_h
#define _ccl_abstracttouchhandler_h

#include "ccl/public/gui/framework/imultitouch.h"
#include "ccl/public/gui/framework/imousehandler.h"
#include "ccl/public/gui/framework/iview.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// AbstractTouchHandler
/** Base class for implementing a touch handler.
\ingroup gui  */
//************************************************************************************************

class AbstractTouchHandler: public ITouchHandler
{
public:
	AbstractTouchHandler (IView* view = nullptr);
	virtual ~AbstractTouchHandler ();

	// View
	IView* getView () const;

	// Required gestures
	void addRequiredGesture (int gestureType, int priority = GestureEvent::kPriorityNormal);

	// ITouchHandler
	void CCL_API begin (const TouchEvent& event) override;
	tbool CCL_API addTouch (const TouchEvent& event) override;
	tbool CCL_API trigger (const TouchEvent& event) override;
	void CCL_API finish (const TouchEvent& event, tbool canceled = false) override;
	tbool CCL_API getRequiredGesture (int& gestureType, int& priority, int index) override;
	tbool CCL_API onGesture (const GestureEvent& event) override;
	tbool CCL_API allowsCompetingGesture (int gestureType) override;

	// overwrite:
	virtual bool onHover (const TouchEvent& event); // kEnter, kHover, kLeave
	virtual void onBegin (const TouchEvent& event);
	virtual bool onMove (const TouchEvent& event);
	virtual void onRelease (const TouchEvent& event, bool canceled);

protected:
	IView* view;

	struct GestureItem
	{
		int gestureType;
		int priority;

		GestureItem (int gestureType = 0, int priority = 0)
		: gestureType (gestureType), priority (priority) {}
	};

	const CCL::Vector<GestureItem>& getRequiredGestures () const;

private:
	CCL::Vector<GestureItem> requiredGestures;
};

//************************************************************************************************
// AbstractTouchMouseHandler
//************************************************************************************************

class AbstractTouchMouseHandler: public AbstractTouchHandler
{
public:
	static MouseEvent makeMouseEvent (int eventType, const TouchEvent& event, IView& view);		///< translates position from window to view client
	static MouseEvent makeMouseEvent (int eventType, const GestureEvent& event, IView& view);	///< translates position from window to view client
	static MouseEvent makeMouseEvent (int eventType, const GestureEvent& event);				///< does not modify position, use when position is already in view coordinates (e.g. inside View::onGesture, AbstractUserControl::onGesture)
	static void triggerSingleClick (IMouseHandler& handler, const GestureEvent& event, IView& view);

	AbstractTouchMouseHandler (IMouseHandler* mouseHandler, IView* view);

	// TouchHandler
	void onBegin (const TouchEvent& event) override;
	bool onMove (const TouchEvent& event) override;
	void onRelease (const TouchEvent& event, bool canceled) override;

protected:
	AutoPtr<IMouseHandler> mouseHandler;
	TouchEvent::InputDevice inputDevice;
	Point lastPos;
	KeyState lastKeys;
};

//************************************************************************************************
// AbstractTouchHandler inline
//************************************************************************************************

inline AbstractTouchHandler::AbstractTouchHandler (IView* view)
: view (view)
{
	ASSERT (view)
	if(view)
		view->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline AbstractTouchHandler::~AbstractTouchHandler ()
{
	if(view)
		view->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractTouchHandler::addRequiredGesture (int gestureType, int priority)
{
	requiredGestures.add (GestureItem (gestureType, priority));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline IView* AbstractTouchHandler::getView () const
{
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void CCL_API AbstractTouchHandler::begin (const TouchEvent& event)
{
	onBegin (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractTouchHandler::addTouch (const TouchEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractTouchHandler::trigger (const TouchEvent& event)
{
	if(event.isHoverEvent ())
		return onHover (event);
	else
		return onMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void CCL_API AbstractTouchHandler::finish (const TouchEvent& event, tbool canceled)
{
	if(event.eventType == TouchEvent::kLeave)
		onHover (event);
	else
		onRelease (event, canceled != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractTouchHandler::getRequiredGesture (int& gestureType, int& priority, int index)
{
	if(index < requiredGestures.count ())
	{
		const GestureItem& item = requiredGestures[index];
		gestureType = item.gestureType;
		priority = item.priority;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const CCL::Vector<AbstractTouchHandler::GestureItem>& AbstractTouchHandler::getRequiredGestures () const
{
	return requiredGestures;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractTouchHandler::onGesture (const GestureEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractTouchHandler::allowsCompetingGesture (int gestureType)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AbstractTouchHandler::onHover (const TouchEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractTouchHandler::onBegin (const TouchEvent& event)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AbstractTouchHandler::onMove (const TouchEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractTouchHandler::onRelease (const TouchEvent& event, bool canceled)
{}

//************************************************************************************************
// AbstractTouchMouseHandler inline
//************************************************************************************************

inline MouseEvent AbstractTouchMouseHandler::makeMouseEvent (int eventType, const TouchEvent& event, IView& view)
{
	const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID);
	ASSERT (touch || eventType == MouseEvent::kMouseUp)
	if(!touch)
		return MouseEvent (eventType);

	Point where = touch->where;
	view.windowToClient (where);

	MouseEvent mouseEvent (eventType, where, event.keys);
	mouseEvent.keys.keys |= KeyState::kLButton;
	mouseEvent.eventTime = touch->time / 1000.;
	mouseEvent.inputDevice = event.inputDevice;
	mouseEvent.penInfo = event.penInfo;
	mouseEvent.dragged = 0;
	mouseEvent.doubleClicked = 0;

	return mouseEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline MouseEvent AbstractTouchMouseHandler::makeMouseEvent (int eventType, const GestureEvent& event, IView& view)
{
	MouseEvent mouseEvent (makeMouseEvent (eventType, event));
	view.windowToClient (mouseEvent.where);
	return mouseEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline MouseEvent AbstractTouchMouseHandler::makeMouseEvent (int eventType, const GestureEvent& event)
{
	MouseEvent mouseEvent (eventType, event.where, event.keys);
	mouseEvent.keys.keys |= KeyState::kLButton;
	mouseEvent.eventTime = event.eventTime;
	mouseEvent.inputDevice = PointerEvent::kTouchInput;
	mouseEvent.dragged = event.getType () == GestureEvent::kLongPress;
	mouseEvent.doubleClicked = event.getType () == GestureEvent::kDoubleTap;

	return mouseEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractTouchMouseHandler::triggerSingleClick (IMouseHandler& handler, const GestureEvent& event, IView& view)
{
	MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseDown, event, view));
	handler.begin (mouseEvent);

	mouseEvent.eventType = MouseEvent::kMouseUp;
	handler.finish (mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline AbstractTouchMouseHandler::AbstractTouchMouseHandler (IMouseHandler* mouseHandler, IView* view)
: AbstractTouchHandler (view)
{
	this->mouseHandler.share (mouseHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractTouchMouseHandler::onBegin (const TouchEvent& event)
{
	MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseDown, event, *view));
	lastPos = mouseEvent.where;
	lastKeys = mouseEvent.keys;
	inputDevice = event.inputDevice;

	if(mouseHandler)
		mouseHandler->begin (mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AbstractTouchMouseHandler::onMove (const TouchEvent& event)
{
	if(mouseHandler)
	{
		if(event.eventType >= TouchEvent::kEnter) // ignore hover events
			return true;

		MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseMove, event, *view));

		int moveFlags = 0;
		if(mouseEvent.where != lastPos)
			moveFlags |= IMouseHandler::kMouseMoved;
		if(mouseEvent.keys != lastKeys)
			moveFlags |= IMouseHandler::kKeysChanged;

		lastPos = mouseEvent.where;
		lastKeys = mouseEvent.keys;

		if(event.eventType == TouchEvent::kEnd)
			onRelease (event, false);
		else
			return mouseHandler->trigger (mouseEvent, moveFlags) != 0;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractTouchMouseHandler::onRelease (const TouchEvent& event, bool canceled)
{
	MouseEvent mouseEvent (makeMouseEvent (MouseEvent::kMouseUp, event, *view));
	if(mouseHandler)
	{
		mouseHandler->finish (mouseEvent, canceled);
		mouseHandler.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_abstracttouchhandler_h
