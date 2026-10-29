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
// Filename    : core/extras/audio/coreaudiodataops.h
// Description : Audio Data Operations
//
//************************************************************************************************

#ifndef _coreaudiodataops_h
#define _coreaudiodataops_h

#include "core/extras/audio/coreaudiovectorops.h"

#include "core/public/audio/coreaudiodata.h"

namespace Core {

//************************************************************************************************
// AudioDataOpsGeneric
/** Generic audio data operations without optimizations. */
//************************************************************************************************

namespace AudioDataOpsGeneric
{
	using namespace AudioVectorOpsGeneric;

	/** Clear all audio samples. */
	INLINE void clear (AudioData& dst)
	{
		if(dst.floatType == kAudioFloat32)
		{
			for(int i = 0; i < dst.numChannels; i++)
				vectorClear (dst.buffers32[i], dst.numSamples);
		}
		else
		{
			for(int i = 0; i < dst.numChannels; i++)
				vectorClear (dst.buffers64[i], dst.numSamples);
		}
	}

	/** Copy data from other buffer. */
	INLINE void copyFrom (AudioData& dst, const AudioData& src)
	{
		int numSamples = get_min (dst.numSamples, src.numSamples);
		int numChannels = get_min (dst.numChannels, src.numChannels);
		
		if(dst.floatType == src.floatType)
		{
			if(dst.floatType == kAudioFloat32)
			{
				for(int i = 0; i < numChannels; i++)
					vectorCopy (dst.buffers32[i], src.buffers32[i], numSamples);
			}
			else
			{
				for(int i = 0; i < numChannels; i++)
					vectorCopy (dst.buffers64[i], src.buffers64[i], numSamples);
			}
		}
		else
		{
			if(dst.floatType == kAudioFloat32) // 64 -> 32 bit floating-point
			{
				for(int i = 0; i < numChannels; i++)
					vectorConvert (dst.buffers32[i], src.buffers64[i], numSamples);
			}
			else // 32 -> 64 bit floating-point
			{
				for(int i = 0; i < numChannels; i++)
					vectorConvert (dst.buffers64[i], src.buffers32[i], numSamples);
			}
		}
	}

	/** Add (accumulate) data from other buffer. */
	INLINE void addFrom (AudioData& dst, const AudioData& src, float gain = 1.f)
	{
		int numSamples = get_min (dst.numSamples, src.numSamples);
		int numChannels = get_min (dst.numChannels, src.numChannels);
		
		if(dst.floatType == src.floatType)
		{
			if(dst.floatType == kAudioFloat32)
			{
				for(int i = 0; i < numChannels; i++)
					vectorAddScaled (dst.buffers32[i], src.buffers32[i], numSamples, gain);
			}
			else
			{
				for(int i = 0; i < numChannels; i++)
					vectorAddScaled (dst.buffers64[i], src.buffers64[i], numSamples, gain);
			}
		}
		else
		{
			if(dst.floatType == kAudioFloat32) // 64 -> 32 bit floating-point
			{
				for(int i = 0; i < numChannels; i++)
					vectorAddScaled (dst.buffers32[i], src.buffers64[i], numSamples, gain);
			}
			else // 32 -> 64 bit floating-point
			{
				for(int i = 0; i < numChannels; i++)
					vectorAddScaled (dst.buffers64[i], src.buffers32[i], numSamples, gain);
			}
		}
	}
}

} // namespace Core

#endif // _coreaudiodataops_h