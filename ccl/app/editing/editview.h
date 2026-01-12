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
// Filename    : ccl/app/editing/editview.h
// Description : Editing View
//
//************************************************************************************************

#ifndef _ccl_editview_h
#define _ccl_editview_h

#include "ccl/app/controls/usercontrol.h"

#include "ccl/app/editing/selection.h"

namespace CCL {

class EditTool;
class EditModel;
class EditLayer;
class EditorComponent;
class AbstractEditCursor;
class UserTooltipPopup;

//************************************************************************************************
// EditView
//************************************************************************************************

class EditView: public UserControl,
				public ISelectionViewer
{
public:
	DECLARE_CLASS (EditView, UserControl)
	DECLARE_METHOD_NAMES (EditView)

	EditView (EditorComponent* component = nullptr, RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~EditView ();

	/** Get model (owned by component). */
	EditModel& getModel () const;

	/** Get selection (owned by model). */
	Selection& getSelection () const;

	/** Get tool assigned to this edit view. */
	EditTool* getTool () const;

	/** Set new tool (shared by edit view)*/
	void setTool (EditTool* tool);

	/** Try to make the given item visible (e.g. by scrolling). */
	virtual void makeItemVisible (Object* item, tbool relaxed = false);
	void deferMakeItemVisible (Object* item);

	/** Check if selection is visible. */
	bool isSelectionVisible () const;

	/** Check if mouse cursor is inside view. */
	bool isMouseInside () const;

	/** Edit layers. */
	void addEditLayer (EditLayer* layer);
	const ObjectList& getEditLayers ();
	bool usesLayer (const EditLayer* layer);
	EditLayer* getEditLayer (MetaClassRef type) const;
	template<class C> C* getEditLayer () const;

	/** EditView wants tools and editor activation. */
	PROPERTY_FLAG (editViewFlags, kToolActivation, wantsToolActivation)

	/** State maintained by edit handler. */
	PROPERTY_FLAG (editViewFlags, kEditHandlerActive, editHandlerActive)

	/** EditView wants crosshair cursor. */
	PROPERTY_FLAG (editViewFlags, kCrossCursor, wantsCrossCursor)

	/** Set this as active EditView when attached (enabled by default). */
	PROPERTY_FLAG (editViewFlags, kActivateOnAttach, activateOnAttach)

	/** Check if crosshair cursor is present. */
	bool hasCrossCursor () const;
	
	/** Check if crosshair cursor is visible. */
	bool isCrossCursorVisible () const;

	/** Show/hide crosshair cursor (if present). */
	void showCrossCursor (bool state);

	/** Move crosshair cursor (if present). */
	virtual void moveCrossCursor (const Point& position);

	/** Show text in a tooltip popup. The popup is moved to the mouse if no position is given. */
	void setEditTooltip (StringRef text, const Point* position = nullptr);

	/** Hide the tooltip popup. */
	void hideEditTooltip ();

	void updateToolCursor (const MouseEvent& event);

	bool inContextMenuScope () const;

	int getCurrentInputDevice () const;
	class InputDeviceScope;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// UserControl overrides
	//////////////////////////////////////////////////////////////////////////////////////////////

	IUnknown* CCL_API getController () const override;
	void attached (IView* parent) override;
	void removed (IView* parent) override;
	void onActivate (bool state) override;
	bool onFocus (const FocusEvent& event) override;
	void onSize (const Point& delta) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	IMouseHandler* CCL_API createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* CCL_API createTouchHandler (const TouchEvent& event) override;
	IDragHandler* CCL_API createDragHandler (const DragEvent& event) override;
	bool onTrackTooltip (const TooltipEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Static members
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Global crosshair cursor configuration changed. */
	static void applyCrossCursorEnabled ();

	/** Find EditView under mouse (does not have to be the active edit view). Optionally returns mousePos relative to found view. */
	static EditView* findUnderMouse (Point* relativeMousePos = nullptr);

protected:
	enum EditViewFlags
	{
		kMouseInside       = 1<<0,	///< mouse is currently inside
		kSelectionVisible  = 1<<1,	///< selection is visible
		kSelectionActive   = 1<<2,	///< selection is active
		kToolActivation    = 1<<3,	///< tool and editor activation enabled
		kEditHandlerActive = 1<<4,	///< an edit handler is currently active
		kCrossCursor       = 1<<5,	///< cross cursor enabled
		kActivateOnAttach  = 1<<6,	///< set as active EditView when attached 
		kContextMenuScope  = 1<<7,	///< set while in method onContextMenu
		kLastEditViewFlag  = kContextMenuScope
	};

	PROPERTY_FLAG (editViewFlags, kMouseInside, mouseInside)
	PROPERTY_FLAG (editViewFlags, kSelectionVisible, selectionVisible)
	PROPERTY_FLAG (editViewFlags, kSelectionActive, selectionActive)
	PROPERTY_FLAG (editViewFlags, kContextMenuScope, contextMenuScope)

	EditorComponent* component;
	EditTool* tool;
	AbstractEditCursor* crossCursor;
	UserTooltipPopup* editTooltip;
	ObjectList editLayers;
	int editViewFlags;
	int currentInputDevice;

	virtual AbstractEditCursor* createCrossCursor ();
	virtual bool getSelectionUpdateSize (Rect& rect);
	void updateSelection (bool redraw = true);
	UserTooltipPopup& getEditTooltip ();
	void updateToolHelp (const MouseEvent& event);

	virtual CCL::String getItemType (Object* item);
	
	// ISelectionViewer
	void showSelection (bool redraw = true) override;
	void hideSelection (bool redraw = true) override;
	void makeSelectedItemsVisible (bool relaxed) override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	void enableCrossCursor (bool state);

private:
	MouseEvent makeAsyncMouseEvent (int eventType);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool EditView::hasCrossCursor () const
{ return crossCursor != nullptr; }

inline bool EditView::inContextMenuScope () const 
{ return contextMenuScope (); }

inline int EditView::getCurrentInputDevice () const
{ return currentInputDevice; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class C>
C* EditView::getEditLayer () const
{ return ccl_cast<C> (getEditLayer (ccl_typeid<C> ())); }

//////////////////////////////////////////////////////////////////////////////////////////////////

class EditView::InputDeviceScope: ScopedVar<int>
{
public:
	InputDeviceScope (EditView& editView, int device) : ScopedVar<int> (editView.currentInputDevice, device) {}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_editview_h
