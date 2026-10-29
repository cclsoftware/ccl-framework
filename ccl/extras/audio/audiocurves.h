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
// Filename    : ccl/extras/audio/audiocurves.h
// Description : Audio Interpolator Curves
//
//************************************************************************************************

#ifndef _ccl_audiocurves_h
#define _ccl_audiocurves_h

#include "ccl/app/paramcurve.h"

#include "core/extras/audio/coreaudiointerpolator.h"

namespace CCL {

using Core::AudioFaderDBInterpolator;
using Core::AudioFaderGainInterpolator;
using Core::AudioFrequencyInterpolator;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Curve Names
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CurveNames
{
	const CStringPtr kFaderDB = "Audio.FaderDB";
	const CStringPtr kFaderGain = "Audio.FaderGain";
	const CStringPtr kFrequency = "Audio.Frequency";
}

//************************************************************************************************
// AudioFaderDBCurve
//************************************************************************************************

class AudioFaderDBCurve: public InterpolatorCurve
{
public:
	DECLARE_CLASS (AudioFaderDBCurve, InterpolatorCurve)

	AudioFaderDBCurve ()
	: InterpolatorCurve (NEW AudioFaderDBInterpolator)
	{}
};

//************************************************************************************************
// AudioFaderGainCurve
//************************************************************************************************

class AudioFaderGainCurve: public InterpolatorCurve
{
public:
	DECLARE_CLASS (AudioFaderGainCurve, InterpolatorCurve)

	static float getMaxFactor ()
	{
		return AudioFaderGainInterpolator::getMaxFactor ();
	}

	AudioFaderGainCurve ()
	: InterpolatorCurve (NEW AudioFaderGainInterpolator)
	{}
};

//************************************************************************************************
// AudioFrequencyCurve
//************************************************************************************************

class AudioFrequencyCurve: public InterpolatorCurve
{
public:
	DECLARE_CLASS (AudioFrequencyCurve, InterpolatorCurve)

	static constexpr float kDefaultMin = AudioFrequencyInterpolator::kDefaultMin;
	static constexpr float kDefaultMax = AudioFrequencyInterpolator::kDefaultMax;

	AudioFrequencyCurve (float minFrequency = kDefaultMin, float maxFrequency = kDefaultMax)
	: InterpolatorCurve (NEW AudioFrequencyInterpolator (minFrequency, maxFrequency))
	{}
};

} // namespace CCL

#endif // _ccl_audiocurves_h