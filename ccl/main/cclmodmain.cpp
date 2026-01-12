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
// Filename    : ccl/main/cclmodmain.cpp
// Description : Module Main
//
//************************************************************************************************

#include "ccl/main/cclmodmain.h"

#include "ccl/base/kernel.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

bool ccl_module_main (int reason); // external!

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Service APIs (linked locally)
//////////////////////////////////////////////////////////////////////////////////////////////////

static ModuleRef g_ModuleReference = nullptr;

ModuleRef System::GetCurrentModuleRef ()
{
	return g_ModuleReference;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLModuleMain
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tbool CCL_API CCLModuleMain (ModuleRef module, ModuleEntryReason reason)
{
	ASSERT (module != nullptr)
	
	// *** Module Init ***
	if(reason == kModuleInit)
	{
		g_ModuleReference = module;

		if(!Kernel::instance ().initialize ())
			return false;

		if(!ccl_module_main (reason))
		{
			Kernel::instance ().terminate ();
			g_ModuleReference = nullptr;
			return false;
		}
	}
	// *** Module Exit ***
	else if(reason == kModuleExit)
	{
		if(g_ModuleReference)
		{
			ccl_module_main (reason);
			Kernel::instance ().terminate ();
		}
	}

	return true;
}
