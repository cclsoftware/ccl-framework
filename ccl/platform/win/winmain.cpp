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
// Filename    : ccl/platform/win/winmain.cpp
// Description : Windows Application Entry
//
//************************************************************************************************

#include "ccl/main/cclargs.h"

#include "ccl/platform/win/cclwindows.h"

#pragma comment (lib, "shell32.lib")

extern int ccl_main_gui (CCL::ModuleRef module, const CCL::PlatformArgs& args);

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ExtendDllSearchPath (HINSTANCE hInstance)
{
	WCHAR path[MAX_PATH] = {0};
	::GetModuleFileNameW (hInstance, path, MAX_PATH);
	WCHAR* ptr = ::wcsrchr (path, '\\');
	ASSERT (ptr != nullptr)
	if(ptr)
		*(ptr + 1) = 0;
	::wcsncat_s (path, MAX_PATH, L"3rd party", 10);
	BOOL result = ::SetDllDirectoryW (path);
	ASSERT (result == TRUE)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// WinMain
//////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// extend DLL search path
	ExtendDllSearchPath (hInstance);

	int argc = 0;
	LPWSTR* argv = ::CommandLineToArgvW (::GetCommandLineW (), &argc);
	int result = ccl_main_gui (hInstance, CCL::PlatformArgs (argc, argv));
	::LocalFree (argv);
	return result;
}
