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
// Filename    : ccl/main/cclinit.h
// Description : Framework Initialization (dynamic linkage)
//
//************************************************************************************************

#ifndef _cclinit_h
#define _cclinit_h

#include "ccl/main/cclmodmain.h"
#include "ccl/main/platformmodule.h"

namespace CCL {

//************************************************************************************************
// FrameworkInitializer
//************************************************************************************************

class FrameworkInitializer: PlatformModuleHelper
{
public:
	FrameworkInitializer ()
	: moduleNames {CCL_MODULE_NAME ("ccltext"),
				   CCL_MODULE_NAME ("cclsystem"),
				   CCL_MODULE_NAME ("cclsecurity"),
				   CCL_MODULE_NAME ("cclnet"),
				   CCL_MODULE_NAME ("cclgui")}
	{}

	void init ()
	{
		for(int i = 0; i < kModuleCount; i++)
			if(ModuleRef ref = getModule (moduleNames[i]))
			{
				callModuleMain (ref, kModuleInit);
				closeModule (ref);
			}
	}

	void exit ()
	{
		for(int i = kModuleCount-1; i >= 0; i--)
			if(ModuleRef ref = getModule (moduleNames[i]))
			{
				callModuleMain (ref, kModuleExit);
				closeModule (ref);
			}
	}

protected:
	static const int kModuleCount = 5;
	ModuleName moduleNames[kModuleCount];

	static void callModuleMain (ModuleRef ref, ModuleEntryReason reason)
	{
		CCLModuleMainProc moduleMain = reinterpret_cast<CCLModuleMainProc> (getFunction (ref, CCL_FUNCTION_NAME ("CCLModuleMain")));
		ASSERT (moduleMain != nullptr)
		if(moduleMain)
			(*moduleMain) (ref, reason);
	}
};

//************************************************************************************************
// FrameworkAutoInitializer
//************************************************************************************************

struct FrameworkAutoInitializer: FrameworkInitializer
{
	FrameworkAutoInitializer ()
	{ init (); }
	~FrameworkAutoInitializer ()
	{ exit (); }
};

} // namespace CCL

#endif // _cclinit_h
