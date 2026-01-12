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
// Filename    : ccl/public/gui/framework/imultitouch.h
// Description : Multitouch Interfaces
//
//************************************************************************************************

#ifndef _ccl_imultitouch_h
#define _ccl_imultitouch_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

interface IWindow;

using Core::TouchInfo;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (TouchInputManager, 0x6da36ba4, 0xb839, 0x440b, 0x98, 0x31, 0xea, 0x71, 0x5d, 0x11, 0x7f, 0xfe);
}

//************************************************************************************************
// ITouchCollection
/** 
	\ingroup gui */
//************************************************************************************************

interface ITouchCollection: IUnknown
{
	/** Get number of touches. */
	virtual int CCL_API getTouchCount () const = 0;

	/** Get touch information at given index. */
	virtual const TouchInfo& CCL_API getTouchInfo (int index) const = 0;

	/** Get touch information by ID. */
	virtual const TouchInfo* CCL_API getTouchInfoByID (TouchID id) const = 0;

	DECLARE_IID (ITouchCollection)
};

DEFINE_IID (ITouchCollection, 0x5ce11e07, 0x9691, 0x48a4, 0x96, 0xf, 0xc6, 0x1a, 0x88, 0xb1, 0x2f, 0xba)

//************************************************************************************************
// ITouchHandler
/** Touch handler interface. 
	\ingroup gui */
//************************************************************************************************

interface ITouchHandler: IUnknown
{
	/** Begin touch tracking. */
	virtual void CCL_API begin (const TouchEvent& event) = 0;

	/** Accept an additional touch or return false. */
	virtual tbool CCL_API addTouch (const TouchEvent& event) = 0;

	/** Handle touch event. */
	virtual tbool CCL_API trigger (const TouchEvent& event) = 0;

	/** Finish touch tracking. */
	virtual void CCL_API finish (const TouchEvent& event, tbool canceled = false) = 0;

	/** Get GestureEvent::EventType codes for required gestures; return false when no more gesture alternatives. */
	virtual tbool CCL_API getRequiredGesture (int& gestureType, int& priority, int index) = 0;

	/** Handle gesture event; position is in window coordinates. */
	virtual tbool CCL_API onGesture (const GestureEvent& event) = 0;

	/** Query if handler wants to pass handling of the given gesture to another handler (e.g. when touches are added / removed). */
	virtual tbool CCL_API allowsCompetingGesture (int gestureType) = 0;

	DECLARE_IID (ITouchHandler)
};

DEFINE_IID (ITouchHandler, 0x30D0BC8C, 0x84EB, 0x48FE, 0xA9, 0xE4, 0x18, 0x54, 0xAD, 0xA2, 0x40, 0xAA)

//************************************************************************************************
// ITouchInputManager
/** Low-level multi-touch input. */
//************************************************************************************************

interface ITouchInputManager: IUnknown
{
	/** Process a touch event. */
	virtual tresult CCL_API processTouches (IWindow* window, const TouchEvent& event) = 0;

	/** Discard all touches in a window. */
	virtual tresult CCL_API discardTouches (IWindow* window) = 0;
	
	DECLARE_IID (ITouchInputManager)
};

DEFINE_IID (ITouchInputManager, 0xed4b627e, 0xf086, 0x4400, 0xb6, 0xd7, 0xba, 0x51, 0x4a, 0xbb, 0x75, 0x85)

} // namespace CCL

#endif // _ccl_imultitouch_h
