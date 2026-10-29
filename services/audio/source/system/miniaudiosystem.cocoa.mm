//************************************************************************************************
//
// Audio Service
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : miniaudiosystem.cocoa.mm
// Description : Audio System based on miniaudio (macOS/iOS)
//
//************************************************************************************************

#include "miniaudiosystem.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/singleton.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/base/ccldefpush.h"
#include <AVFoundation/AVFoundation.h>

namespace CCL {

//************************************************************************************************
// CocoaAudioPermissionProvider
/** Handles the audio capture permission. */
//************************************************************************************************

class CocoaAudioPermissionProvider: public Unknown,
									public ISystemPermissionProvider,
									public StaticSingleton<CocoaAudioPermissionProvider>
{
public:
	// ISystemPermissionProvider
	PermissionState CCL_API getPermissionState (StringID permissionId) override;
	IAsyncOperation* CCL_API requestPermission (StringID permissionId) override;

	CLASS_INTERFACE (ISystemPermissionProvider, Unknown)
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// CocoaAudioPermissionProvider
//************************************************************************************************

PermissionState CCL_API CocoaAudioPermissionProvider::getPermissionState (StringID permissionId)
{
	if(permissionId == System::kPermissionAudioCapture)
	{
		switch([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio])
		{
		case AVAuthorizationStatusAuthorized :
			return System::kPermissionGranted;

		case AVAuthorizationStatusNotDetermined :
			return System::kPermissionUndetermined;

		default : // denied or restricted
			return System::kPermissionDenied;
		}
	}
	return System::kPermissionUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API CocoaAudioPermissionProvider::requestPermission (StringID permissionId)
{
	if(permissionId != System::kPermissionAudioCapture)
		return nullptr;

	PermissionState state = getPermissionState (permissionId);
	if(state != System::kPermissionUndetermined)
		return AsyncOperation::createCompleted (int(state), true);

	SharedPtr<AsyncOperation> operation = NEW AsyncOperation;
	operation->setState (AsyncOperation::kStarted);

	[AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted)
	{
		// the handler is called on an arbitrary queue, complete the operation on the main thread
		dispatch_async (dispatch_get_main_queue (), ^
		{
			operation->setResult (int(granted ? System::kPermissionGranted : System::kPermissionDenied));
			operation->setState (AsyncOperation::kCompleted);
		});
	}];

	return return_shared<IAsyncOperation> (operation);
}

//************************************************************************************************
// MiniAudioSystem
//************************************************************************************************

void MiniAudioSystem::initPlatform ()
{
	System::GetSystemShell ().registerPermissionProvider (&CocoaAudioPermissionProvider::instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioSystem::uninitPlatform ()
{
	System::GetSystemShell ().unregisterPermissionProvider (&CocoaAudioPermissionProvider::instance ());
}
