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
// Filename    : delayloadhook.win.cpp
// Description : Loader hook for redirecting queries for node.exe to another module
//
//************************************************************************************************

// Adapted from https://github.com/nwjs/nw.js/blob/nw18/tools/win_delay_load_hook.cc

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <delayimp.h>
#include <string.h>

static HMODULE nodeModuleHandle = nullptr;

static FARPROC WINAPI loaderHook (unsigned int notify, PDelayLoadInfo info)
{
	FARPROC ret = nullptr;

	switch(notify)
	{
	case dliStartProcessing:
		if(nodeModuleHandle)
			break;

		nodeModuleHandle = GetModuleHandle (L"node.exe");
		if(nodeModuleHandle)
			break;

		nodeModuleHandle = GetModuleHandle (L"node.dll");
		if(nodeModuleHandle)
			break;

		// Fallback: Maybe the main executable module exports the Node API (e.g. Electron)
		nodeModuleHandle = GetModuleHandle (nullptr);
		if(nodeModuleHandle)
			break;

		OutputDebugString (L"Failed to obtain module handle for Node.js");
		break;

	case dliNotePreLoadLibrary:
		if(_stricmp (info->szDll, "node.exe") == 0)
			ret = reinterpret_cast<FARPROC>(nodeModuleHandle);
		break;
	}

	return ret;
}

extern "C" const PfnDliHook __pfnDliNotifyHook2 = loaderHook;
