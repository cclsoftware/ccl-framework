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
// Filename    : core/extras/audio/coreaudiometerballistics.h
// Description : Audio Meter Ballistics
//
//************************************************************************************************

#ifndef _coreaudiometerballistics_h
#define _coreaudiometerballistics_h

#include "core/public/audio/coreaudiometerdata.h"

#include "core/public/coremath.h"
#include "core/public/coreprimitives.h"

namespace Core {

//************************************************************************************************
// AudioMeterBallistics
/** GUI-side audio meter ballistics: turns a raw, undecayed (linear) AudioMeterData snapshot into
	smoothly-falling peak, RMS and peak hold levels in dB plus a held clip state. RMS, clip and
	peak hold are each individually enabled/disabled (off by default). */
//************************************************************************************************

class AudioMeterBallistics
{
public:
	bool setChannelCount (int channelCount);
	bool reset ();

	/**	Feed one freshly-read raw snapshot plus real elapsed time since the previous call.
		Returns true if any channel's peak/RMS/clip/peak-hold state changed as a result. */
	bool update (const AudioMeterData& rawData, TSeconds elapsedSeconds);

	/**	Clear both clip-hold and peak-hold state back to silence/off.
		Does not affect live peak/RMS. */
	bool resetHolds ();

	int getChannelCount () const { return channelCount; }

	// Levels in dB
	float getPeakLevelDb (int channel) const { return peakLevelsDb[channel]; }
	float getRMSLevelDb (int channel) const { return rmsLevelsDb[channel]; }
	float getPeakHoldLevelDb (int channel) const { return peakHoldLevelsDb[channel]; }

	// Clip state
	bool isClipped (int channel) const { return get_bit (clipState, channel); }
	uint32 getClipState () const { return clipState; } ///< bit field, one bit per channel

	static constexpr float kDefaultPeakDecayRate = 20.f;		///< dB/sec, classic peak-meter fall rate
	static constexpr TSeconds kDefaultRMSSmoothingTime = 0.2;	///< one-pole, much lighter than peak hold
	static constexpr TSeconds kDefaultClipHoldTime = 1.5;
	static constexpr TSeconds kDefaultPeakHoldTime = 2.0;
	static constexpr TSeconds kInfiniteHold = -1.;				///< never auto-release; call resetHolds () to clear

	void setPeakDecayRate (float dbPerSecond)	{ peakDecayRate = dbPerSecond; }
	void setRMSSmoothingTime (TSeconds seconds)	{ rmsSmoothingTime = seconds; }
	void setClipHoldTime (TSeconds seconds)		{ clipHoldTime = seconds; }
	void setPeakHoldTime (TSeconds seconds)		{ peakHoldTime = seconds; }

	bool isRMSEnabled () const { return rmsEnabled; }
	void setRMSEnabled (bool state);

	bool isClipEnabled () const { return clipEnabled; }
	void setClipEnabled (bool state);

	bool isPeakHoldEnabled () const { return peakHoldEnabled; }
	void setPeakHoldEnabled (bool state);

protected:
	static float getSilenceDb () { return MathUtils::factorToDb (0.f); }

	int channelCount = 0;
	uint32 clipState = 0;
	float peakLevelsDb[kMaxAudioMeterChannels] = {};
	float rmsLevelsDb[kMaxAudioMeterChannels] = {};
	float peakHoldLevelsDb[kMaxAudioMeterChannels] = {};
	float rmsSmoothed[kMaxAudioMeterChannels] = {};		///< linear, state of the RMS smoothing filter
	TSeconds clipHoldRemaining[kMaxAudioMeterChannels] = {};
	TSeconds peakHoldRemaining[kMaxAudioMeterChannels] = {};

	float peakDecayRate = kDefaultPeakDecayRate;
	TSeconds rmsSmoothingTime = kDefaultRMSSmoothingTime;
	TSeconds clipHoldTime = kDefaultClipHoldTime;
	TSeconds peakHoldTime = kDefaultPeakHoldTime;

