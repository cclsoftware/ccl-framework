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
// Filename    : ccl/platform/wasm/wasmmain.cpp
// Description : WebAssembly application entry
//
//************************************************************************************************

#include "ccl/main/cclargs.h"
#include "ccl/main/cclmodmain.h"

#include <dlfcn.h>

extern int ccl_main_gui (CCL::ModuleRef module, const CCL::PlatformArgs& args);

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
	void* mainModule = ::dlopen (nullptr, RTLD_LAZY | RTLD_NOLOAD);
	return ccl_main_gui (mainModule, PlatformArgs (argc, argv));
}
