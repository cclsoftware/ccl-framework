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
// Filename    : core/platform/cocoa/coredynamiclibrary.cocoa.cpp
// Description : Dynamic library Cocoa implementation
//
//************************************************************************************************

#include "coredynamiclibrary.cocoa.h"

#include "core/portable/corefile.h"
#include "core/text/coreutfcodec.h"

#include <CoreFoundation/CoreFoundation.h>

#include <sys/stat.h>
#include <dlfcn.h>

using namespace Core;
using namespace Portable;
using namespace Platform;

//************************************************************************************************
// CocoaDynamicLibrary
//************************************************************************************************

CocoaDynamicLibrary::CocoaDynamicLibrary ()
: moduleIsBundle (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaDynamicLibrary::load (CStringPtr library)
{
	ASSERT (nativeRef == nullptr);

	FileName path (library);

	CFStringRef pathStr = CFStringCreateWithCString (kCFAllocatorDefault, path, kCFStringEncodingUTF8);
	CFURLRef bundleUrl = CFURLCreateWithFileSystemPath (kCFAllocatorDefault, pathStr, kCFURLPOSIXPathStyle, TRUE);
	CFBundleRef bundle = CFBundleCreate (kCFAllocatorDefault, bundleUrl);
	if(bundleUrl)
		CFRelease (bundleUrl);
	if(bundle)
	{
		if(CFBundleLoadExecutable (bundle))
		{
			nativeRef = bundle;
			moduleIsBundle = true;
		}
	}
	else
	{
		PosixDynamicLibrary::load (library);
		moduleIsBundle = false;
	}

	if(pathStr)
		CFRelease(pathStr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaDynamicLibrary::unload ()
{
	if(!nativeRef)
		return;

	if(moduleIsBundle)
	{
		if(CFBundleIsExecutableLoaded ((CFBundleRef)nativeRef))
			CFBundleUnloadExecutable ((CFBundleRef)nativeRef);
		CFRelease (nativeRef);
	}
	else
		PosixDynamicLibrary::unload ();
	
	nativeRef = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CocoaDynamicLibrary::getFunctionPointer (CStringPtr name) const
{
	ASSERT (nativeRef != nullptr)
	
	if(moduleIsBundle)
	{
		CFStringRef functionString = CFStringCreateWithCString (0, name, kCFStringEncodingUTF8);
		void* result = CFBundleGetFunctionPointerForName ((CFBundleRef)nativeRef, functionString);
		CFRelease (functionString);
		return result;
	}
	else
		return PosixDynamicLibrary::getFunctionPointer (name);
}
