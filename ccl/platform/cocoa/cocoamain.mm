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
// Filename    : ccl/platform/cocoa/cocoamain.mm
// Description : Mac/iOS application entry
//
//************************************************************************************************

#include "core/public/coreplatform.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include <CoreFoundation/CFBundle.h>
#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <mach/task.h>

#include "ccl/main/cclargs.h"
#include "ccl/main/cclmodmain.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

extern int ccl_main_gui (CCL::ModuleRef module, const CCL::PlatformArgs& args);

CFBundleRef g_MainBundle = NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
	int result  = 0;
	@autoreleasepool
	{
		// set the number of files that our process can open to the maximum
		struct rlimit limit;
		int err = getrlimit (RLIMIT_NOFILE, &limit);
		if(err == 0)
		{
			limit.rlim_cur = OPEN_MAX;  // note: fails with RLIM_INFINITY
			limit.rlim_max = OPEN_MAX;
			err = setrlimit (RLIMIT_NOFILE, &limit);
			ASSERT (err == 0)
		}
		
		g_MainBundle = CFBundleGetMainBundle ();
		result = ccl_main_gui (g_MainBundle, CCL::PlatformArgs (argc, argv));
	}
	
	return result;
}
