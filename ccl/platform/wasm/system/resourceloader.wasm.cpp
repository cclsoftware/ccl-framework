//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/system/resourceloader.wasm.cpp
// Description : WebAssembly Resource Loader
//
//************************************************************************************************

#include "ccl/platform/shared/host/frameworkresourceloader.h"

using namespace CCL;

//************************************************************************************************
// ResourceFileSystem
//************************************************************************************************

ResourceFileSystem& ResourceFileSystem::instance ()
{
	static FrameworkResourceFileSystem theInstance;
	return theInstance;
}
