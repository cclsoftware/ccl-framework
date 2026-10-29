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
// Filename    : ccl/extras/audio/meter/audiometerbuffer.h
// Description : Audio Meter Buffer
//
//************************************************************************************************

#ifndef _ccl_audiometerbuffer_h
#define _ccl_audiometerbuffer_h

#include "ccl/base/object.h"

#include "ccl/public/audio/cclaudiotypes.h"
#include "ccl/public/collections/vector.h"

#include "core/extras/audio/coreaudiometerballistics.h"

namespace CCL {

using Core::AudioMeterBallistics;

class AudioMeterBuffer;

//************************************************************************************************
// IAudioMeterClient
/**	Client interface of AudioMeterBuffer, called synchronously on the main thread. */
//************************************************************************************************

interface IAudioMeterClient
{
	/** Levels, clip state or channel count of the buffer changed. */
	virtual void meterDataChanged (const AudioMeterBuffer& buffer) = 0;

	/** Settings instance replaced, or a value of the current settings changed. */
	virtual void meterSettingsChanged (const AudioMeterBuffer& buffer) = 0;
};

//************************************************************************************************
// AudioMeterSettings
/**	Sharable meter ballistics/display settings. */
//************************************************************************************************

class AudioMeterSettings: public Object
{
public:
	DECLARE_CLASS (AudioMeterSettings, Object)

	static constexpr float kDefaultPeakDecayRate = AudioMeterBallistics::kDefaultPeakDecayRate;
	static constexpr TSeconds kDefaultRMSSmoothingTime = AudioMeterBallistics::kDefaultRMSSmoothingTime;
	static constexpr TSeconds kDefaultClipHoldTime = AudioMeterBallistics::kDefaultClipHoldTime;
	static constexpr TSeconds kDefaultPeakHoldTime = AudioMeterBallistics::kDefaultPeakHoldTime;
	static constexpr TSeconds kInfiniteHold = AudioMeterBallistics::kInfiniteHold;

	static constexpr float kDefaultMinDb = -60.f;
	static constexpr float kDefaultMaxDb = 6.f;

	float getPeakDecayRate () const { return peakDecayRate; }
	void setPeakDecayRate (float dbPerSecond);

	TSeconds getRMSSmoothingTime () const { return rmsSmoothingTime; }
	void setRMSSmoothingTime (TSeconds seconds);

	TSeconds getClipHoldTime () const { return clipHoldTime; }
	void setClipHoldTime (TSeconds seconds);

	TSeconds getPeakHoldTime () const { return peakHoldTime; }
	void setPeakHoldTime (TSeconds seconds);

	bool isRMSEnabled () const { return rmsEnabled; }
	void setRMSEnabled (bool state);

	bool isClipEnabled () const { return clipEnabled; }
	void setClipEnabled (bool state);

	bool isPeakHoldEnabled () const { return peakHoldEnabled; }
	void setPeakHoldEnabled (bool state);

	float getMinDb () const { return minDb; }
	float getMaxDb () const { return maxDb; }
	void setRange (float minDb, float maxDb);

protected:
	float peakDecayRate = kDefaultPeakDecayRate;
	TSeconds rmsSmoothingTime = kDefaultRMSSmoothingTime;
	TSeconds clipHoldTime = kDefaultClipHoldTime;
	TSeconds peakHoldTime = kDefaultPeakHoldTime;
	float minDb = kDefaultMinDb;
	float maxDb = kDefaultMaxDb;

	bool rmsEnabled = false;
	bool clipEnabled = false;
	bool peakHoldEnabled = false;
};

//************************************************************************************************
// AudioMeterBuffer
/**	Data model holding decayed peak/RMS/peak-hold levels (in dB) and clip state.
	Registered IAudioMeterClient instances are notified about changes.
	AudioMeterSettings can be shared across multiple instances. */
//************************************************************************************************

class AudioMeterBuffer: public Object
{
public:
	DECLARE_CLASS (AudioMeterBuffer, Object)

	~AudioMeterBuffer ();

	/** (Re)configure for the given channel count, resetting decayed state. */
	void setChannelCount (int channelCount);
	int getChannelCount () const { return ballistics.getChannelCount (); }

	/** Register a client. */
	bool addClient (IAudioMeterClient* client);

	/** Unregister a client. */
	bool removeClient (IAudioMeterClient* client);

	/**	Feed one freshly-read raw snapshot plus elapsed time since the previous call.
		Runs the ballistics and calls meterDataChanged () of the clients only if something
		actually changed, and returns whether it did. */
	bool update (const AudioMeterData& rawData, TSeconds elapsedSeconds);

	bool reset ();

	/**	Clear both clip-hold and peak-hold state back to silence/off.
		Does not affect live peak/RMS. */
	bool resetHolds ();

	// Levels in dB
	float getPeakLevelDb (int channel) const;
	float getRMSLevelDb (int channel) const;
	float getPeakHoldLevelDb (int channel) const;

	bool isClipped (int channel) const;

	/**	Share an existing settings instance (retained),
		calls meterSettingsChanged () of the clients. */
	void setSettings (AudioMeterSettings* newSettings);
	AudioMeterSettings& getSettings ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	AudioMeterBallistics ballistics;
	SharedPtr<AudioMeterSettings> settings;
	Vector<IAudioMeterClient*> clients;

	void applySettings ();
	void signalDataChanged ();
	void signalSettingsChanged ();
};

} // namespace CCL

#endif // _ccl_audiometerbuffer_h
