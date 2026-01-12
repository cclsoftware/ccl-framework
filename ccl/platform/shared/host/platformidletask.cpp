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
// Filename    : ccl/platform/shared/host/platformidletask.cpp
// Description : Platform Idle Task
//
//************************************************************************************************

#include "ccl/platform/shared/host/platformidletask.h"

#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace PlatformIntegration;

//************************************************************************************************
// PlatformIdleTask
//************************************************************************************************

PlatformIdleTask::PlatformIdleTask ()
: platformGUI (CCLGUI_PACKAGE_ID)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformIdleTask::initialize (CStringPtr applicationId)
{	
	platformGUI.load ();
	if(platformGUI)
		platformGUI->startup (applicationId);
	enableTimer (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformIdleTask::terminate ()
{	
	enableTimer (false);
	platformGUI.reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformIdleTask::onIdleTimer ()
{
	if(platformGUI)
		platformGUI->onIdle ();
}
