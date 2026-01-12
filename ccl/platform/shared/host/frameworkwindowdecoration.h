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
// Filename    : ccl/platform/shared/host/frameworkwindowdecoration.h
// Description : Window decoration using generic framework controls only
//
//************************************************************************************************

#ifndef _ccl_frameworkwindowdecoration_h
#define _ccl_frameworkwindowdecoration_h

#include "ccl/gui/windows/window.h"

#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/iparamobserver.h"

namespace CCL {

//************************************************************************************************
// WindowDecorationController
//************************************************************************************************

class WindowDecorationController: public Object,
								  public AbstractNode,
								  public AbstractController,
								  public IParamObserver,
								  public IWindowEventHandler
{
public:
	DECLARE_CLASS (WindowDecorationController, Object)

	WindowDecorationController ();
	
	enum Flags
	{
		kCanMinimize = 1<<0,
		kCanMaximize = 1<<1,
		kCanClose = 1<<2
	};
	
	PROPERTY_VARIABLE (Coord, titleBarHeight, TitleBarHeight)
	PROPERTY_VARIABLE (Coord, borderWidth, BorderWidth)

	PROPERTY_FLAG (flags, kCanMinimize, canMinimize)
	PROPERTY_FLAG (flags, kCanMaximize, canMaximize)
	PROPERTY_FLAG (flags, kCanClose, canClose)
	
	void attach (Window* window);
	View* getDecorationView ();
	void updateDecoration ();
	void setIcon (IImage* icon);
	
	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	
	// AbstractController
	DECLARE_PARAMETER_LOOKUP (paramList)
	
	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}
	
	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;
	
	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IObjectNode)
		QUERY_INTERFACE (IController)
		QUERY_INTERFACE (IParamObserver)
		QUERY_INTERFACE (IWindowEventHandler)
	END_CLASS_INTERFACES (Object)
	
protected:
	int flags;
	ParamList paramList;
	AutoPtr<View> decorationView;
	Window* targetWindow;
	
	virtual void onMinimize ();
	virtual void onMaximize (bool state);
	virtual void onClose ();
	virtual void onMoveWindow ();
	virtual void onShowMenu ();
};

} // namespace CCL

#endif // _ccl_frameworkwindowdecoration_h
