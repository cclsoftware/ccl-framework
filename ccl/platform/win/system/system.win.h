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
// Filename    : ccl/platform/win/system/system.win.h
// Description : Windows system helpers
//
//************************************************************************************************

#ifndef _ccl_system_win32_h
#define _ccl_system_win32_h

#include "ccl/platform/win/cclwindows.h"

#include "ccl/public/base/datetime.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// DllDirectoryModifier
//************************************************************************************************

struct DllDirectoryModifier
{
	WCHAR oldDllDir[MAX_PATH];
	
	DllDirectoryModifier (const WCHAR* newDllDir)
	{
		oldDllDir[0] = 0;
		::GetDllDirectoryW (MAX_PATH, oldDllDir);
		oldDllDir[MAX_PATH-1] = 0;

		BOOL result = ::SetDllDirectoryW (newDllDir);
		ASSERT (result)
	}
	
	~DllDirectoryModifier ()
	{
		if(oldDllDir[0])
		{
			BOOL result = ::SetDllDirectoryW (oldDllDir);
			ASSERT (result)
		}
	}
};

//************************************************************************************************
// DateTime
//************************************************************************************************

inline DateTime& fromSystemTime (DateTime& dateTime, SYSTEMTIME& st)
{
	dateTime.setTime (Time (st.wHour, st.wMinute, st.wSecond, st.wMilliseconds));
	dateTime.setDate (Date (st.wYear, st.wMonth, st.wDay));
	return dateTime;
}

inline void toSystemTime (SYSTEMTIME& st, const DateTime& dateTime)
{
	st.wYear = (WORD)dateTime.getDate ().getYear ();
	st.wMonth = (WORD)dateTime.getDate ().getMonth ();	
	st.wDayOfWeek = 0;
	st.wDay = (WORD)dateTime.getDate ().getDay ();
	st.wHour = (WORD)dateTime.getTime ().getHour ();
	st.wMinute = (WORD)dateTime.getTime ().getMinute ();
	st.wSecond = (WORD)dateTime.getTime ().getSecond ();
	st.wMilliseconds = (WORD)dateTime.getTime ().getMilliseconds ();
}

//************************************************************************************************
// HRESULT as string
//************************************************************************************************

#if DEBUG
inline void formatSystemDebugMessage (String& string, HRESULT hr)
{
	LPVOID lpMsgBuf = nullptr;
	::FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
					  nullptr, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&lpMsgBuf, 0, nullptr);
	string.empty ();
	string.appendASCII ((const char*)lpMsgBuf);
	::LocalFree (lpMsgBuf);
}
#endif

} // namespace Win32
} // namespace CCL

#endif // _ccl_system_win32_h
