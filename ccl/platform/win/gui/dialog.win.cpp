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
// Filename    : ccl/platform/win/gui/dialog.win.cpp
// Description : Platform-specific Dialog implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/controls/editbox.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/win/gui/windowhelper.h"
#include "ccl/platform/win/gui/windowclasses.h"
#include "ccl/platform/win/gui/oledragndrop.h"
#include "ccl/platform/win/gui/touchhelper.h"
#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/gui/accessibility.win.h"

#include "ccl/public/cclversion.h"

// used in gui.win.cpp
void* DialogWindowClassProc = nullptr;
LRESULT CALLBACK CCLDialogWindowClassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// internal
static INT_PTR CALLBACK CCLDialogProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK CCLMouseHook (int nCode, WPARAM wParam, LPARAM lParam);
static HHOOK gActiveMouseHook = NULL;
static bool mustRestoreDpiHostingBehavior = false;

using namespace CCL;

//************************************************************************************************
// Dialog
//************************************************************************************************

IAsyncOperation* Dialog::showPlatformDialog (IWindow* parent)
{
	struct CCLDialogTemplate
	{
		DLGTEMPLATE base;
		WORD extra[100]; // should be enough
	};

	CCLDialogTemplate t = {0};
	t.base.style = WS_POPUP|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	if(needsLayeredRenderTarget ())
	{
		t.base.dwExtendedStyle |= WS_EX_LAYERED;
		setLayeredRenderTarget (true);
	}
	else
	{
		if(style.isCustomStyle (Styles::kWindowAppearanceTitleBar))
			t.base.style |= WS_CAPTION|WS_SYSMENU; //Note: DS_MODALFRAME hides the icon
		if(style.isCustomStyle (Styles::kWindowBehaviorSizable))
			t.base.style |= WS_SIZEBOX;
	}

	//t.base.dwExtendedStyle |= WS_EX_TOPMOST;

	if(style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) && !needsLayeredRenderTarget ())
		style.setCustomStyle (Styles::kWindowAppearanceDropShadow);

	if(style.isCustomStyle (Styles::kWindowAppearanceDropShadow))
		wcscpy ((WCHAR*)&t.extra[1], Win32::kShadowDialogClass);
	else
		wcscpy ((WCHAR*)&t.extra[1], Win32::kDialogWindowClass);

	// prepare for foreign views that aren't DPI-aware (Windows 10 1803 and later)
	if(style.isCustomStyle (Styles::kWindowBehaviorPluginViewHost) && Win32::gDpiInfo.canSwitchDpiHostingBehavior ())
	{
		if(Win32::gDpiInfo.switchToDpiHostingBehavior (Win32::kDpiHostingMixed))
			mustRestoreDpiHostingBehavior = true;
	}

	Win32Dialog::beginModalMode (this, true);
	HWND hwndParent = parent ? (HWND)parent->getSystemWindow () : nullptr;
	

	DialogBoxIndirectParam (g_hMainInstance, (DLGTEMPLATE*)&t, hwndParent, CCLDialogProc, (LPARAM)this);
	
	Win32Dialog::beginModalMode (this, false); // already done on WM_CLOSE, kept here to be safe

	// set focus back to parent window as closing the dialog while the parent was still disabled may have enabled
	// another window, see https://docs.microsoft.com/windows/win32/api/winuser/nf-winuser-enablewindow#remarks
	// (should not be necessary anymore, as we re-enable our windows on WM_CLOSE)
	if(hwndParent)
	{
		::SetForegroundWindow (hwndParent);
		::SetFocus (hwndParent);
	}

	Desktop.removeWindow (this);

	// return an AsyncOperation (already completed, since we ran modally)
	return AsyncOperation::createCompleted (dialogResult);
}

//************************************************************************************************
// Win32Dialog
//************************************************************************************************

