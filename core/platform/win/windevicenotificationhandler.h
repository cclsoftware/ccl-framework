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
// Filename    : core/platform/win/windevicenotificationhandler.h
// Description : Windows Device Notification Handler
//
//************************************************************************************************

#ifndef _windevicenotificationhandler_h
#define _windevicenotificationhandler_h

#include "core/public/corestringbuffer.h"
#include "core/public/corevector.h"

#include <windows.h>
#include <Dbt.h>

namespace Core {

//************************************************************************************************
// WinDeviceNotificationHandler
//************************************************************************************************

class WinDeviceNotificationHandler
{
public:
	typedef void (*Callback) (void* context, bool devicesRemoved);

	WinDeviceNotificationHandler (Callback callback, void* context);
	~WinDeviceNotificationHandler ();

	bool registerNotification (const GUID& classguid);
	void unregisterNotifications ();

protected:
	Callback callback;
	void* context;
	HINSTANCE hInstance;
	HWND hwnd;
	CString32 windowClassName;
	Vector<HDEVNOTIFY> notificationHandles;

	void trigger (bool devicesRemoved);
	static LRESULT CALLBACK windowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace Core

#endif // _windevicenotificationhandler_h
