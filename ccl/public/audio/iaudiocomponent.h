//************************************************************************************************
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
// Filename    : ccl/public/audio/iaudiocomponent.h
// Description : Audio Component Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudiocomponent_h
#define _ccl_iaudiocomponent_h

#include "ccl/public/plugins/icomponent.h"

#include "ccl/public/audio/iaudiolayout.h"

namespace CCL {

//************************************************************************************************
// AudioProcessConfiguration
//************************************************************************************************

struct AudioProcessConfiguration
{
	int byteSize = sizeof(AudioProcessConfiguration); ///< size of this struct in bytes

	SampleRate sampleRate = 0;
	int maxBlockSize = 0;
	AudioFloatType floatType = kAudioFloat32;

	bool isValid () const;

	bool operator == (const AudioProcessConfiguration& other) const;
	bool operator != (const AudioProcessConfiguration& other) const;
};

//************************************************************************************************
// IAudioComponent
/**	Base interface for an audio component with one or more streams. */
//************************************************************************************************

interface IAudioComponent: IComponent
{
	/** Audio component capabilities. */
	DEFINE_ENUM (CapabilityFlags)
	{
		kCapabilityNone = 0,
		kCapabilityKnownStreamLayout = 1<<0,	///< component has a well-defined stream layout
		kCapabilityMutableStreamLayout = 1<<1,	///< component stream layout can be changed
		kCapabilityStreamActivation = 1<<2,		///< component supports stream activation
		kCapabilityKnownLatency = 1<<3			///< component knows (and reports) its latency
	};

	/** Get component capability flags. */
	virtual CapabilityFlags CCL_API getCapabilities () const = 0;
		
	/** Set stream layout of this component. */
	virtual tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) = 0;

	/** Get stream layout of this component. */
	virtual const IAudioStreamLayout& CCL_API getStreamLayout () const = 0;

	/**	Set activation state of a single stream at given direction/index. */
	virtual tresult CCL_API setStreamActive (AudioStreamDirection direction,
											 int index,
											 tbool active) = 0;

	/** Get current audio process configuration. */
	virtual tresult CCL_API getProcessConfiguration (AudioProcessConfiguration& configuration) const = 0;

	/**	Set new audio process configuration. */
	virtual tresult CCL_API setProcessConfiguration (const AudioProcessConfiguration& configuration) = 0;

	/**	Get current latency for given direction in audio sample frames. */
	virtual tresult CCL_API getLatency (int& latencyInSamples, AudioStreamDirection direction) const = 0;

	/** Check if the whole component is active (see setActive ()). */
	virtual tbool CCL_API isActive () const = 0;

	/**	Activate/deactivate the whole component, e.g. allocating/releasing resources.
		Not realtime-safe. */
	virtual tresult CCL_API setActive (tbool state) = 0;

	DECLARE_IID (IAudioComponent)

	//////////////////////////////////////////////////////////////////////////////////////////////

	bool hasKnownStreamLayout () const;
	bool hasMutableStreamLayout () const;
	bool hasStreamActivation () const;
	bool hasKnownLatency () const;
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioComponent, 0xbfc91ebe, 0x820b, 0x4ecc, 0xbb, 0xb5, 0x6c, 0xf4, 0x9a, 0xdc, 0xfc, 0xa5)
#endif

//************************************************************************************************
// AudioBlockSizeRange
//************************************************************************************************

struct AudioBlockSizeRange
{
	DEFINE_ENUM (Unit)
	{
		kSampleFrames,	///< sizes are in audio frames
		kMilliseconds	///< sizes are in milliseconds
	};

	Unit unit = kSampleFrames;
	int minBlockSize = 0;
	int maxBlockSize = 0;
	int preferredBlockSize = 0;
	int step = 0; ///< 0 = any size in [min,max]; -1 = power-of-two sizes only; above 0 = min + n*step

	static constexpr int kStepAny = 0;
	static constexpr int kStepPowerOfTwo = -1;
};

//************************************************************************************************
// IAudioDeviceSpecifics
/**	Additional interface for hardware or virtual audio devices. */
//************************************************************************************************

interface IAudioDeviceSpecifics: IUnknown
{
	/** Set audio device option by identifier. */
	virtual tresult CCL_API setDeviceOption (StringID optionId, VariantRef value) = 0;

	/** Get audio device option by identifier. */
	virtual tresult CCL_API getDeviceOption (Variant& value, StringID optionId) const = 0;

	/** Get supported block size range for the device. */
	virtual tresult CCL_API getBlockSizeRange (AudioBlockSizeRange& range) const = 0;

	DECLARE_IID (IAudioDeviceSpecifics)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioDeviceSpecifics, 0xeaca0ebd, 0xa552, 0x41de, 0x92, 0xef, 0xf9, 0x48, 0x80, 0x6c, 0x15, 0x73)
#endif

//************************************************************************************************
// IAudioDeviceNotificationCallback
/**	Pass to IComponent::initialize () to receive notifications from audio device. */
//************************************************************************************************

interface IAudioDeviceNotificationCallback: IUnknown
{
	DEFINE_ENUM (EventType)
	{
		kDeviceLost,				///< device stopped without being asked (unplugged, disabled, backend error)
		kConfigurationChanged,		///< sample rate, block size, latency or routing changed
		kInterruptionBegan,			///< OS took the device away temporarily (e.g. phone call on iOS)
		kInterruptionEnded,			///< interruption is over, the host may restart the device
		kDeviceUnlocked				///< output allowed after a user gesture (Web Audio)
	};

	/** Notification from device, possibly called on a non-main thread. */
	virtual void CCL_API onDeviceNotification (IAudioComponent* component, EventType eventType) = 0;

	DECLARE_IID (IAudioDeviceNotificationCallback)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioDeviceNotificationCallback, 0xc821e07, 0xa245, 0x45dd, 0x88, 0x94, 0xd, 0x50, 0xf1, 0xf4, 0x2e, 0x5f)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioComponent inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IAudioComponent::hasKnownStreamLayout () const
{ return get_flag<int32> (getCapabilities (), kCapabilityKnownStreamLayout); }

inline bool IAudioComponent::hasMutableStreamLayout () const
{ return get_flag<int32> (getCapabilities (), kCapabilityMutableStreamLayout); }

inline bool IAudioComponent::hasStreamActivation () const
{ return get_flag<int32> (getCapabilities (), kCapabilityStreamActivation); }

inline bool IAudioComponent::hasKnownLatency () const
{ return get_flag<int32> (getCapabilities (), kCapabilityKnownLatency); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioProcessConfiguration inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioProcessConfiguration::isValid () const
{
	return sampleRate > 0 && maxBlockSize > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioProcessConfiguration::operator == (const AudioProcessConfiguration& other) const
{
	return	sampleRate == other.sampleRate &&
			maxBlockSize == other.maxBlockSize &&
			floatType == other.floatType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioProcessConfiguration::operator != (const AudioProcessConfiguration& other) const
{
	return !(*this == other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iaudiocomponent_h
