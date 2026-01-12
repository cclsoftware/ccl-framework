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
// Filename    : ccl/public/gui/framework/ipopupselector.h
// Description : Popup Selector Interface
//
//************************************************************************************************

#ifndef _ccl_ipopupselector_h
#define _ccl_ipopupselector_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/cclmacros.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

interface IPopupSelectorClient;
interface IParameter;
interface IView;
interface ITheme;
interface IVisualStyle;
interface IWindow;
interface IMenu;
interface ITouchHandler;
interface IAsyncOperation;

struct MouseEvent;
struct KeyEvent;
struct TouchEvent;
struct GUIEvent;
struct MouseWheelEvent;

//////////////////////////////////////////////////////////////////////////////////////////////////
// PopupSelector class
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Popup Selector. */
	DEFINE_CID (PopupSelector, 0xFCDB7599, 0x685E, 0x4E20, 0x9C, 0x7B, 0x4C, 0xC2, 0x1A, 0x2B, 0xDE, 0x00);
}

//************************************************************************************************
// PopupSizeInfo
/** Info about positioning & size limits. 
	\ingroup gui */
//************************************************************************************************

struct PopupSizeInfo
{
	/// align at parent edges specified by flags
	PopupSizeInfo (IView* parent, int flags, PointRef offset = Point ());

	/// place at given position relative to parent
	PopupSizeInfo (PointRef where, IView* parent = nullptr);

	enum Flags
	{
		/// alignment of popup view in relation to parent
		kLeft		= 0x01,		///< align popup to the left parent edge
		kRight		= 0x02,		///< align popup to the right parent edge
		kHCenter	= 0x03,		///< center popup horizontally in parent
		kHMouse		= 0x04,		///< place popup horizonally at mouse position
		kHCenterRel = 0x08,     ///< center popup horizontally relative to "where.x"
		kHMask		= 0x0F,

		kTop		= 0x10,		///< align popup to the top parent edge
		kBottom		= 0x20,		///< align popup to the bottom parent edge
		kVCenter	= 0x30,		///< center popup vertically in parent
		kVMouse		= 0x40,		///< place popup vertically at mouse position
		kVCenterRel = 0x80,     ///< center popup vertically relative to "where.y"
		kVMask		= 0xF0,

		kHFillWindow		= 0x0100,	///< enlarge up to parent window width, respecting size limits and an also specified anchor edge
		kVFillWindow		= 0x0200,	///< enlarge up to parent window height, respecting size limits and an also specified anchor edge; e\.g\. "bottom vfillwindow": from parent bottom to window bottom

		kHasOffset			= 0x0400,	///< an additional offset to the position determined by the other flags is specified in "where"
		kCanFlipParentEdge	= 0x0800,	///< if there is not enough space in the given direction, the position will be mirrored on parent center before trying the other direction
		kForceFixedPosition	= 0x1000	///< popup keeps its initial position and is not moved inside the screen rect
	};

	PROPERTY_FLAG (flags, kCanFlipParentEdge, canFlipParentEdge)
	PROPERTY_FLAG (flags, kForceFixedPosition, forceFixedPosition)

	Point where;		///< position relative to parent
	Rect anchorRect;	///< in parent coords; a platform implementation might use this for a visual indication
	IView* parent;
	SizeLimit sizeLimits;
	int flags;
};

//************************************************************************************************
// IPopupSelector
/**
	\ingroup gui_dialog */
//************************************************************************************************

interface IPopupSelector: IUnknown
{
	/** Behavior flags. */
	enum BehaviorFlags
	{
		kCloseAfterDrag			= 1<<0,	///< close the popup after a drag operation
		kRestoreMousePos		= 1<<1,	///< move the mouse cursor to it's old position after popup
		kWantsMouseUpOutside	= 1<<2,	///< onMouseUp is called even if mouse is outside the popup view
		kHideHScroll		    = 1<<3,	///< hide horizontal scrollbar
		
		kStayOpenOnClick        = 1<<4,	///< do not close when clicked
		kMenuSelectCheckedItem  = 1<<5,	///< select checked items - used together with popup (IMenu* menu, ..., MenuPresentation::kTree)
		kAcceptsOnClickOutside  = 1<<6,	///< return kOkay if a click outside caused the popup to close
		kAcceptsAfterSwipe		= 1<<7	///< return kOkay after a swipe gesture in the popup
	};

	/** Set theme. */
	virtual void CCL_API setTheme (ITheme* theme) = 0;

	/** Set visual style (optional). */
	virtual void CCL_API setVisualStyle (IVisualStyle* visualStyle) = 0;

	/** Set popup behavior flags. */
	virtual void CCL_API setBehavior (int32 behavior) = 0;

	/** Assign a controller that creates an optional skin view that embeds the actual popup content. */
	virtual void CCL_API setDecor (StringID decorName, IUnknown* decorController) = 0;

	/** Popup any view (takes ownership of view). */
	virtual tbool CCL_API popup (IView* view, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) = 0;

	/** Popup a view created by IPopupSelectorClient. */
	virtual tbool CCL_API popup (IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) = 0;

	/** Popup a menu. */
	virtual tbool CCL_API popup (IMenu* menu, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) = 0;

