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
// Filename    : ccl/platform/cocoa/cocoaconsolemain.mm
// Description : Mac OS application entry
//
//************************************************************************************************

#include <CoreFoundation/CFBundle.h>
#include <unistd.h>

#include "ccl/main/cclargs.h"
#include "ccl/main/cclmodmain.h"

using namespace CCL;

extern int __ccl_main (CCL::ModuleRef module, const CCL::PlatformArgs& args);

CFBundleRef g_MainBundle = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
    int result  = 0;
    @autoreleasepool
    {
        g_MainBundle = CFBundleGetMainBundle ();
		
        result = __ccl_main (g_MainBundle, CCL::PlatformArgs (argc, argv));
    }

	return result;
}
