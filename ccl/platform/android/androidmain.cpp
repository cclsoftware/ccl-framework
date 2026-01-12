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
// Filename    : ccl/platform/android/androidmain.cpp
// Description : Android Application Entry (this has to be linked into the main application module)
//
//************************************************************************************************

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

// cclmain.cpp
extern bool ccl_main_gui_init (ModuleRef module, const PlatformArgs& args);
extern void ccl_main_gui_exit ();

//************************************************************************************************
// "main" entry point
//************************************************************************************************

CCL_EXPORT int CCL_API CCLAndroidMain (ModuleRef module, tbool startup)
{
	int exitCode = kExitSuccess;

	if(startup)
	{
		ccl_main_gui_init (module, PlatformArgs ());

		exitCode = System::GetGUI ().runEventLoop (); // (returns immediately, does not perform a loop)
	}
	else
		ccl_main_gui_exit ();

	return exitCode;
}
