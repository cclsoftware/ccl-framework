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
// Filename    : ccl/platform/shared/posix/system/safetymanager.posix.cpp
// Description : POSIX Safety Manager
//
//************************************************************************************************

#include "ccl/system/safetymanager.h"

#include "ccl/public/storage/iurl.h"

#include "core/text/coreutfcodec.h"

#include <dlfcn.h>
#include <execinfo.h>
#include <ucontext.h>

namespace CCL {

//************************************************************************************************
// PosixSafetyManager
//************************************************************************************************

class PosixSafetyManager: public SafetyManager
{
	
protected:
	NativePath modulePath;
	
	// SafetyManager
	void enableCrashRecovery (bool state);
	void CCL_API reportException (void* exceptionInformation, const uchar* systemDumpFile);
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// PosixSafetyManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SafetyManager, PosixSafetyManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixSafetyManager::enableCrashRecovery (bool state)
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PosixSafetyManager::reportException (void* exceptionInformation, const uchar* systemDumpFile)
{
	modulePath[0] = '\0';
	
	void* crashSite = nullptr;
	
	ucontext_t* uc = static_cast<ucontext_t*> (exceptionInformation);
	if(uc)
	{
#if defined(CCL_PLATFORM_ARM) && defined(CCL_PLATFORM_64BIT)
		crashSite = (void*)uc->uc_mcontext.pc;
#elif defined(CCL_PLATFORM_ARM)
		crashSite = (void*)uc->uc_mcontext.arm_pc;
#elif defined(CCL_PLATFORM_64BIT)
		crashSite = (void*)uc->uc_mcontext.gregs[REG_RIP];
#else
		crashSite = (void*)uc->uc_mcontex.gregs[REG_EIP];
#endif
	}
	
	if(crashSite)
	{
		// find crashing module
		Dl_info info;
		if(dladdr (crashSite, &info) != 0)
			Core::Text::UTFFunctions::decodeUTF8String (modulePath, modulePath.size (), info.dli_fname, (int)strlen (info.dli_fname));
	
		if(modulePath[0] != '\0')
		{
			reportCrash (modulePath, systemDumpFile);
			
			// find additional modules in the call stack
			void* previousModule = info.dli_fbase;
			void* callstack[30];
			int count = backtrace (callstack, ARRAY_COUNT (callstack));
			for(int i = 3; i < count; i++)
			{
				if(dladdr (callstack[i], &info) != 0)
				{
					if(info.dli_fbase != previousModule)
					{
						Core::Text::UTFFunctions::decodeUTF8String (modulePath, modulePath.size (), info.dli_fname, (int)strlen (info.dli_fname));
						if(modulePath[0] != '\0')
						{
							reportCallingModule (modulePath);
							previousModule = info.dli_fbase;
						}
					}
				}
			}
		}
	}
}
