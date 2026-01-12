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
// Filename    : ccl/platform/linux/linuxmain.cpp
// Description : Linux application entry
//
//************************************************************************************************

#include "ccl/main/cclargs.h"
#include "ccl/main/cclmodmain.h"

#include <dlfcn.h>
#include <signal.h>
#include <sys/resource.h>

extern int __ccl_main (CCL::ModuleRef module, const CCL::PlatformArgs& args);

//////////////////////////////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
	// set the number of files that our process can open to the maximum
	rlimit limit;
	int err = getrlimit (RLIMIT_NOFILE, &limit);
	if(err == 0)
	{
		limit.rlim_cur = limit.rlim_max;
		err = setrlimit (RLIMIT_NOFILE, &limit);
		ASSERT (err == 0)
	}
	
	void* mainModule = ::dlopen (nullptr, RTLD_LAZY | RTLD_NOLOAD);
	
	return __ccl_main (mainModule, CCL::PlatformArgs (argc, argv));
}
