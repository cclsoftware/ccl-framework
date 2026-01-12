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
// Filename    : ccl/platform/shared/host/platformidletask.h
// Description : Platform Idle Task
//
//************************************************************************************************

#ifndef _ccl_platformidletask_h
#define _ccl_platformidletask_h

#include "ccl/platform/shared/interfaces/platformgui.h"
#include "ccl/platform/shared/host/iplatformintegrationloader.h"

#include "ccl/public/base/unknown.h"

#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {
namespace PlatformIntegration {
	
//************************************************************************************************
// PlatformIdleTask
//************************************************************************************************

class PlatformIdleTask: public Unknown,
						public IdleClient
{
public:
	PlatformIdleTask ();
	
	void initialize (CStringPtr applicationId = nullptr);
	void terminate ();
	
	CLASS_INTERFACE (ITimerTask, Unknown)
	
protected:
	PlatformIntegration::PlatformImplementationPtr<PlatformIntegration::IPlatformGUI> platformGUI;
	
	// IdleClient
	void onIdleTimer () override;
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformidletask_h
