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
// Filename    : ccl/app/controls/usercontrol.h
// Description : User Control
//
//************************************************************************************************

#ifndef _ccl_usercontrol_h
#define _ccl_usercontrol_h

#include "ccl/app/controls/draghandler.h"

#include "ccl/public/gui/framework/usercontrolbase.h"
#include "ccl/public/gui/framework/abstracttouchhandler.h"
#include "ccl/public/gui/framework/iaccessibility.h"

namespace CCL {

interface IHelpInfoBuilder;

//************************************************************************************************
// UserControl
//************************************************************************************************

class UserControl: public Object,
				   public AbstractUserControl
{
public:
	DECLARE_CLASS (UserControl, Object)

	UserControl (RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~UserControl ();

	/** Cast IView to UserControl. */
	template <class T>
	static T* cast_IView (IView* view)
	{
		UnknownPtr<IUserControlHost> host (view);
		return unknown_cast<T> (host ? host->getUserControl () : nullptr);
	}

	class MouseHandler;
	class DragHandler;
	class TouchMouseHandler;
	class TouchHandler;
	class GestureHandler;
	class AccessibilityProvider;

	void resetSizeLimits ();

	// AbstractUserControl
	IAccessibilityProvider* CCL_API getCustomAccessibilityProvider () const override;

	CLASS_INTERFACE (IUserControl, Object)

protected:
	AccessibilityProvider* customAccessibilityProvider;

	AbstractTouchMouseHandler* wrapMouseHandler (const TouchEvent& event);
};

//********************************************************************************************
// UserControl::MouseHandler
//********************************************************************************************

class UserControl::MouseHandler: public Object,
								 public AbstractMouseHandler
{
public:
	DECLARE_CLASS (MouseHandler, Object)

	MouseHandler (UserControl* control = nullptr, int flags = 0);

	UserControl* getControl () const;

	// AbstractMouseHandler
	void CCL_API begin (const MouseEvent& event) override;

	CLASS_INTERFACE (IMouseHandler, Object)

protected:
	UserControl* control;

	virtual bool getHelp (IHelpInfoBuilder& helpInfo);
};

//********************************************************************************************
// UserControl::DragHandler
//********************************************************************************************

class UserControl::DragHandler: public CCL::DragHandler
{
public:
	DECLARE_CLASS_ABSTRACT (UserControl::DragHandler, CCL::DragHandler)

	DragHandler (UserControl& control);

	UserControl& getControl () const;

protected:
	UserControl& control;
};

//********************************************************************************************
// UserControl::TouchMouseHandler
//********************************************************************************************

class UserControl::TouchMouseHandler: public Object,
									  public AbstractTouchMouseHandler
{
public:
	TouchMouseHandler (IMouseHandler* mouseHandler, UserControl& control);

	UserControl& getControl () const;

	CLASS_INTERFACE (ITouchHandler, Object)

protected:
	UserControl& control;
	AutoPtr<IAutoScroller> autoScroller;

	// AbstractTouchMouseHandler
	tbool CCL_API trigger (const TouchEvent& event) override;
};

//********************************************************************************************
// UserControl::TouchHandler
//********************************************************************************************

class UserControl::TouchHandler: public Object,
								 public AbstractTouchHandler
{
public:
	TouchHandler (UserControl& control);

	UserControl& getControl () const;

	CLASS_INTERFACE (ITouchHandler, Object)

protected:
	UserControl& control;
};

//********************************************************************************************
// UserControl::GestureHandler
//********************************************************************************************

class UserControl::GestureHandler: public Object,
								   public AbstractTouchHandler
{
public:
	GestureHandler (UserControl& control);

	// AbstractTouchHandler
	tbool CCL_API onGesture (const GestureEvent& event) override;

	CLASS_INTERFACE (ITouchHandler, Object)

protected:
	UserControl& control;
};

//********************************************************************************************
// UserControl::AccessibilityProvider
//********************************************************************************************

class UserControl::AccessibilityProvider: public Object,
										  public AbstractAccessibilityProvider
{
public:
	AccessibilityProvider (UserControl& control);

	CLASS_INTERFACE (IAccessibilityProvider, Object)

protected:
	UserControl& control;
};

} // namespace CCL

#endif // _ccl_usercontrol_h
