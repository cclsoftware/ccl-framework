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
// Filename    : miniaudiosystem.h
// Description : Audio System based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudiosystem_h
#define _miniaudiosystem_h

#include "miniaudioglue.h"

#include "ccl/base/singleton.h"

#include "ccl/extras/devices/deviceenumerator.h"

namespace CCL {

// Some platforms need additional initialization
#if CCL_PLATFORM_WASM || CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
	#define AUDIOSYSTEM_PLATFORMINIT_NEEDED 1
#endif

//************************************************************************************************
// MiniAudioSystem
//************************************************************************************************

class MiniAudioSystem: public StaticSingleton<MiniAudioSystem>
{
public:
	bool startup ();
	void shutdown ();

	ma_context& getContext () { return context; }

	bool getDevices (IUnknownList& resultList, int flags);

protected:
	static void onLog (void* userData, ma_uint32 level, const char* message);

	ma_log log = {};
	bool logInitialized = false;
	ma_context context = {};

	void enableLogging (bool state);
	void cleanup ();

	#if AUDIOSYSTEM_PLATFORMINIT_NEEDED
	void initPlatform ();
	void uninitPlatform ();
	#endif
};

//************************************************************************************************
// MiniAudioDeviceEnumerator
//************************************************************************************************

class MiniAudioDeviceEnumerator: public DeviceEnumerator
{
public:
	DECLARE_CLASS (MiniAudioDeviceEnumerator, DeviceEnumerator)

	// DeviceEnumerator
	tresult CCL_API enumerateDevices (IUnknownList& resultList, int flags) override;
};

} // namespace CCL

#endif // _miniaudiosystem_h
