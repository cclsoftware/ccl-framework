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
// Filename    : ccl/public/gui/framework/popupselectorclient.h
// Description : Popup Selector Client base class
//
//************************************************************************************************

#ifndef _ccl_popupselectorclient_h
#define _ccl_popupselectorclient_h

#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/base/unknown.h"

namespace CCL {

//************************************************************************************************
// PopupSelectorClient
/** Base Class for implementing IPopupSelectorClient. 
	\ingroup gui_dialog */
//************************************************************************************************

class PopupSelectorClient: public IPopupSelectorClient
{
public:
	PopupSelectorClient (int flags = kAcceptOnMouseUp);

	PROPERTY_FLAG (flags, kAcceptOnMouseDown, acceptOnMouseDown)
	PROPERTY_FLAG (flags, kAcceptOnMouseUp, acceptOnMouseUp)
	PROPERTY_FLAG (flags, kAcceptOnDoubleClick, acceptOnDoubleClick)
	PROPERTY_FLAG (flags, kAcceptAfterSwipe, acceptAfterSwipe)

	PROPERTY_FLAG (behavior, IPopupSelector::kCloseAfterDrag, closeAfterDrag)
	PROPERTY_FLAG (behavior, IPopupSelector::kRestoreMousePos, restoreMousePos)
	PROPERTY_FLAG (behavior, IPopupSelector::kWantsMouseUpOutside, wantsMouseUpOutside)
	PROPERTY_FLAG (behavior, IPopupSelector::kHideHScroll, hideHScroll)

	bool isIgnoringMouseClick () const;

protected:
	virtual bool hasPopupResult (); ///< check if there is a current selection that can be accepted (with code kOkay)

	void checkPopupLimits (IView* view, const SizeLimit& limits); ///< useful for checking size limist in createPopupView

	// IPopupSelectorClient
	IView* CCL_API createPopupView (SizeLimit& limits) override;
	void CCL_API attached (IWindow& popupWindow) override;
	Result CCL_API onMouseDown (const MouseEvent& event, IWindow& popupWindow) override;
	Result CCL_API onMouseUp (const MouseEvent& event, IWindow& popupWindow) override;
	Result CCL_API onKeyDown (const KeyEvent& event) override;
	Result CCL_API onKeyUp (const KeyEvent& event) override;
	Result CCL_API onEventProcessed (const GUIEvent& event, IWindow& popupWindow, IView* view) override;
	void CCL_API onPopupClosed (Result result) override;
	int32 CCL_API getPopupBehavior () override;
	tbool CCL_API mouseWheelOnSource (const MouseWheelEvent& event, IView* source) override;
	ITouchHandler* CCL_API createTouchHandler (const TouchEvent& event, IWindow* window) override;
	void CCL_API setCursorPosition (PointRef where) override;
	tbool CCL_API setToDefault () override;

	enum Flags
	{
		kAcceptOnMouseDown		= 1 << 0,
		kAcceptOnMouseUp		= 1 << 1,
		kAcceptOnDoubleClick	= 1 << 2,
		kAcceptAfterSwipe		= 1 << 3,
		kIgnoreMouseUp			= 1 << 4	// internal use
	};
	int32 flags;
	int32 behavior;
	Point cursorPosition;
};

//************************************************************************************************
// PopupSourceControllerAccess
/** Mixin class for a PopupSelectorClient that implements access to the "source" controller in a popup form. 
	A class that implements IObjectNode must be specified as template argument; it will be used as SuperClass.
	\ingroup gui_dialog */
//************************************************************************************************

template <class NodeSuperClass>
class PopupSourceControllerAccess: public NodeSuperClass
{
public:
	PROPERTY_SHARED_AUTO (IObjectNode, sourceController, SourceController) // returned as child "source" for accessing it in the popup form

	// IObjectNode
	IObjectNode* CCL_API findChild (StringRef id) const override
	{
		if(id=="source")
			return sourceController;

		return NodeSuperClass::findChild (id);
	}
};

//************************************************************************************************
// SimplePopupSelectorClient
/** Can be instantiated when no special behavior is necessary. 
	\ingroup gui_dialog */
//************************************************************************************************

class SimplePopupSelectorClient: public PopupSelectorClient,
								 public PopupSourceControllerAccess<AbstractNode>,
								 public Unknown
{
public:
	CLASS_INTERFACE2 (IPopupSelectorClient, IObjectNode, Unknown)

	SimplePopupSelectorClient ();

	PROPERTY_BOOL (popupResult, PopupResult)

protected:
	// PopupSelectorClient
	bool hasPopupResult () override;
};

} // namespace CCL

#endif // _ccl_popupselectorclient_h
