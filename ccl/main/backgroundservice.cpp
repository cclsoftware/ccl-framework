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
// Filename    : ccl/main/backgroundservice.cpp
// Description : Background Service
//
//************************************************************************************************

#include "ccl/main/backgroundservice.h"
#include "ccl/main/cclargs.h"

#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// BackgroundService
//************************************************************************************************

BackgroundService* BackgroundService::theInstance = nullptr;
BackgroundService* BackgroundService::getInstance () { return theInstance; }

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (BackgroundService, Object)
DEFINE_STRINGID_MEMBER_ (BackgroundService, kDeviceNotification, "deviceNotification")

//////////////////////////////////////////////////////////////////////////////////////////////////

BackgroundService::BackgroundService (StringRef name, StringRef description, StringRef company, int versionInt)
: name (name),
  description (description),
  runningAsService (false),
  highPerformanceMode (false)
{
	ASSERT (theInstance == 0)
	theInstance = this;

	System::GetSystem ().setApplicationName (company, name, versionInt);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BackgroundService::~BackgroundService ()
{
	cancelSignals ();

	ASSERT (theInstance == this)
	theInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef BackgroundService::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef BackgroundService::getDescription () const
{
	return description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundService::isRunningAsService () const
{
	return runningAsService;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BackgroundService::run (bool developerMode)
{
	if(developerMode)
		return startDevelopmentService () ? 0 : -1;
	else
	{
		ScopedVar<bool> runningGuard (runningAsService, true);
		return startPlatformService () ? 0 : -1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundService::onIdle ()
{
	System::GetSignalHandler ().flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if !CCL_PLATFORM_WINDOWS // not used on other platforms
void BackgroundService::enableDeviceNotifications (const ConstVector<UID>& filter) {}
#endif

