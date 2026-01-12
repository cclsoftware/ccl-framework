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
// Filename    : ccl/platform/win/gui/gui.win.cpp
// Description : Platform-specific GUI implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/gui.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/platform/win/gui/win32graphics.h"
#include "ccl/platform/win/gui/windowhelper.h"
#include "ccl/platform/win/gui/windowclasses.h"
#include "ccl/platform/win/gui/touchhelper.h"
#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/gui/exceptionhandler.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include <commctrl.h>
#include <shlwapi.h>
#include <powrprof.h>

#pragma comment (lib, "Comctl32.lib")

namespace CCL {

//************************************************************************************************
// MessageWindowHandler
//************************************************************************************************

class MessageWindowHandler: public Object,
							public SystemEventHandler
{
public:
	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;
};

//************************************************************************************************
// WindowsUserInterface
//************************************************************************************************

class WindowsUserInterface: public UserInterface
{
public:
	WindowsUserInterface ();

	// UserInterface
	bool finishStartup () override;
	bool startupPlatform (ModuleRef module) override;
	void shutdownPlatform () override;
	void quitPlatform () override;
	tbool CCL_API activateApplication (tbool startupMode, ArgsRef args) override;

	int CCL_API runEventLoop () override;
	tbool CCL_API flushUpdates (tbool wait = true) override;
	tbool CCL_API flushWindowEvents (IWindow* window) override;

	void CCL_API getKeyState (KeyState& keys) const override;
	tresult CCL_API detectKeyPressed (VirtualKey vkey, uchar character) const override;

	Point& CCL_API getMousePosition (Point& pos) const override;
	void CCL_API setMousePosition (const Point& pos) override;
	bool detectDrag (View* view, const Point& where) override;
	bool detectDoubleClick (View* view, const Point& where) override;
	void tryDoubleClick () override;
	void resetCursor () override;

	void updateNativeUserActivity () override;

	void processMouseMove (bool fromTimer) override;
	tresult CCL_API simulateEvent (const GUIEvent& event) override;
	ITimer* CCL_API createTimer (unsigned int period) const override;

	CLASS_INTERFACES (UserInterface)

protected:
	AutoPtr<MessageWindowHandler> messageWindowHandler;

	bool inDoubleClickDetection;
	int syntheticMouseUpTime;
	HPOWERNOTIFY hPowerNotify;

	bool nextEvent ();

	static ULONG CALLBACK powerManagementCallback (PVOID context, ULONG type, PVOID setting);
	void onPowerManagementEvent (ULONG type, PVOID setting);

