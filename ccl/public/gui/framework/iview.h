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
// Filename    : ccl/public/gui/framework/iview.h
// Description : Framework View Interface
//
//************************************************************************************************

#ifndef _ccl_iview_h
#define _ccl_iview_h

#include "ccl/public/gui/framework/styleflags.h"

#include "ccl/public/gui/graphics/updatergn.h"

namespace CCL {

struct GUIEvent;
struct MouseEvent;
struct KeyEvent;
interface IView;
interface IWindow;
interface IMouseCursor;
interface IViewChildren;
interface IViewIterator;
interface IParameter;
interface IVisualStyle;
interface IGraphicsLayer;
interface IAttributeList;
template <class T> class ObservedPtr;

/** Observed view pointer class. */
typedef ObservedPtr<IView> ViewPtr;

/** Update messages sent by some views on certain events. */
DEFINE_STRINGID (kSizeChanged, "sizeChanged")
DEFINE_STRINGID (kOnAttached, "onAttached")
DEFINE_STRINGID (kOnRemoved, "onRemoved")

//************************************************************************************************
// IView
/** View interface
	\ingroup gui_view */
//************************************************************************************************

interface IView: IUnknown
{
	/** View attributes. */
	enum ViewAttributes
	{
		kName,					///< [String]
		kTitle,					///< [String]
		kTooltip,				///< [String]
		kStyleFlags,			///< [int64]
		kTheme,					///< [ITheme]
		kVisualStyle,			///< [IVisualStyle]
		kController,			///< [IUnknown]
		kSizeMode,				///< [int]
		kSizeModeDisabled,		///< [boolean]
		kInputEnabled,			///< [boolean]
		kMouseState,			///< [int]
		kThemeElementState,		///< [int]
		kFocusEnabled,			///< [boolean]
		kTooltipTrackingEnabled,///< [boolean]
		kLayerBackingEnabled,	///< [boolean]
		kGraphicsLayer,			///< [IGraphicsLayer]
		kAccessibilityEnabled	///< [boolean]
	};

	/** Size modes. */
	enum SizeModes
	{
		kAttachLeft		= 1<<0, ///< the view's left edge keeps it's distance to the parent's left edge
		kAttachTop		= 1<<1, ///< the view's top edge keeps it's distance to the parent's top edge
		kAttachRight	= 1<<2, ///< the view's right edge keeps it's distance to the parent's top edge
		kAttachBottom	= 1<<3, ///< the view's bottom edge keeps it's distance to the left parent's bottom edge
		kAttachAll		= kAttachLeft|kAttachTop|kAttachRight|kAttachBottom, ///< all of the view's edges keep their distances to the corresponding parent edge

		kHCenter		= 1<<4,	///< the view is centered horizontally in its parent
		kVCenter		= 1<<5,	///< the view is centered vertically in its parent

		kHFitSize		= 1<<6,	///< when childs are sized, the view adjusts it's width to fit the childs
		kVFitSize		= 1<<7,	///< when childs are sized, the view adjusts it's height to fit the childs
		kFitSize		= kHFitSize|kVFitSize, ///< when childs are sized, the view adjusts it's size to fit the childs

		kPreferCurrentSize	= 1<<8,	///< this view accepts any size given by parent, and keeps that on autoSize
		kFill				= 1<<9,	///< in a layout view, this view is preferred when additonal space is distributed among siblings

		kLastSizeMode = 9
	};

	/** Mouse states. */
	enum MouseStates
	{
		kMouseNone = 0,
		kMouseDown = 1<<0,
		kMouseOver = 1<<1
	};

	/** View attribute id. */
	typedef int AttrID;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set view attribute. */
	virtual tbool CCL_API getViewAttribute (Variant& value, AttrID id) const = 0;

	/** Get view attribute. */
	virtual tbool CCL_API setViewAttribute (AttrID id, VariantRef value) = 0;

	/** Get the visual style. */
	virtual const IVisualStyle& CCL_API getVisualStyle () const = 0;

	/** Get the style flags. */
	virtual StyleRef CCL_API getStyle () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Size
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get size in parent coordinates. */
	virtual RectRef CCL_API getSize () const = 0;

	/** Set size in parent coordinates. */
	virtual void CCL_API setSize (RectRef size, tbool invalidate = true) = 0;

	/** Get size of visible client rectangle. */
	virtual tbool CCL_API getVisibleClient (Rect& r) const = 0;

	/** Set size limits. */
	virtual void CCL_API setSizeLimits (const SizeLimit& sizeLimits) = 0;

