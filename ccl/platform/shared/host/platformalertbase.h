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
// Filename    : ccl/platform/shared/host/platformalertbase.h
// Description : Platform Alert Dialog
//
//************************************************************************************************

#ifndef _ccl_platformalertbase_h
#define _ccl_platformalertbase_h

#include "ccl/platform/shared/interfaces/platformalert.h"
#include "ccl/platform/shared/host/iplatformintegrationloader.h"
#include "ccl/platform/shared/host/frameworkalert.h"

namespace CCL {
namespace PlatformIntegration {
	
//************************************************************************************************
// PlatformAlertBoxBase
//************************************************************************************************

class PlatformAlertBoxBase: public FrameworkAlertBox,
							public PlatformIntegration::IPlatformAlertObserver
{
public:
	DECLARE_CLASS_ABSTRACT (PlatformAlertBoxBase, FrameworkAlertBox)

	PlatformAlertBoxBase ();
	
	// AlertBox
	void closePlatform () override;
	IAsyncOperation* runAsyncPlatform () override;
	
	// IPlatformAlertObserver
	void closed (int result) override;
	void opened (void* nativeWindowHandle) override;
	
protected:
	PlatformIntegration::PlatformImplementationPtr<PlatformIntegration::IPlatformAlert> platformAlert;
	SharedPtr<AsyncOperation> operation;
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformalertbase_h
