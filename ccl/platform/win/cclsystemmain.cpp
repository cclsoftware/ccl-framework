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
// Filename    : ccl/platform/win/cclsystemmain.cpp
// Description : cclsystem DLL Entry
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/main/cclmodmain.h"
#include "ccl/public/base/debug.h"
#include "ccl/system/threading/thread.h"

#include "ccl/platform/win/cclwindows.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// DllMain
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	// PLEASE NOTE: DLL_PROCESS_ATTACH/DLL_PROCESS_DETACH isn't handled here as CCLModuleMain is exported directly.
	// The primary purpose of using DllMain here is to catch foreign threads to be displayed in CCL Spy.

	if(fdwReason == DLL_THREAD_ATTACH)
	{
		CCL_PRINTF ("*** cclsystem: Thread attach %d ***\n", ::GetCurrentThreadId ())
		CCL::Threading::NativeThreadRegistrar::addThread ((CCL::Threading::ThreadID)::GetCurrentThreadId ());
	}
	else if(fdwReason == DLL_THREAD_DETACH)
	{
		CCL_PRINTF ("*** cclsystem: Thread detach %d ***\n", ::GetCurrentThreadId ())
		CCL::Threading::NativeThreadRegistrar::removeThread ((CCL::Threading::ThreadID)::GetCurrentThreadId ());
	}
	return TRUE;
}
