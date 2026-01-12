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
// Filename    : ccl/platform/win/gui/windowhelper.cpp
// Description : Win32 window helpers
//
//************************************************************************************************

#include "ccl/platform/win/gui/windowhelper.h"
#include "ccl/platform/win/gui/windowclasses.h"
#include "ccl/platform/win/gui/transparentwindow.win.h"

#include "ccl/gui/windows/desktop.h"

#include "ccl/public/gui/iapplication.h"

#include "ccl/main/cclargs.h"

#include "ccl/base/storage/configuration.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// Win32::WindowFinder
//************************************************************************************************

struct WindowFinder
{
	String title;
	const WCHAR* className;
	HWND hwndResult;

	WindowFinder (StringRef title, const WCHAR* className)
	: title (title),
	  className (className),
	  hwndResult (nullptr)
	{}

	static BOOL CALLBACK callback (HWND hwnd, LPARAM lParam)
	{
		WindowFinder* finder = (WindowFinder*)lParam;
		WCHAR charBuffer[1024] = {0};
		::GetClassName (hwnd, charBuffer, 1024);
		if(::wcscmp (charBuffer, finder->className) == 0)
		{
			::GetWindowText (hwnd, charBuffer, 1024);
			String windowTitle (charBuffer);
			if(windowTitle.contains (finder->title, false))
			{
				finder->hwndResult = hwnd;
				return FALSE; // stop enumeration
			}
		}
		return TRUE;
	}

	static HWND findTopLevelWindow (StringRef title)
	{
		WindowFinder finder (title, Win32::kDefaultWindowClass);
		::EnumWindows (callback, (LPARAM)&finder);
		return finder.hwndResult;
	}

	static HWND findMessageWindow (StringRef title)
	{
		// Note: message-only windows can't be enumerated, neither via EnumWindows() nor EnumThreadWindows()! 
		return ::FindWindowEx (HWND_MESSAGE, NULL, Win32::kMessageWindowClass, StringChars (title));
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void* GetPtrFromNativeHandle (HWND hwnd)
{
	// check handle
	if(hwnd == nullptr)
		return nullptr;

	// check process id
	DWORD processID = 0;
	::GetWindowThreadProcessId (hwnd, &processID);
	if(processID != ::GetCurrentProcessId ())
		return nullptr;

	// check class name
	wchar_t className[128] = {0};
	::GetClassName (hwnd, className, 127);
	if(::wcsncmp (className, CCL_WINDOW_CLASS_PREFIX, ::wcslen (CCL_WINDOW_CLASS_PREFIX)) != 0)
		return nullptr;

	if(::wcscmp (className, Win32::kTransparentWindowClass) == 0)
		return nullptr;

	return (void*)::GetWindowLongPtr (hwnd, GWLP_USERDATA);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* GetWindowFromNativeHandle (HWND hwnd)
{
	return reinterpret_cast<Window*> (GetPtrFromNativeHandle (hwnd));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HWND FindTopLevelWindow (HWND hwnd, bool onlyCCL)
{
	// find top-level parent in case it's a child window...
	while(hwnd != nullptr)
	{
		LONG style = ::GetWindowLong (hwnd, GWL_STYLE);
		if((style & WS_CHILD) == 0)
			break;

		HWND hwndParent = ::GetParent (hwnd);
		if(onlyCCL && !Win32::GetWindowFromNativeHandle (hwndParent))
			break;

		hwnd = hwndParent;
	}
	return hwnd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HWND CreateMessageWindow (SystemEventHandler* handler)
{
	return ::CreateWindow (Win32::kMessageWindowClass, L"", 0, 
						   0, 0, 0, 0, 
						   HWND_MESSAGE, nullptr, g_hMainInstance, 
						   handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SetAlwaysOnTop (HWND hwnd, bool state)
{
	::SetWindowPos (hwnd, state ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActivateApplication (IApplication* application, bool startupMode, ArgsRef args)
{
	bool activate = true;
	HWND hwnd = Win32::WindowFinder::findTopLevelWindow (application->getApplicationTitle ());
	if(!hwnd && startupMode)
	{	
		String altName;
		if(Configuration::Registry::instance ().getValue (altName, "CCL.Win32", "AltApplicationName"))
			hwnd = Win32::WindowFinder::findTopLevelWindow (altName);
	}
	if(!hwnd && startupMode)
	{
		// second try: find message window (see gui.win.cpp)
		hwnd = Win32::WindowFinder::findMessageWindow (application->getApplicationTitle ());
		activate = false;
	}

	if(!hwnd)
		return false;

	if(startupMode)
	{
		// transfer command line

		String commandLine;
		args.toString (commandLine);
		StringChars stringChars (commandLine);
		const uchar* stringData = stringChars;

		COPYDATASTRUCT data = {0};
		data.dwData = 'Cmdl';
		data.lpData = (void*)stringData;
		data.cbData = (commandLine.length () + 1) * sizeof(WCHAR);

		::SendMessage (hwnd, WM_COPYDATA, 0, (LPARAM)&data);
	}

	if(activate)
		::SetForegroundWindow (hwnd); // activate application window
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL HandleCopyData (IApplication* application, COPYDATASTRUCT* data)
{
	if(data->dwData == 'Cmdl') // Command Line
	{
		String commandLine;
		commandLine.append ((uchar*)data->lpData, data->cbData/sizeof(uchar));
		if(application && !commandLine.isEmpty ())
		{
			int argc = 0;
			LPWSTR* argv = ::CommandLineToArgvW (StringChars (commandLine), &argc);
			MutableArgumentList args (argc, argv);
			::LocalFree (argv);
			
			application->processCommandLine (args);
		}
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EnforceWindowOrder ()
{
	#if 0
	if(!Desktop.hasFullScreenWindow ())
	{
		// if there is no fullscreen window, we can exit early in some trivial cases
		if(Desktop.getTopWindow (kWindowLayerFloating) == 0 || Desktop.getTopWindow (kWindowLayerIntermediate) == 0)
			return;

		if(Desktop.getTopWindow (kDialogLayer) != 0 || Desktop.getTopWindow (kPopupLayer) != 0)
			return;
	}
	#endif

	Vector<HWND> windowHandles;
	Vector<HWND> fullscreenWindowHandles;
	for(int i = Desktop.countWindows () - 1; i >= 0; i--)
	{
		Window* w = unknown_cast<Window> (Desktop.getWindow (i));
		if(w->getLayer () == kWindowLayerBase)
			break;

		Vector<HWND>& handles = w->isFullscreen () ? fullscreenWindowHandles : windowHandles;

		IterForEachReverse (w->getTransparentWindows (), WindowsTransparentWindow, tw)
			handles.add ((HWND)tw->getNativeWindow ());
		EndFor

		handles.add ((HWND)w->getSystemWindow ());
	}

	// add fullscreen windows last (below others) to prevent e.g. dialogs getting lost below them
	windowHandles.addAll (fullscreenWindowHandles);

	HWND hwndAfter = nullptr;
	HDWP hdwp = ::BeginDeferWindowPos (24);
	VectorForEach (windowHandles, HWND, hwnd)
		if(hwndAfter)
			hdwp = ::DeferWindowPos (hdwp, hwnd, hwndAfter, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOSIZE);
		hwndAfter = hwnd;
	EndFor
	::EndDeferWindowPos (hdwp);
}

} // namespace Win32
} // namespace CCL
