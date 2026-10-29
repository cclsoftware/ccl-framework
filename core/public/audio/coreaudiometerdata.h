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
// Filename    : core/public/audio/coreaudiometerdata.h
// Description : Audio Meter Data
//
//************************************************************************************************

#ifndef _coreaudiometerdata_h
#define _coreaudiometerdata_h

#include "core/public/audio/coreaudiodatatypes.h"

namespace Core {

//************************************************************************************************
// AudioMeterData
/** Per-channel VU meter snapshot: peak/RMS levels (linear) and a clip bit field. */
//************************************************************************************************

struct AudioMeterData
{
	int byteSize = sizeof(AudioMeterData);	///< size of this struct in bytes

	int numChannels = 0;
	float* peakLevels = nullptr;	///< linear, one entry per channel
	float* rmsLevels = nullptr;		///< linear, one entry per channel
	uint32 clipState = 0;			///< bit field, one bit per channel (max. 32 channels)

	AudioMeterData (int numChannels = 0)
	: numChannels (numChannels)
	{}
};

//************************************************************************************************
// StaticAudioMeterData
/** Template for statically allocated audio meter data. */
//************************************************************************************************

template <int kMaxChannels>
struct StaticAudioMeterData: AudioMeterData
{
	StaticAudioMeterData (int numChannels = 0)
	: AudioMeterData (numChannels)
	{
		peakLevels = peakBuffer;
		rmsLevels = rmsBuffer;
	}

	StaticAudioMeterData (const StaticAudioMeterData& other)
	{
		peakLevels = peakBuffer;
		rmsLevels = rmsBuffer;
		copyFrom (other);
	}
	
	StaticAudioMeterData& operator= (const StaticAudioMeterData& other)
	{
		return copyFrom (other);
	}

	StaticAudioMeterData& copyFrom (const AudioMeterData& other)
	{
		ASSERT (other.numChannels <= kMaxChannels)
		numChannels = other.numChannels;
		for(int i = 0; i < numChannels; i++)
		{
			peakLevels[i] = other.peakLevels[i];
			rmsLevels[i] = other.rmsLevels[i];
		}
		clipState = other.clipState;
		return *this;
	}

private:
	float peakBuffer[kMaxChannels] = {};
	float rmsBuffer[kMaxChannels] = {};
};

static constexpr int kMaxAudioMeterChannels = 16;

typedef StaticAudioMeterData<kMaxAudioMeterChannels> AudioMeterDataMax;

} // namespace Core

#endif // _coreaudiometerdata_h
