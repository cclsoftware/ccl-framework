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
// Filename    : ccl/app/editing/editview.cpp
// Description : Editing View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editor.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editlayer.h"
#include "ccl/app/editing/editcursor.h"
#include "ccl/app/editing/edithandler.h"
#include "ccl/app/editing/selectaction.h"
#include "ccl/app/editing/tools/edittool.h"
#include "ccl/app/editing/tools/toolcollection.h"
#include "ccl/app/editing/tools/toolbar.h"
#include "ccl/app/editing/addins/editenvironment.h"

#include "ccl/app/utilities/boxedguitypes.h"

#include "ccl/base/storage/configuration.h"
#include "ccl/base/message.h"

#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/itooltip.h"
#include "ccl/public/gui/framework/usertooltip.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// EditView static members
//************************************************************************************************

static ObjectList crossCursorViews;
static CCL::Configuration::BoolValue crossCursorEnabled ("Editing", "crossCursorEnabled", true);

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::applyCrossCursorEnabled ()
{
	// apply to existing views
	ForEach (crossCursorViews, EditView, editView)
		editView->enableCrossCursor (crossCursorEnabled);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView* EditView::findUnderMouse (Point* relativeMousePos)
{
	struct ViewFinder
	{
		static EditView* findEditView (IView* parent, const Point& where)
		{
			ForEachChildViewReverse (parent, v)
				Point where2 (where);
				where2.offset (-v->getSize ().left, -v->getSize ().top);

				Rect client;
				if(v->getVisibleClient (client) && client.pointInside (where2))
				{
					if(EditView* result = findEditView (v, where2))
						return result;

					if(EditView* editView = cast_IView<EditView> (v))
						return editView;
				}
			EndFor
			return nullptr;
		}
	};

	// find window under mouse
	Point mousePos;
	System::GetGUI ().getMousePosition (mousePos);
	if(UnknownPtr<IView> window = System::GetDesktop ().findWindow (mousePos))
	{
		// find EditView under mouse
		window->screenToClient (mousePos);
		EditView* editView = ViewFinder::findEditView (window, mousePos);
		if(editView && relativeMousePos)
		{
			editView->windowToClient (mousePos);
			*relativeMousePos = mousePos;
		}
		return editView;
	}
	return nullptr;
}

//************************************************************************************************
// EditView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView::EditView (EditorComponent* component, RectRef size, StyleRef style, StringRef title)
: UserControl (size, style, title),
  component (component),
  tool (nullptr),
  crossCursor (nullptr),
  editViewFlags (kActivateOnAttach),
  currentInputDevice (PointerEvent::kPointerInput),
  editTooltip (nullptr)
{
	wantsFocus (true);
	selectionVisible (true);

	ASSERT (component != nullptr)
	if(component)
		component->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView::~EditView ()
{
	signal (Message (kDestroyed));

	if(tool)
		tool->release ();

	if(component)
		component->release ();

	if(crossCursor)
		crossCursor->release ();

	delete editTooltip;

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel& EditView::getModel () const
{
	ASSERT (component != nullptr)
	return component->getModel ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Selection& EditView::getSelection () const
{
	return getModel ().getSelection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::makeItemVisible (Object* item, tbool relaxed)
{
	Rect rect;
	if(getModel ().getItemSize (rect, *this, item))
		makeVisible (rect, relaxed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::deferMakeItemVisible (Object* item)
{
	if(item)
		(NEW Message ("makeItemVisible", item->asUnknown ()))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "makeItemVisible")
		makeItemVisible (unknown_cast<Object> (msg[0]), true);
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::enableCrossCursor (bool state)
{
	if(state)
	{
		if(crossCursor == nullptr)
		{
			crossCursor = createCrossCursor ();
			crossCursor->setVisible (mouseInside () && tool && tool->wantsCrossCursor ());

			if(isAttached ())
				crossCursor->attached ();
		}
	}
	else
	{
		if(crossCursor)
		{
			if(isAttached ())
				crossCursor->removed ();

			crossCursor->release ();
			crossCursor = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AbstractEditCursor* EditView::createCrossCursor ()
{
	return NEW CrossHairCursor (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::showCrossCursor (bool state)
{
	if(crossCursor)
		crossCursor->setVisible (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::isCrossCursorVisible () const
{
	return crossCursor && crossCursor->isVisible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::moveCrossCursor (const Point& position)
{
	if(crossCursor)
		crossCursor->move (position);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserTooltipPopup& EditView::getEditTooltip ()
{
	if(!editTooltip)
		editTooltip = NEW UserTooltipPopup (*this);
	return *editTooltip;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::setEditTooltip (StringRef text, const Point* position)
{
	getEditTooltip ().setTooltip (text, position);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::hideEditTooltip ()
{
	getEditTooltip ().hideTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::updateToolHelp (const MouseEvent& event)
{
	if(System::GetGUI ().isDragActive ())
		return;
	if(!System::GetHelpManager ().hasInfoViewers ())
		return;

	AutoPtr<IPresentable> info;
	if(event.eventType != MouseEvent::kMouseLeave)
	{
		ASSERT (tool != nullptr)
		info = tool->createHelpInfo (*this, event);
	}
	System::GetHelpManager ().showInfo (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::updateToolCursor (const MouseEvent& event)
{
	if(tool)
	{
		tool->mouseMove (*this, event);
		setCursor (tool->getMouseCursor ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseEvent EditView::makeAsyncMouseEvent (int eventType)
{
	Point p;
	System::GetGUI ().getMousePosition (p);
	screenToClient (p);

	KeyState keys;
	System::GetGUI ().getKeyState (keys);

	return MouseEvent (eventType, p, keys);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* EditView::getTool () const
{
	return tool;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::setTool (EditTool* _tool)
{
	bool wasMouseInside = mouseInside ();
	if(wasMouseInside)
		onMouseLeave (makeAsyncMouseEvent (MouseEvent::kMouseLeave));

	if(_tool)
		_tool = _tool->getActiveModeHandler ();

	if(tool && tool != _tool)
		tool->onAttached (*this, false);

	take_shared<EditTool> (tool, _tool);

	if(tool)
		tool->onAttached (*this, true);

	if(wasMouseInside)
		onMouseEnter (makeAsyncMouseEvent (MouseEvent::kMouseEnter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::addEditLayer (EditLayer* layer)
{
	editLayers.add (layer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectList& EditView::getEditLayers ()
{
	return editLayers;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::usesLayer (const EditLayer* layer)
{
	return editLayers.contains (layer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditLayer* EditView::getEditLayer (MetaClassRef type) const
{
	for(auto editLayer : iterate_as<EditLayer> (editLayers))
		if(editLayer->canCast (type))
			return editLayer;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::isSelectionVisible () const
{
	return selectionVisible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::isMouseInside () const
{
	return mouseInside ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::getSelectionUpdateSize (Rect& rect)
{
	return getModel ().getSelectionSize (rect, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::updateSelection (bool withRedraw)
{
	Rect rect;
	if(getSelectionUpdateSize (rect))
	{
		invalidate (rect);

		if(withRedraw)
			redraw ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::hideSelection (bool redraw)
{
	selectionVisible (false);

	updateSelection (redraw);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::showSelection (bool redraw)
{
	selectionVisible (true);

	updateSelection (redraw);

	if(mouseInside () && !editHandlerActive ())
		updateToolCursor (makeAsyncMouseEvent (MouseEvent::kMouseMove)); // new scenario e.g. after key command, undo, ...
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::makeSelectedItemsVisible (bool relaxed)
{
	Rect rect;
	if(getModel ().getSelectionSize (rect, *this))
		makeVisible (rect, relaxed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API EditView::getController () const
{
	return ccl_as_unknown (component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::attached (IView* parent)
{
	SuperClass::attached (parent);

	ASSERT (component != nullptr)
	if(component)
	{
		component->addEditView (this);
		getModel ().onViewAttached (this);

		if(wantsToolActivation ())
		{
			EditTool* tool = component->getActiveTool ();
			if(!tool)
				tool = component->getDefaultTool ();
			setTool (tool);
			if(activateOnAttach () || component->getActiveEditView () == nullptr)
			{
				CCL_PRINTF ("%s activated on attach\n", myClass ().getPersistentName ())
				component->setActiveEditView (this, true);
			}
		}
	}

	if(wantsCrossCursor ())
	{
		crossCursorViews.add (this);
		enableCrossCursor (crossCursorEnabled);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::removed (IView* parent)
{
	if(wantsCrossCursor ())
	{
		enableCrossCursor (false);
		crossCursorViews.remove (this);
	}

	if(tool && mouseInside ())
		tool->mouseLeave (*this, MouseEvent (MouseEvent::kMouseLeave));

	hideEditTooltip ();

	ASSERT (component != nullptr)
	if(component)
		component->removeEditView (this);

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::onActivate (bool state)
{
	if(!component->hasEditView (this)) // attached () might not have been called yet (but isAttached () would not work here)
		return;

	if(state != selectionActive ())
	{
		selectionActive (state);
		CCL_PRINTF ("%s '%s' %s\n", myClass ().getPersistentName (), MutableCString (getName ()).str (), state ? "activated" : "deactivated")

		updateSelection (false);
	}

	if(state)
	{
		ASSERT (component != nullptr)
		if(component)
		{
			EditView* toActivate = this;

			if(EditView* activeView = component->getActiveEditView ())
			{
				// don't steal "ActiveEditView" state from a fellow view in the same WindowBase (all get activated in order of view tree)
				IWindowBase* windowBase1 = GetViewInterfaceUpwards<IWindowBase> (*activeView);
				IWindowBase* windowBase2 = GetViewInterfaceUpwards<IWindowBase> (*this);
				if(windowBase1 && windowBase1 == windowBase2)
					toActivate = activeView;
			}

			component->setActiveEditView (toActivate);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::onFocus (const FocusEvent& event)
{
	if(event.eventType == FocusEvent::kSetFocus)
	{
		if(wantsToolActivation ())
		{
			CCL_PRINTF ("%s activated on focus\n", myClass ().getPersistentName ())
			ASSERT (component != nullptr)
			if(component)
				component->setActiveEditView (this);
		}
	}
	return SuperClass::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditView::onSize (const Point& delta)
{
	SuperClass::onSize (delta);

	if(crossCursor)
		crossCursor->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::onContextMenu (const ContextMenuEvent& event)
{
	Selection& selection = getSelection ();
	
	ScopedFlag<kContextMenuScope> scope (editViewFlags);

	if(event.wasKeyPressed)
	{
		if(Object* firstSelected = selection.getFirst ())
		{
			event.contextMenu.setFocusItem (firstSelected->asUnknown ());

			// adjust menu postion
			Rect r;
			if(getModel ().getItemSize (r, *this, firstSelected))
			{
				Point pos  (r.getLeftTop ());
				pos.offset (ccl_min (2, r.getWidth () / 2), r.getHeight () / 2);
				ccl_lower_limit (pos.x, 0);
				event.setPosition (pos);
			}
		}
	}
	else
	{
		AutoPtr<Object> clickedItem = getModel ().findItem (*this, event.where);
		if(clickedItem)
		{
			event.contextMenu.setFocusItem (clickedItem->asUnknown ());

			// clicked item must be selected
			if(!selection.isSelected (clickedItem))
			{
				selection.hide (false);

				selection.unselectAll ();
				if(selection.canSelect (clickedItem) && getModel ().canSelectItem (clickedItem))
				{
					getModel ().selectItem (clickedItem);

					if(!selection.isSelected (clickedItem))
						event.contextMenu.setFocusItem (nullptr);
				}

				selection.show (true);
			}
			getModel ().setFocusItem (clickedItem, this);
		}
	}
	return false; // continue distributing the context menu event
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::onMouseDown (const MouseEvent& event)
{
	if(event.keys.isSet (KeyState::kMButton))
	{
		if(component)
			if(ToolBar* toolBar = component->getTools ().getToolBar ())
				toolBar->popup (event.where, *this);
		return true;
	}
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseHandler* CCL_API EditView::createMouseHandler (const MouseEvent& event)
{
	if(tool)
	{
		SharedPtr<IView> holder (*this);

		updateToolCursor (event);

		EditHandler* mouseHandler = tool->mouseDown (*this, event);
		if(mouseHandler)
			return mouseHandler;

		// swallow mouse click here!
		updateToolCursor (event); // new scenario after tool action was performed
		return NEW NullEditHandler (this);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* CCL_API EditView::createTouchHandler (const TouchEvent& event)
{
	EditView::InputDeviceScope scope (*this, event.inputDevice);

	if(tool)
		return tool->createTouchHandler (*this, event);

	return SuperClass::createTouchHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::onMouseEnter (const MouseEvent& event)
{
	if(!tool)
		return false;

	if(!editHandlerActive ())
	{
		mouseInside (true);
		tool->mouseEnter (*this, event);
		setCursor (tool->getMouseCursor ());

		if(crossCursor)
		{
			crossCursor->setVisible (tool->wantsCrossCursor ());
			moveCrossCursor (event.where);
		}

		updateToolHelp (event);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::onMouseMove (const MouseEvent& event)
{
	if(!tool)
		return false;

	if(!editHandlerActive ())
	{
		//CCL_PRINTF ("EditView::onMouseMove %d %d\n", event.where.x, event.where.y)

		updateToolCursor (event);

		if(crossCursor)
		{
			moveCrossCursor (event.where);
			crossCursor->setVisible (tool->wantsCrossCursor ());
		}

		updateToolHelp (event);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::onMouseLeave (const MouseEvent& event)
{
	if(!editHandlerActive ())
	{
		if(tool)
			tool->mouseLeave (*this, event);
		mouseInside (false);

		if(crossCursor)
			crossCursor->setVisible (false);

		updateToolHelp (event);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CCL_API EditView::createDragHandler (const DragEvent& event)
{
	return getModel ().createDragHandler (*this, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditView::onTrackTooltip (const TooltipEvent& event)
{
	if(event.eventType == TooltipEvent::kShow || event.eventType == TooltipEvent::kMove)
	{
		String text;
		if(tool)
			text = tool->getTooltip ();

		if(text.isEmpty ())
		{
			AutoPtr<Object> item (getModel ().findItemDeep (*this, event.where));
			if(item)
				getModel ().getItemTooltip (text, *this, item);
		}

		if(!text.isEmpty ())
		{
			if(text != event.tooltip.getText ())
			{
				event.tooltip.setText (text);
				event.tooltip.moveToMouse ();
				event.tooltip.setDuration (ITooltipPopup::kDefaultDuration);
				event.tooltip.show ();
			}
			return true;
		}
		event.tooltip.hide ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditView::getItemType (Object* item)
{
	return getModel ().getItemType (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditView::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "selection")
	{
		var = getSelection ().asUnknown ();
		return true;
	}
	if(propertyId == "model")
	{
		var = getModel ().asUnknown ();
		return true;
	}
	if(propertyId == "environment")
	{
		ASSERT (component != nullptr)
		var = ccl_as_unknown (component->getEditEnvironment ());
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (EditView)
	DEFINE_METHOD_NAME ("findItem")
	DEFINE_METHOD_NAME ("findItemPart")
	DEFINE_METHOD_NAME ("findItemDeep")
	DEFINE_METHOD_NAME ("isSameItem")
	DEFINE_METHOD_NAME ("getItemType")
	DEFINE_METHOD_NAME ("getEditArea")
	DEFINE_METHOD_NAME ("getSelectionSize")
	DEFINE_METHOD_NAME ("setFocusItem")
	DEFINE_METHOD_NAME ("setAnchorItem")
	DEFINE_METHOD_NAME ("select")
	DEFINE_METHOD_NAME ("unselect")
	DEFINE_METHOD_NAME ("canSelect")
	DEFINE_METHOD_NAME ("dragSelection")
	DEFINE_METHOD_NAME ("drawSelection")
	DEFINE_METHOD_NAME ("dragEraser")
	DEFINE_METHOD_NAME ("deleteSelected")
	DEFINE_METHOD_NAME ("deleteItem")
	DEFINE_METHOD_NAME ("editItem")
	DEFINE_METHOD_NAME ("createEditHandler")
	DEFINE_METHOD_NAME ("getItemSize")
	DEFINE_METHOD_NAME ("detectDoubleClick")
	DEFINE_METHOD_NAME ("detectDrag")
	DEFINE_METHOD_NAME ("takeFocus")
	DEFINE_METHOD_NAME ("createSelectFunctions")
	DEFINE_METHOD_NAME ("showSelection")
	DEFINE_METHOD_NAME ("setCursor")
	DEFINE_METHOD_NAME ("moveCrossCursor")
END_METHOD_NAMES (EditView)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditView::invokeMethod (Variant& returnValue, MessageRef msg)
{
	Variant& result = returnValue;

	if(msg == "findItem")
	{
		Point* loc = unknown_cast<Boxed::Point> (msg[0].asUnknown ());
		ASSERT (loc)
		if(loc)
		{
			AutoPtr<Object> item = getModel ().findItem (*this, *loc);
			if(item)
				result.takeShared (item->asUnknown ());
		}
		return true;
	}

	if(msg == "findItemPart")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		Point* loc = unknown_cast<Boxed::Point> (msg[1].asUnknown ());
		ASSERT (obj && loc)
		if(obj && loc)
		{
			AutoPtr<Object> itemPart = getModel ().findItemPart (*this, obj, *loc);
			if(itemPart)
				result.takeShared (itemPart->asUnknown ());
		}
		return true;
	}

	if(msg == "findItemDeep")
	{
		Point* loc = unknown_cast<Boxed::Point> (msg[0].asUnknown ());
		ASSERT (loc)
		if(loc)
		{
			AutoPtr<Object> item = getModel ().findItemDeep (*this, *loc);
			if(item)
				result.takeShared (item->asUnknown ());
		}
		return true;
	}

	if(msg == "isSameItem")
	{
		const Object* item1 = unknown_cast<Object> (msg[0]);
		const Object* item2 = unknown_cast<Object> (msg[1]);
		ASSERT (item1 && item2)
		returnValue = item1 && item2 && item1->equals (*item2); // Object compares pointers, might be specialized in derived classes
		return true;
	}

	if(msg == "getItemType")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
		{
			String type = getItemType (obj);
			result = type;
			CCL_PRINTF ("getItemType: %s\n", MutableCString (type).str ())
			result.share ();
		}
		return true;
	}

	if(msg == "getEditArea")
	{
		Point* loc = unknown_cast<Boxed::Point> (msg[0].asUnknown ());
		ASSERT (loc)
		if(loc)
		{
			String type = getModel ().getEditArea (*this, *loc);
			result = type;
			result.share ();
		}
		return true;
	}

	if(msg == "getSelectionSize")
	{
		AutoPtr<Boxed::Rect> size = NEW Boxed::Rect;
		getModel ().getSelectionSize (*size, *this);
		result.takeShared (size->asUnknown ());
		return true;
	}

	if(msg == "setFocusItem")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
			getModel ().setFocusItem (obj, this);
		return true;
	}

	if(msg == "setAnchorItem")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
			getModel ().setAnchorItem (obj, this);
		return true;
	}

	if(msg == "select")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
			getModel ().selectItem (obj);
		return true;
	}

	if(msg == "unselect")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
			getModel ().unselectItem (obj);
		return true;
	}

	if(msg == "canSelect")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
			returnValue = getModel ().canSelectItem (obj);
		return true;
	}

	if(msg == "dragSelection")
	{
		MouseEvent* me = unknown_cast<Boxed::MouseEvent> (msg[0].asUnknown ());
		ASSERT (me)
		if(me)
			getModel ().dragSelection (*this, *me);
		return true;
	}

	if(msg == "drawSelection")
	{
		MouseEvent* me = unknown_cast<Boxed::MouseEvent> (msg[0].asUnknown ());
		ASSERT (me)
		if(me)
		{
			String hint;
			if(msg.getArgCount () >= 3)
				hint = msg[2];

			AutoPtr<EditHandler> handler = getModel ().drawSelection (*this, *me, hint);
			if(handler)
			{
				handler->setHookFromArgument (msg, 1);
				result.takeShared (handler->asUnknown ());
			}
		}
		return true;
	}

	if(msg == "dragEraser")
	{
		MouseEvent* me = unknown_cast<Boxed::MouseEvent> (msg[0].asUnknown ());
		ASSERT (me)
		if(me)
		{
			AutoPtr<EditHandler> handler = getModel ().dragEraser (*this, *me);
			if(handler)
				result.takeShared (handler->asUnknown ());
		}
		return true;
	}

	if(msg == "deleteSelected")
	{
		getModel ().deleteSelected ();
		return true;
	}

	if(msg == "deleteItem")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
			getModel ().deleteItem (obj);
		return true;
	}

	if(msg == "editItem")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
			result = getModel ().editItem (obj, *this);
		return true;
	}

	if(msg == "createEditHandler")
	{
		Object* itemPart = unknown_cast<Object> (msg[0].asUnknown ());
		MouseEvent* me = unknown_cast<Boxed::MouseEvent> (msg[1].asUnknown ());
		ASSERT (itemPart && me)
		if(itemPart && me)
		{
			AutoPtr<EditHandler> handler = getModel ().createEditHandler (itemPart, *this, *me);
			if(handler)
				result.takeShared (handler->asUnknown ());
		}
		return true;
	}

	if(msg == "getItemSize")
	{
		Object* obj = unknown_cast<Object> (msg[0].asUnknown ());
		ASSERT (obj)
		if(obj)
		{
			AutoPtr<Boxed::Rect> rect = NEW Boxed::Rect ();
			getModel ().getItemSize (*rect, *this, obj);
			result.takeShared (rect->asUnknown ());
		}
		return true;
	}

	if(msg == "detectDrag")
	{
		MouseEvent* me = unknown_cast<Boxed::MouseEvent> (msg[0].asUnknown ());
		ASSERT (me)
		if(me)
			result = detectDrag (*me);
		return true;
	}

	if(msg == "detectDoubleClick")
	{
		MouseEvent* me = unknown_cast<Boxed::MouseEvent> (msg[0].asUnknown ());
		ASSERT (me)
		if(me)
			result = detectDoubleClick (*me);
		return true;
	}

	if(msg == "takeFocus")
	{
		result = takeFocus ();
		return true;
	}

	if(msg == "createSelectFunctions")
	{
		ASSERT (component != nullptr)
		AutoPtr<SelectFunctions> functions = SelectFunctions::createInstance (*component, msg[0]);
		result.takeShared (functions->asUnknown ());
		return true;
	}

	if(msg == "showSelection")
	{
		bool show   = msg.getArgCount () > 0 ? msg[0].asBool () : true;
		bool redraw = msg.getArgCount () > 1 ? msg[1].asBool () : false;
		if(show)
			getSelection ().show (redraw);
		else
			getSelection ().hide (redraw);
		return true;
	}

	if(msg == "setCursor")
	{
		String cursorName (msg[0].asString ());
		setCursor (getTheme ().getCursor (MutableCString (cursorName)));
		return true;
	}

	if(msg == "moveCrossCursor")
	{
		Boxed::Point* position = unknown_cast<Boxed::Point> (msg[0]);
		if(position)
			moveCrossCursor (*position);
		return true;
	}

	return SuperClass::invokeMethod (returnValue, msg);
}