	/** Popup a menu or palette for the given parameter. */
	virtual tbool CCL_API popup (IParameter* parameter, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) = 0;

	/** Asynchronously popup a view created by IPopupSelectorClient. */
	virtual IAsyncOperation* CCL_API popupAsync (IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) = 0;

	/** Asynchronously popup any view (takes ownership of view). */
	virtual IAsyncOperation* CCL_API popupAsync (IView* view, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo) = 0;

	/** Asynchronously popup a menu. */
	virtual IAsyncOperation* CCL_API popupAsync (IMenu* menu, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) = 0;

	/** Asynchronously popup a menu or palette for the given parameter. */
	virtual IAsyncOperation* CCL_API popupAsync (IParameter* parameter, const PopupSizeInfo& sizeInfo, StringID menuType = nullptr) = 0;

	/** Popup a slider for the given parameter. */
	virtual tbool CCL_API popupSlider (IParameter* parameter, const PopupSizeInfo& sizeInfo, tbool horizontal = false) = 0;

	/** Check if the popup is still open. */
	virtual tbool CCL_API isOpen () = 0;

	/** Close current popup window. */
	virtual void CCL_API close () = 0;

	DECLARE_IID (IPopupSelector)
};

DEFINE_IID (IPopupSelector, 0x5E35098C, 0x1335, 0x4C17, 0x8C, 0x1A, 0x5C, 0x8B, 0x50, 0x77, 0xAD, 0xA2)

//************************************************************************************************
// IPopupSelectorClient
/**
	\ingroup gui_dialog */
//************************************************************************************************

interface IPopupSelectorClient: IUnknown
{
	/** Result codes for onMouseDown / onKeyDown. */ 
	DEFINE_ENUM (Result)
	{
		kOkay   = 1,	///< close the popup with code kOkay
		kCancel = 0,	///< close the popup with code kCancel
		kIgnore = -1,	///< don't close popup, process the event
		kSwallow = -2	///< don't close popup, don't continue processing the event
	};

	/** Create the view that should popup. Obey the given limits. */
	virtual IView* CCL_API createPopupView (SizeLimit& limits) = 0;

	/** Called when the window gets attached on the screen. */
	virtual void CCL_API attached (IWindow& popupWindow) = 0;

	/** Called before the event is processed. Return kOkay or kCancel to close popup. Point is in window coords. */
	virtual Result CCL_API onMouseDown (const MouseEvent& event, IWindow& popupWindow) = 0;

	/** Called before the event is processed. Return kOkay or kCancel to close popup. Point is in window coords. */
	virtual Result CCL_API onMouseUp (const MouseEvent& event, IWindow& popupWindow) = 0;

	/** Called before the event is processed. Return kOkay or kCancel to close popup. */
	virtual Result CCL_API onKeyDown (const KeyEvent& event) = 0;

	/** Called before the event is processed. Return kOkay or kCancel to close popup. */
	virtual Result CCL_API onKeyUp (const KeyEvent& event) = 0;

	/** Called after a mouseDown or keyDown has been processed. */
	virtual Result CCL_API onEventProcessed (const GUIEvent& event, IWindow& popupWindow, IView* view) = 0;

	/** Called when popup has been closed. Passes the result from onMouseDown / onKeyDown. */
	virtual void CCL_API onPopupClosed (Result result) = 0;

	/** Get flags defining the behavior of the popup window. */
	virtual int32 CCL_API getPopupBehavior () = 0;

	/** Called on mousewheel over a source view that invokes a popup selector. */
	virtual tbool CCL_API mouseWheelOnSource (const MouseWheelEvent& event, IView* source) = 0;

	/** Called when touches begin on a source view. Event coordinates are translated to the popup window. */
	virtual ITouchHandler* CCL_API createTouchHandler (const TouchEvent& event, IWindow* window) = 0;

	/** Sets the position where the client should be anchored (screen coordinates), typically the mouse cursor or a touch position **/
	virtual void CCL_API setCursorPosition (PointRef where) = 0;
	
	/** Set to default. */
	virtual tbool CCL_API setToDefault () = 0;

	DECLARE_IID (IPopupSelectorClient)
};

DEFINE_IID (IPopupSelectorClient, 0x9B608C73, 0xA913, 0x4599, 0xB5, 0x6F, 0xEA, 0xCB, 0xB4, 0xD2, 0xA3, 0xED)

//************************************************************************************************
// MenuPopupSelectorBehavior
//************************************************************************************************

namespace MenuPopupSelectorBehavior
{
	DEFINE_STRINGID (kMustCloseMenuOnSelect, "mustCloseMenuOnSelect") ///< menu must be closed after select
	DEFINE_STRINGID (kCondensedMenuSeparators, "condensedMenuSeparators") ///< use condensed menu separators
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline PopupSizeInfo::PopupSizeInfo (IView* parent, int flags, PointRef offset)
: where (offset), parent (parent), flags (flags)
{ sizeLimits.setUnlimited (); anchorRect.setReallyEmpty (); if(!offset.isNull ()) this->flags |= kHasOffset; }

inline PopupSizeInfo::PopupSizeInfo (PointRef where, IView* parent)
: where (where), parent (parent), flags (0)
{ sizeLimits.setUnlimited (); anchorRect.setReallyEmpty (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_ipopupselector_h
