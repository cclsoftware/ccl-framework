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
// Filename    : ccl/main/cclconsolemain.cpp
// Description : Console Application Main
//
//************************************************************************************************

#include "ccl/main/cclargs.h"
#include "ccl/main/cclinit.h"
#include "ccl/main/cclterminate.h"

#include "ccl/base/kernel.h"

#if CCLGUI_AVAILABLE

#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// GUIStartupAndShutdown
/** Helper for GUI startup/shutdown. */
//************************************************************************************************

struct GUIStartupAndShutdown
{
	bool success;
	GUIStartupAndShutdown (ModuleRef module)
	{ 
		success = System::GetGUI ().startup (module) != 0;
	}
	
	~GUIStartupAndShutdown ()
	{
		System::GetGUI ().shutdown (); 
	}
};

} // namespace CCL

#endif // CCLGUI_AVAILABLE

using namespace CCL;

int ccl_main (ArgsRef args); // external!
const ArgumentList* CCL::g_ArgumentList = nullptr; // cclargs.h

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Service APIs (linked locally)
//////////////////////////////////////////////////////////////////////////////////////////////////

static ModuleRef g_ModuleReference = nullptr;

ModuleRef System::GetCurrentModuleRef ()
{
	return g_ModuleReference;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// __ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int __ccl_main (ModuleRef module, const PlatformArgs& args)
{
	g_ModuleReference = module;

	FrameworkAutoInitializer frameworkInitalizer;

	// now that ccltext has been initialized, MutableArgumentList can be used
	MutableArgumentList arguments (args);
	g_ArgumentList = &arguments;

	#if CCLGUI_AVAILABLE
	GUIStartupAndShutdown guiStartupAndShutdown (module);
	if(!guiStartupAndShutdown.success)
		CCL_WARN ("%s\n", "Failed to startup GUI.")
	#endif

	if(!Kernel::instance ().initialize ())
	{
		Kernel::instance ().terminate (); // hmm... :-(
		return 0;
	}

	int result = ccl_main (arguments);

	Kernel::instance ().terminate ();

	ccl_terminate ();

	return result;
}
