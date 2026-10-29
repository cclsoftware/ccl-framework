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
// Filename    : miniaudiosystem.cpp
// Description : Audio System based on miniaudio
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "miniaudiosystem.h"

#include "ccl/public/collections/iunknownlist.h"

using namespace CCL;

//************************************************************************************************
// MiniAudioSystem
//************************************************************************************************

void MiniAudioSystem::onLog (void* userData, ma_uint32 level, const char* message)
{
	if(level <= MA_LOG_LEVEL_WARNING)
		CCL_WARN ("[miniaudio %s] %s", ma_log_level_to_string (level), message)
	else
		CCL_PRINTF ("[miniaudio %s] %s", ma_log_level_to_string (level), message)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioSystem::enableLogging (bool state)
{
	if(state)
	{
		if(!logInitialized)
		{
			logInitialized = ma_log_init (nullptr, &log) == MA_SUCCESS;
			if(logInitialized)
				ma_log_register_callback (&log, ma_log_callback_init (onLog, this));
		}
	}
	else
	{
		if(logInitialized)
		{
			ma_log_uninit (&log);
			logInitialized = false;
		}

	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioSystem::cleanup ()
{
	#if AUDIOSYSTEM_PLATFORMINIT_NEEDED
	uninitPlatform ();
	#endif

	enableLogging (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioSystem::startup ()
{
	enableLogging (true);

	ma_context_config config = ma_context_config_init ();
	if(logInitialized)
		config.pLog = &log;

	#if CCL_PLATFORM_LINUX
	// The default priority results in a normal time-shared thread on Linux (SCHED_OTHER).
	// Request SCHED_FIFO for the audio thread; silently falls back to normal priority 
	// if the process is not permitted to.
	config.threadPriority = ma_thread_priority_realtime;

	#elif AUDIOSYSTEM_PLATFORMINIT_NEEDED
	initPlatform ();
	#endif

	if(ma_context_init (nullptr, 0, &config, &context) != MA_SUCCESS)
	{
		cleanup ();
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioSystem::shutdown ()
{
	ma_context_uninit (&context);

	cleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioSystem::getDevices (IUnknownList& resultList, int flags)
{
	ma_device_info* playbackDeviceInfos = nullptr;
	ma_uint32 playbackDeviceCount = 0;
	ma_device_info* captureDeviceInfos = nullptr;
	ma_uint32 captureDeviceCount = 0;
	if(ma_context_get_devices (&context, &playbackDeviceInfos, &playbackDeviceCount, &captureDeviceInfos, &captureDeviceCount) != MA_SUCCESS)
		return false;

	const ma_context* contextPointer = &context;
	auto createDeviceDescription = [contextPointer] (const ma_device_info& deviceInfo, int flags) -> IDeviceDescription*
	{
		String name;
		name.appendCString (Text::kUTF8, deviceInfo.name);
		String id = MiniAudioUtils::deviceIdToString (deviceInfo.id, contextPointer->backend);

		if(deviceInfo.isDefault)
			flags |= IDeviceDescription::kIsSystemDefault;

		return NEW DeviceDescription (id, name, flags);
	};

	if(get_flag (flags, IDeviceDescription::kMediaRenderCapability))
	{
		for(ma_uint32 i = 0; i < playbackDeviceCount; i++)
			resultList.add (createDeviceDescription (playbackDeviceInfos[i], IDeviceDescription::kMediaRenderCapability));
	}

	if(get_flag (flags, IDeviceDescription::kMediaCaptureCapability))
	{
		for(ma_uint32 i = 0; i < captureDeviceCount; i++)
			resultList.add (createDeviceDescription (captureDeviceInfos[i], IDeviceDescription::kMediaCaptureCapability));
	}

	return true;
}

//************************************************************************************************
// MiniAudioDeviceEnumerator
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioDeviceEnumerator, DeviceEnumerator)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDeviceEnumerator::enumerateDevices (IUnknownList& resultList, int flags)
{
	if(!MiniAudioSystem::instance ().getDevices (resultList, flags))
		return kResultFailed;
	return kResultOk;
}
