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
// Filename    : core/platform/cocoa/coredynamiclibrary.cocoa.h
// Description : Dynamic library Cocoa implementation
//
//************************************************************************************************

#ifndef _coredynamiclibrary_cocoa_h
#define _coredynamiclibrary_cocoa_h

#include "core/platform/shared/posix/coredynamiclibrary.posix.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// CocoaDynamicLibrary
//************************************************************************************************

class CocoaDynamicLibrary: public PosixDynamicLibrary
{
public:
	CocoaDynamicLibrary ();

	// PosixDynamicLibrary
	void* getFunctionPointer (CStringPtr name) const override;
	void load (CStringPtr library) override;
	void unload () override;

protected:
	bool moduleIsBundle;
};

typedef CocoaDynamicLibrary DynamicLibrary;

} // namespace Platform
} // namespace Core

#endif // _coredynamiclibrary_cocoa_h
