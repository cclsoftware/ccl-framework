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
// Filename    : ccl/gui/controls/autoscroller.cpp
// Description : AutosScroller
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/autoscroller.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/views/view.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/controls/scrollbar.h"
#include "ccl/gui/views/scrollview.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/controls/usercontrolhost.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// AutoScroller
//************************************************************************************************

DEFINE_CLASS (AutoScroller, Object)
DEFINE_CLASS_UID (AutoScroller, 0x2A38F2E9, 0x2AD2, 0x4C3F, 0x9C, 0x46, 0xA6, 0xDD, 0xE5, 0xBF, 0x5A, 0xE6)

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoScroller::AutoScroller (View* view)
: targetView (nullptr),
  dragSession (nullptr),
  directionFlags (MouseHandler::kAutoScroll),
  state (kDisabled),
  minSpeed (8.f),
  maxSpeed (500.f),
  turboStartFactor (4),
  turboFactor (4),
  turboMaxSpeed (15000),
  turboBoostFactor (1.66f),
  nextTurboBoostTime (0),
  innerMargin (20),
  outerMargin (80),
  outerStartMargin (50),
  nextTime (0),
  lastPos (kMinCoord, kMinCoord),
  didScroll (false),
  inTryScrolling (false)
{
	setTargetView (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AutoScroller::construct (IView* targetView)
{
	setTargetView (unknown_cast<View> (targetView));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoScroller::setTargetView (View* view)
{
	targetView = view;

	// find view that implements IScrollable
	baseScrollable = GetViewInterfaceUpwards<IScrollable> (targetView);

	state = baseScrollable ? kObserving : kDisabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoScroller::setDragSession (DragSession* session)
{
	dragSession = session;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point AutoScroller::getMousePos ()
{
	// quick & dirty to distinguish touch (called via trigger ()) from mouse
	if(lastPos.x != kMinCoord)
		return lastPos;

	Point mousePos;
	return GUI.getMousePosition (mousePos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AutoScroller::calcScrollFactor (Coord mouseDist, Coord range)
{
	float f = ccl_bound (mouseDist / (float)range, -1.f, +1.f);
	return ccl_sign (f) * f * f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScrollable* AutoScroller::findScrollable ()
{
	float x, y;
	return getScrollFactors (x, y, getMousePos ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScrollable* AutoScroller::getScrollFactors (float& x, float& y, PointRef mousePos)
{
	IScrollable* result = nullptr;

	if(getScrollFactors (x, y, mousePos, baseScrollable))
		result = baseScrollable;

	ScrollView* scrollView = unknown_cast<ScrollView> (baseScrollable);
	if(scrollView)
	{
		// check if scrollable can scroll at all
		if(!scrollView->canScrollH ())
			x = 0;
		if(!scrollView->canScrollV ())
			y = 0;

		if(x == 0 && y == 0)
		{
			// try to find a parent scrollable
			View* view = scrollView;
			if(view)
			{
				view = view->getParent ();
				IScrollable* parentScrollable = GetViewInterfaceUpwards<IScrollable> (view);
				if(parentScrollable != nullptr)
				{
					if(getScrollFactors (x, y, mousePos, parentScrollable))
					{
						result = parentScrollable;
						// todo: recursion until a useful scrollable found (check if it's a scrollview, canScroll, ...)
					}
				}
			}
		}
	}
	return (x != 0 || y != 0) ? result : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoScroller::getScrollFactors (float& x, float& y, PointRef mousePos, IScrollable* scrollable)
{
	x = y = 0;

	Rect inside;
	scrollable->getClipViewRect (inside);
	Rect outside (inside);
	inside.contract (innerMargin);

	// outside rect is used to determine the available range, outside it we scroll at max speed
	outside.expand (outerMargin);
	if(Window* dragWindow = dragSession ? targetView->getWindow () : nullptr)
		outside.bound (dragWindow->getSize ());
	else
	{
		int monitor = Desktop.findNearestMonitor (inside);
		Rect screenRect;
		Desktop.getMonitorSize (screenRect, monitor, false);
		outside.bound (screenRect);
	}

	if(directionFlags & MouseHandler::kAutoScrollH)
	{
		Coord rangeL = ccl_max (1, inside.left - outside.left);
		Coord rangeR = ccl_max (1, outside.right - inside.right);

		if(mousePos.x < inside.left)
			x = calcScrollFactor (mousePos.x - inside.left, rangeL);
		else if(mousePos.x > inside.right)
			x = calcScrollFactor (mousePos.x - inside.right, rangeR);
	}

	if(directionFlags & MouseHandler::kAutoScrollV)
	{
		Coord rangeT = ccl_max (1, inside.top - outside.top);
		Coord rangeB = ccl_max (1, outside.bottom - inside.bottom);

		if(mousePos.y < inside.top)
			y = calcScrollFactor (mousePos.y - inside.top, rangeT);
		else if(mousePos.y > inside.bottom)
			y = calcScrollFactor (mousePos.y - inside.bottom, rangeB);
	}

	return x != 0 || y != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoScroller::checkTurbo (float& factorX, float& factorY)
{
	if(GUI.isKeyPressed (VKey::kSpace))
	{
		int64 now = System::GetSystemTicks ();
		if(now >= nextTurboBoostTime)
		{
			if(nextTurboBoostTime == 0)
				turboFactor = turboStartFactor;
			else
				turboFactor *= turboBoostFactor;

			nextTurboBoostTime = now + kTurboTimeout;
		}

		factorX *= turboFactor;
		factorY *= turboFactor;
	}
	else
		nextTurboBoostTime = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline float AutoScroller::calcSpeed (float scrollFactor)
{
	float speed = scrollFactor * maxSpeed;

	if(speed > 0)
		return ccl_bound (speed, minSpeed, turboMaxSpeed);
	else if(speed < 0)
		return ccl_bound (speed, -turboMaxSpeed, -minSpeed);

	return speed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoScroller::tryScrolling ()
{
	// must not reenter: would swallow gui events in this calling sequence:
	// Window::onMouseMove -> MouseHandler::trigger -> GUI.flushUpdates () -> (timer) -> AutoScroller::tryScrolling () -> Window::onMouseMove ...
	if(inTryScrolling)
		return;

	ScopedVar<bool> guard (inTryScrolling, true);

	float factorX = 0, factorY = 0;
	Point mousePos (getMousePos ());

	if(IScrollable* scrollable = getScrollFactors (factorX, factorY, mousePos))
	{
		int64 now = System::GetSystemTicks ();
		if(now >= nextTime)
		{
			int64 passedTime = now - lastTime;
			float passedSeconds = 0.001f * passedTime;

			checkTurbo (factorX, factorY);

			// calc speeds
			float speedX = calcSpeed (factorX);
			float speedY = calcSpeed (factorY);

			Coord offsetX = Coord (speedX * passedSeconds);
			Coord offsetY = Coord (speedY * passedSeconds);
			if(offsetX || offsetY)
			{
				UnknownPtr<IScrollView> scrollView (scrollable);
				if(scrollView)
				{
					PointRef snap (scrollView->getSnap ());
					if(offsetX && ccl_abs (offsetX) < snap.x)
						offsetX = snap.x * (int)ccl_sign (factorX);
					if(offsetY && ccl_abs (offsetY) < snap.y)
						offsetY = snap.y * (int)ccl_sign (factorY);
				}
				CCL_PRINTF ("AutoScroller: factors (%.1f, %.1f) speed (%.1f, %.1f) passed %3d, scroll (%2d, %2d)\n", factorX, factorY, speedX, speedY, int(passedTime), offsetX, offsetY)

				// get old scroll values
				IParameter* hParam = scrollable->getHScrollParam ();
				IParameter* vParam = scrollable->getHScrollParam ();
				Variant oldValueH = 0;
				Variant oldValueV = 0;
				if(hParam)
					oldValueH = hParam->getValue ();
				if(vParam)
					oldValueV = vParam->getValue ();

				scrollable->scrollByH (offsetX);
				scrollable->scrollByV (offsetY);

				// check if it did scroll
				didScroll = (hParam && (oldValueH != hParam->getValue ())) ||
							(vParam && (oldValueV != vParam->getValue ()));

				lastTime = now;
				nextTime = now + kScrollTimeout;

				Window* window = targetView->getWindow ();
				if(window)
				{
					// trigger a mouse move event (eg. for updating a mouse handler)
					window->screenToClient (mousePos); // (to window)
					MouseEvent event (MouseEvent::kMouseMove, mousePos);
					GUI.getKeyState (event.keys);

					// prevent damage when AutoScroller gets destroyed during onMouseMove (e.g. owned by a mouse handler)
					// (seen after another application came to foreground during mouse editing)
					SharedPtr<Object> holder (this);
					window->onMouseMove (event);
					if(getRetainCount () == 1)
						return;

					// update an active drag handler
					if(IDragHandler* dragHandler = dragSession ? dragSession->getDragHandler () : nullptr)
					{
						DragEvent e (*dragSession, DragEvent::kDragOver);
						GUI.getKeyState (e.keys);
						e.where = mousePos;
						targetView->windowToClient (e.where);
						dragHandler->dragOver (e);
					}
				}
			}
		}
	}
	else
	{
		CCL_PRINTLN ("AutoScroller: observing.")
		state = kObserving;
		stopTimer ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoScroller::tryStartScrolling ()
{
	ASSERT (state == kWaiting)

	IScrollable* scrollable = findScrollable ();
	if(!scrollable)
		return;

	bool canStart = true;

	if(dragSession)
	{
		// when dragging: only start scrolling if mouse is still near clip view edge
		Point mousePos (getMousePos ());

		Rect outerLimit;
		scrollable->getClipViewRect (outerLimit);
		Rect innerLimit (outerLimit);

		outerLimit.expand (outerStartMargin);
		innerLimit.contract (innerMargin);

		canStart = outerLimit.pointInside (mousePos) && !innerLimit.pointInside (mousePos);
	}

	if(canStart)
	{
		CCL_PRINTLN ("----- AutoScroller: starting scrolling")
		state = kScrolling;
		nextTime = System::GetSystemTicks ();
		lastTime = nextTime - kScrollTimeout;
		didScroll = false;

		tryScrolling ();
	}
	else
	{
		CCL_PRINTLN ("AutoScroller: observing.")
		state = kObserving;
		stopTimer ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoScroller::onMouseMove (int flags)
{
	directionFlags = flags;
	if(state == kObserving)
	{
		if(findScrollable ())
		{
			state = kWaiting;
			nextTime = System::GetSystemTicks () + kStartTimeout;
			startTimer ();
			CCL_PRINTLN ("AutoScroller: waiting...")
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AutoScroller::trigger (PointRef screenPos, int flags)
{
	lastPos = screenPos;
	onMouseMove (flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoScroller::onIdleTimer ()
{
	if(state == kScrolling)
		tryScrolling ();
	else if(state == kWaiting)
	{
		int64 now = System::GetSystemTicks ();
		if(now >= nextTime)
			tryStartScrolling ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoScroller::isScrolling ()
{
	return (state == kScrolling) && didScroll;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* AutoScroller::getScrollView ()
{
	return unknown_cast<ScrollView> (findScrollable ());
}