	bool rmsEnabled = false;
	bool clipEnabled = false;
	bool peakHoldEnabled = false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioMeterBallistics inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioMeterBallistics::setChannelCount (int newChannelCount)
{
	ASSERT (newChannelCount >= 0 && newChannelCount <= kMaxAudioMeterChannels)

	if(newChannelCount == channelCount)
		return false;

	channelCount = newChannelCount;

	reset ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioMeterBallistics::resetHolds ()
{
	bool changed = false;
	float silenceDb = getSilenceDb ();

	for(int ch = 0; ch < channelCount; ch++)
	{
		if(peakHoldLevelsDb[ch] != silenceDb)
			changed = true;

		peakHoldLevelsDb[ch] = silenceDb;
		peakHoldRemaining[ch] = 0.;

		if(get_bit (clipState, ch))
		{
			set_bit (clipState, ch, false);
			changed = true;
		}
		clipHoldRemaining[ch] = 0.;
	}
	return changed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioMeterBallistics::reset ()
{
	bool changed = resetHolds ();
	float silenceDb = getSilenceDb ();

	for(int ch = 0; ch < channelCount; ch++)
	{
		if(peakLevelsDb[ch] != silenceDb || rmsLevelsDb[ch] != silenceDb)
			changed = true;

		peakLevelsDb[ch] = silenceDb;
		rmsLevelsDb[ch] = silenceDb;
		rmsSmoothed[ch] = 0.f;
	}

	return changed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AudioMeterBallistics::setRMSEnabled (bool state)
{
	rmsEnabled = state;
	if(!state)
	{
		float silenceDb = getSilenceDb ();
		for(int ch = 0; ch < channelCount; ch++)
		{
			rmsSmoothed[ch] = 0.f;
			rmsLevelsDb[ch] = silenceDb;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AudioMeterBallistics::setClipEnabled (bool state)
{
	clipEnabled = state;
	if(!state)
	{
		clipState = 0;
		for(int ch = 0; ch < channelCount; ch++)
			clipHoldRemaining[ch] = 0.;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AudioMeterBallistics::setPeakHoldEnabled (bool state)
{
	peakHoldEnabled = state;
	if(!state)
	{
		float silenceDb = getSilenceDb ();
		for(int ch = 0; ch < channelCount; ch++)
		{
			peakHoldLevelsDb[ch] = silenceDb;
			peakHoldRemaining[ch] = 0.;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioMeterBallistics::update (const AudioMeterData& rawData, TSeconds elapsedSeconds)
{
	bool changed = false;
	int numChannels = get_min (rawData.numChannels, channelCount);
	float decayDb = peakDecayRate * float(elapsedSeconds);

	for(int ch = 0; ch < numChannels; ch++)
	{
		float previousPeakDb = peakLevelsDb[ch];
		float previousRMSDb = rmsLevelsDb[ch];
		bool previousClip = get_bit (clipState, ch);
		float previousPeakHoldDb = peakHoldLevelsDb[ch];

		float rawPeakDb = MathUtils::factorToDb (rawData.peakLevels[ch]);

		// peak: instant attack, release at peakDecayRate dB/sec (but never below the raw level)
		peakLevelsDb[ch] = get_max (rawPeakDb, previousPeakDb - decayDb);

		// RMS: light one-pole smoothing (on the linear value)
		if(rmsEnabled)
		{
			float rawRMS = rawData.rmsLevels[ch];
			float coeff = rmsSmoothingTime > 0. ? expf (float(-elapsedSeconds / rmsSmoothingTime)) : 0.f;
			rmsSmoothed[ch] = rawRMS + (rmsSmoothed[ch] - rawRMS) * coeff;
			rmsLevelsDb[ch] = MathUtils::factorToDb (rmsSmoothed[ch]);
		}

		// clip: latch and hold for clipHoldTime (or indefinitely) after the last raw clip bit seen
		if(clipEnabled)
		{
			bool rawClipped = get_bit (rawData.clipState, ch);
			TSeconds& holdRemaining = clipHoldRemaining[ch];
			bool latched = previousClip;

			if(rawClipped)
			{
				holdRemaining = clipHoldTime;
				latched = true;
			}
			else if(clipHoldTime == kInfiniteHold)
			{
				// stays latched until resetHolds () - holdRemaining is irrelevant here
			}
			else if(holdRemaining > 0.)
			{
				holdRemaining -= elapsedSeconds;
				// still counting down - latched stays true
			}
			else
			{
				latched = false;
			}

			set_bit (clipState, ch, latched);
		}

		// peak hold: latch the highest raw peak, hold for peakHoldTime (or indefinitely), then
		// release by decaying at peakDecayRate like the main peak bar
		if(peakHoldEnabled)
		{
			if(rawPeakDb >= peakHoldLevelsDb[ch])
			{
				peakHoldLevelsDb[ch] = rawPeakDb;
				peakHoldRemaining[ch] = peakHoldTime;
			}
			else if(peakHoldTime != kInfiniteHold)
			{
				if(peakHoldRemaining[ch] > 0.)
					peakHoldRemaining[ch] -= elapsedSeconds;
				else
					peakHoldLevelsDb[ch] = get_max (rawPeakDb, peakHoldLevelsDb[ch] - decayDb);
			}
		}

		if(peakLevelsDb[ch] != previousPeakDb ||
		   rmsLevelsDb[ch] != previousRMSDb ||
		   get_bit (clipState, ch) != previousClip ||
		   peakHoldLevelsDb[ch] != previousPeakHoldDb)
			changed = true;
	}

	return changed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coreaudiometerballistics_h
