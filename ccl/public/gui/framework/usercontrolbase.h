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
// Filename    : ccl/public/gui/framework/usercontrolbase.h
// Description : Abstract User Control
//
//************************************************************************************************

#ifndef _ccl_usercontrolbase_h
#define _ccl_usercontrolbase_h

#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iusercontrol.h"
#include "ccl/public/gui/framework/imousehandler.h"

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface ISprite;
interface ITouchHandler;

//************************************************************************************************
// AbstractUserControl
/** Base class to implement a user control.
	\ingroup gui_appview */
//************************************************************************************************

class AbstractUserControl: public ViewBox,
						   public IUserControl
{
public:
	AbstractUserControl () {}
	virtual ~AbstractUserControl () {}

	/** When attached, the framework view is our owner. We have to avoid
		circular references. In general, if the control is passed back 
		to the framework, you do not call dispose. Use this method for clean up 
		if something fails, before passing back the object. */
	void dispose ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// IUserControl
	//////////////////////////////////////////////////////////////////////////////////////////////

	tbool CCL_API onViewEvent (const GUIEvent& event) override;
	IMouseHandler* CCL_API createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* CCL_API createTouchHandler (const TouchEvent& event) override;
	IDragHandler* CCL_API createDragHandler (const DragEvent& event) override;
	IUnknown* CCL_API getController () const override; ///< Note: hides ViewBox::getController()!
	IAccessibilityProvider* CCL_API getCustomAccessibilityProvider () const override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Misc. Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual void onViewsChanged ();
	virtual void attached (IView* parent);
	virtual void removed (IView* parent);
	virtual void onActivate (bool state);

	virtual void onSize (PointRef delta);
	virtual void onMove (PointRef delta);
	virtual void onChildSized (IView* child, PointRef delta);
	virtual void onVisualStyleChanged ();
	virtual void onDisplayPropertiesChanged (const DisplayChangedEvent& event);
	virtual void onColorSchemeChanged (const ColorSchemeEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Drawing
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual void draw (const DrawEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Mouse Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool onMouseDown    (const MouseEvent& event);
	virtual bool onMouseUp      (const MouseEvent& event);
	virtual bool onMouseEnter   (const MouseEvent& event);
	virtual bool onMouseMove    (const MouseEvent& event);
	virtual bool onMouseLeave   (const MouseEvent& event);

	virtual bool onMouseWheel   (const MouseWheelEvent& event);
	virtual bool onContextMenu  (const ContextMenuEvent& event);
	virtual bool onTrackTooltip (const TooltipEvent& event);

	void setMouseHandler (IMouseHandler* handler);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Multitouch Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool onGesture (const GestureEvent& event); ///< position is in client coordinates

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Keyboard Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool onFocus   (const FocusEvent& event);
	virtual bool onKeyDown (const KeyEvent& event);
	virtual bool onKeyUp   (const KeyEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Drag Events
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual bool onDragEnter (const DragEvent& event);
	virtual bool onDragOver  (const DragEvent& event);
	virtual bool onDragLeave (const DragEvent& event);
	virtual bool onDrop      (const DragEvent& event);

	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Cast to IUnknown (when querying for additional interfaces of a UserControl, e.g. via UnknownPtr, avoid quering the UserControlHost via ViewBox::operator IView*). */
	operator IUnknown* () { return static_cast<IUserControl*> (this); }

protected:
	AbstractUserControl (const AbstractUserControl&) {}
	AbstractUserControl& operator = (AbstractUserControl&) { return *this; }

	/** To be called in ctor of derived class when vtable is set up correctly! */
	void construct (RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);
};

//************************************************************************************************
// AbstractMouseHandler
//************************************************************************************************

class AbstractMouseHandler: public IMouseHandler
{
public:
	AbstractMouseHandler (int flags = 0)
	: flags (flags)
	{}

	virtual ~AbstractMouseHandler () {}

	PROPERTY_FLAG (flags, kCheckKeys,		  checkKeys)
	PROPERTY_FLAG (flags, kPeriodic,		  periodic)
	PROPERTY_FLAG (flags, kCanEscape,		  canEscape)
	PROPERTY_FLAG (flags, kNullHandler,		  isNullHandler)
	PROPERTY_FLAG (flags, kAutoScrollV,		  autoScrollV)
	PROPERTY_FLAG (flags, kAutoScrollH,		  autoScrollH)
	PROPERTY_FLAG (flags, kAutoScroll,		  autoScroll)
	PROPERTY_FLAG (flags, kBeginAtCurrentPos, beginAtCurrentPos)

	// IMouseHandler
	int CCL_API getFlags () const override;
	void CCL_API begin (const MouseEvent& event) override;
	tbool CCL_API trigger (const MouseEvent& event, int moveFlags) override;
	tbool CCL_API triggerKey (const KeyEvent& event) override;
	void CCL_API finish (const MouseEvent& event, tbool canceled = false) override;

	// overwrite:
	virtual void onBegin ();
	virtual bool onMove (int moveFlags);
	virtual void onRelease (bool canceled);
	virtual bool onKeyEvent (const KeyEvent& event);

protected:
	int flags;
	MouseEvent first;
	MouseEvent previous;
	MouseEvent current;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Get an interface from a view / user control or one of it's parents.
//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* GetViewInterfaceUpwards (UIDRef iid, IView* view);
template <class T> T* GetViewInterfaceUpwards (IView* view) { return (T*) GetViewInterfaceUpwards (ccl_iid<T> (), view); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_usercontrolbase_h
