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
// Filename    : core/platform/win/windevicenotificationhandler.cpp
// Description : Windows Device Notification Handler
//
//************************************************************************************************

#include "windevicenotificationhandler.h"

using namespace Core;

//************************************************************************************************
// WinDeviceNotificationHandler
//************************************************************************************************

WinDeviceNotificationHandler::WinDeviceNotificationHandler (Callback callback, void* context)
: callback (callback),
  context (context),
  hInstance (::GetModuleHandle (nullptr)),
  hwnd (NULL)
{
	ASSERT (callback)

	WNDCLASSEXA windowClass = {0};
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.hInstance = hInstance;
	windowClassName.appendFormat ("DeviceNotification%p", this);
	windowClass.lpszClassName = windowClassName.str ();
	windowClass.lpfnWndProc = windowProc;
	::RegisterClassExA (&windowClass);

	hwnd = ::CreateWindowA (windowClassName, "DeviceNotification",
							0, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, this);
	ASSERT (hwnd != NULL)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WinDeviceNotificationHandler::~WinDeviceNotificationHandler ()
{
	ASSERT (notificationHandles.isEmpty ())

	::DestroyWindow (hwnd);
	::UnregisterClassA (windowClassName, hInstance);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WinDeviceNotificationHandler::registerNotification (const GUID& classguid)
{
	DEV_BROADCAST_DEVICEINTERFACE filter = {0};
	filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	filter.dbcc_classguid = classguid;

	HDEVNOTIFY hDevNotify = ::RegisterDeviceNotification (hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
	if(hDevNotify)
		notificationHandles.add (hDevNotify);
	return hDevNotify != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WinDeviceNotificationHandler::unregisterNotifications ()
{
	VectorForEachFast (notificationHandles, HDEVNOTIFY, hDevNotify)
		::UnregisterDeviceNotification (hDevNotify);
	EndFor
	notificationHandles.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WinDeviceNotificationHandler::trigger (bool devicesRemoved)
{
	(*callback) (context, devicesRemoved);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WinDeviceNotificationHandler::windowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto handler = reinterpret_cast<WinDeviceNotificationHandler*> (::GetWindowLongPtr (hwnd, GWLP_USERDATA));

	switch(msg)
	{
	case WM_CREATE :
		{
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*> (lParam);
			handler = reinterpret_cast<WinDeviceNotificationHandler*> (cs->lpCreateParams);
			::SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR)handler);
		}
		break;

	case WM_DEVICECHANGE :
		if(handler)
		{
			switch(wParam)
			{
			case DBT_DEVICEARRIVAL :
				handler->trigger (false);
				break;
			case DBT_DEVICEREMOVECOMPLETE :
				handler->trigger (true);
				break;
			}
		}
		break;
	}

	return ::DefWindowProc (hwnd, msg, wParam, lParam);
}
