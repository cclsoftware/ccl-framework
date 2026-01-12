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
// Filename    : ccl/platform/win/gui/notifyicon.win.cpp
// Description : Win32 Notification Icon
//
//************************************************************************************************
// see http://msdn2.microsoft.com/en-us/library/ms647738.aspx

#include "ccl/gui/system/notifyicon.h"
#include "ccl/platform/win/gui/windowhelper.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/system/systemevent.h"
#include "ccl/gui/windows/systemwindow.h"

#include "ccl/base/message.h"

#include <windowsx.h>

#define CCL_NOTIFYICON_MESSAGE WM_USER+66

namespace CCL {
namespace Win32 {

//************************************************************************************************
// NotifyIconWin
//************************************************************************************************

class NotifyIconWin: public NotifyIcon,
					 public SystemEventHandler
{
public:
	DECLARE_CLASS (NotifyIconWin, NotifyIcon)

	NotifyIconWin ();
	~NotifyIconWin ();

protected:
	friend struct NotifyIconData;
	HWND handle;

	HICON createIconHandle ();
	void popupContextMenu (PointRef p);

	// NotifyIcon
	void updateVisible (bool state) override;
	void updateTitle () override;
	void updateImage () override;
	void showInfo (const Alert::Event& e) override;

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;
};

//************************************************************************************************
// NotifyIconData
//************************************************************************************************

struct NotifyIconData: ::NOTIFYICONDATA
{
	NotifyIconData (NotifyIconWin& icon)
	{
		::memset (this, 0, sizeof(::NOTIFYICONDATA));
		cbSize = NOTIFYICONDATA_V2_SIZE;//sizeof(::NOTIFYICONDATA);
		hWnd = icon.handle;
		uID = 1;

		uFlags = NIF_TIP|NIF_SHOWTIP; // always show standard toolip
		icon.title.copyTo (szTip, CCLSTRSIZE (szTip));
	}
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

extern HICON CreateIconIndirectFromImage (Image& image, const Point& hotspot, const Point& sizeInPixel, float scaleFactor, BOOL isIcon); // mousecursor.win.cpp

//************************************************************************************************
// NotifyIconWin
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (NotifyIconWin, NotifyIcon, "NotifyIcon")
DEFINE_CLASS_UID (NotifyIconWin, 0x6d51b752, 0xb1c9, 0x44c2, 0xb5, 0xb4, 0x88, 0x6c, 0x61, 0x10, 0xc, 0xe4)

//////////////////////////////////////////////////////////////////////////////////////////////////

NotifyIconWin::NotifyIconWin ()
: handle (nullptr)
{
	handle = CreateMessageWindow (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NotifyIconWin::~NotifyIconWin ()
{
	setVisible (false);

	::DestroyWindow (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HICON NotifyIconWin::createIconHandle ()
{
	if(image)
	{
		// Note: Small icon size depends on system DPI settings!
		int smallIconSize = ::GetSystemMetrics (SM_CXSMICON); 
		float scaleFactor = (float)smallIconSize / 16.f; // 16px on standard low-dpi systems

		HICON iconHandle = CreateIconIndirectFromImage (*image, Point (), Point (smallIconSize, smallIconSize), scaleFactor, TRUE);
		if(iconHandle)
			return iconHandle;
	}

	// fall back to application icon
	return ::LoadIcon (g_hMainInstance, MAKEINTRESOURCE (1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotifyIconWin::updateVisible (bool state)
{
	NotifyIconData data (*this);

	if(state)
	{
		data.uFlags |= NIF_ICON;
		data.hIcon = createIconHandle ();
		data.uFlags |= NIF_MESSAGE;
		data.uCallbackMessage = CCL_NOTIFYICON_MESSAGE;
			
		::Shell_NotifyIcon (NIM_ADD, &data);

		if(!title.isEmpty ())
			updateTitle ();

		// set icon to newer Shell behavior
		#if 0
		NotifyIconData data2 (*this);
		data2.uVersion = NOTIFYICON_VERSION_4;
		::Shell_NotifyIcon (NIM_SETVERSION, &data2);
		#endif
	}
	else
	{
		::Shell_NotifyIcon (NIM_DELETE, &data); 
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotifyIconWin::updateTitle ()
{
	NotifyIconData data (*this);

	//data.uFlags |= NIF_TIP;
	//title.copyTo (data.szTip, CCLSTRSIZE (data.szTip));

	::Shell_NotifyIcon (NIM_MODIFY, &data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotifyIconWin::updateImage ()
{
	NotifyIconData data (*this);

	data.uFlags |= NIF_ICON;
	data.hIcon = createIconHandle ();

	::Shell_NotifyIcon (NIM_MODIFY, &data); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotifyIconWin::showInfo (const Alert::Event& e)
{
	NotifyIconData data (*this);

	data.uFlags |= NIF_INFO;
	e.message.copyTo (data.szInfo, CCLSTRSIZE (data.szInfo));
	title.copyTo (data.szInfoTitle, CCLSTRSIZE (data.szInfoTitle));

	switch(e.type)
	{
	case Alert::kWarning : data.dwInfoFlags |= NIIF_WARNING; break;
	case Alert::kError : data.dwInfoFlags |= NIIF_ERROR; break;
	case Alert::kInformation : data.dwInfoFlags |= NIIF_INFO; break;
	}

	data.dwInfoFlags |= NIIF_LARGE_ICON;

	::Shell_NotifyIcon (NIM_MODIFY, &data); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult NotifyIconWin::handleEvent (SystemEvent& e)
{
	EventResult result = nullptr;
	if(e.msg == CCL_NOTIFYICON_MESSAGE)
	{
		int msg = LOWORD (e.lParam);
		switch(msg)
		{
		case NIN_BALLOONTIMEOUT :
		case NIN_BALLOONUSERCLICK :
			if(autoShow)
				setVisible (false);
			break;

		#if 0 // requires new Shell behavior!
		case WM_CONTEXTMENU :
			popupContextMenu (Point (GET_X_LPARAM (e.wParam), GET_Y_LPARAM (e.wParam)));
			break;
		#else
		case WM_RBUTTONUP :
			{
				Point p;
				GUI.getMousePosition (p);
				popupContextMenu (p);
			}
			break;
		#endif

		case WM_LBUTTONDOWN :
			if(UnknownPtr<IObserver> observer = handler)
				observer->notify (this, Message (kIconClicked));
			break;

		case WM_LBUTTONDBLCLK : 
			if(UnknownPtr<IObserver> observer = handler)
				observer->notify (this, Message (kIconDoubleClicked));
			break;

		default :
			e.notHandled = true;
		}
	}
	else
		e.notHandled = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NotifyIconWin::popupContextMenu (PointRef p)
{
	AutoPtr<PopupMenu> menu = createContextMenu ();
	if(menu)
	{
		AutoPtr<SystemWindow> systemWindow = NEW SystemWindow (handle);
		if(MenuItem* item = menu->popup (p, systemWindow))
			item->select ();
		menu->markForGC ();
	}
}
