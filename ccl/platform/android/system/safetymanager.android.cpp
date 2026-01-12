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
// Filename    : ccl/platform/android/system/safetymanager.android.cpp
// Description : Android Safety Manager
//
//************************************************************************************************

#include "ccl/system/safetymanager.h"

namespace CCL {

//************************************************************************************************
// AndroidSafetyManager
//************************************************************************************************

class AndroidSafetyManager: public SafetyManager
{
	
protected:
	// SafetyManager
	void enableCrashRecovery (bool state) override;
	void CCL_API reportException (void* exceptionInformation, const uchar* systemDumpFile) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AndroidSafetyManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SafetyManager, AndroidSafetyManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidSafetyManager::enableCrashRecovery (bool state)
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidSafetyManager::reportException (void* exceptionInformation, const uchar* systemDumpFile)
{
	// TODO
}
