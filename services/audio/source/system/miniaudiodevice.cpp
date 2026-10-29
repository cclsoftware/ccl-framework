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
// Filename    : miniaudiodevice.cpp
// Description : Audio Device based on miniaudio
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "miniaudiodevice.h"
#include "miniaudiosystem.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// MiniAudioDeviceFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioDeviceFactory, AudioDeviceFactoryBase)

//////////////////////////////////////////////////////////////////////////////////////////////////
 
tresult CCL_API MiniAudioDeviceFactory::createInstance (IAudioComponent*& component,
														const AudioDeviceComponentDesc& description)
{
	ASSERT (description.isValid () == true)
	if(!description.isValid ())
		return kResultInvalidArgument;

	component = NEW MiniAudioDevice (description);
	return kResultOk;
}

//************************************************************************************************
// MiniAudioDevice::ProcessData
//************************************************************************************************

void MiniAudioDevice::ProcessData::restart (SampleRate sampleRate, TSeconds _bufferDuration)
{
	context.samplePos = 0;
	context.sampleRate = sampleRate;
	bufferDuration = _bufferDuration;
	streamStartTime = 0; // managed by callback
	processingOverran = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioDevice::ProcessData::beginCallback (int frameCount)
{
	ioData.numSamples = frameCount;

	context.systemTime = System::GetProfileTime ();
	context.flags = 0;

	periodDuration = TSeconds(frameCount) / context.sampleRate;

	// Anchor to the actual first callback
	if(context.samplePos == 0)
		streamStartTime = context.systemTime;

	// Overload check 1: compare the actual arrival time of this callback against where it should be
	// on the stream's continuous sample timeline. miniaudio does not report backend-level under-/overruns
	// so falling behind schedule is the only observable symptom.

	// Callbacks are not delivered at an exact cadence (devices work with several buffered periods and may deliver in
	// bursts), so allow some scheduling jitter, independent of how short the period is.
	static constexpr TSeconds kMinJitterTolerance = 0.01;
	static constexpr int kJitterTolerancePeriods = 3;
	TSeconds tolerance = get_max<TSeconds> (TSeconds(kJitterTolerancePeriods) * periodDuration, kMinJitterTolerance);

	// Server-driven backends work in chunks that can be much larger than our period and
	// then deliver several callbacks back to back. A callback is only really late when it arrives after the audio
	// buffered by the device has run out, so never be stricter than the device buffer duration.
	tolerance = get_max<TSeconds> (tolerance, bufferDuration);

	TSeconds streamTime = TSeconds(context.samplePos) / context.sampleRate;
	TSeconds expectedTime = streamStartTime + streamTime;
	if(context.systemTime > expectedTime + tolerance)
	{
		// report a stall once and continue on the new timeline
		context.flags |= AudioCallbackContext::kOverload;
		streamStartTime = context.systemTime - streamTime;
	}
	else if(context.systemTime < expectedTime - tolerance)
	{
		// the device caught up after a stall
		streamStartTime = context.systemTime - streamTime;
	}

	// Overload check 2: did the previous processAudio() call itself exceed its time budget? This can
	// only be known after that call returns, so it is necessarily reported one callback late.

	if(processingOverran)
		context.flags |= AudioCallbackContext::kOverload;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioDevice::ProcessData::endCallback ()
{
	TSeconds processingDuration = System::GetProfileTime () - context.systemTime;

	processingOverran = processingDuration > periodDuration;
	context.samplePos += ioData.numSamples;
}

//************************************************************************************************
// MiniAudioDevice
//************************************************************************************************

void MiniAudioDevice::dataCallback (ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	auto* This = reinterpret_cast<MiniAudioDevice*> (pDevice->pUserData);
	ASSERT (This != nullptr)

	// sanity check in case configuration changes unexpectedly
	ASSERT (int(frameCount) <= This->streamBufferCapacity)
	if(int(frameCount) > This->streamBufferCapacity)
	{
		if(pOutput != nullptr)
			ma_silence_pcm_frames (pOutput, frameCount, pDevice->playback.format, pDevice->playback.channels);
		return;
	}

	ProcessData& processData = This->processData;
	processData.beginCallback (frameCount);

	AudioIOData& ioData = processData.ioData;

	if(pInput != nullptr && ioData.numInputs > 0)
	{
		ASSERT (pDevice->capture.format == ma_format_f32)
		ASSERT (get_min<int> (int(pDevice->capture.channels), kMaxAudioStreamChannels) == ioData.inputs[0].numChannels)

		// the device can have more channels than we expose
		MiniAudioUtils::deinterleaveFloat32 (ioData.inputs[0].buffers32, ioData.inputs[0].numChannels,
											 pInput, int(pDevice->capture.channels), 0,
											 frameCount);
	}

	if(This->processCallback)
		This->processCallback->processAudio (processData.context, ioData);

	if(pOutput != nullptr && ioData.numOutputs > 0)
	{
		ASSERT (pDevice->playback.format == ma_format_f32)
		ASSERT (get_min<int> (int(pDevice->playback.channels), kMaxAudioStreamChannels) == ioData.outputs[0].numChannels)

		// the device can have more channels than we expose, the others are silent
		MiniAudioUtils::interleaveFloat32 (pOutput, int(pDevice->playback.channels), 0,
										   ioData.outputs[0].buffers32, ioData.outputs[0].numChannels, 
										   frameCount);
	}

	if(This->processCallback)
		This->processCallback->postProcess ();

	processData.endCallback ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioDevice::onPlatformNotification (const ma_device_notification* pNotification)
{
	auto* This = reinterpret_cast<MiniAudioDevice*> (pNotification->pDevice->pUserData);
	ASSERT (This != nullptr)

	CCL_PRINTF ("MiniAudioDevice notification received: type = %d...\n", int(pNotification->type))

	switch(pNotification->type)
	{
	case ma_device_notification_type_started :
		// nothing here
		break;

	case ma_device_notification_type_stopped :
		if(This->stopRequested == 0)
			This->sendNotification (IAudioDeviceNotificationCallback::kDeviceLost);
		break;

	case ma_device_notification_type_rerouted :
		This->sendNotification (IAudioDeviceNotificationCallback::kConfigurationChanged);
		break;

	case ma_device_notification_type_interruption_began :
		This->sendNotification (IAudioDeviceNotificationCallback::kInterruptionBegan);
		break;

	case ma_device_notification_type_interruption_ended :
		This->sendNotification (IAudioDeviceNotificationCallback::kInterruptionEnded);
		break;

	case ma_device_notification_type_unlocked :
		This->sendNotification (IAudioDeviceNotificationCallback::kDeviceUnlocked);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioDevice, AudioDeviceBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioDevice::MiniAudioDevice (const AudioDeviceComponentDesc& description)
: description (description)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::initialize (IUnknown* context)
{
	ASSERT (isValid () == false)
	if(isValid ())
		return kResultUnexpected;

	ma_result result = initPlatformDevice ();
	if(result != MA_SUCCESS)
		return kResultFailed;

	#if (1 && DEBUG)
	inspectPlatformDevice ();
	#endif

	allocateStreams ();

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::terminate ()
{
	stopRequested = 1;

	uninitPlatformDevice ();
	deallocateStreams ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioComponent::CapabilityFlags CCL_API MiniAudioDevice::getCapabilities () const
{
	return	kCapabilityKnownStreamLayout|
			kCapabilityStreamActivation|
			kCapabilityKnownLatency;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::getProcessConfiguration (AudioProcessConfiguration& configuration) const
{
	ASSERT (isValid () == true)
	if(!isValid ())
		return kResultUnexpected;

	configuration.sampleRate = platformDevice.sampleRate;
	configuration.maxBlockSize = getMaxBlockSize ();
	configuration.floatType = kAudioFloat32;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::setProcessConfiguration (const AudioProcessConfiguration& configuration)
{
	ASSERT (isValid () == true && isActive () == false)
	if(!isValid () || isActive ())
		return kResultUnexpected;

	requestedSampleRate = configuration.sampleRate;
	requestedBlockSize = configuration.maxBlockSize;

	// miniaudio has no live-reconfigure API for an open device - tear it down and recreate it
	stopRequested = 1;
	uninitPlatformDevice ();
	deallocateStreams ();

	ma_result result = initPlatformDevice ();
	if(result != MA_SUCCESS)
		return kResultFailed;

	allocateStreams ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::setDeviceOption (StringID optionId, VariantRef value)
{
	if(optionId == AudioDeviceOptions::kExclusiveMode)
	{
		wantExclusiveMode = value.asBool ();
		return kResultOk;
	}

	return SuperClass::setDeviceOption (optionId, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::getDeviceOption (Variant& value, StringID optionId) const
{
	if(optionId == AudioDeviceOptions::kCanExclusiveMode)
	{
		value = canUseExclusiveMode ();
		return kResultOk;
	}
	if(optionId == AudioDeviceOptions::kExclusiveMode)
	{
		value = isValid () ? isExclusiveModeActive () : wantExclusiveMode;
		return kResultOk;
	}

	return SuperClass::getDeviceOption (value, optionId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::getBlockSizeRange (AudioBlockSizeRange& range) const
{
	range = AudioBlockSizeRange ();
	range.unit = AudioBlockSizeRange::kSampleFrames;

	switch(MiniAudioSystem::instance ().getContext ().backend)
	{
	case ma_backend_coreaudio :
		range.minBlockSize = 32;
		range.maxBlockSize = 4096;
		range.preferredBlockSize = 512;
		range.step = AudioBlockSizeRange::kStepPowerOfTwo; // CoreAudio devices commonly reject non-power-of-two sizes
		break;

	case ma_backend_alsa :
		range.minBlockSize = 32;
		range.maxBlockSize = 8192;
		range.preferredBlockSize = 1024;
		range.step = AudioBlockSizeRange::kStepAny;
		break;

	case ma_backend_pulseaudio :
		// The sound server (PulseAudio, or PipeWire's pulse server) schedules in time-based chunks of its own
		// (typically 20-25 ms): requesting less results in high load and underruns.
		range.unit = AudioBlockSizeRange::kMilliseconds;
		range.minBlockSize = 20;
		range.maxBlockSize = 100;
		range.preferredBlockSize = 30;
		range.step = AudioBlockSizeRange::kStepAny;
		break;

	case ma_backend_webaudio :
		#if defined(MA_ENABLE_AUDIO_WORKLETS)
		range.minBlockSize = range.maxBlockSize = range.preferredBlockSize = 128; // fixed AudioWorklet processing quantum
		range.step = AudioBlockSizeRange::kStepAny;
		#else
		// ScriptProcessorNode fallback (runs on the main thread): miniaudio forces the period to a power of two in [256, 16384]
		range.minBlockSize = 256;
		range.maxBlockSize = 16384;
		range.preferredBlockSize = 1024;
		range.step = AudioBlockSizeRange::kStepPowerOfTwo;
		#endif
		break;

	case ma_backend_wasapi :
		if((isValid () && isExclusiveModeActive () == false) || wantExclusiveMode == false)
		{
			// shared mode: the period is time-based and the resulting frame count depends on the sample rate
			range.unit = AudioBlockSizeRange::kMilliseconds;
			range.minBlockSize = 10;
			range.maxBlockSize = 100;
			range.preferredBlockSize = 10;
		}
		else
		{
			range.minBlockSize = 64;
			range.maxBlockSize = 2048;
			range.preferredBlockSize = 128;
		}
		range.step = AudioBlockSizeRange::kStepAny;
		break;

	default :
		range.minBlockSize = 64;
		range.maxBlockSize = 4096;
		range.preferredBlockSize = 512;
		range.step = AudioBlockSizeRange::kStepAny;
		break;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::getLatency (int& latencyInSamples, AudioStreamDirection direction) const
{
	ASSERT (isValid () == true)
	if(!isValid ())
		return kResultUnexpected;

	if(direction == kAudioRenderDirection)
	{
		const auto& sub = platformDevice.playback;
		latencyInSamples = int(sub.internalPeriodSizeInFrames) * int(sub.internalPeriods);
	}
	else
	{
		const auto& sub = platformDevice.capture;
		latencyInSamples = int(sub.internalPeriodSizeInFrames) * int(sub.internalPeriods);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MiniAudioDevice::isActive () const
{
	return isValid () && ma_device_is_started (&platformDevice);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDevice::setActive (tbool state)
{
	ASSERT (isValid () == true)
	if(!isValid ())
		return kResultUnexpected;

	if(state)
	{
		stopRequested = 0;
		processData.restart (platformDevice.sampleRate, getBufferDuration ());

		ma_result result = ma_device_start (&platformDevice);
		if(result != MA_SUCCESS)
			return kResultFailed;
	}
	else
	{
		stopRequested = 1;
		ma_result result = ma_device_stop (&platformDevice);
		if(result != MA_SUCCESS)
			return kResultFailed;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ma_result MiniAudioDevice::initPlatformDevice ()
{
	ASSERT (isValid () == false)

	ma_context& context = MiniAudioSystem::instance ().getContext ();

	ma_device_type deviceType = description.isFullDuplex () ? ma_device_type_duplex :
								description.isRenderOnly () ? ma_device_type_playback :
								ma_device_type_capture;

	ma_device_config config = ma_device_config_init (deviceType);
	config.dataCallback = dataCallback;
	config.notificationCallback = onPlatformNotification;
	config.pUserData = this;
	config.sampleRate = ma_uint32(requestedSampleRate);

	ma_device_id playbackDeviceId = {};
	ma_device_id captureDeviceId = {};
	if(description.hasRenderId ())
	{
		MiniAudioUtils::stringToDeviceId (playbackDeviceId, description.renderDeviceId, context.backend);
		config.playback.pDeviceID = &playbackDeviceId;
	}
	if(description.hasCaptureId ())
	{
		MiniAudioUtils::stringToDeviceId (captureDeviceId, description.captureDeviceId, context.backend);
		config.capture.pDeviceID = &captureDeviceId;
	}

	config.playback.format = ma_format_f32; // pin to 32 bit floating-point
	config.capture.format = ma_format_f32;

	ma_share_mode shareMode = wantExclusiveMode ? ma_share_mode_exclusive : ma_share_mode_shared;
	config.playback.shareMode = shareMode;
	config.capture.shareMode = shareMode;

	#if CCL_PLATFORM_WINDOWS
	// WASAPI only: register the audio thread with MMCSS ("Pro Audio" task) for scheduling guarantees
	config.wasapi.usage = ma_wasapi_usage_pro_audio;
	#endif

	if(requestedBlockSize > 0)
		config.periodSizeInFrames = ma_uint32(clampBlockSize (requestedBlockSize, requestedSampleRate)); // explicit request takes priority
	else
	{
		AudioBlockSizeRange range;
		getBlockSizeRange (range);
		if(range.unit == AudioBlockSizeRange::kMilliseconds)
			config.periodSizeInMilliseconds = ma_uint32(range.preferredBlockSize); // miniaudio converts using the actual sample rate
		else
			config.periodSizeInFrames = ma_uint32(range.preferredBlockSize);
	}

	return ma_device_init (&context, &config, &platformDevice);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioDevice::uninitPlatformDevice ()
{
	ma_device_uninit (&platformDevice);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioDevice::allocateStreams ()
{
	int renderChannelCount = int(platformDevice.playback.channels);
	int captureChannelCount = int(platformDevice.capture.channels);

	// Devices can have more channels than the maximum supported by one audio stream
	// LATER TODO: Expose multiple (mono) streams...
	if(renderChannelCount > kMaxAudioStreamChannels || captureChannelCount > kMaxAudioStreamChannels)
	{
		CCL_WARN ("MiniAudioDevice: Limiting channels to %d (render: %d, capture: %d)\n",
				  kMaxAudioStreamChannels, renderChannelCount, captureChannelCount)
	}

	renderChannelCount = get_min (renderChannelCount, kMaxAudioStreamChannels);
	captureChannelCount = get_min (captureChannelCount, kMaxAudioStreamChannels);
	int maxChannels = get_max (renderChannelCount, captureChannelCount);

	bool hasRender = renderChannelCount > 0;
	bool hasCapture = captureChannelCount > 0;

	processData.ioData.allocateStreams (hasCapture ? 1 : 0, hasRender ? 1 : 0, maxChannels);

	int maxBlockSize = getMaxBlockSize ();
	streamBufferCapacity = maxBlockSize;

	if(hasRender)
	{
		String name (Text::kUTF8, platformDevice.playback.name);
		SpeakerBitField speakerType = MiniAudioUtils::toSpeakerFormat (platformDevice.playback.channelMap, renderChannelCount);

		auto* streamBuffer = NEW StreamBuffer (name, maxBlockSize, renderChannelCount, speakerType);
		streamLayout.add (kAudioRenderDirection, streamBuffer);

		processData.ioData.setOutputStreamBuffers (0, streamBuffer->getBuffers32 (), renderChannelCount);
	}

	if(hasCapture)
	{
		String name (Text::kUTF8, platformDevice.capture.name);
		SpeakerBitField speakerType = MiniAudioUtils::toSpeakerFormat (platformDevice.capture.channelMap, captureChannelCount);

		auto* streamBuffer = NEW StreamBuffer (name, maxBlockSize, captureChannelCount, speakerType);
		streamLayout.add (kAudioCaptureDirection, streamBuffer);

		processData.ioData.setInputStreamBuffers (0, streamBuffer->getBuffers32 (), captureChannelCount);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioDevice::deallocateStreams ()
{
	streamBufferCapacity = 0;
	processData.ioData.deallocateStreams ();
	streamLayout.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioDevice::isValid () const
{
	return platformDevice.pUserData == this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioDevice::clampBlockSize (int blockSize, SampleRate sampleRate) const
{
	AudioBlockSizeRange range;
	getBlockSizeRange (range);

	int minFrames = range.minBlockSize;
	int maxFrames = range.maxBlockSize;
	if(range.unit == AudioBlockSizeRange::kMilliseconds)
	{
		if(sampleRate <= 0)
			return blockSize; // frame count of the range limits not known yet

		minFrames = int(sampleRate * range.minBlockSize / 1000.);
		maxFrames = int(sampleRate * range.maxBlockSize / 1000.);
	}

	return ccl_bound<int> (blockSize, minFrames, maxFrames);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSeconds MiniAudioDevice::getBufferDuration () const
{
	int frames = get_max (int(platformDevice.playback.internalPeriodSizeInFrames) * int(platformDevice.playback.internalPeriods),
						  int(platformDevice.capture.internalPeriodSizeInFrames) * int(platformDevice.capture.internalPeriods));
	return platformDevice.sampleRate > 0 ? TSeconds(frames) / TSeconds(platformDevice.sampleRate) : 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioDevice::getMaxBlockSize () const
{
	return get_max (platformDevice.playback.intermediaryBufferCap,
					platformDevice.capture.intermediaryBufferCap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioDevice::canUseExclusiveMode () const
{
	#if CCL_PLATFORM_WINDOWS	
	return true; // supported on Windows with WASAPI
	#else

	/*
	ASSERT (isValid () == true)
	if(!isValid ())
		return false;

	auto supportsExclusive = [] (const ma_device_info& deviceInfo)
	{
		for(uint32 i = 0; i < deviceInfo.nativeDataFormatCount; i++)
			if(get_flag (deviceInfo.nativeDataFormats[i].flags, MA_DATA_FORMAT_FLAG_EXCLUSIVE_MODE))
				return true;
		return false;
	};

	if(platformDevice.playback.channels > 0)
	{
		ma_device_info deviceInfo = {};
		if(ma_device_get_info (const_cast<ma_device*> (&platformDevice), ma_device_type_playback, &deviceInfo) != MA_SUCCESS || 
		   !supportsExclusive (deviceInfo))
			return false;
	}

	if(platformDevice.capture.channels > 0)
	{
		ma_device_info deviceInfo = {};
		if(ma_device_get_info (const_cast<ma_device*> (&platformDevice), ma_device_type_capture, &deviceInfo) != MA_SUCCESS ||
		   !supportsExclusive (deviceInfo))
			return false;
	}
	*/

	return false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioDevice::isExclusiveModeActive () const
{
	if(platformDevice.playback.channels > 0)
		return platformDevice.playback.shareMode == ma_share_mode_exclusive;

	if(platformDevice.capture.channels > 0)
		return platformDevice.capture.shareMode == ma_share_mode_exclusive;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void MiniAudioDevice::inspectPlatformDevice ()
{
	// see what we got in debugger...
 	ma_result result = MA_SUCCESS;

	ma_uint32 sampleRate = platformDevice.sampleRate;
	CCL_PRINTF ("Sample rate = %d\n", int(sampleRate))

	ma_format playbackSampleFormat = platformDevice.playback.format;
	ma_uint32 playbackChannelCount = platformDevice.playback.channels;
	ma_uint32 playbackBufferSize = platformDevice.playback.intermediaryBufferCap;

	CCL_PRINTF ("Playback: sample format = %d, channel count = %d, buffer size = %d\n",
				int(playbackSampleFormat), int(playbackChannelCount), int(playbackBufferSize))

	ma_uint32 captureChannelCount = platformDevice.capture.channels;
	ma_format captureSampleFormat = platformDevice.capture.format;
	ma_uint32 captureBufferSize = platformDevice.capture.intermediaryBufferCap;

	CCL_PRINTF ("Capture: sample format = %d, channel count = %d, buffer size = %d\n",
				int(captureSampleFormat), int(captureChannelCount), int(captureBufferSize))

	auto inspectDeviceInfo = [] (CStringPtr hint, const ma_device_info& deviceInfo)
	{
		for(uint32 i = 0; i < deviceInfo.nativeDataFormatCount; i++)
		{
			const auto& nativeFormat = deviceInfo.nativeDataFormats[i];
			
			ma_format sampleFormat = nativeFormat.format;
			ma_uint32 channels = nativeFormat.channels;
			ma_uint32 sampleRate = nativeFormat.sampleRate;
			bool exclusiveMode = get_flag (nativeFormat.flags, MA_DATA_FORMAT_FLAG_EXCLUSIVE_MODE);

			CCL_PRINTF ("%s Native Format %d: sample format = %d, channel count = %d, sample rate = %d (exclusive mode = %s)\n",
						hint, int(i), int(sampleFormat), int(channels), int(sampleRate), exclusiveMode ? "true" : "false")
		}
	};

	if(platformDevice.type == ma_device_type_duplex)
	{
		ma_device_info playbackDeviceInfo = {};
		result = ma_device_get_info (&platformDevice, ma_device_type_playback, &playbackDeviceInfo);
		ASSERT (result == MA_SUCCESS)
		inspectDeviceInfo ("Playback", playbackDeviceInfo);

		ma_device_info captureDeviceInfo = {};
		result = ma_device_get_info (&platformDevice, ma_device_type_capture, &captureDeviceInfo);
		ASSERT (result == MA_SUCCESS)
		inspectDeviceInfo ("Capture", captureDeviceInfo);
	}
	else
	{
		ma_device_info deviceInfo = {};
		result = ma_device_get_info (&platformDevice, platformDevice.type, &deviceInfo);
		ASSERT (result == MA_SUCCESS)
		inspectDeviceInfo (platformDevice.type == ma_device_type_playback ? "Playback" : "Capture", deviceInfo);
	}
}
#endif
