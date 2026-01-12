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
// Filename    : ccl/gui/controls/usercontrolhost.h
// Description : User Control Host
//
//************************************************************************************************

#ifndef _ccl_usercontrolhost_h
#define _ccl_usercontrolhost_h

#include "ccl/gui/views/view.h"

#include "ccl/public/gui/framework/iusercontrol.h"

namespace CCL {

//************************************************************************************************
// UserControlHost
//************************************************************************************************

class UserControlHost: public View,
					   public IUserControlHost,
					   public IBackgroundView
{
public:
	DECLARE_CLASS (UserControlHost, View)

	UserControlHost ();
	~UserControlHost ();

	// IUserControlHost
	void CCL_API setUserControl (IUserControl* control) override;
	IUserControl* CCL_API getUserControl () override;
	void CCL_API setMouseHandler (IMouseHandler* handler) override;

	// View
	void onViewsChanged () override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onActivate (bool state) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	void onDisplayPropertiesChanged (const DisplayChangedEvent& event) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void onVisualStyleChanged () override;
	void onChildSized (View* child, const Point& delta) override;
	void CCL_API setSizeLimits (const SizeLimit& sizeLimits) override;
	StringRef getHelpIdentifier () const override;
	bool setHelpIdentifier (StringRef id) override;

	IUnknown* CCL_API getController () const override;

	tbool CCL_API makeVisible (RectRef rect, tbool relaxed = false) override;
	void draw (const UpdateRgn& updateRgn) override;

	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseUp (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	bool onTrackTooltip (const TooltipEvent& event) override;
	bool onGesture (const GestureEvent& event) override;

	bool onFocus (const FocusEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;

	bool onDragEnter (const DragEvent& event) override;
	bool onDragOver (const DragEvent& event) override;
	bool onDragLeave (const DragEvent& event) override;
	bool onDrop (const DragEvent& event) override;
	IDragHandler* createDragHandler (const DragEvent& event) override;

	AccessibilityProvider* getAccessibilityProvider () override;

	CLASS_INTERFACE2 (IUserControlHost, IBackgroundView, View)

protected:
	IUserControl* userControl;
	String helpId;

	// IBackgroundView
	tbool CCL_API canDrawControlBackground () const override;
	void CCL_API drawControlBackground (IGraphics& graphics, RectRef r, PointRef offset) override;
};

} // namespace CCL

#endif // _ccl_usercontrolhost_h
