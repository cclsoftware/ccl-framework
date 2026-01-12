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
// Filename    : ccl/public/gui/framework/imousehandler.h
// Description : Mouse Handler Interface
//
//************************************************************************************************

#ifndef _ccl_imousehandler_h
#define _ccl_imousehandler_h

#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

interface IView;

namespace ClassID
{
	DEFINE_CID (AutoScroller, 0x2A38F2E9, 0x2AD2, 0x4C3F, 0x9C, 0x46, 0xA6, 0xDD, 0xE5, 0xBF, 0x5A, 0xE6);
}

//************************************************************************************************
// IMouseHandler
/** Mouse handler interface. 
	\ingroup gui */
//************************************************************************************************

interface IMouseHandler: IUnknown
{
	enum CheckFlags
	{
		kCheckKeys		   = 1<<0,	///< move is called if key state changes
		kPeriodic		   = 1<<1,	///< move is called periodically
		kCanEscape		   = 1<<2,	///< cancel if [Escape] is pressed
		kNullHandler	   = 1<<3,	///< null handler used to swallow mouse click
		kAutoScrollH	   = 1<<4,	///< try autoscrolling at scrollview edges, might change during tracking
		kAutoScrollV	   = 1<<5,	///< try autoscrolling at scrollview edges, might change during tracking
		kBeginAtCurrentPos = 1<<6,	///< update current mouse position on begin (), instead of using the one from kMouseDown (useful for relative calculations, mouse might have moved during waitDrag)
		kAutoScroll		   = kAutoScrollH|kAutoScrollV
	};

	enum MoveFlags
	{ 
		kMouseMoved   = 1<<0,	///< mouse position has changed
		kKeysChanged  = 1<<1,	///< key state has changed
		kPeriodicMove = 1<<2	///< timer period has elapsed
	};

	/** Get check flag. */
	virtual int CCL_API getFlags () const = 0;

	/** Begin mouse tracking. */
	virtual void CCL_API begin (const MouseEvent& event) = 0;

	/** Handle mouse event. */
	virtual tbool CCL_API trigger (const MouseEvent& event, int moveFlags) = 0;

	/** Handle key event. */
	virtual tbool CCL_API triggerKey (const KeyEvent& event) = 0;

	/** Finish mouse tracking. */
	virtual void CCL_API finish (const MouseEvent& event, tbool canceled = false) = 0;

	DECLARE_IID (IMouseHandler)
};

DEFINE_IID (IMouseHandler, 0x46fe558, 0x669c, 0x4cf4, 0xa3, 0x85, 0x2d, 0xb6, 0xd2, 0xa0, 0xb2, 0x58)

//************************************************************************************************
// IAutoScroller
/** AutoScroller interface. Triggered during mouse/touch/drag operations.
	\ingroup gui */
//************************************************************************************************

interface IAutoScroller: IUnknown
{
	/** Initialize AutoScroller (created via ccl_new) with targetView. */
	virtual void CCL_API construct (IView* targetView) = 0;

	/** Trigger when mouse/touch has moved. Specify allowed autoscroll directions using IMouseHandler::kAutoScroll flags. */
	virtual void CCL_API trigger (PointRef screenPos, int autoScrollFlags) = 0;

	DECLARE_IID (IAutoScroller)
};

DEFINE_IID (IAutoScroller, 0x2A38F2E9, 0x2AD2, 0x4C3F, 0x9C, 0x46, 0xA6, 0xDD, 0xE5, 0xBF, 0x5A, 0xE6)

} // namespace CCL

#endif // _ccl_imousecursor_h
