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
// Filename    : ccl/gui/views/mousehandler.h
// Description : Mouse Handler
//
//************************************************************************************************

#ifndef _ccl_mousehandler_h
#define _ccl_mousehandler_h

#include "ccl/base/object.h"

#include "ccl/public/gui/framework/itimer.h"
#include "ccl/public/gui/framework/imousehandler.h"
#include "ccl/public/gui/framework/usertooltip.h"

namespace CCL {

class View;
class AutoScroller;

//************************************************************************************************
// MouseHandler
/** Basic mouse handler. */
//************************************************************************************************

class MouseHandler: public Object,
					public IMouseHandler,
					public ITimerTask
{
public:
	DECLARE_CLASS (MouseHandler, Object)

	MouseHandler (View* view = nullptr, int flags = 0);
	~MouseHandler ();

	View* getView () const;

	PROPERTY_FLAG (flags, kCheckKeys,		  checkKeys)
	PROPERTY_FLAG (flags, kPeriodic,		  periodic)
	PROPERTY_FLAG (flags, kCanEscape,		  canEscape)
	PROPERTY_FLAG (flags, kNullHandler,		  isNullHandler)
	PROPERTY_FLAG (flags, kAutoScrollV,		  autoScrollV)
	PROPERTY_FLAG (flags, kAutoScrollH,		  autoScrollH)
	PROPERTY_FLAG (flags, kAutoScroll,		  autoScroll)
	PROPERTY_FLAG (flags, kBeginAtCurrentPos, beginAtCurrentPos)

	// IMouseHandler
	int CCL_API getFlags () const override;
	void CCL_API begin (const MouseEvent& event) override;
	tbool CCL_API trigger (const MouseEvent& event, int moveFlags = 0) override;
	tbool CCL_API triggerKey (const KeyEvent& event) override;
	void CCL_API finish (const MouseEvent& event, tbool canceled = false) override;

	// overwrite:
	virtual void onBegin ();					///< begin mouse gesture
	virtual bool onMove (int moveFlags);		///< mouse moved or keys changed
	virtual void onRelease (bool canceled);		///< mouse released or canceled

	virtual bool onKeyEvent (const KeyEvent& event);	///< key event occurred during mouse tracking
    
    bool hasStartedOnDoubleClick () const;

	CLASS_INTERFACE2 (IMouseHandler, ITimerTask, Object)

protected:
	View* view;
	MouseEvent first;
	MouseEvent previous;
	MouseEvent current;
	AutoScroller* autoScroller;
	UserTooltipPopup tooltipPopup;

private:
	enum PrivateFlags
	{
		kTimerTask = 1<<8,
		kCanceled = 1<<9
	};

	int flags;
	PROPERTY_FLAG (flags, kTimerTask, hasTimer)
	PROPERTY_FLAG (flags, kCanceled, wasCanceled)

	void CCL_API onTimer (ITimer*) override; // ITimerTask
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool needTimer () const;
	void cancel ();
};

//************************************************************************************************
// NullMouseHandler
/** Empty mouse handler to swallow mouse click. */
//************************************************************************************************

class NullMouseHandler: public MouseHandler
{
public:
	DECLARE_CLASS (NullMouseHandler, MouseHandler)

	NullMouseHandler (View* view = nullptr);
};

//************************************************************************************************
// PeriodicMouseHandler
/** Periodic mouse handler. */
//************************************************************************************************

class PeriodicMouseHandler: public MouseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (PeriodicMouseHandler, MouseHandler)

	PeriodicMouseHandler (View* view = nullptr);

	enum Constants
	{
		kWaitAfterFirstClick = 500,
		kWaitAfterRepeat = 100
	};

	PROPERTY_VARIABLE (int64, waitAfterFirstClick, WaitAfterFirstClick)
	PROPERTY_VARIABLE (int64, waitAfterRepeat, WaitAfterRepeat)

	virtual bool onPeriodic () = 0;	///< overwrite instead of onMove()

	// MouseHandler
	bool onMove (int moveFlags) override;

protected:
	bool firstMove;
	int64 nextTime;
};

//************************************************************************************************
// MouseHandlerDelegate
/** Delagates to an IMouseHandler. */
//************************************************************************************************

class MouseHandlerDelegate: public MouseHandler
{
public:
	MouseHandlerDelegate (View* view, IMouseHandler* handler)
	: MouseHandler (view, handler->getFlags ()),
	  handler (handler)
	{}

	~MouseHandlerDelegate ()
	{
		handler->release ();
	}

	int CCL_API getFlags () const override
	{
		return handler->getFlags ();
	}

	void onBegin () override
	{
		handler->begin (first);
	}

	bool onMove (int moveFlags) override
	{
		return handler->trigger (current, moveFlags) != 0;
	}

	void onRelease (bool canceled) override
	{
		handler->finish (current, canceled);
	}

	bool onKeyEvent (const KeyEvent& event) override
	{
		return handler->triggerKey (event) != 0;
	}

protected:
	IMouseHandler* handler;
};

} // namespace CCL

#endif // _ccl_mousehandler_h
