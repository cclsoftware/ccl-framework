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
// Filename    : miniaudiosystem.wasm.cpp
// Description : Audio System based on miniaudio (WebAssembly)
//
//************************************************************************************************

#include "miniaudiosystem.h"

#include <emscripten.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
// Web Audio callback bridge
//////////////////////////////////////////////////////////////////////////////////////////////////

// Defined by miniaudio's Web Audio backend and called by name from its EM_ASM code.
extern "C" void ma_device_process_pcm_frames_playback__webaudio (ma_device* pDevice, int frameCount, float* pFrames);
extern "C" void ma_device_process_pcm_frames_capture__webaudio (ma_device* pDevice, int frameCount, float* pFrames);
extern "C" void ma_device__on_notification_unlocked (ma_device* pDevice);

// miniaudio's EM_ASM code references these exports as global JS functions (e.g. _ma_device_process_pcm_frames_playback__webaudio).
// That only resolves when miniaudio is linked into the main module. Exports of a dynamically loaded side module are not
// visible under these names, so publish them explicitly and forward to the function pointers via dynCall.
EM_JS_DEPS (miniAudioSystemDependencies, "$dynCall");

EM_JS (void, installWebAudioCallbacks, (void* processPlayback, void* processCapture, void* notifyUnlocked),
{
	globalThis._ma_device_process_pcm_frames_playback__webaudio = (device, frameCount, frames) => dynCall ('viii', processPlayback, [device, frameCount, frames]);
	globalThis._ma_device_process_pcm_frames_capture__webaudio = (device, frameCount, frames) => dynCall ('viii', processCapture, [device, frameCount, frames]);
	globalThis._ma_device__on_notification_unlocked = (device) => dynCall ('vi', notifyUnlocked, [device]);
})

EM_JS (void, uninstallWebAudioCallbacks, (),
{
	delete globalThis._ma_device_process_pcm_frames_playback__webaudio;
	delete globalThis._ma_device_process_pcm_frames_capture__webaudio;
	delete globalThis._ma_device__on_notification_unlocked;
})

using namespace CCL;

//************************************************************************************************
// MiniAudioSystem
//************************************************************************************************

void MiniAudioSystem::initPlatform ()
{
	installWebAudioCallbacks (reinterpret_cast<void*> (ma_device_process_pcm_frames_playback__webaudio),
							  reinterpret_cast<void*> (ma_device_process_pcm_frames_capture__webaudio),
							  reinterpret_cast<void*> (ma_device__on_notification_unlocked));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioSystem::uninitPlatform ()
{
	uninstallWebAudioCallbacks ();
}
