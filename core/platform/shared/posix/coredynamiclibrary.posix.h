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
// Filename    : core/platform/shared/posix/coredynamiclibrary.posix.h
// Description : Dynamic library POSIX implementation
//
//************************************************************************************************

#ifndef _coredynamiclibrary_posix_h
#define _coredynamiclibrary_posix_h

#include "core/platform/shared/coreplatformdynamiclibrary.h"
#include "core/platform/corefeatures.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// PosixDynamicLibrary
//************************************************************************************************

class PosixDynamicLibrary: public IDynamicLibrary
{
public:
	PosixDynamicLibrary ();
	~PosixDynamicLibrary ();

	// IDynamicLibrary
	ModuleRef getNativeReference () const override;
	void* getFunctionPointer (CStringPtr name) const override;
	void load (CStringPtr library) override;
	void unload () override;

protected:
	ModuleRef nativeRef;
};

#if CORE_DYNAMICLIBRARY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixDynamicLibrary DynamicLibrary;
#endif

} // namespace Platform
} // namespace Core

#endif // _coredynamiclibrary_posix_h