	/** Get size limits. */
	virtual const SizeLimit& CCL_API getSizeLimits () = 0;

	/** Check if size limits were set explicitly from outside (e.g. from skin). */
	virtual tbool CCL_API hasExplicitSizeLimits () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Coordinates
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Convert client to window coordinates. */
	virtual Point& CCL_API clientToWindow (Point& p) const = 0;
	
	/** Convert window to client coordinates. */
	virtual Point& CCL_API windowToClient (Point& p) const = 0;
	
	/** Convert client to screen coordinates. */
	virtual Point& CCL_API clientToScreen (Point& p) const = 0;

	/** Convert screen to client coordinates. */
	virtual Point& CCL_API screenToClient (Point& p) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Nesting
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get parent view. */
	virtual IView* CCL_API getParentView () const = 0;

	/** Get parent view of given class. */
	virtual IView* CCL_API getParentByClass (UIDRef cid) const = 0;

	/** Get window. */
	virtual IWindow* CCL_API getIWindow () const = 0;

	/** Get view children. */
	virtual IViewChildren& CCL_API getChildren () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Adjust size to content. */
	virtual void CCL_API autoSize (tbool horizontal = true, tbool vertical = true) = 0;

	/** Redraw invalidated area. */
	virtual void CCL_API redraw () = 0;

	/** Invalidate part of client area. */
	virtual void CCL_API invalidate (RectRef rect) = 0;

	/** Update part of client area (invalidate + redraw or direct update). */
	virtual void CCL_API updateClient (RectRef rect) = 0;

	/** Scroll pixel data in client area. */
	virtual void CCL_API scrollClient (RectRef rect, PointRef delta) = 0;

	/** Try to make part of client view visible (e.g. by scrolling); relaxed: don't scroll if the rect is already partly visible. */
	virtual tbool CCL_API makeVisible (RectRef rect, tbool relaxed = false) = 0;

	/** Request keyboard focus. */
	virtual tbool CCL_API takeFocus (tbool directed = true) = 0;

	/** Remove keyboard focus (if view currently owns it). */
	virtual tbool CCL_API killFocus () = 0;	

	/** Set mouse cursor (reset when mouse leaves client area). */
	virtual void CCL_API setCursor (IMouseCursor* cursor) = 0;

	/** Detect drag at given location. */
	virtual tbool CCL_API detectDrag (const MouseEvent& event) = 0;
	
	/** Detect double click at given location. */
	virtual tbool CCL_API detectDoubleClick (const MouseEvent& event) = 0;
	
	/** Get associated controller. */
	virtual IUnknown* CCL_API getController () const = 0;

	/** Get parent layer, create a root layer if necessary. */
	virtual IGraphicsLayer* CCL_API getParentLayer (Point& offset) const = 0;

	/** Set zoom factor: factor between original and current "zoomed" size. */
	virtual void CCL_API setZoomFactor (float factor) = 0;

	/** Get zoom factor. */
	virtual float CCL_API getZoomFactor () const = 0;

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kHelpId)

	DECLARE_IID (IView)
};

DEFINE_IID (IView, 0xf5b09f71, 0x6f3f, 0x4f03, 0xa5, 0x4a, 0xb2, 0x49, 0xb8, 0x29, 0x62, 0xd6)
DEFINE_STRINGID_MEMBER (IView, kHelpId, "helpid")

//************************************************************************************************
// IViewChildren
/** View children interface. 
	\ingroup gui_view */
//************************************************************************************************

interface IViewChildren: IUnknown
{
	/** Check if child views present. */
	virtual tbool CCL_API isEmpty () const = 0;
	
	/** Remove (and release) all child views. */
	virtual void CCL_API removeAll () = 0;

	/** Add child view, parent view takes ownership. */
	virtual tbool CCL_API add (IView* view) = 0;

	/** Insert  child view at given index, parent view takes ownership. */
	virtual tbool CCL_API insert (int index, IView* view) = 0;
	
	/** Remove child view, ownership is transferred to caller. */
	virtual tbool CCL_API remove (IView* view) = 0;
	
	/** Move child view to a new position in the list. Before can be null, which means it is appended. */
	virtual tbool CCL_API moveBefore (IView* view, IView* before) = 0;

	/** Get first child view. */
	virtual IView* CCL_API getFirstView () const = 0;

	/** Get last child view. */
	virtual IView* CCL_API getLastView () const = 0;

