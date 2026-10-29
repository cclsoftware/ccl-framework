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
// Filename    : ccl/public/audio/audioparamids.h
// Description : Audio Parameter Identifiers
//
//************************************************************************************************

#ifndef _ccl_audioparamids_h
#define _ccl_audioparamids_h

#include "ccl/public/base/platform.h"

namespace CCL {

//************************************************************************************************
// Audio Parameter Identifiers from CCL Audio Service Components
//************************************************************************************************

namespace AudioParamID
{
	namespace Generator
	{
		const CStringPtr kOn = "on";
		const CStringPtr kAmplitude = "amplitude";
	}

	namespace Oscillator
	{
		/** Waveform shape. */
		DEFINE_ENUM (WaveType)
		{
			kSine = 0,
			kSquare,
			kTriangle,
			kSawtooth
		};

		const CStringPtr kWaveType = "waveType";
		const CStringPtr kFrequency = "frequency";
	}

	namespace NoiseGenerator
	{
		/** Noise color. */
		DEFINE_ENUM (NoiseType)
		{
			kWhite = 0,
			kPink,
			kBrown
		};

		const CStringPtr kNoiseType = "noiseType";
		const CStringPtr kSeed = "seed";
	}
}

} // namespace CCL

#endif // _ccl_audioparamids_h