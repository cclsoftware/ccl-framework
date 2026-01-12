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
// Filename    : ccl/platform/win/cclwindows.h
// Description : Include this file instead of windows.h!
//
//************************************************************************************************

#ifndef _ccl_cclwindows_h
#define _ccl_cclwindows_h

#include "ccl/public/base/ccldefpush.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS2
#endif

#include <sdkddkver.h>
#include <windows.h>
#include <oleidl.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef IID_PPV_ARGS
	#undef IID_PPV_ARGS // original macro conflicts with CCL::IUnknown!

	extern "C++"
	{
		template<typename T> void** __CCL_IID_PPV_ARGS_Helper (T** pp)
		{
			static_cast<::IUnknown*>(*pp);    // make sure everyone derives from IUnknown
			return reinterpret_cast<void**>(pp);
		}
	}

	#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), __CCL_IID_PPV_ARGS_Helper(ppType)
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////

#if _MSC_VER
    #pragma warning (disable : 4091)  // 'keyword' : ignored on left of 'type' when no variable is declared
	// => issue in shlobj.h: 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#endif

#include <shlobj.h>

#include "ccl/public/base/ccldefpop.h"

extern HINSTANCE g_hMainInstance;

#endif // _ccl_cclwindows_h
