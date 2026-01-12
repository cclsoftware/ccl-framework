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
// Filename    : ccl/main/platformmodule.h
// Description : Platform Module Functions
//
//************************************************************************************************

#ifndef _ccl_platformmodule_h
#define _ccl_platformmodule_h

#include "ccl/public/cclversion.h"

#if CCL_PLATFORM_WINDOWS
#include "ccl/platform/win/cclwindows.h"

#elif CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
#include <CoreFoundation/CFBundle.h>

#elif CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID
#include <dlfcn.h>
#endif

#ifdef CCL_ISOLATION_POSTFIX
#define CCL_FUNCTION_POSTFIX CCL_ISOLATION_POSTFIX
#else
#define CCL_FUNCTION_POSTFIX
#endif

namespace CCL {

//************************************************************************************************
// PlatformModuleHelper
//************************************************************************************************

struct PlatformModuleHelper
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	#if CCL_PLATFORM_WINDOWS
	//////////////////////////////////////////////////////////////////////////////////////////////
		
	#define CCL_MODULE_NAME(name) L##name CCL_FUNCTION_POSTFIX ".dll"
	#define CCL_FUNCTION_NAME(name) name

	typedef LPCWSTR ModuleName;
	typedef LPCSTR FunctionName;

	static ModuleRef getModule (ModuleName name) { return ::GetModuleHandle (name); }
	static void* getFunction (ModuleRef ref, FunctionName name) { return ::GetProcAddress ((HMODULE)ref, name); }
	static void closeModule (ModuleRef ref) {}

	//////////////////////////////////////////////////////////////////////////////////////////////
	#elif CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	#define CCL_MODULE_NAME(name) CFSTR (CCL_PACKAGE_DOMAIN "." name CCL_FUNCTION_POSTFIX)
	#define CCL_FUNCTION_NAME(name) CFSTR (name)

	typedef CFStringRef ModuleName;
	typedef CFStringRef FunctionName;

	static ModuleRef getModule (ModuleName name) { return CFBundleGetBundleWithIdentifier (name); }
	static void* getFunction (ModuleRef ref, FunctionName name) { return CFBundleGetFunctionPointerForName ((CFBundleRef)ref, name); }
	static void closeModule (ModuleRef ref) {}

	//////////////////////////////////////////////////////////////////////////////////////////////
	#elif CCL_PLATFORM_LINUX || CCL_PLATFORM_ANDROID
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	#define CCL_MODULE_NAME(name) "lib" name CCL_FUNCTION_POSTFIX ".so"
	#define CCL_FUNCTION_NAME(name) name

	typedef CStringPtr ModuleName;
	typedef CStringPtr FunctionName;

	static ModuleRef getModule (ModuleName name) { return ::dlopen (name, RTLD_NOW | RTLD_NOLOAD | RTLD_LOCAL); } // noload: only returns handle if already loaded
	static void* getFunction (ModuleRef ref, FunctionName name) { return ::dlsym (ref, name); }
	static void closeModule (ModuleRef ref) { ::dlclose (ref); }
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	#else
	//////////////////////////////////////////////////////////////////////////////////////////////

	#error Unknown platform

	#endif
};

} // namespace CCL

#endif // _ccl_platformmodule_h
