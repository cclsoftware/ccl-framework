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
// Filename    : ccl/gui/windows/appwindow.h
// Description : Application Window
//
//************************************************************************************************

#ifndef _ccl_appwindow_h
#define _ccl_appwindow_h

#include "ccl/gui/windows/popupwindow.h"

namespace CCL {

interface IApplication;

//************************************************************************************************
// ApplicationWindow
/** Base class for single window application. Closing the window quits the application. */
//************************************************************************************************

class ApplicationWindow: public PopupWindow
{
public:
	DECLARE_CLASS (ApplicationWindow, PopupWindow)

	enum Defaults
	{
		kDefaultWidth  = 800,
		kDefaultHeight = 600,
		kDefaultStyle  = Styles::kWindowAppearanceTitleBar|Styles::kWindowBehaviorSizable|Styles::kWindowBehaviorCenter
	};

	ApplicationWindow  (IApplication* application = nullptr, 
						const Rect& size = Rect (0, 0, kDefaultWidth, kDefaultHeight),
						StyleRef style = StyleFlags (0, kDefaultStyle),
						StringRef title = nullptr);
	~ApplicationWindow ();

	static bool isUsingCustomMenuBar ();

	// Window
	void updateMenuBar () override;
	tbool CCL_API setFullscreen (tbool state) override;
	IDragHandler* createDragHandler (const DragEvent& event) override;
	bool onClose () override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;

protected:
	IApplication* application;
	bool optionKeyDownHandled;
};

} // namespace CCL

#endif //_ccl_appwindow_h
