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
// Filename    : ccl/platform/win/gui/desktop.win.cpp
// Description : Desktop Management
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/childwindow.h"

#include "ccl/platform/win/gui/windowhelper.h"
#include "ccl/platform/win/gui/screenscaling.h"

namespace CCL {

//************************************************************************************************
// WindowsDesktopManager
//************************************************************************************************

class WindowsDesktopManager: public DesktopManager
{
public:
	// DesktopManager
	void onAppActivate (bool state) override;
	void addWindow (Window* window, WindowLayer layer) override;
	IWindow* CCL_API findWindow (PointRef screenPos, int flags = 0) override;
	int CCL_API countMonitors () const override;
	int CCL_API getMainMonitor () const override;
	int CCL_API findMonitor (PointRef where, tbool fallbackToDefault) const override;
	tbool CCL_API getMonitorSize (Rect& rect, int index, tbool useWorkArea) const override;
	float CCL_API getMonitorScaleFactor (int index) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsDesktopManager winDesktop;
DesktopManager& Desktop = winDesktop;

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// WindowsDesktopManager
//************************************************************************************************

void WindowsDesktopManager::onAppActivate (bool state)
{
	DesktopManager::onAppActivate (state);

	WPARAM wParam = state;
	Window* modal = getTopWindow (kDialogLayer);
	if(modal)
		::SendMessage ((HWND)modal->getSystemWindow (), WM_NCACTIVATE, wParam, 0);
	else
	{
		for(int i = 0, count = countWindows (); i < count; i++)
		{
			IWindow* window = getWindow (i);
			::SendMessage ((HWND)window->getSystemWindow (), WM_NCACTIVATE, wParam, 0);
		}
	}

	// progress window must not disappear behind application window
	if(state && isProgressMode ())
		flushUpdatesWithProgressWindows ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsDesktopManager::addWindow (Window* window, WindowLayer layer)
{
	DesktopManager::addWindow (window, layer);

	Win32::EnforceWindowOrder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API WindowsDesktopManager::findWindow (PointRef _screenPos, int flags)
{
	Point screenPos (_screenPos);
	Win32::gScreens->toPixelPoint (screenPos);

	POINT p = {screenPos.x, screenPos.y};
	HWND hwnd = ::WindowFromPoint (p);

	while(hwnd)
	{
		Window* window = Win32::GetWindowFromNativeHandle (hwnd);
		if(window)
		{
			// Find the topmost CCL window. This might be a ChildWindow with a non-CCL parent.
			ChildWindow* childWindow = ccl_cast<ChildWindow> (window);
			if(childWindow == nullptr || (childWindow && childWindow->getParent () == nullptr))
				return window;
		}
		hwnd = ::GetParent (hwnd);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsDesktopManager::countMonitors () const
{
	return Win32::gScreens.getCount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsDesktopManager::getMainMonitor () const
{
	return Win32::gScreens.getPrimaryIndex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsDesktopManager::findMonitor (PointRef where, tbool defaultToPrimary) const
{
	return Win32::gScreens.getIndexAtCoord (where, defaultToPrimary != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsDesktopManager::getMonitorSize (Rect& rect, int index, tbool useWorkArea) const
{
	const Win32::ScreenInformation& screen = Win32::gScreens.getAt (index);
	if(useWorkArea)
		rect = screen.coordWorkArea;
	else
		rect = screen.coordRect;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API WindowsDesktopManager::getMonitorScaleFactor (int index) const
{
	const Win32::ScreenInformation& screen = Win32::gScreens.getAt (index);
	return screen.scaleFactor;
}
