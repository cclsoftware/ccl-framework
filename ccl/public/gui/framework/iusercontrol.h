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
// Filename    : ccl/public/gui/framework/iusercontrol.h
// Description : User Control Interface
//
//************************************************************************************************

#ifndef _ccl_iusercontrol_h
#define _ccl_iusercontrol_h

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

interface IGraphics;
interface IMouseHandler;
interface ITouchHandler;
interface IDragHandler;
interface IAccessibilityProvider;

//************************************************************************************************
// ViewEvent
/** Base class for view events. 
	\ingroup gui_appview */
//************************************************************************************************

struct ViewEvent: GUIEvent
{
	enum EventType
	{
		kDraw,			///< DrawEvent
		kViewsChanged,	///< subviews added or removed
		kActivate,		///< view was activated
		kDeactivate,	///< view was deactivated
		kAttached,		///< ViewParentEvent
		kRemoved,		///< ViewParentEvent
		kSized,			///< ViewSizeEvent
		kMoved,			///< ViewSizeEvent
		kChildSized,	///< ViewSizeEvent
		kVisualStyleChanged	///< the visual style has changed
	};

	ViewEvent (int type)
	: GUIEvent (kViewEvent, type, 0)
	{}
};

//************************************************************************************************
// DrawEvent
/** Draw event. 
	\ingroup gui_appview */
//************************************************************************************************

struct DrawEvent: ViewEvent
{
	IGraphics& graphics;
	const UpdateRgn& updateRgn;

	DrawEvent (IGraphics& graphics,
			   const UpdateRgn& updateRgn)
	: ViewEvent (kDraw),
	  graphics (graphics),
	  updateRgn (updateRgn)
	{}
};

//************************************************************************************************
// ViewParentEvent
/** Attach/remove event. 	
	\ingroup gui_appview */
//************************************************************************************************

struct ViewParentEvent: ViewEvent
{
	IView* parent;

	ViewParentEvent (IView* parent, int type = kAttached)
	: ViewEvent (type),
	  parent (parent)
	{}
};

//************************************************************************************************
// ViewSizeEvent
/** Size/move event. 
	\ingroup gui_appview */
//************************************************************************************************

struct ViewSizeEvent: ViewEvent
{
	Point delta;
	IView* child;

	ViewSizeEvent (PointRef delta, int type = kSized, IView* child = nullptr)
	: ViewEvent (type),
	  delta (delta),
	  child (child)
	{}
};

//************************************************************************************************
// IUserControl
/** User-side interface for user controls. 
	\ingroup gui_appview */
//************************************************************************************************

interface IUserControl: IUnknown
{
	/** Handle view event. */
	virtual tbool CCL_API onViewEvent (const GUIEvent& event) = 0;

	/** Create handler for mouse tracking. */
	virtual IMouseHandler* CCL_API createMouseHandler (const MouseEvent& event) = 0;

	/** Create handler for touch tracking. */
	virtual ITouchHandler* CCL_API createTouchHandler (const TouchEvent& event) = 0;

	/** Create handler for mouse tracking. */
	virtual IDragHandler* CCL_API createDragHandler (const DragEvent& event) = 0;

	/** Get associated controller. */
	virtual IUnknown* CCL_API getController () const = 0;

	/** Get custom accessibility provider (optional). */
	virtual IAccessibilityProvider* CCL_API getCustomAccessibilityProvider () const = 0;

	DECLARE_IID (IUserControl)
};

DEFINE_IID (IUserControl, 0x39ca430, 0x93ad, 0x45d1, 0x95, 0x19, 0x95, 0xa4, 0x81, 0xa, 0x66, 0xb5)

//************************************************************************************************
// IUserControlHost
/** Framework-side interface for user controls.
	\ingroup gui_appview */
//************************************************************************************************

interface IUserControlHost: IUnknown
{
	/** Assign user control. */
	virtual void CCL_API setUserControl (IUserControl* control) = 0;

	/** Get associated user control. */
	virtual IUserControl* CCL_API getUserControl () = 0;

	/** Set mouse handler. */
	virtual void CCL_API setMouseHandler (IMouseHandler* handler) = 0;

	DECLARE_IID (IUserControlHost)
};

DEFINE_IID (IUserControlHost, 0x4003f312, 0x1c59, 0x46f8, 0x92, 0xc0, 0x54, 0xec, 0xcd, 0x45, 0xd4, 0xef)

//************************************************************************************************
// IBackgroundView
/** Implemented by a framework view or user control that can draw the background for composited updates.
	\ingroup gui_view  */
//************************************************************************************************

interface IBackgroundView: IUnknown
{
	/** Check if view can draw an opaque background. */
	virtual tbool CCL_API canDrawControlBackground () const = 0;
	
	/** Draw opaque background for composited update. */
	virtual void CCL_API drawControlBackground (IGraphics& graphics, RectRef src, PointRef offset) = 0;

	DECLARE_IID (IBackgroundView)
};

DEFINE_IID (IBackgroundView, 0xc575ca1c, 0xf965, 0x4587, 0x8c, 0x4, 0x1e, 0x1f, 0xf4, 0x6a, 0x9d, 0xdd)

} // namespace CCL

#endif // _ccl_iusercontrol_h
