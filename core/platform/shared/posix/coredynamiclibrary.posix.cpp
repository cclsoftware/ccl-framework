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
// Filename    : core/platform/shared/posix/coredynamiclibrary.posix.cpp
// Description : Dynamic library POSIX implementation
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coredynamiclibrary)
#elif CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
#include "core/platform/shared/posix/coredynamiclibrary.posix.h"
#endif

#include "core/portable/corefile.h"
#include "core/text/coreutfcodec.h"
#include "core/system/coredebug.h"

#include <sys/stat.h>
#include <dlfcn.h>

using namespace Core;
using namespace Portable;
using namespace Platform;

//************************************************************************************************
// PosixDynamicLibrary
//************************************************************************************************

PosixDynamicLibrary::PosixDynamicLibrary ()
: nativeRef (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixDynamicLibrary::~PosixDynamicLibrary ()
{
	ASSERT (nativeRef == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixDynamicLibrary::load (CStringPtr library)
{
	ASSERT (nativeRef == nullptr);

	FileName path (library);

	void* handle = ::dlopen (path, RTLD_NOW | RTLD_LOCAL);
	if(handle)
		nativeRef = handle;
	else
	{
		CORE_PRINTF ("Could not open %s\n %s\n", library, ::dlerror ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixDynamicLibrary::unload ()
{
	if(!nativeRef)
		return;
	
	::dlclose (nativeRef);
	
	nativeRef = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef PosixDynamicLibrary::getNativeReference () const
{
	return nativeRef;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* PosixDynamicLibrary::getFunctionPointer (CStringPtr name) const
{
	ASSERT (nativeRef != nullptr)
	return ::dlsym (nativeRef, name);
}