void Win32Dialog::beginModalMode (IWindow* dialog, bool state)
{
	if(Desktop.getStackDepth (kDialogLayer) != 1)
		return;

	bool enabled = state == false;
	for(int i = 0, count = Desktop.countWindows (); i < count; i++)
	{
		IWindow* window = Desktop.getWindow (i);
		if(window != dialog)
		{
			::EnableWindow ((HWND)window->getSystemWindow (), enabled);
			::SendMessage ((HWND)window->getSystemWindow (), WM_NCACTIVATE, enabled, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32Dialog::Win32Dialog (const Rect& size, StyleRef style, StringRef title)
: Win32Window (size, style, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult Win32Dialog::handleEvent (SystemEvent& e)
{
	switch(e.msg)
	{
	case WM_INITDIALOG :
		{
			handle = e.hwnd;

			if(mustRestoreDpiHostingBehavior)
			{
				Win32::gDpiInfo.switchToDpiHostingBehavior (Win32::kDpiHostingDefault);
				mustRestoreDpiHostingBehavior = false;
			}

			const Win32::ScreenInformation& screen = Win32::gScreens.screenForCoordRect (size);
			savedDpiFactor = screen.scaleFactor;

			updateBackgroundColor ();

			setSize (size);
			setTitle (title);
			attached (this);
			
			initSize ();
			((Dialog*)this)->initFocusView (); // after init size, native controls might loose focus on move

			Win32::DropTarget* dropTarget = NEW Win32::DropTarget (this);
			::RegisterDragDrop ((HWND)handle, dropTarget); // calls addRef
			dropTarget->release ();

			Win32::TouchHelper::prepareWindow (*this);

			// don't steal the focus from a view that creates a NativeTextControl (EditBox, ComboBox)
			UnknownPtr<ITextParamProvider> textParamProvider (static_cast<IObject*> (getFocusView ()));
			if(textParamProvider)
				return nullptr;

			::SetFocus ((HWND)handle); // for mouse wheel
		}
		return nullptr;

	case WM_ERASEBKGND : // background is erased when painting
		return (EventResult)TRUE;

	case WM_PAINT :
		Win32Window::handleEvent (e);
		return (EventResult)TRUE; // DialogProc must return true if message was processed!
/*
	case WM_COMMAND : // sent by [Return] and [Esc]
		{
			WORD id = LOWORD ((WPARAM)e.wParam);
			if(id == IDOK || id == IDCANCEL)
			{
				dialogResult = id == IDOK ? DialogResult::kOkay : DialogResult::kCancel;
				close ();
				return (EventResult)TRUE;
			}
		} break;
*/
	case WM_GETDLGCODE :
		if(MSG* msg = (MSG*)e.lParam)
		{
			if(msg->message == WM_KEYDOWN ||
				msg->message == WM_KEYUP)
			{
				// these keys would normally be swallowed by the Windows function IsDialogMessage ()
				switch(msg->wParam)
				{
				case VK_TAB:
				case VK_RETURN:
				case VK_ESCAPE:
				case VK_LEFT:
				case VK_UP:
				case VK_RIGHT:
				case VK_DOWN:
					{
						static DWORD lastMessageTime = 0;
						if(msg->time != lastMessageTime)
						{
							CCL_PRINTF ("  pass msg from WM_GETDLGCODE: msg 0x%x, hwnd 0x%x (0x%x, 0x%x)\n", msg->message, msg->hwnd, msg->wParam, msg->lParam)

							lastMessageTime = msg->time;
							CCL::SystemEvent e (msg->hwnd, msg->message, (void*)msg->wParam, (void*)msg->lParam);
							handleEvent (e);
							return (EventResult)DLGC_WANTMESSAGE;
						}
						else
						{
							CCL_PRINTF ("  ignoring WM_GETDLGCODE: msg 0x%x, hwnd 0x%x (0x%x, 0x%x)\n", msg->message, msg->hwnd, msg->wParam, msg->lParam)
						}
					}
				}
			}
		}
		break;
/*
	case WM_SYSKEYDOWN :
		{
			WPARAM wParam = (WPARAM)e.wParam;
			if(wParam == VK_ESCAPE || wParam == VK_RETURN)
			{
				bool canceled = wParam == VK_ESCAPE;
				dialogResult = canceled ? DialogResult::kCancel : DialogResult::kOkay;
				::EndDialog ((HWND)handle, canceled ? IDCANCEL : IDOK);
				return 0;
			}
		}
		break;
*/
	case WM_ACTIVATE :
		onActivate (e.wParam != WA_INACTIVE);
		return (EventResult)TRUE; // DialogProc must return true if message was processed!

	case WM_CLOSE :
		if(onClose())
		{
			Dialog* dialog = (Dialog*)this;
			if(dialog->getDialogResult () == DialogResult::kNone) // maybe already set!
				dialog->setDialogResult (DialogResult::kCancel);

			ASSERT (!inCloseEvent)
			setInCloseEvent (true);

			// Re-enable our other windows. See counterpart in Dialog::showPlatformDialog. Doing this after the blocking DialogBoxIndirectParam there
			// or on WM_DESTROY can be too late and lead to flicker when another application has already been activated.
			// see https://docs.microsoft.com/windows/win32/api/winuser/nf-winuser-enablewindow#remarks
			// "the application must enable the main window before destroying the dialog box. Otherwise, another window will receive the keyboard focus and be activated"
			Win32Dialog::beginModalMode (this, false);

			bool canceled = dialog->getDialogResult () == DialogResult::kCancel;
			::EndDialog ((HWND)handle, canceled ? IDCANCEL : IDOK);

			setInCloseEvent (false);
		}
		return nullptr;

	case WM_DESTROY :
		setInDestroyEvent (true);
		::RevokeDragDrop ((HWND)e.hwnd); // release IDropTarget
		::SetWindowLongPtr ((HWND)e.hwnd, GWLP_USERDATA, 0);

		// From MSDN: When a window that previously returned providers has been destroyed, notify UI Automation.
		if(AccessibilityManager::isEnabled () && accessibilityProvider)
		{
			accessibilityProvider->disconnect ();
			safe_release (accessibilityProvider);
			::UiaReturnRawElementProvider ((HWND)e.hwnd, 0, 0, nullptr);
		}

		removed (nullptr);
		onDestroy ();
		handle = nullptr;
		// Note: do NOT destroy the Dialog object here!
		return nullptr;
		
	case WM_POINTERDOWN :
	case WM_POINTERUPDATE :
	case WM_POINTERUP :
		return Win32Window::handleEvent (e) == nullptr ? (EventResult)TRUE : (EventResult)FALSE;
	}

	return Win32Window::handleEvent (e);
}

//************************************************************************************************
// PopupSelectorWindow
//************************************************************************************************

IAsyncOperation* PopupSelectorWindow::showPlatformDialog (IWindow* parent)
{
	HHOOK previousHook = gActiveMouseHook;
	if(previousHook == NULL)
		gActiveMouseHook = ::SetWindowsHookEx (WH_MOUSE, (HOOKPROC)CCLMouseHook, g_hMainInstance, ::GetCurrentThreadId ());

	ObservedPtr<IWindow> parentWindow (parent);
	IAsyncOperation* operation = Dialog::showPlatformDialog (parent);

	if(previousHook == NULL)
	{
		::UnhookWindowsHookEx (gActiveMouseHook);
		gActiveMouseHook = NULL;
	}

	// workaround after popup closed: "re-focus" EditBox
	if(parentWindow)
		if(EditBox* editBox = unknown_cast<EditBox> (parentWindow->getFocusIView ()))
		{
			editBox->onFocus (FocusEvent (FocusEvent::kKillFocus));
			editBox->onFocus (FocusEvent (FocusEvent::kSetFocus));
		}

	return operation;
}

//************************************************************************************************
// PopupSelectorWindow
//************************************************************************************************

void PopupSelectorWindow::onActivate (bool state)
{
	CCL_PRINTF ("PopupSelectorWindow::onActivate %d\n", state)

	if(state)
	{
		// popup window activated: but let the owner window look like activated
		if(Window* w = unknown_cast<Window> (parentWindow))
			Win32Window::cast (w)->sendNCActivate ();
	}
	SuperClass::onActivate (state);
}

//************************************************************************************************
// CCLDialogWindowClassProc
//************************************************************************************************

LRESULT CALLBACK CCLDialogWindowClassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_NCCREATE)
		Win32::gDpiInfo.enableNonClientDpiScaling (hwnd);

	ASSERT (DialogWindowClassProc != nullptr)

	// When asked for the accessibility provider, bypass the default dialog procedure and return our custom provider
	if(msg == WM_GETOBJECT && DWORD(lParam) == UiaRootObjectId)
		return ::CallWindowProc ((WNDPROC)CCLDialogProc, hwnd, msg, wParam, lParam);
	// For all other messages, call the default dialog procedure, which in turn calls our custom dialog procedure
	else
		return ::CallWindowProc ((WNDPROC)DialogWindowClassProc, hwnd, msg, wParam, lParam);
}

//************************************************************************************************
// CCLDialogProc
//************************************************************************************************

INT_PTR CALLBACK CCLDialogProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Dialog* dialog = (Dialog*)::GetWindowLongPtr (hwnd, GWLP_USERDATA);

	if(msg == WM_INITDIALOG)
	{
		dialog = (Dialog*)lParam;
		::SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR)dialog);
	}

	if(dialog)
	{
		CCL::SystemEvent e (hwnd, msg, (void*)wParam, (void*)lParam);
		LRESULT result = (LRESULT)dialog->handleEvent (e);
		if(e.wasHandled ())
			return result;
	}
	return 0;
}

