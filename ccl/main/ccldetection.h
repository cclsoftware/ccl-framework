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
// Filename    : ccl/main/ccldetection.h
// Description : CCL Detection
//
//************************************************************************************************

#ifndef _ccldetection_h
#define _ccldetection_h

#include "ccl/main/platformmodule.h"

/** Detect if CCL libraries are loaded in current process. */
inline bool IsCCLFrameworkHostProcess ()
{
	typedef CCL::tbool (CCL_API *IsFrameworkHostProcessProc) ();

	static int frameworkDetected = -1;
	if(frameworkDetected == -1)
	{
		bool moduleFound = false;
		IsFrameworkHostProcessProc hostProcessCheck = 0;

		if(CCL::ModuleRef moduleRef = CCL::PlatformModuleHelper::getModule (CCL_MODULE_NAME ("cclgui")))
		{
			moduleFound = true;
			hostProcessCheck = reinterpret_cast<IsFrameworkHostProcessProc> (CCL::PlatformModuleHelper::getFunction (moduleRef, CCL_FUNCTION_NAME ("IsFrameworkHostProcess")));
		}
		
		if(moduleFound == false) // no CCL libraries loaded
			frameworkDetected = 0;
		else
		{
			if(hostProcessCheck == 0) // older version of CCL loaded
				frameworkDetected = 1;
			else
				frameworkDetected = (*hostProcessCheck) () ? 1 : 0;
		}
	}
	return frameworkDetected == 1;
}

#endif // _ccldetection_h
