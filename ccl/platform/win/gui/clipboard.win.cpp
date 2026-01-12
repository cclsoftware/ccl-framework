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
// Filename    : ccl/platform/win/gui/clipboard.win.cpp
// Description : Windows Clipboard
//
//************************************************************************************************

#include "ccl/gui/system/clipboard.h"

#include "ccl/gui/windows/desktop.h"

#include "ccl/public/text/cclstring.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {

//************************************************************************************************
// WindowsClipboard
//************************************************************************************************

class WindowsClipboard: public Clipboard
{
public:
	WindowsClipboard ();

	// Clipboard
	bool setNativeText (StringRef text) override;
	bool getNativeText (String& text) const override;
	bool hasNativeContentChanged () override;

private:
	DWORD sequenceNumber;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

static HWND getClipboardWindow ()
{
	IWindow* w = Desktop.getApplicationWindow ();
	HWND hwnd = w ? (HWND)w->getSystemWindow () : nullptr;
	if(!hwnd)
	{
		extern HWND g_hMessageWindow; // gui.win.cpp
		hwnd = g_hMessageWindow;
	}
	return hwnd;
}

//************************************************************************************************
// WindowsClipboard
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Clipboard, WindowsClipboard)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsClipboard::WindowsClipboard ()
: sequenceNumber (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsClipboard::setNativeText (StringRef text)
{
	bool result = false;
	if(::OpenClipboard (getClipboardWindow ()))
	{
		::EmptyClipboard ();

		StringChars textChars (text);
		unsigned int byteSize = (text.length () + 1) * sizeof(uchar);
		HGLOBAL hData = ::GlobalAlloc (GMEM_MOVEABLE, byteSize);
		if(hData)
		{
			void* address = ::GlobalLock (hData);
			ASSERT (address != nullptr)
			if(address)
			{
				::memcpy (address, textChars, byteSize);
				::GlobalUnlock (hData);
			}

			if(::SetClipboardData (CF_UNICODETEXT, hData))
				result = true;
			#if DEBUG_LOG
			else
			{
				DWORD error = ::GetLastError ();
				CCL_PRINTLN ("Failed to set clipboard data!")
			}
			#endif
		}
		::CloseClipboard ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsClipboard::getNativeText (String& text) const
{
	text.empty ();
	bool result = false;
	if(::OpenClipboard (getClipboardWindow ()))
	{
		HANDLE hData = ::GetClipboardData (CF_UNICODETEXT);
		if(hData)
		{
			uchar* textBuffer = (uchar*)::GlobalLock (hData);
			if(textBuffer)
			{
				text.append (textBuffer);
				result = true;
			}
			::GlobalUnlock (hData);
		}
		::CloseClipboard ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsClipboard::hasNativeContentChanged ()
{
	DWORD n = GetClipboardSequenceNumber ();
	if(n != sequenceNumber)
	{
		sequenceNumber = n;
		return true;
	}
	return false;
}