	using SuperClass = UserInterface;
};

//************************************************************************************************
// WindowsTimer
//************************************************************************************************

class WindowsTimer: public SystemTimer
{
public:
	WindowsTimer (unsigned int period);
	~WindowsTimer ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsUserInterface windowsGUI;
UserInterface& GUI = windowsGUI;

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CCLWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // window.win.cpp
LRESULT CALLBACK CCLDialogWindowClassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // dialog.win.cpp
LRESULT CALLBACK CCLMessageWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // window.win.cpp
LRESULT CALLBACK CCLTransparentWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // transparentwindow.win.cpp
void InitIEBrowserEmulationVersion (); // webbrowserview.win.cpp

HINSTANCE g_hMainInstance = nullptr;
HWND g_hMessageWindow = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {
namespace Win32 {

// user can swap primary & secondary mouse buttons, but GetAsyncKeyState always checks the physical buttons
inline int getLogical_LBUTTON () { return GetSystemMetrics (SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON; }
inline int getLogical_RBUTTON () { return GetSystemMetrics (SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON; }

static int lastDoubleTapTime = 0;

} // namespace Win32
} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// WindowsUserInterface
//************************************************************************************************

ULONG CALLBACK WindowsUserInterface::powerManagementCallback (PVOID context, ULONG type, PVOID setting)
{
	static_cast<WindowsUserInterface*> (context)->onPowerManagementEvent (type, setting);
	return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsUserInterface::WindowsUserInterface ()
: inDoubleClickDetection (false),
  syntheticMouseUpTime (0),
  hPowerNotify (NULL)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsUserInterface::queryInterface (UIDRef iid, void** ptr)
{
#if EXCEPTION_HANDLER_ENABLED
	if(iid == ccl_iid<IDiagnosticDataProvider> ())
		return Win32::ExceptionHandler::instance ().queryInterface (iid, ptr);
#endif
	return UserInterface::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsUserInterface::finishStartup ()
{
	IApplication* application = getApplication ();
	if(application && g_hMessageWindow)
		::SetWindowText (g_hMessageWindow, StringChars (application->getApplicationTitle ())); // set title for Win32::WindowFinder

	return SuperClass::finishStartup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsUserInterface::startupPlatform (ModuleRef module)
{
	bool ownProcess = module == System::GetMainModuleRef ();
	g_hMainInstance = (HINSTANCE)module;
	if(!g_hMainInstance)
		g_hMainInstance = (HINSTANCE)System::GetCurrentModuleRef ();

#if EXCEPTION_HANDLER_ENABLED
	// install exception handler for main application
	if(module)
		Win32::ExceptionHandler::instance ().install ();
#endif

	// DPI-Awareness
	if(ownProcess == true)
		gDpiInfo.init (DpiInfo::kSetProcessDpiAwareness);
	else
		gDpiInfo.init (DpiInfo::kUseProcessDpiAwareness);

	gScreens.update ();

	if(gDpiInfo.isPerMonitorDpi ())
		Bitmap::setResolutionNamingMode (Bitmap::kMultiResolution);
	else
	{
		bool highDpi = Bitmap::isHighResolutionScaling (gDpiInfo.getSystemDpiFactor ());
		Bitmap::setResolutionNamingMode (highDpi ? Bitmap::kHighResolution : Bitmap::kStandardResolution);
	}

	// Initialize OLE for Drag'n'Drop
	::OleInitialize (nullptr);

	WNDCLASS wc;
	wc.style = 0;//CS_DBLCLKS;
	wc.lpfnWndProc = CCLWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(void*);   // Reserve for extra storage
	wc.hInstance = g_hMainInstance;
	wc.hIcon = ::LoadIcon (g_hMainInstance, MAKEINTRESOURCE (1));
	wc.hCursor = nullptr;//::LoadCursor (0, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = Win32::kDefaultWindowClass;
	::RegisterClass (&wc);

	// extra class for shaded windows
	wc.lpszClassName = Win32::kShadowWindowClass;
	::RegisterClass (&wc);

	// dialog class
	::GetClassInfo (nullptr, L"#32770", &wc);
	extern void* DialogWindowClassProc; // dialog.win.cpp
	DialogWindowClassProc = wc.lpfnWndProc;
	wc.lpfnWndProc = CCLDialogWindowClassProc;
	wc.hInstance = g_hMainInstance;
	wc.hIcon = ::LoadIcon (g_hMainInstance, MAKEINTRESOURCE (1));
	wc.style &= ~CS_DBLCLKS; // <-- we implement our own double-click handling!
	wc.lpszClassName = Win32::kDialogWindowClass;
	::RegisterClass (&wc);

	// dialog class with shadow style
	wc.style |= CS_DROPSHADOW;
	wc.lpszClassName = Win32::kShadowDialogClass;
	::RegisterClass (&wc);

	// transparent window class
	::memset (&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = CCLTransparentWindowProc;
	wc.hInstance = g_hMainInstance;
	wc.lpszClassName = Win32::kTransparentWindowClass;
	::RegisterClass (&wc);

	// message-only window class
	::GetClassInfo (nullptr, L"Message", &wc);
	wc.hInstance = g_hMainInstance;
	wc.lpfnWndProc = CCLMessageWindowProc;
	wc.lpszClassName = Win32::kMessageWindowClass;
	::RegisterClass (&wc);

	messageWindowHandler = NEW MessageWindowHandler;
	g_hMessageWindow = Win32::CreateMessageWindow (messageWindowHandler);

	::InitCommonControls ();

	setTooltipDelay (::GetDoubleClickTime ());

	bool disableWindowGhosting = false;
	Configuration::Registry::instance ().getValue (disableWindowGhosting, "CCL.Win32", "DisableWindowGhosting");
	if(disableWindowGhosting && ownProcess)
		::DisableProcessWindowsGhosting ();

	TouchHelper::onPlatformStarted (ownProcess);

	if(ownProcess == true)
	{
		InitIEBrowserEmulationVersion ();

		// register for system suspend/resume notifications
		DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS dnsp = {powerManagementCallback, this};
		hPowerNotify = ::RegisterSuspendResumeNotification (&dnsp, DEVICE_NOTIFY_CALLBACK);
		ASSERT (hPowerNotify)
	}

	auto isWindows11orGreater = []
	{
		OSVERSIONINFO osvi = {};
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		::GetVersionEx (&osvi);

		return osvi.dwMajorVersion >= 10 && osvi.dwBuildNumber >= 22000;
	};

	customMenuBarSupported = roundedWindowCornersSupported = isWindows11orGreater ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUserInterface::shutdownPlatform ()
{
	if(hPowerNotify)
		::UnregisterSuspendResumeNotification (hPowerNotify),
		hPowerNotify = NULL;
	
	::DestroyWindow (g_hMessageWindow);
	g_hMessageWindow = nullptr;

	::UnregisterClass (Win32::kDefaultWindowClass, g_hMainInstance);
	::UnregisterClass (Win32::kShadowWindowClass, g_hMainInstance);
	::UnregisterClass (Win32::kDialogWindowClass, g_hMainInstance);
	::UnregisterClass (Win32::kShadowDialogClass, g_hMainInstance);
	::UnregisterClass (Win32::kTransparentWindowClass, g_hMainInstance);
	::UnregisterClass (Win32::kMessageWindowClass, g_hMainInstance);

	// Close OLE
	::OleUninitialize ();

#if EXCEPTION_HANDLER_ENABLED
	Win32::ExceptionHandler::instance ().uninstall ();
	Win32::ExceptionHandler::cleanupInstance ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsUserInterface::runEventLoop ()
{
	if(!finishStartup ())
		return kExitError;
	
	if(appProvider)
		if(!appProvider->onInit ())
			return exitCode;

	onAppStateChanged (IApplication::kUIInitialized);

	if(quitDone) // quit has been requested during startup
	{
		// see also: UserInterface::quit()
		Desktop.closeAll ();
		quitPlatform ();
	}

	ScopedVar<bool> scope (eventLoopRunning, true);
	while(nextEvent ());
	return exitCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUserInterface::flushUpdates (tbool wait)
{
	if(wait) // save some cpu time...
	{
		DWORD diff = ::GetTickCount () - lastUpdateTime;
		if(diff < kUpdateDelay)
			::Sleep (kUpdateDelay - diff);
	}

	lastUpdateTime = ::GetTickCount ();

	auto handleQuitMessage = [this](const MSG& msg)
	{
		if(msg.message == WM_QUIT)
		{
			::PostQuitMessage (exitCode);
			return true;
		}
		return false;
	};

	MSG msg;

	if(::PeekMessage (&msg, nullptr, WM_PAINT, WM_PAINT, PM_REMOVE))
	{
		if(handleQuitMessage (msg))
			return true;

		::DispatchMessage (&msg);
	}

	static const int kMaxTimers = 10;
	int timerCount = 0;
	while(::PeekMessage (&msg, nullptr, WM_TIMER, WM_TIMER, PM_REMOVE) && ++timerCount < kMaxTimers)
	{
		if(handleQuitMessage (msg))
			return true;

		::DispatchMessage (&msg);
	}

	if(Desktop.isProgressMode ())
	{
		if(::PeekMessage (&msg, nullptr, WM_SYSCOMMAND, WM_SYSCOMMAND, PM_REMOVE))
		{
			if(handleQuitMessage (msg))
				return true;

			// handle the restore message in case window was hidden
			// this drops all system commands other then SC_RESTORE during a running progress!
			if(msg.message == WM_SYSCOMMAND && msg.wParam == SC_RESTORE)
				::DispatchMessage (&msg);
		}
	}

	::PeekMessage (&msg, nullptr, 0, 0, PM_NOREMOVE); // update timestamp of oldest message to avoid Windows setting the application to 'not responding'

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUserInterface::flushWindowEvents (IWindow* _window)
{
	Window* window = unknown_cast<Window> (_window);
	ASSERT (window != nullptr)
	if(window == nullptr || window->isInDestroyEvent ())
		return false;

	HWND hwnd = (HWND)window->getSystemWindow ();
	ASSERT (hwnd != nullptr)

	MSG msg;
	while(true)
	{
		if(::PeekMessage (&msg, hwnd, 0, 0, PM_REMOVE))
		{
			if(msg.message >= WM_LBUTTONDOWN && msg.message <= WM_MBUTTONUP && msg.hwnd && msg.hwnd != hwnd)
			{
				Point pos;
				getMousePosition (pos);
				window->screenToClient (pos);
				if(window->isInsideClient (pos))
				{
					if(msg.message == WM_LBUTTONDOWN)
					{
						MouseEvent event (MouseEvent::kMouseDown, pos, KeyState (KeyState::kLButton), 0);
						window->onMouseDown (event);
					}
					else if(msg.message == WM_LBUTTONUP)
					{
						MouseEvent event (MouseEvent::kMouseUp, pos, KeyState (KeyState::kLButton), 0);
						window->onMouseUp (event);
					}
				}
				continue;
			}
			::DispatchMessage (&msg);
		}
		else
			break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsUserInterface::nextEvent ()
{
	TRY_MESSAGE
	{
		MSG msg;
		if(!::GetMessage (&msg, nullptr, 0, 0))
			return false;

		HWND toFocus = nullptr;

		if(msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
		{
			if(::IsWindowVisible (msg.hwnd))
			{
				// check if message target can be trusted or if extra handling is needed
				bool trustedTarget = false;
				if(Win32::GetWindowFromNativeHandle (msg.hwnd))
					trustedTarget = true;
				else
				{
					WCHAR className[32] = {0};
					::GetClassNameW (msg.hwnd, className, 32);
					if(StrStrI (className, L"Edit") != nullptr ||
						StrCmpI (className, L"Button") == 0 ||
						StrCmpI (className, L"Scrollbar") == 0 ||
						StrCmpI (className, L"Combobox") == 0 ||
						StrCmpI (className, L"ListBox") == 0 ||
						StrCmpI (className, L"Internet Explorer_Server") == 0)
						trustedTarget = true;
				}

				if(trustedTarget == false)
				{
					HWND cclParentHwnd = nullptr;
					HWND hwnd = msg.hwnd;
					while(hwnd && cclParentHwnd == nullptr)
					{
						if((::GetWindowLong (hwnd, GWL_STYLE) & WS_CHILD) != 0)
						{
							hwnd = ::GetParent (hwnd);
							if(hwnd && Win32::GetWindowFromNativeHandle (hwnd))
								cclParentHwnd = hwnd;
						}
						else
							hwnd = nullptr;
					}

					if(cclParentHwnd)
					{
						msg.hwnd = cclParentHwnd;

						if(msg.message == WM_KEYUP)
						{
							// on enter set the focus to the CCL window (assuming typing in a control has finished)
							KeyEvent key;
							VKey::fromSystemEvent (key, SystemEvent (nullptr, msg.message, (void*)msg.wParam, (void*)msg.lParam));
							if(key.vKey == VKey::kEnter || key.vKey == VKey::kReturn)
							{
								toFocus = cclParentHwnd;
							}
						}
					}
				}
			}
		}

		::TranslateMessage (&msg);
		::DispatchMessage (&msg);

		if(toFocus)
			::SetFocus (toFocus);
	}
	EXCEPT_MESSAGE
	{}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUserInterface::onPowerManagementEvent (ULONG type, PVOID setting)
{
	if(type == PBT_APMSUSPEND)
	{
		onAppStateChanged (IApplication::kAppSuspended);
	}
	else if(type == PBT_APMRESUMESUSPEND)
	{
		onAppStateChanged (IApplication::kAppResumed);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUserInterface::quitPlatform ()
{
	ASSERT (System::IsInMainThread ())
	::PostQuitMessage (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsUserInterface::activateApplication (tbool startupMode, ArgsRef args)
{
	IApplication* application = getApplication ();
	if(!application)
		return false;

	return Win32::ActivateApplication (application, startupMode, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsUserInterface::detectKeyPressed (VirtualKey vkey, uchar character) const
{
	if(vkey != VKey::kUnknown)
	{
		if(::GetAsyncKeyState (VKey::toSystemKey (vkey)) & 0x8000)
			return kResultTrue;

		return kResultFalse;
	}
	else
	{
		// ASCII values of uppercase letters and digits equal their platform virtual-key codes
		character = Unicode::toUppercase (character);

		auto isAlpahaNumericKey = [&] (uchar character)
		{
			if((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9'))
				return true;

			// additional check if it's the character produced by any digit key with / without caps lock
			for(uchar c = '0'; c <= '9'; c++)
			{
				if(character == System::GetLocaleManager ().getCharacterOnKey (c, true)
					|| character == System::GetLocaleManager ().getCharacterOnKey (c, false))
					return true;
			}
			return false;
		};

		if(isAlpahaNumericKey (character))
		{
			if(::GetAsyncKeyState (character) & 0x8000)
				return kResultTrue;

			return kResultFalse;
		}
	}
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsUserInterface::getKeyState (KeyState& keys) const
{
	keys.keys = 0;

	if(::GetAsyncKeyState (Win32::getLogical_LBUTTON ()) & 0x8000)
		keys.keys |= KeyState::kLButton;
	if(::GetAsyncKeyState (VK_MBUTTON) & 0x8000)
		keys.keys |= KeyState::kMButton;
	if(::GetAsyncKeyState (Win32::getLogical_RBUTTON ()) & 0x8000)
		keys.keys |= KeyState::kRButton;

	if(isKeyPressed (VKey::kShift))
		keys.keys |= KeyState::kShift;
	if(isKeyPressed (VKey::kCommand))
		keys.keys |= KeyState::kCommand;
	if(isKeyPressed (VKey::kOption))
		keys.keys |= KeyState::kOption;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API WindowsUserInterface::getMousePosition (Point& pos) const
{
	POINT p;
	::GetCursorPos (&p);
	pos (p.x, p.y);
	Win32::gScreens->toCoordPoint (pos);
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsUserInterface::setMousePosition (const Point& _pos)
{
	Point pos (_pos);
	Win32::gScreens->toPixelPoint (pos);
	::SetCursorPos (pos.x, pos.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsUserInterface::detectDrag (View* view, const Point& where)
{
	CCL_PRINTLN ("UserInterface::detectDrag")

	Window* w = view ? view->getWindow () : nullptr;
	HWND hwnd = w ? (HWND)w->getSystemWindow () : nullptr;
	if(!hwnd)
		return false;

	Point where2 (where);
	view->clientToScreen (where2);
	Win32::gScreens->toPixelPoint (where2);

	POINT p = {where2.x, where2.y};
	bool wasMousePressed = (::GetAsyncKeyState (Win32::getLogical_LBUTTON ()) & 0x8000) != 0;
	bool dragDetected = (::DragDetect (hwnd, p) == TRUE);
	bool isMousePressed = (::GetAsyncKeyState (Win32::getLogical_LBUTTON ()) & 0x8000) != 0;

	if(dragDetected == false && wasMousePressed && isMousePressed == false)
	{
		// DragDetect swallows mouse up messages (which is not the case on MacOS).
		// When a mouse handler is created after detectDrag has returned false, this handler is not ended and runs without mouse being pressed.
		// So we add the mouse up event manually:
		where2 = where;
		view->clientToWindow (where2);
		Win32::gScreens->toPixelPoint (where2);
		PostMessage (hwnd, WM_LBUTTONUP, 0, MAKELONG (where2.x, where2.y));
		syntheticMouseUpTime = ::GetMessageTime ();
	}
	return dragDetected && isMousePressed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsUserInterface::detectDoubleClick (View* view, const Point& where)
{
	ScopedVar<bool> guard (inDoubleClickDetection, true);
	bool touchHandled = Win32::TouchHelper::didHandleCurrentMessage ();
	bool buttonHandledFromTouch = !touchHandled && Win32::TouchHelper::isButtonMessageFromTouch ();
	bool isTouch = touchHandled || buttonHandledFromTouch;
	CCL_PRINTF ("UserInterface::detectDoubleClick (%s) %s\n", isTouch ? "Touch" : "Mouse", buttonHandledFromTouch ? "as Button" : "" )

	if(touchHandled &&::GetMessageTime () == lastDoubleTapTime)
	{
		// ignore second touch of a double tap
		doubleClicked = kDoubleClickFalse;
		return false;
	}

	if(doubleClicked > kDoubleClickReset)
		return doubleClicked == kDoubleClickTrue;

	doubleClicked = kDoubleClickFalse;

	Point pos (where);
	if(view)
		view->clientToScreen (pos);

	Coord tolerance = isTouch ? 20 : 2;
	Rect clickRect (pos.x - tolerance, pos.y - tolerance, pos.x + tolerance, pos.y + tolerance);

	#if DEBUG_LOG
	DWORD dblClickTime = ::GetDoubleClickTime ();
	DWORD msgTime = ::GetMessageTime ();
	DWORD tickCount = ::GetTickCount ();
	CCL_PRINTF ("dblClickTime %d, msgTime %d, tickCount %d\n", dblClickTime, msgTime, tickCount)
	#endif

	DWORD maxTime = ::GetMessageTime () + ::GetDoubleClickTime ();
	while(::GetTickCount () < maxTime)
	{
		getMousePosition (pos);
		if(!clickRect.pointInside (pos))
			break;

		// deliver pointer messages on Windows 8, otherwise events are delayed too long
		MSG msg;
		if(touchHandled)
		{
			if(PeekMessage (&msg, nullptr, WM_POINTERDOWN, WM_POINTERDOWN, PM_REMOVE|PM_NOYIELD))
			{
				getMousePosition (pos);
				if(!clickRect.pointInside (pos))
					break;

				doubleClicked = kDoubleClickTrue;
				lastDoubleTapTime = msg.time;
				break;
			}
		}
		else
		{
			if(PeekMessage (&msg, nullptr, WM_NCPOINTERDOWN, WM_POINTERLEAVE, PM_REMOVE|PM_NOYIELD))
				::DispatchMessage (&msg);

			// remove a WM_LBUTTONUP that was posted in detectDrag, during the same mouse down event: it would cancel a mouse handler created after this double click
			if(syntheticMouseUpTime == ::GetMessageTime ())
				PeekMessage (&msg, nullptr, WM_LBUTTONUP, WM_LBUTTONUP, PM_REMOVE|PM_NOYIELD);

			if(PeekMessage (&msg, nullptr, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE|PM_NOYIELD))
			{
				getMousePosition (pos);
				if(!clickRect.pointInside (pos))
					break;

				doubleClicked = kDoubleClickTrue;
				break;
			}
		}

		flushUpdates ();
	}

	CCL_PRINTLN ("doubleclick end")
	return doubleClicked == kDoubleClickTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUserInterface::tryDoubleClick ()
{
	DWORD maxTime = ::GetMessageTime () + ::GetDoubleClickTime ();
	while(::GetTickCount () < maxTime)
	{
		MSG msg;
		if(PeekMessage (&msg, nullptr, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_NOYIELD))
		{
			doubleClicked = kDoubleClickPending;
			break;
		}
		flushUpdates ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUserInterface::resetCursor ()
{
	::SetCursor (::LoadCursor (nullptr, IDC_ARROW));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUserInterface::updateNativeUserActivity ()
{
	EXECUTION_STATE result = ::SetThreadExecutionState (ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED);
	ASSERT (result != NULL)

	// TODO: check if screensaver already running and try to kill it...
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUserInterface::processMouseMove (bool fromTimer)
{
	if(fromTimer && inDoubleClickDetection) // prevent mouse move events during double click detection
		return;

	Point mousePos;
	bool mousePosValid = false;
	bool mousePosChanged = false;
	auto getChangedMousePos = [&] (Point& p)
	{
		if(!mousePosValid)
		{
			getMousePosition (mousePos);
			mousePosValid = true;
			mousePosChanged = mousePos != lastMousePos;
		}
		p = mousePos;
		return mousePosChanged;
	};

	// in case we are not notified about mouse moves by the system
	// (this happens sometimes) we do it manually...
	// ...but only if there is no active mouse handler or popup menu!
	Window* mouseWindow = mouseView ? mouseView->getWindow () : nullptr;
	if(mouseWindow && !mouseWindow->getMouseHandler () && !mouseWindow->isInMenuLoop ())
	{
		Point p;
		if(getChangedMousePos (p))
		{
			Window* windowUnderMouse = unknown_cast<Window> (Desktop.findWindow (p));
			if(windowUnderMouse != mouseWindow)
			{
				mouseView->onMouseLeave (MouseEvent (MouseEvent::kMouseLeave));
				mouseView = nullptr;
				setCursor (nullptr);
				lastMousePos = p;
			}
			else
			{
				#if DEBUG_LOG
				static int counter = 0;
				Debugger::printf ("Manual move %d mouseWindow %x\n", counter++, (int)((int64)mouseWindow));
				#endif

				mouseWindow->screenToClient (p);
				KeyState keys;
				getKeyState (keys);
				onMouseMove (mouseWindow, MouseEvent (MouseEvent::kMouseMove, p, keys));
			}
		}
	}

	if(tooltipView)
	{
		Point p;
		if(getChangedMousePos (p))
		{
			// e.g. when mouse is over a (plug-in) child window, we don't get kMouseMove events, but must be able to
			// a) hide the tooltip popup
			// b) discard the tooltipView to prevent showing a tooltip (at wrong position) in onTimer
			if(Window* window = tooltipView->getWindow ())
			{
				window->screenToClient (p);

				KeyState keys;
				getKeyState (keys);
				handleTooltip (window, MouseEvent (MouseEvent::kMouseMove, p, keys));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsUserInterface::simulateEvent (const GUIEvent& event)
{
	if(auto mouseEvent = event.as<MouseEvent> ())
	{
		int type = 0;
		switch(mouseEvent->eventType)
		{
		case MouseEvent::kMouseDown :
			if(mouseEvent->keys.isSet (KeyState::kLButton))
				type = MOUSEEVENTF_LEFTDOWN;
			else if(mouseEvent->keys.isSet (KeyState::kRButton))
				type = MOUSEEVENTF_RIGHTDOWN;
			else if(mouseEvent->keys.isSet (KeyState::kMButton))
				type = MOUSEEVENTF_MIDDLEDOWN;
			break;

		case MouseEvent::kMouseMove :
			type = MOUSEEVENTF_MOVE;
			break;

		case MouseEvent::kMouseUp :
			if(mouseEvent->keys.isSet (KeyState::kLButton))
				type = MOUSEEVENTF_LEFTUP;
			else if(mouseEvent->keys.isSet (KeyState::kRButton))
				type = MOUSEEVENTF_RIGHTUP;
			else if(mouseEvent->keys.isSet (KeyState::kMButton))
				type = MOUSEEVENTF_MIDDLEUP;
			break;
		}

		ASSERT (type != 0)
		if(type == 0)
			return kResultInvalidArgument;

		Point screenSize (::GetSystemMetrics (SM_CXVIRTUALSCREEN), ::GetSystemMetrics (SM_CYVIRTUALSCREEN));
		Point screenPos (::GetSystemMetrics (SM_XVIRTUALSCREEN), ::GetSystemMetrics (SM_YVIRTUALSCREEN));
		if(screenSize.x > 1 && screenSize.y > 1)
		{
			Point whereInPixel (mouseEvent->where);
			gScreens->toPixelPoint (whereInPixel);

			INPUT ip = { 0 };
			ip.type = INPUT_MOUSE;
			ip.mi.dwFlags = type | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
			ip.mi.dx = (whereInPixel.x - screenPos.x) * 65535 / (screenSize.x - 1);	 // "normalized" coordinates
			ip.mi.dy = (whereInPixel.y - screenPos.y) * 65535 / (screenSize.y - 1);

			CCL_PRINTF ("simulateEvent %d (%d,%d) -> (%d, %d)\n", type, mouseEvent->where.x, mouseEvent->where.y, ip.mi.dx, ip.mi.dy)
			::SendInput (1, &ip, sizeof(INPUT));
		}
		return kResultOk;
	}
	else
		return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer* CCL_API WindowsUserInterface::createTimer (unsigned int period) const
{
	return NEW WindowsTimer (period);
}

//************************************************************************************************
// MessageWindowHandler
//************************************************************************************************

EventResult MessageWindowHandler::handleEvent (SystemEvent& e)
{
	EventResult result = nullptr;
	if(e.msg == WM_COPYDATA)
	{
		result = EventResult(LRESULT(Win32::HandleCopyData (GUI.getApplication (), static_cast<COPYDATASTRUCT*> (e.lParam))));
		e.notHandled = false;
	}
	else
		e.notHandled = true;
	return result;
}

//************************************************************************************************
// WindowsTimer
//************************************************************************************************

static void CALLBACK CCLTimerProc (HWND, UINT, UINT_PTR idEvent, DWORD)
{
	SystemTimer::trigger ((void*)idEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTimer::WindowsTimer (unsigned int period)
: SystemTimer (period)
{
	systemTimer = (void*)::SetTimer (nullptr, 0, period, CCLTimerProc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTimer::~WindowsTimer ()
{
	if(systemTimer)
		::KillTimer (nullptr, (UINT_PTR)systemTimer);
	systemTimer = nullptr;
}
