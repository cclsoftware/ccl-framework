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
// Filename    : ccl/gui/windows/appwindow.cpp
// Description : Application Window
//
//************************************************************************************************

#include "ccl/gui/windows/appwindow.h"
#include "ccl/gui/windows/windowmanager.h"
#include "ccl/gui/popup/menubarcontrol.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/gui.h"

#include "ccl/public/gui/iapplication.h"

#include "ccl/base/storage/configuration.h"

using namespace CCL;

//************************************************************************************************
// ApplicationWindow
//************************************************************************************************

bool ApplicationWindow::isUsingCustomMenuBar ()
{
	static const Configuration::BoolValue useCustomMenuBar ("GUI.ApplicationWindow", "CustomMenuBar", false);
	
	return GUI.isCustomMenuBarSupported () && useCustomMenuBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ApplicationWindow, PopupWindow)

//////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationWindow::ApplicationWindow (IApplication* application, const Rect& size, StyleRef style, StringRef title)
: PopupWindow (size, style, title),
  application (application),
  optionKeyDownHandled (false)
{
	addToDesktop ();

	ASSERT (application != nullptr)
	if(!application)
		return;

	setController (application); // calls retain

	if(title.isEmpty ())
		setTitle (application->getApplicationTitle ());

	// create menubar
	MenuBar* menuBar = unknown_cast<MenuBar> (application->createMenuBar ());
	if(menuBar)
		setMenuBar (menuBar);

	// These platforms have no native orientation, take application window
	#if CCL_PLATFORM_DESKTOP
	GUI.setInterfaceOrientation (size.getWidth () > size.getHeight () ? Styles::kLandscape : Styles::kPortrait);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationWindow::~ApplicationWindow ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationWindow::updateMenuBar ()
{
	if(isUsingCustomMenuBar ())
	{
		MenuBarControl* menuBarControl = nullptr;
		if(menuBar)
		{
			Rect size (0, 0, getWidth (), 0);
			menuBarControl = NEW MenuBarControl (size);
			menuBarControl->setSizeMode (View::kAttachLeft | View::kAttachRight);
			menuBarControl->setMenuBar (menuBar);
			menuBarControl->autoSize (false, true);
		}
		WindowManager::instance ().setMenuBarView (menuBarControl);
	}
	else
		SuperClass::updateMenuBar ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ApplicationWindow::setFullscreen (tbool state)
{
	// remove custom menubar when switching to fullscreen
	if(isUsingCustomMenuBar () && state && !isFullscreen () && menuBar)
		WindowManager::instance ().setMenuBarView (nullptr);

	return SuperClass::setFullscreen (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationWindow::onClose ()
{
	if(!Window::onClose ()) // ask window event handler first
		return false;

	setInCloseEvent (true);

	// try to quit application
	bool result = true;
	if(application)
		result = application->requestQuit () != 0;

	setInCloseEvent (false);
	return result; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* ApplicationWindow::createDragHandler (const DragEvent& event)
{
	if(application)
		return application->createDragHandler (event, this);

	return SuperClass::createDragHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationWindow::onKeyDown (const KeyEvent& event)
{
	bool result = SuperClass::onKeyDown (event);

	if(isUsingCustomMenuBar ())
	{
		// remember if option key was handled as modifier (combined with other another key, e.g. as command or by a view): must ignore on key up
		optionKeyDownHandled = result && event.state.isSet (KeyState::kOption);

		// delegate to menu bar control: option + character activates a specific menu
		if(!result && event.state.getModifiers () == KeyState::kOption && Unicode::isAlpha (event.character))
		{
			if(auto* menuBarControl = MenuBarControl::findInWindow (*this))
				result = menuBarControl->onKeyDown (event);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationWindow::onKeyUp (const KeyEvent& event)
{
	bool result = SuperClass::onKeyUp (event);

	if(!result && !optionKeyDownHandled && isUsingCustomMenuBar () && isActive ())
	{
		// delegate to menu bar control: option key (up) sets focus to menu bar
		if(event.vKey == VKey::kOption)
		{
			if(auto* menuBarControl = MenuBarControl::findInWindow (*this))
				result = menuBarControl->onKeyUp (event);
		}
	}
	return result;
}