//************************************************************************************************
// CCLMouseHook
//************************************************************************************************

LRESULT CALLBACK CCLMouseHook (int nCode, WPARAM wParam, LPARAM lParam)
{
	auto shouldIngoreSpyWindow = [] (IWindow* clickedWindow, IWindow* topModalWindow) -> bool
	{
		// ignore click in spy (don't close popup)
		Window* w = unknown_cast<Window> (clickedWindow);
		if(w && w->getTitle () == CCL_SPY_NAME)
		{
			// but do close popup in the spy itself
			PopupSelectorWindow* popup = unknown_cast<PopupSelectorWindow> (topModalWindow);
			return !popup || popup->getParentWindow () != clickedWindow;
		}
		return false;
	};

	if(nCode == HC_ACTION && ((wParam == WM_LBUTTONDOWN) || (wParam == WM_NCLBUTTONDOWN) || (wParam == WM_RBUTTONDOWN) || (wParam == WM_NCRBUTTONDOWN)))
	{
		CCL_PRINTF ("Mouse down hook\n")

		// @@DPI_AWARENESS_CONTEXT: make sure calculations happen in physical pixels!
		Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextDefault);

		PMOUSEHOOKSTRUCT mhs = (PMOUSEHOOKSTRUCT)lParam;
		Point screenPos (mhs->pt.x, mhs->pt.y);
		Win32::gScreens->toCoordPoint (screenPos);

		IWindow* window = Desktop.findWindow (screenPos);

		IWindow* topModal = Desktop.getTopWindow (kPopupLayer);

		// don't steal clicks from system windows (e.g. Alert)
		// note: findWindow does not find modal system windows, as they are added to the desktop without size information
		if(unknown_cast<SystemWindow> (topModal))
			window = topModal;

		if(window != topModal && topModal && !shouldIngoreSpyWindow (window, topModal))
		{
			bool isTouch = Win32::TouchHelper::isButtonMessageFromTouch (mhs->dwExtraInfo);
			bool wasHandledAsTouch = isTouch && Win32::TouchHelper::didHandleCurrentMessage ();

			PopupSelectorWindow* popup = unknown_cast<PopupSelectorWindow> (topModal);
			if(popup && !wasHandledAsTouch)
			{
				bool swallow = popup->onPopupDeactivated ();
				if(swallow == false && window) // repeat event that caused the popup selector to close
				{
					if(isTouch)
						return TRUE; // but not for touch input (sending only the "down" event can cause a stuck mouse handler)

					UINT mouseMsg = (wParam == WM_LBUTTONDOWN || wParam == WM_NCLBUTTONDOWN) ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
					WPARAM mouseWParam = mouseMsg == WM_LBUTTONDOWN ? MK_LBUTTON : MK_RBUTTON;
					POINT p = mhs->pt;
					::ScreenToClient ((HWND)window->getSystemWindow (), &p);
					LPARAM mouseLParam = MAKELPARAM (p.x, p.y);
					::PostMessage ((HWND)window->getSystemWindow (), mouseMsg, mouseWParam, mouseLParam);
				}
				return TRUE;
			}

			// If the click goes into a window that does not belong to us, we pass
			if(mhs->hwnd == NULL || ::GetWindowLongPtr (mhs->hwnd, GWLP_USERDATA) != NULL)
				return TRUE;
		}
	}
    return ::CallNextHookEx (gActiveMouseHook, nCode, wParam, lParam);
}
