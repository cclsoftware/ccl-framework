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
// Filename    : ccl/platform/linux/backgroundservice.linux.cpp
// Description : Linux Background Service
//
//************************************************************************************************

#include "ccl/main/backgroundservice.h"

using namespace CCL;

//************************************************************************************************
// BackgroundService
//************************************************************************************************

bool BackgroundService::startPlatformService ()
{
	ASSERT (0) // implement me!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundService::startDevelopmentService ()
{
	ASSERT (0) // implement me!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackgroundService::flushPlatformUpdates ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackgroundService::install (bool state)
{
	ASSERT (0) // implement me!
	return false;
}