	/** Create child view iterator. */
	virtual IViewIterator* CCL_API createIterator () const = 0;

	/** Check if given view is child of this view. */
	virtual tbool CCL_API isChildView (IView* view, tbool deep = false) const = 0;

	/** Find child view at position. */
	virtual IView* CCL_API findChildView (PointRef where, tbool deep = false) const = 0;

	/** Delegate event to child views. */
	virtual tbool CCL_API delegateEvent (const GUIEvent& event) = 0;

	DECLARE_IID (IViewChildren)
};

DEFINE_IID (IViewChildren, 0x1a944c1f, 0xd8ac, 0x4ba1, 0x93, 0x9, 0x5b, 0xc4, 0x12, 0xa0, 0xf0, 0x60)

//************************************************************************************************
// ILayoutView
//************************************************************************************************

interface ILayoutView: IUnknown
{
	/** Get attributes of the layout currently in use */
	virtual tbool CCL_API getLayoutAttributes (IAttributeList& attributes) const = 0;

	/** Get attributes of layout items associated with a child view */
	virtual tbool CCL_API getChildLayoutAttributes (IAttributeList& attributes, IView* view) const = 0;
	
	DECLARE_IID (ILayoutView)
};

DEFINE_IID (ILayoutView, 0x0B48B11B, 0xF00C, 0xE645, 0x8A, 0xF7, 0x92, 0xA8, 0x1A, 0x81, 0xC6, 0x60)

//************************************************************************************************
// IControl
/** Control interface. 
	\ingroup gui_view */
//************************************************************************************************

interface IControl: IUnknown
{
	/** Get associated parameter object. */
	virtual IParameter* CCL_API getParameter () const = 0;

	/** Assign parameter object to control. */
	virtual void CCL_API setParameter (IParameter* param) = 0;

	DECLARE_IID (IControl)
};

DEFINE_IID (IControl, 0x8186d4bb, 0x76b6, 0x46f6, 0x88, 0x9f, 0x9a, 0xd0, 0xef, 0x50, 0x19, 0x81)

//************************************************************************************************
// IButton
/** Button control interface. 
	\ingroup gui_view */
//************************************************************************************************

interface IButton: IUnknown
{
	/** Trigger button push programmatically.
		Behavior depends on button type (button, toggle, radio button, etc.). */
	virtual void CCL_API push () = 0;

	/** Signaled by buttons with Styles::kTrigger when button was pushed. */
	DECLARE_STRINGID_MEMBER (kOnPush)

	DECLARE_IID (IButton)
};

DEFINE_IID (IButton, 0x79ac4932, 0xb3eb, 0x4fa1, 0x8e, 0x68, 0x76, 0x67, 0x1e, 0x6c, 0xa6, 0x34)
DEFINE_STRINGID_MEMBER (IButton, kOnPush, "onPush")

//************************************************************************************************
// IEditControlHost
/** Implemented by a view that can embed a view as temporary edit control.
	\ingroup gui_view  */
//************************************************************************************************

interface IEditControlHost: IUnknown
{
	/** Edit control delegates key event for navigation. */
	virtual tbool CCL_API onEditNavigation (const KeyEvent& event, IView* control) = 0;

	/** Edit control has lost focus. */
	virtual void CCL_API onEditControlLostFocus (IView* control) = 0;

	DECLARE_IID (IEditControlHost)
};

DEFINE_IID (IEditControlHost, 0xF2EFC091, 0x9B48, 0x4B3C, 0xBD, 0x83, 0x06, 0x35, 0xA5, 0xA4, 0xF1, 0x46)

//************************************************************************************************
// IViewIterator
/** View children iteration interface. 
	\ingroup gui_view */
//************************************************************************************************

interface IViewIterator: IUnknown
{
	/** Check if iteration finished. */
	virtual tbool CCL_API done () const = 0;

	/** Get next view. */
	virtual IView* CCL_API next () = 0;

	/** Get previous view. */
	virtual IView* CCL_API previous () = 0;

	/** Move to last. */
	virtual void CCL_API last () = 0;

	DECLARE_IID (IViewIterator)
};

DEFINE_IID (IViewIterator, 0xb89b0be9, 0x89c7, 0x4cc3, 0x86, 0x43, 0x14, 0xa9, 0x88, 0x32, 0xee, 0x53)

} // namespace CCL

#include "ccl/public/gui/framework/controlclasses.h" // include control classes
#include "ccl/public/gui/framework/controlstyles.h"  // include control styles

#endif // _ccl_iview_h
