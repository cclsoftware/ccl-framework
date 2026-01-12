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
// Filename    : ccl/platform/win/gui/alert.win.cpp
// Description : platform alert dialog
//
//************************************************************************************************

#include "ccl/gui/dialogs/alert.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// AlertHandler
//************************************************************************************************

namespace AlertHandler
{
	static int showTaskDialog (AlertBox& alert);
	static HRESULT CALLBACK taskDialogCallback (HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);
};

} // namespace Win32
} // namespace CCL

namespace CCL {

//************************************************************************************************
// WindowsAlertBox
//************************************************************************************************

class WindowsAlertBox: public AlertBox
{
public:
	DECLARE_CLASS (WindowsAlertBox, AlertBox)

	// AlertBox
	void closePlatform () override;
	IAsyncOperation* runAsyncPlatform () override;
};

} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// Alert::ButtonMapping
//************************************************************************************************

int Alert::ButtonMapping::getResultAtButtonIndex (int buttonIndex) const
{
	switch(buttonIndex)
	{
	case 0 : return defaultResult;
	case 1 : return alternateResult;
	case 2 : return otherResult;
	}
	return kUndefined;
}

//************************************************************************************************
// WindowsAlertBox
//************************************************************************************************

DEFINE_CLASS (WindowsAlertBox, AlertBox)
DEFINE_CLASS_UID (WindowsAlertBox, 0x9bf3ecb5, 0x5bb2, 0x4eb4, 0xaa, 0xac, 0x29, 0xaf, 0xf4, 0x66, 0x45, 0xa5) // ClassID::AlertBox

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsAlertBox::closePlatform ()
{
	ASSERT (platformHandle != nullptr)
	if(platformHandle != nullptr)
	{
		HWND hwnd = (HWND)platformHandle;

		// WM_CLOSE does not work when IDCANCEL is not present (Win10 ?) -> simulate a button click 
		if(::GetDlgItem (hwnd, IDCANCEL) == nullptr) 
		{
			static int ids [] = {IDOK, IDRETRY, IDYES, IDNO};
			for(int i = 0; i < ARRAY_COUNT (ids); i++)
				if(::GetDlgItem (hwnd, ids[i]))
				{
					::PostMessage(hwnd, WM_COMMAND, MAKEWPARAM (ids[i], BN_CLICKED), 0);
					return;
				}
		}
		::SendMessage (hwnd, WM_CLOSE, 0, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WindowsAlertBox::runAsyncPlatform ()
{
	int result = AlertHandler::showTaskDialog (*this);

	// return an AsyncOperation (already completed, since we ran modally)
	return AsyncOperation::createCompleted (result);
}

//************************************************************************************************
// Win32::AlertHandler
//************************************************************************************************

int AlertHandler::showTaskDialog (AlertBox& alert)
{
	StringChars textChars (alert.getText ()), titleChars (alert.getTitle ());
	StringChars firstChars (alert.getFirstButton ()), secondChars (alert.getSecondButton ()), thirdChars (alert.getThirdButton ());

	static const int kButtonIDStart = 100; ///< some distance from the predefined IDs, e.g. IDCANCEL (2) when user closes the window via [X] button

	TASKDIALOG_BUTTON buttons[3] = {0};
	buttons[0].nButtonID = kButtonIDStart;
	buttons[0].pszButtonText = firstChars;
	int buttonCount = 1;

	if(!alert.getSecondButton ().isEmpty ())
	{
		buttons[1].nButtonID = kButtonIDStart+1;
		buttons[1].pszButtonText = secondChars;
		buttonCount = 2;
	}

	if(!alert.getThirdButton ().isEmpty ())
	{
		buttons[2].nButtonID = kButtonIDStart+2;
		buttons[2].pszButtonText = thirdChars;
		buttonCount = 3;
	}

	IWindow* parentWindow = Desktop.getDialogParentWindow ();
		
	TASKDIALOGCONFIG config = {};
	config.cbSize = sizeof(TASKDIALOGCONFIG);
	config.hwndParent = parentWindow ? (HWND)parentWindow->getSystemWindow () : NULL;
	config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION|TDF_SIZE_TO_CONTENT|TDF_POSITION_RELATIVE_TO_WINDOW;
	config.hInstance = g_hMainInstance;
	config.pszWindowTitle = titleChars;

	// Icon
	if(alert.getAlertType () != Alert::kUndefined)
	{
		switch(alert.getAlertType ())
		{
		case Alert::kInformation :
			config.pszMainIcon = TD_INFORMATION_ICON; 
			break;
		case Alert::kWarning : 
			config.pszMainIcon = TD_WARNING_ICON; 
			break;
		default :
			config.pszMainIcon = TD_ERROR_ICON; 
			break;
		}
	}
	else
	{
		config.hMainIcon = ::LoadIcon (g_hMainInstance, MAKEINTRESOURCE (1)); // app icon
		config.dwFlags |= TDF_USE_HICON_MAIN;
	}
	
	//config.pszMainInstruction = textChars;
	config.pszContent = textChars;
	config.cButtons = buttonCount;
	config.pButtons = buttons;
	config.nDefaultButton = kButtonIDStart;
	config.pfCallback = taskDialogCallback;
	config.lpCallbackData = (LONG_PTR)&alert;

	int buttonId = 0;
	AutoPtr<ModalSystemWindow> systemWindow = NEW ModalSystemWindow; // disable other windows
	HRESULT hr = ::TaskDialogIndirect (&config, &buttonId, nullptr, nullptr);
	systemWindow.release ();
	ASSERT (SUCCEEDED (hr))

	// translate button ID to alert result expected by caller
	int result = Alert::kUndefined;
	if(buttonId >= kButtonIDStart)
		result = alert.getButtonResult (buttonId - kButtonIDStart);
	else
	{
		// dialog was canceled via Escape key or window close button
		if(buttonId == IDCANCEL)
		{
			if(alert.isUsingCustomButtonResults ())
				result = Alert::kEscapePressed;
			else
				result = Alert::kCancel;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CALLBACK AlertHandler::taskDialogCallback (HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	AlertBox* alert = reinterpret_cast<AlertBox*> (dwRefData);
	ASSERT (alert != nullptr)

	switch(uNotification)
	{
	case TDN_CREATED :
		alert->setPlatformHandle (hwnd);
		::SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
		break;

	case TDN_DESTROYED :
		alert->setPlatformHandle (NULL);
		break;
	}

	return S_OK;
}
