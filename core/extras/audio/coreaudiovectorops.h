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
// Filename    : core/extras/audio/coreaudiovectorops.h
// Description : Audio Vector Operations
//
//************************************************************************************************

#ifndef _coreaudiovectorops_h
#define _coreaudiovectorops_h

#include "core/public/coremath.h"
#include "core/public/coreprimitives.h"

namespace Core {
	
//************************************************************************************************
// AudioSampleOpsGeneric
/** Generic audio sample operations without optimizations. */
//************************************************************************************************

namespace AudioSampleOpsGeneric
{
	template <typename T>
	INLINE bool isValid (const T& v)
	{
		return (v > T(-10000) && v < T(10000)); 
	}
}

//************************************************************************************************
// AudioVectorOpsGeneric
/** Generic audio vector operations without optimizations. */
//************************************************************************************************

namespace AudioVectorOpsGeneric
{
	INLINE void vectorClear (float32 dst[], int numSamples)
	{
		::memset (dst, 0, numSamples * sizeof(float32));
	}

	INLINE void vectorClear (float64 dst[], int numSamples)
	{
		::memset (dst, 0, numSamples * sizeof(float64));
	}

	INLINE void vectorCopy (float32 dst[], const float32 src[], int numSamples)
	{
		::memcpy (dst, src, numSamples * sizeof(float32));
	}

	INLINE void vectorCopy (float64 dst[], const float64 src[], int numSamples)
	{
		::memcpy (dst, src, numSamples * sizeof(float64));
	}

	template <typename TDst, typename TSrc>
	INLINE void vectorConvert (TDst dst[], const TSrc src[], int numSamples)
	{
		for(int i = 0; i < numSamples; i++)
			dst[i] = TDst(src[i]);
	}

	template <typename TDst, typename TSrc>
	INLINE void vectorAdd (TDst dst[], const TSrc src[], int numSamples)
	{
		for(int i = 0; i < numSamples; i++)
			dst[i] += TDst(src[i]);
	}

	template <typename TDst, typename TSrc>
	INLINE void vectorAddScaled (TDst dst[], const TSrc src[], int numSamples, float gain = 1.f)
	{
		for(int i = 0; i < numSamples; i++)
			dst[i] += TDst(src[i] * gain);
	}

	template <typename T>
	INLINE void vectorRamp (T dst[], int numSamples, float startGain, float endGain, int maxRampSamples)
	{
		int rampLength = get_min<int> (numSamples, maxRampSamples);
		int i = 0;

		if(rampLength > 0 && startGain != endGain)
		{
			const T step = (T(endGain) - T(startGain)) / T(rampLength);
			T gain = T(startGain);
			for(; i < rampLength - 1; i++)
			{
				gain += step;
				dst[i] *= gain;
			}
			dst[i++] *= T(endGain); // exact end of ramp
		}

		if(endGain != 1.f)
		{
			const T constantGain = T(endGain);
			for(; i < numSamples; i++)
				dst[i] *= constantGain;
		}
	}

	template <typename T>
	INLINE void vectorGetMin (T& result, const T src[], int numSamples)
	{
		for(int i = 0; i < numSamples; i++)
			result = get_min<T> (src[i], result);
	}

	template <typename T>
	INLINE void vectorGetMax (T& result, const T src[], int numSamples)
	{
		for(int i = 0; i < numSamples; i++)
			result = get_max<T> (src[i], result);
	}

	template <typename T>
	INLINE void vectorGetMinMax (T& minPeak, T& maxPeak, const T src[], int numSamples)
	{
		vectorGetMin (minPeak, src, numSamples);
		vectorGetMax (maxPeak, src, numSamples);
	}

	template <typename T>
	INLINE void vectorGetPeakRms (T& peak, T& rms, const T src[], int numSamples)
	{
		peak = 0;
		T sumSquares = 0;
		for(int i = 0; i < numSamples; i++)
		{
			T sample = src[i];
			T absSample = sample < 0 ? -sample : sample;
			if(absSample > peak)
				peak = absSample;
			sumSquares += sample * sample;
		}

		rms = MathUtils::Functions<T>::sqrt (sumSquares / T(numSamples));
	}
}

} // namespace Core

#endif // _coreaudiovectorops_h