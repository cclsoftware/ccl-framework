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
// Filename    : core/extras/audio/coreaudiointerpolator.h
// Description : Audio Interpolator
//
//************************************************************************************************

#ifndef _coreaudiointerpolator_h
#define _coreaudiointerpolator_h

#include "core/public/coreinterpolator.h"
#include "core/public/corebasicmacros.h"
#include "core/public/coreprimitives.h"
#include "core/public/coremath.h"

namespace Core {
	
//************************************************************************************************
// AudioFaderDBInterpolator
/**	Traditional mixer fader interpolator (-144dB to +10dB). 

	Reference: https://www.musicdsp.org/en/latest/Other/94-millimeter-to-db-faders.html */
//************************************************************************************************

class AudioFaderDBInterpolator: public SegmentInterpolator
{
public:
	static constexpr float kMaxDB = 10.f;
	static constexpr float kMinDB = -144.f;

	AudioFaderDBInterpolator ()
	: SegmentInterpolator (getCurvePoints ())
	{}

	using SegmentInterpolator::SegmentInterpolator;

protected:
	static BreakPointData getCurvePoints ()
	{
		static constexpr BreakPoint kCurvePoints[] = 
		{
			{0.f, kMinDB},
			{0.04f, -60.f},
			{0.16f, -40.f},
			{0.52f, -10.f},
			{1.f, kMaxDB}
		};
		return {ARRAY_COUNT (kCurvePoints), kCurvePoints};
	}		
};

//************************************************************************************************
// AudioFaderGainInterpolator
/**	Fader interpolator where value is a gain factor applied to the audio signal. */
//************************************************************************************************

class AudioFaderGainInterpolator: public Interpolator
{
public:
	static float getMaxFactor ()
	{
		return MathUtils::dbToFactor (AudioFaderDBInterpolator::kMaxDB);
	}

	AudioFaderGainInterpolator ()
	: Interpolator (0.f, getMaxFactor ())
	{}
	
	float normalizedToDb (float normalized) const
	{
		return dbInterpolator.normalizedToRange (normalized);
	}

	float dbToNormalized (float value) const
	{
		return dbInterpolator.rangeToNormalized (value);
	}

	// Interpolator
	float normalizedToRange (float normalized) const override
	{
		return MathUtils::dbToFactor (normalizedToDb (normalized));
	}

	float rangeToNormalized (float value) const override
	{
		return dbToNormalized (MathUtils::factorToDb (value));
	}

protected:
	AudioFaderDBInterpolator dbInterpolator;
	
	using BreakPoint = SegmentInterpolator::BreakPoint;

	AudioFaderGainInterpolator (int numPoints, const BreakPoint points[])
	: dbInterpolator (numPoints, points)
	{
		minRange = MathUtils::dbToFactor (normalizedToDb (0.f));
		maxRange = MathUtils::dbToFactor (normalizedToDb (1.f));
	}
};

//************************************************************************************************
// AudioFrequencyInterpolator
/**	Logarithmic (octave-linear) conversion: value = min * (max/min)^normalized.
	Requires 0 < minRange < maxRange; midRange is ignored.

	Reference: https://www.musicdsp.org/en/latest/Other/87-exponential-parameter-mapping.html */
//************************************************************************************************

class AudioFrequencyInterpolator: public Interpolator
{
public:
	static constexpr float kDefaultMin = 20.f;
	static constexpr float kDefaultMax = 20000.f;

	AudioFrequencyInterpolator (float minRange = kDefaultMin, float maxRange = kDefaultMax)
	{
		setRange (minRange, maxRange);
	}

	// Interpolator
	void setRange (float minRange, float maxRange, float midRange = -1.f) override
	{
		Interpolator::setRange (minRange, maxRange, midRange);

		if(minRange > 0 && maxRange > minRange)
		{
			minLogRange = toLog2 (minRange);
			logRange = toLog2 (maxRange) - minLogRange;
			invLogRange = 1.f / logRange;
		}
		else
			minLogRange = logRange = invLogRange = 0.f; // invalid range, conversions return 0
	}

	float normalizedToRange (float normalized) const override
	{
		if(logRange == 0.f)
			return 0.f;

		normalized = ((0 > normalized) ? 0 : ((1.f > normalized) ? normalized : 1.f));
		return bound (powf (2.f, (normalized * logRange) + minLogRange), minRange, maxRange);
	}

	float rangeToNormalized (float value) const override
	{
		if(logRange == 0.f)
			return 0.f;

		value = ((minRange > value) ? minRange : ((maxRange > value) ? value : maxRange));
		return (toLog2 (value) - minLogRange) * invLogRange;
	}

protected:
	float minLogRange = 0.f;	///< log2 (min)
	float logRange = 0.f;		///< log2 (max) - log2 (min), 0 if range is invalid
	float invLogRange = 0.f;	///< 1 / logRange

	static float toLog2 (float val)
	{
		const float kLogTwoToEInv = 1.442695040889f;
		return kLogTwoToEInv * logf (val);
	}
};

} // namespace Core

#endif // _coreaudiointerpolator_h