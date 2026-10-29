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
// Filename    : miniaudiodevice.h
// Description : Audio Device based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudiodevice_h
#define _miniaudiodevice_h

#include "miniaudioglue.h"

#include "../base/audiodevicebase.h"

#include "ccl/extras/audio/audiodatabuffer.h"

#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// MiniAudioDeviceFactory
//************************************************************************************************

class MiniAudioDeviceFactory: public AudioDeviceFactoryBase
{
public:
	DECLARE_CLASS (MiniAudioDeviceFactory, AudioDeviceFactoryBase)

	// AudioDeviceFactoryBase
	tresult CCL_API createInstance (IAudioComponent*& component,
									const AudioDeviceComponentDesc& description) override;
};

//************************************************************************************************
// MiniAudioDevice
//************************************************************************************************

class MiniAudioDevice: public AudioDeviceBase
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioDevice, AudioDeviceBase)

	MiniAudioDevice (const AudioDeviceComponentDesc& description);

	// AudioDeviceBase
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	CapabilityFlags CCL_API getCapabilities () const override;
	tresult CCL_API getProcessConfiguration (AudioProcessConfiguration& configuration) const override;
	tresult CCL_API setProcessConfiguration (const AudioProcessConfiguration& configuration) override;
	tresult CCL_API setDeviceOption (StringID optionId, VariantRef value) override;
	tresult CCL_API getDeviceOption (Variant& value, StringID optionId) const override;
	tresult CCL_API getBlockSizeRange (AudioBlockSizeRange& range) const override;
	tresult CCL_API getLatency (int& latencyInSamples, AudioStreamDirection direction) const override;
	tbool CCL_API isActive () const override;
	tresult CCL_API setActive (tbool active) override;

protected:
	static void dataCallback (ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	static void onPlatformNotification (const ma_device_notification* pNotification);

	class StreamBuffer: public AudioStream
	{
	public:
		StreamBuffer (StringRef name, int numSamples, int channelCount, SpeakerBitField speakerType)
		: AudioStream (name, kActive, channelCount, speakerType)
		{
			resize (numSamples);
		}

		void resize (int numSamples)
		{
			buffer.resize (numSamples, channelCount, kAudioFloat32); 
		}

		float32** getBuffers32 ()
		{
			return buffer.getAudioData ().buffers32; 
		}

	protected:
		AudioDataBuffer buffer;
	};

	AudioDeviceComponentDesc description;
	ma_device platformDevice = {};
	bool wantExclusiveMode = false;
	Threading::AtomicInt stopRequested;
	int streamBufferCapacity = 0;
	SampleRate requestedSampleRate = 0; // 0 = let the backend pick its native/default rate
	int requestedBlockSize = 0; // 0 = let the backend pick its default period size

	struct ProcessData
	{
		AudioCallbackContext context;
		AudioIOData ioData;

		void restart (SampleRate sampleRate, TSeconds bufferDuration);
		void beginCallback (int frameCount);
		void endCallback ();

	private:
		TSeconds streamStartTime = 0;
		TSeconds periodDuration = 0;
		TSeconds bufferDuration = 0;
		bool processingOverran = false;
	};

	ProcessData processData;

	ma_result initPlatformDevice ();
	void uninitPlatformDevice ();
	void allocateStreams ();
	void deallocateStreams ();
	bool isValid () const;
	int getMaxBlockSize () const;
	int clampBlockSize (int blockSize, SampleRate sampleRate) const;
	TSeconds getBufferDuration () const;

	bool canUseExclusiveMode () const;
	bool isExclusiveModeActive () const;

	#if DEBUG
	void inspectPlatformDevice ();
	#endif
};

} // namespace CCL

#endif // _miniaudiodevice_h