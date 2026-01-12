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
// Filename    : ccl/platform/linux/gui/alert.linux.cpp
// Description : Linux Alert Dialog Implementation
//
//************************************************************************************************

#include "ccl/gui/windows/systemwindow.h"

#include "ccl/platform/linux/gui/platformdialog.linux.h"

#include "ccl/platform/shared/host/platformalertbase.h"

namespace CCL {

//************************************************************************************************
// LinuxAlertBox
//************************************************************************************************

class LinuxAlertBox: public PlatformIntegration::PlatformAlertBoxBase,
					 public LinuxPlatformDialog
{
public:
	DECLARE_CLASS (LinuxAlertBox, PlatformAlertBoxBase)

	LinuxAlertBox ();
	
	// PlatformAlertBoxBase
	IAsyncOperation* runAsyncPlatform () override;
	void closed (int result) override;
	void opened (void* nativeWindowHandle) override;
};

} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxAlertBox
//************************************************************************************************

DEFINE_CLASS (LinuxAlertBox, PlatformAlertBoxBase)
DEFINE_CLASS_UID (LinuxAlertBox, 0x9bf3ecb5, 0x5bb2, 0x4eb4, 0xaa, 0xac, 0x29, 0xaf, 0xf4, 0x66, 0x45, 0xa5) // ClassID::AlertBox

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxAlertBox::LinuxAlertBox ()
{
	nativeDialog = platformAlert;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* LinuxAlertBox::runAsyncPlatform ()
{
	AutoPtr<AsyncSequence> sequence = NEW AsyncSequence;
	sequence->setCancelOnError (false);
	sequence->add ([=] () { return setParentWindow (); });
	sequence->add ([=] () { return SuperClass::runAsyncPlatform (); });
	return return_shared<IAsyncOperation> (sequence->start ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxAlertBox::opened (void* nativeWindowHandle)
{
	onPlatformDialogOpened (static_cast<PlatformIntegration::NativeWindowHandle*> (nativeWindowHandle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxAlertBox::closed (int result)
{
	SuperClass::closed (result);
	onPlatformDialogClosed ();
}
