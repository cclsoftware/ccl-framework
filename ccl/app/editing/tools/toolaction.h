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
// Filename    : ccl/app/editing/tools/toolaction.h
// Description : Tool Actions
//
//************************************************************************************************

#ifndef _ccl_toolaction_h
#define _ccl_toolaction_h

#include "ccl/app/editing/tools/edittool.h"
#include "ccl/public/base/smartptr.h"
#include "ccl/public/gui/framework/themeelements.h"

namespace CCL {

interface IThemeStatics;

//************************************************************************************************
// ToolAction
//************************************************************************************************

class ToolAction: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ToolAction, Object)

	enum ToolGesture
	{
		// touch gestures
		kSwipe			= 1<<GestureEvent::kSwipe,
		kZoom			= 1<<GestureEvent::kZoom,
		kRotate			= 1<<GestureEvent::kRotate,
        kLongPress		= 1<<GestureEvent::kLongPress,
        kSingleTap		= 1<<GestureEvent::kSingleTap,
		kDoubleTap		= 1<<GestureEvent::kDoubleTap,

		// touch constraints
		kHorizontal		= GestureEvent::kHorizontal,
		kVertical		= GestureEvent::kVertical,
		kExclusiveTouch	= GestureEvent::kExclusiveTouch,
		kSuppressTouchContextMenu = GestureEvent::kSuppressContextMenu,

		// mouse gestures
		kClick			= KeyState::kClick,
		kDrag			= KeyState::kDrag,
		kDoubleClick	= KeyState::kDoubleClick,
		kSingleClick	= KeyState::kSingleClick,

		kMouseMask		= kClick|kDrag|kDoubleClick|kSingleClick,
		kTouchMask		= kSwipe|kZoom|kRotate|kLongPress|kSingleTap|kDoubleTap,

		// additional behavior flags
		kRawTouches		= 1<<30,	///< for touch / pen: no gesture recognition required, EditHandler should be triggered on TouchEvent::kBegin / kMove / kEnd
		kPreview		= 1<<31		///< for touch / pen: this action's handler will be triggered already while recognition of competing gestures is still pending
	};

	ToolAction ();

	PROPERTY_VARIABLE (int, gestures, Gestures)
	PROPERTY_VARIABLE (int, gesturePriority, GesturePriority)
	PROPERTY_STRING (tooltip, Tooltip)
	PROPERTY_STRING (helpText, HelpText)
	PROPERTY_MUTABLE_CSTRING (cursor, Cursor)
	void setThemeCursor (ThemeCursorID which);
	PROPERTY_BOOL (wantsCrossCursor, WantsCrossCursor)

	PROPERTY_SHARED_AUTO (Object, item, Item)
			
	virtual EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) { return nullptr; }
	virtual bool canPerform (const KeyState& event) { return true; }
	virtual void onGesture (EditView& editView, const GestureEvent& event, PointRef where) {}
	virtual tresult allowsCompetingGesture (int gestureType) const { return kResultNotImplemented; } ///< returns kResultTrue, kResultFalse or kResultNotImplemented 
	virtual bool addHelp (IHelpInfoBuilder& helpInfo); // returns false to ignore following actions

	static KeyState getKeys (const GUIEvent& event);

protected:
	static IThemeStatics& getThemeStatics ();

	void selectItem (EditView& view, bool exclusive = true);
	MouseEvent makeMouseEvent (const GUIEvent& event, PointRef where); ///< utility e.g. to feed EditModel methods

	static PointerEvent::InputDevice getInputDevice (const GUIEvent& event);
};

//************************************************************************************************
// ToolActionList
//************************************************************************************************

class ToolActionList
{
public:
	ToolActionList ();

	PROPERTY_VARIABLE (int, gestureMask, GestureMask)

	void addAction (ToolAction* action);
	void addAction (ToolAction* action, Object* item, int gestures);
	void addActionWithModifiers (ToolAction* action, Object* item, int gestures, int modifiers); ///< add action requiring a modifier combination (0 is a valid combination that needs to be matched as "no modifier")
	void addHelpOption (uint32 modifiers, StringID iconName, StringRef text);
	void removeAll ();
	void removeAction (int gestures, const KeyState* keys = nullptr);

	Iterator* newIterator () const;
	Iterator* newIterator (int gestures, const KeyState* keys = nullptr) const;
	ToolAction* getFirstAction (int gestures, const KeyState* keys = nullptr) const;
	const ObjectList& getActions () const;

protected:
	ObjectList actions;
};

//************************************************************************************************
// ActionTool
//************************************************************************************************

class ActionTool: public EditTool
{
public:
	DECLARE_CLASS_ABSTRACT (ActionTool, EditTool)

	ActionTool (StringID name = nullptr, StringRef title = nullptr);

	PROPERTY_BOOL (translateHoverEvents, TranslateHoverEvents)

	// EditTool
	void mouseEnter (EditView& editView, const MouseEvent& mouseEvent) override;
	void mouseMove (EditView& editView, const MouseEvent& mouseEvent) override;
	void mouseLeave (EditView& editView, const MouseEvent& mouseEvent) override;
	EditHandler* mouseDown (EditView& editView, const MouseEvent& mouseEvent) override;
	String getTooltip () override;
	ITouchHandler* createTouchHandler (EditView& editView, const TouchEvent& event) override;
	IPresentable* createHelpInfo (EditView& editView, const MouseEvent& mouseEvent) override;

protected:
	ToolActionList mouseActions;
	bool performingActions;

	virtual void findActions (ToolActionList& actions, EditView& editView, PointRef where, const GUIEvent& event) = 0;

	static KeyState getKeys (const GUIEvent& event);
};
	
//************************************************************************************************
// ToolTouchHandler
//************************************************************************************************

class ToolTouchHandler: public UserControl::TouchMouseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (ToolTouchHandler, TouchMouseHandler)

	ToolTouchHandler (EditView& editView);

	ToolActionList& getActions ();
	void prepareGestures ();

	PROPERTY_BOOL (translateHoverEvents, TranslateHoverEvents)

	// TouchHandler
	tbool CCL_API onGesture (const GestureEvent& event) override;
	bool onHover (const TouchEvent& event) override;
	void onBegin (const TouchEvent& event) override;
	bool onMove (const TouchEvent& event) override;
	tbool CCL_API addTouch (const TouchEvent& event) override;
	tbool CCL_API allowsCompetingGesture (int gestureType) override;

private:
	EditView& editView;
	ToolActionList actions;
	ToolAction* gestureAction;
	ToolAction* previewAction;
	Vector<Point> touches;
	TouchID editHandlerTouchID;
	bool handlesTouchEvents;

	EditHandler* createEditHandler (ToolAction& action, const TouchEvent& event);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ToolAction::ToolAction ()
: gestures (0), gesturePriority (GestureEvent::kPriorityHighest), wantsCrossCursor (false) {}

inline KeyState ToolAction::getKeys (const GUIEvent& event)
{ auto mouseEvent = event.as<MouseEvent> (); return mouseEvent ? mouseEvent->keys : KeyState (); }

inline KeyState ActionTool::getKeys (const GUIEvent& event)
{ return ToolAction::getKeys (event); }

inline ToolActionList::ToolActionList ()
: gestureMask (~0)
{ actions.objectCleanup (); }

inline void ToolActionList::addAction (ToolAction* action)
{ actions.add (action); }

inline Iterator* ToolActionList::newIterator () const
{ return actions.newIterator (); }

inline const ObjectList& ToolActionList::getActions () const
{ return actions; }

inline ToolActionList& ToolTouchHandler::getActions ()
{ return actions; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_toolaction_h
