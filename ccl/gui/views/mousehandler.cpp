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
// Filename    : ccl/gui/views/mousehandler.cpp
// Description : Mouse Handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/mousehandler.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/controls/autoscroller.h"

#include "ccl/base/message.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// MouseHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MouseHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler::MouseHandler (View* view, int flags)
: view (view),
  flags (flags),
  autoScroller (flags & kAutoScroll ? NEW AutoScroller (view) : nullptr),
  tooltipPopup (view)
{
	// keep a view reference, in case it is destroyed during mouse handling!
	ASSERT (view != nullptr)
	if(view)
		view->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler::~MouseHandler ()
{
	cancelSignals ();

	if(hasTimer ())
		GUI.removeIdleTask (this);

	if(autoScroller)
		autoScroller->release ();

	if(view)
		view->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* MouseHandler::getView () const 
{ 
	return view; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MouseHandler::needTimer () const
{
	return checkKeys () || periodic () || canEscape ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MouseHandler::getFlags () const
{
	return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MouseHandler::begin (const MouseEvent& event)
{
	first = event;

	if(beginAtCurrentPos () && !event.wasTouchEvent ())
	{
		GUI.getMousePosition (first.where);
		view->screenToClient (first.where);
	}
	previous = current = first;

	if(needTimer ())
	{
		GUI.addIdleTask (this);
		hasTimer (true);
	}

	onBegin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MouseHandler::trigger (const MouseEvent& event, int)
{
	if(view && !view->isAttached ())
		return false;

	int moveFlags = 0;
	if(event.where != current.where)
		moveFlags |= kMouseMoved;
	if(event.keys != current.keys)
		moveFlags |= kKeysChanged;

	if(moveFlags == 0)
	{
		#if DEBUG_LOG
		static int counter = 0;
		Debugger::printf ("Mouse not moved (%d)!\n", counter++);
		#endif
	}
	
	CCL_PRINTF ("Mouse trigger %4d %4d\n", (int)event.where.x, (int)event.where.y)

	SharedPtr<MouseHandler> lifeGuard (this); // prevent self destruction in 'onMove'

	current = event;
	
	Point currentAfterMove (current.where);
	getView ()->clientToWindow (currentAfterMove);
	bool result = onMove (moveFlags);
	if(wasCanceled ())
		return false;

	getView ()->windowToClient (currentAfterMove);
	current.where = currentAfterMove;
	
	if(result && autoScroller)
		autoScroller->onMouseMove (getFlags ());

	// flush timers and redraws, otherwise control updates look too sluggish!
	if(!event.wasTouchEvent ())
		GUI.flushUpdates (false);

	previous = current;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MouseHandler::triggerKey (const KeyEvent& event)
{
	return onKeyEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MouseHandler::finish (const MouseEvent& event, tbool canceled)
{
	if(!canceled)
	{
		if(current.where != event.where)
		{
			int moveFlags = 0;
			if(event.where != current.where)
				moveFlags |= kMouseMoved;
			if(event.keys != current.keys)
				moveFlags |= kKeysChanged;
			onMove (moveFlags);		
		}
		current = event;
	}
	else
		wasCanceled (true);

	if(hasTimer ())
	{
		GUI.removeIdleTask (this);
		hasTimer (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MouseHandler::hasStartedOnDoubleClick () const
{
    return first.doubleClicked == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MouseHandler::onBegin ()
{
	CCL_PRINTF ("BEGIN move at %d/%d\n", current.where.x, current.where.y)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MouseHandler::onMove (int moveFlags)
{
	CCL_PRINTF ("mouse move at %d/%d\n", current.where.x, current.where.y)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MouseHandler::onRelease (bool canceled)
{
#if DEBUG_LOG
	CCL_PRINTF ("END move at %d/%d\n", current.where.x, current.where.y)
	if(canceled)
		CCL_PRINTLN ("Canceled!")
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MouseHandler::onKeyEvent (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MouseHandler::cancel ()
{
	Message* m = NEW Message ("cancel");
	m->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MouseHandler::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "cancel")
		if(Window* window = view->getWindow ())
			window->setMouseHandler (nullptr); // handler is destroyed here!!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MouseHandler::onTimer (ITimer*)
{
	if(canEscape () && GUI.isKeyPressed (VKey::kEscape))
	{
		cancel ();
		return;
	}

	int moveFlags = 0;
	if(periodic ())
		moveFlags |= kPeriodicMove;

	if(checkKeys ())
	{
		KeyState keys;
		GUI.getKeyState (keys);
		if(keys != current.keys)
		{
			moveFlags |= kKeysChanged;
			current.keys = keys;
		}
	}

	if(moveFlags != 0)
	{
		if(!onMove (moveFlags))
			cancel ();
	}
}

//************************************************************************************************
// NullMouseHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NullMouseHandler, MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

NullMouseHandler::NullMouseHandler (View* view)
: MouseHandler (view)
{
	isNullHandler (true);
}

//************************************************************************************************
// PeriodicMouseHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PeriodicMouseHandler, MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PeriodicMouseHandler::PeriodicMouseHandler (View* view)
: MouseHandler (view),
  waitAfterFirstClick (kWaitAfterFirstClick),
  waitAfterRepeat (kWaitAfterRepeat),
  firstMove (true),
  nextTime (0)
{
	periodic (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PeriodicMouseHandler::onMove (int moveFlags)
{
	if((moveFlags & kPeriodicMove) == 0)
		return true;

	if(firstMove)
	{
		nextTime = System::GetSystemTicks () + waitAfterFirstClick;
		firstMove = false;
	}
	else
	{
		int64 now = System::GetSystemTicks ();
		if(now < nextTime)
			return true;
		nextTime = now + waitAfterRepeat;
	}

	return onPeriodic ();
}
