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
// Filename    : ccl/gui/views/triggerview.h
// Description : Trigger View
//
//************************************************************************************************

#ifndef _ccl_triggerview_h
#define _ccl_triggerview_h

#include "ccl/gui/views/view.h"

namespace CCL {

//************************************************************************************************
// TriggerView styles
//************************************************************************************************

namespace Styles 
{
	enum TriggerViewStyles
	{
		kTriggerViewBehaviorSwallowDrag = 1<<0
	};
}

//************************************************************************************************
// TriggerView
//************************************************************************************************

class TriggerView: public View
{
public:
	DECLARE_CLASS (TriggerView, View)
	DECLARE_STYLEDEF (customStyles)
	DECLARE_STYLEDEF (gesturePriorities)
	DECLARE_PROPERTY_NAMES (TriggerView)

	TriggerView (IUnknown* controller = nullptr, const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	DECLARE_STRINGID_MEMBER (kOnMouseDown)
	DECLARE_STRINGID_MEMBER (kOnSingleClick)
	DECLARE_STRINGID_MEMBER (kOnRightClick)
	DECLARE_STRINGID_MEMBER (kOnDoubleClick)
	DECLARE_STRINGID_MEMBER (kOnDrag)
	DECLARE_STRINGID_MEMBER (kOnSingleTap)
	DECLARE_STRINGID_MEMBER (kOnDoubleTap)
	DECLARE_STRINGID_MEMBER (kOnFirstTap)	///< first tap of a possible double tap
	DECLARE_STRINGID_MEMBER (kOnLongPress)
	DECLARE_STRINGID_MEMBER (kOnSwipe)
	DECLARE_STRINGID_MEMBER (kOnSwipeH)
	DECLARE_STRINGID_MEMBER (kOnSwipeV)
	DECLARE_STRINGID_MEMBER (kOnTouch)
	DECLARE_STRINGID_MEMBER (kOnContextMenu)
	DECLARE_STRINGID_MEMBER (kOnDisplayPropertiesChanged)
	// also sends kOnAttached, kOnRemoved

	PROPERTY_VARIABLE (int, gesturePriority, GesturePriority)

	// View
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onDisplayPropertiesChanged (const DisplayChangedEvent& event) override;
	tbool CCL_API setController (IUnknown* controller) override;
	IUnknown* CCL_API getController () const override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onDragEnter (const DragEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	void CCL_API signal (MessageRef msg) override;

protected:
	SharedPtr<IUnknown> controller;
	GestureEvent lastGestureEvent;

	class EventStateGuard;
	struct EventState
	{
		View* delegateView;
		const ContextMenuEvent* contextMenuEvent;
		bool eventHandled;

		EventState ();
	};
	static EventState eventState; ///< temporary state, valid during processing of a gui event

	enum Flags
	{
		kTriggersChecked	= 1<<0,
		kWantsSingleClick	= 1<<1,
		kWantsDoubleClick	= 1<<2,
		kWantsDrag			= 1<<3,
		kWantsSingleTap		= 1<<4,
		kWantsDoubleTap		= 1<<5,
		kWantsLongPress		= 1<<6,
		kWantsSwipe			= 1<<7,
		kWantsSwipeH		= 1<<8,
		kWantsSwipeV		= 1<<9,
		kWantsTouch			= 1<<10,
		kWantsContextMenu	= 1<<11,
		kWantsAttached		= 1<<12,
		kWantsRemoved		= 1<<13,
		kWantsFirstTap		= 1<<14,
		kWantsRightClick	= 1<<15
	};

	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, kTriggersChecked,		triggersChecked)
	PROPERTY_FLAG (flags, kWantsSingleClick,	wantsSingleClick)
	PROPERTY_FLAG (flags, kWantsRightClick,		wantsRightClick)
	PROPERTY_FLAG (flags, kWantsDoubleClick,	wantsDoubleClick)
	PROPERTY_FLAG (flags, kWantsDrag,			wantsDrag)
	PROPERTY_FLAG (flags, kWantsSingleTap,		wantsSingleTap)
	PROPERTY_FLAG (flags, kWantsDoubleTap,		wantsDoubleTap)
	PROPERTY_FLAG (flags, kWantsFirstTap,		wantsFirstTap)
	PROPERTY_FLAG (flags, kWantsLongPress,		wantsLongPress)
	PROPERTY_FLAG (flags, kWantsSwipe,			wantsSwipe)
	PROPERTY_FLAG (flags, kWantsSwipeH,			wantsSwipeH)
	PROPERTY_FLAG (flags, kWantsSwipeV,			wantsSwipeV)
	PROPERTY_FLAG (flags, kWantsTouch,			wantsTouch)
	PROPERTY_FLAG (flags, kWantsContextMenu,	wantsContextMenu)
	PROPERTY_FLAG (flags, kWantsAttached,		wantsAttached)
	PROPERTY_FLAG (flags, kWantsRemoved,		wantsRemoved)

	void checkTriggers ();
};

} // namespace CCL

#endif // _ccl_triggerview_h
