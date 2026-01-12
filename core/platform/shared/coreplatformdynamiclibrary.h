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
// Filename    : core/platform/shared/coreplatformdynamiclibrary.h
// Description : Dynamic library platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformdynamiclibrary_h
#define _coreplatformdynamiclibrary_h

#include "core/platform/corefeatures.h"

#include "core/public/coretypes.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// IDynamicLibrary
//************************************************************************************************

struct IDynamicLibrary
{
	virtual ~IDynamicLibrary () {}

	virtual ModuleRef getNativeReference () const = 0;
	virtual void* getFunctionPointer (CStringPtr name) const = 0;
	
	virtual void load (CStringPtr library) = 0;
	virtual void unload () = 0;
};

#if CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

//************************************************************************************************
// DynamicLibrary stub implementation
//************************************************************************************************

class DynamicLibraryStub: public IDynamicLibrary
{
public:
	ModuleRef getNativeReference () const { return nullptr; }
	void* getFunctionPointer (CStringPtr name) const { return nullptr; }
	void load (CStringPtr library) {}
	void unload () {}
};

typedef DynamicLibraryStub DynamicLibrary;

#endif

} // namespace Platform
} // namespace Core

#endif // _coreplatformdynamiclibrary_h
