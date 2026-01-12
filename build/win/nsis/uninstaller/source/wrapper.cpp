//************************************************************************************************
//
// Uninstall Wrapper
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
// Filename    : wrapper.cpp
// Description : Uninstall Wrapper
//
//************************************************************************************************

#include <windows.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
// WinMain
//////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// build uninstaller path
	WCHAR uninstaller[MAX_PATH];
	GetModuleFileNameW (hInstance, uninstaller, MAX_PATH);
	WCHAR* ptr = ::wcsrchr (uninstaller, '\\');
	if(ptr)
		*(ptr + 1) = 0;
	wcsncat_s (uninstaller, MAX_PATH, L"Uninstall.exe", 14);

	// call the real uninstaller
	ShellExecuteW (NULL, L"open", uninstaller, NULL, NULL, SW_SHOW);

	return 0;
}
