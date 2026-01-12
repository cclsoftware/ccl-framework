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
// Filename    : core/platform/win/coredynamiclibrary.win.cpp
// Description : Dynamic library Windows implementation
//
//************************************************************************************************

#include "coredynamiclibrary.win.h"

#include "core/portable/corefile.h"
#include "core/text/coreutfcodec.h"

#include <windows.h>

using namespace Core;
using namespace Portable;
using namespace Platform;

//************************************************************************************************
// Win32DynamicLibrary
//************************************************************************************************

Win32DynamicLibrary::Win32DynamicLibrary ()
: nativeRef (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32DynamicLibrary::~Win32DynamicLibrary ()
{
	ASSERT (nativeRef == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32DynamicLibrary::load (CStringPtr library)
{
	ASSERT (nativeRef == nullptr);

	FileName path (library);

	HMODULE hModule = ::LoadLibraryA (path);
	if(hModule)
		nativeRef = hModule;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32DynamicLibrary::unload ()
{
	if(!nativeRef)
		return;

	::FreeLibrary ((HMODULE)nativeRef);
	VectorForEach (directoryCookies, DLL_DIRECTORY_COOKIE, cookie)
		::RemoveDllDirectory (cookie);
	EndFor
    directoryCookies.removeAll ();

	nativeRef = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32DynamicLibrary::addSearchPath (CStringPtr dir)
{
	uchar dirUTF16[MAX_PATH] = {0};
	Core::Text::UTFFunctions::decodeUTF8String (dirUTF16, MAX_PATH, dir, ConstString (dir).length ());
	if(::SetDefaultDllDirectories (LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))
	{
		DLL_DIRECTORY_COOKIE cookie = ::AddDllDirectory (dirUTF16);
		if(cookie)
			directoryCookies.add (cookie);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef Win32DynamicLibrary::getNativeReference () const
{
	return nativeRef;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* Win32DynamicLibrary::getFunctionPointer (CStringPtr name) const
{
	ASSERT (nativeRef != nullptr)
	return ::GetProcAddress ((HMODULE)nativeRef, name);
}
