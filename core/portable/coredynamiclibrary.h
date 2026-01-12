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
// Filename    : core/portable/coredynamiclibrary.h
// Description : Dynamic library class
//
//************************************************************************************************

#ifndef _coredynamiclibrary_h
#define _coredynamiclibrary_h

#include "core/platform/corefeatures.h"

#if CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coredynamiclibrary)
#elif CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coredynamiclibrary)
#elif CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/coredynamiclibrary.posix.h"
#elif CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED
	#include "core/platform/shared/coreplatformdynamiclibrary.h"
#endif

namespace Core {
namespace Portable {

//************************************************************************************************
// DynamicLibrary
//************************************************************************************************

class DynamicLibrary
{
public:
	ModuleRef getNativeReference () const;
	void* getFunctionPointer (CStringPtr name) const;
	
	void load (CStringPtr library);
	void unload ();
	
	/**
	* \brief Get the platform-specific dynamic library implementation object
	* \details This object might reveal additional methods and attributes.
	* \return The platform-specific dynamic library implementation object
	*/
	Platform::DynamicLibrary& getPlatformDynamicLibrary ();

protected:
	Platform::DynamicLibrary platformDynamicLibrary;
};

//************************************************************************************************
// DynamicLibrary implementation
//************************************************************************************************

inline ModuleRef DynamicLibrary::getNativeReference () const
{ return platformDynamicLibrary.getNativeReference (); }

inline void* DynamicLibrary::getFunctionPointer (CStringPtr name) const
{ return platformDynamicLibrary.getFunctionPointer (name); }

inline void DynamicLibrary::load (CStringPtr library)
{ platformDynamicLibrary.load (library); }

inline void DynamicLibrary::unload ()
{ platformDynamicLibrary.unload (); }

inline Platform::DynamicLibrary& DynamicLibrary::getPlatformDynamicLibrary ()
{ return platformDynamicLibrary; }

} //namespace Portable
} //namespace Core

#endif //_coredynamiclibrary_h
