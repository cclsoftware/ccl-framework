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
// Filename    : ccl/gui/touch/touchhandler.h
// Description : TouchHandler
//
//************************************************************************************************

#ifndef _ccl_touchhandler_h
#define _ccl_touchhandler_h

#include "ccl/public/gui/framework/abstracttouchhandler.h"

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchcollection.h"

namespace CCL {

class PopupSelector;
class Window;

//************************************************************************************************
// TouchHandler
//************************************************************************************************

class TouchHandler: public Object,
					public AbstractTouchHandler
{
public:
	DECLARE_CLASS_ABSTRACT (TouchHandler, Object)

	TouchHandler (IView* view = nullptr);
	~TouchHandler ();

	// Object
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

	CLASS_INTERFACE (ITouchHandler, Object)
};

//************************************************************************************************
// GestureHandler
/** Delegates gesture events to a view. */
//************************************************************************************************

class GestureHandler: public TouchHandler
{
public:
	DECLARE_CLASS_ABSTRACT (GestureHandler, TouchHandler)

	GestureHandler (View* view = nullptr);
	GestureHandler (View* view, int gestureType, int priority = GestureEvent::kPriorityNormal);

	// TouchHandler
	tbool CCL_API onGesture (const GestureEvent& event) override;
};

//************************************************************************************************
// TouchMouseHandler
// Wraps a MouseHandler as a TouchHandler
//************************************************************************************************

class TouchMouseHandler: public Object,
						 public AbstractTouchMouseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (TouchMouseHandler, TouchHandler)

	static void applyGesturePriorities (AbstractTouchHandler& handler, View* view);

	TouchMouseHandler (MouseHandler* mouseHandler, View* view = nullptr);
	~TouchMouseHandler ();

	// AbstractTouchMouseHandler
	bool onMove (const TouchEvent& event) override;
	void onRelease (const TouchEvent& event, bool canceled) override;

	CLASS_INTERFACE (ITouchHandler, Object)
};

//************************************************************************************************
// ViewTouchHandler
// Sends mouseDown / mouseUp events to the view
//************************************************************************************************

class ViewTouchHandler: public TouchMouseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (ViewTouchHandler, TouchMouseHandler)

	ViewTouchHandler (View* view = nullptr);

	// TouchHandler
	void onBegin (const TouchEvent& event) override;
	void onRelease (const TouchEvent& event, bool canceled) override;
};

//************************************************************************************************
// NullTouchHandler
/** Swallows touches. */
//************************************************************************************************

class NullTouchHandler: public TouchHandler
{
public:
	NullTouchHandler (IView* view);
};

//************************************************************************************************
// RemotePopupTouchHandler
/** Delegates touch events from a source view to a touch handler in a PopupSelector.
 	Used for remote controlling a popup from a touch sequence originating on the source view. */
//************************************************************************************************

class RemotePopupTouchHandler: public TouchHandler
{
public:
	RemotePopupTouchHandler (View* sourceView, bool overridePosition = false);
	~RemotePopupTouchHandler ();

	PROPERTY_FLAG (flags, 1<<0, overridePosition)
	PROPERTY_FLAG (flags, 1<<1, openPopupImmediately)
	PROPERTY_FLAG (flags, 1<<2, openPopupOnLongPress)

	PROPERTY_VARIABLE (Coord, minMoveDistance, MinMoveDistance) ///< minimum distance for detecting a "move"

protected:
	int flags;

	PROPERTY_FLAG (flags, 1<<3, isAsyncPopup)
	PROPERTY_FLAG (flags, 1<<4, isSingleTap)
	PROPERTY_FLAG (flags, 1<<5, handlerChecked)
	PROPERTY_FLAG (flags, 1<<6, popupOpened)
	PROPERTY_FLAG (flags, 1<<7, hasMoved)
	PROPERTY_FLAG (flags, 1<<8, wasInsidePopup)

	static constexpr int kLastFlag = 8;

	void openPopupInternal ();
	void createRemoteTouchhandler (const TouchEvent& event);
	void simulateRemoteGesture (int state, const TouchEvent& event);
	void forwardGestureProcessed (const GestureEvent& remoteEvent);
	void determineWindowOffset ();

	View* getSourceView () const;
	Window* getPopupWindow () const;
	static Point getTouchPosition (const TouchEvent& event);

	// to be implemented by derived class
	virtual void openPopup ();
	virtual PopupSelector* getPopupSelector () const;
	virtual ITouchHandler* createTouchHandlerInPopup (const TouchEvent& event, Window& popupWindow);

	// TouchHandler
	void onBegin (const TouchEvent& event) override;
	bool onMove (const TouchEvent& event) override;
	void onRelease (const TouchEvent& event, bool canceled) override;
	tbool CCL_API onGesture (const GestureEvent& event) override;

	class RemoteTouchEvent: public TouchEvent
	{
	public:
		RemoteTouchEvent (const TouchEvent& event, PointRef offset);

		PROPERTY_OBJECT (Point, position, Position)

	private:
		TouchCollection touches;
	};

private:
	AutoPtr<ITouchHandler> remoteTouchHandler;
	Point windowOffset;
	Point initialTouchPos;
	int simulatedGesture;
	int64 startTime;
};


} // namespace CCL

#endif // _ccl_touchhandler_h
