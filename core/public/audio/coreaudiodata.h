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
// Filename    : core/public/audio/coreaudiodata.h
// Description : Audio Data
//
//************************************************************************************************

#ifndef _coreaudiodata_h
#define _coreaudiodata_h

#include "core/public/audio/coreaudiodatatypes.h"

namespace Core {

//************************************************************************************************
// AudioData
/** Audio data block definition. */
//************************************************************************************************

struct AudioData
{
	int numChannels;				///< number of audio channels
	int numSamples;					///< number of audio samples per buffer
	SilenceBitField silenceBits;	///< audio channel silence bit field (max. 32 channels)
	AudioFloatType floatType;		///< audio floating-point type (32 or 64 Bit)
	union
	{
		float32** buffers32;		///< 32 Bit audio buffers
		float64** buffers64;		///< 64 Bit audio buffers
	};

	AudioData ()
	: numChannels (0),
	  numSamples (0),
	  silenceBits (0),
	  floatType (kAudioFloat32),
	  buffers32 (nullptr)
	{}

	/** Construct with 32 Bit buffers. */
	AudioData (int numChannels,
			   int numSamples,
			   float32** buffers,
			   AudioFloatType floatType = kAudioFloat32)
	: numChannels (numChannels),
	  numSamples (numSamples),
	  silenceBits (0),
	  floatType (floatType),
	  buffers32 (buffers)
	{}

	/** Construct with 64 Bit buffers. */
	AudioData (int numChannels,
			   int numSamples,
			   float64** buffers,
			   AudioFloatType floatType = kAudioFloat64)
	: numChannels (numChannels),
	  numSamples (numSamples),
	  silenceBits (0),
	  floatType (floatType),
	  buffers64 (buffers)
	{}

	int getBitDepth () const { return floatType == kAudioFloat32 ? 32 : 64; }

	template<class FloatType> FloatType* getBuffer (int index) const;
	template<class FloatType> FloatType** getBuffers () const;
};

//************************************************************************************************
// AudioDataSpan
/** Provides storage for buffer pointers, no sample memory. */
//************************************************************************************************

struct AudioDataSpan: AudioData
{
	AudioDataSpan ()
	{
		buffers32 = (float32**)bufferArray;
	}

	AudioDataSpan (const AudioData& src, int offset, int sampleCount)
	: AudioDataSpan ()
	{
		make (*this, src, offset, sampleCount);
	}

	AudioDataSpan (const AudioDataSpan& other)
	: AudioDataSpan ()
	{
		make (*this, other, 0, other.numSamples);
	}

	AudioDataSpan& operator= (const AudioDataSpan& other)
	{
		make (*this, other, 0, other.numSamples);
		return *this;
	}

private:
	static void make (AudioDataSpan& dst, const AudioData& src, int offset, int sampleCount)
	{
		ASSERT (offset + sampleCount <= src.numSamples)
		dst.numChannels = src.numChannels;
		dst.numSamples = sampleCount;
		dst.silenceBits = src.silenceBits;
		dst.floatType = src.floatType;
		if(src.floatType == kAudioFloat32)
		{
			for(int i = 0; i < src.numChannels; i++)
				dst.buffers32[i] = src.buffers32[i] + offset;
		}
		else
		{
			for(int i = 0; i < src.numChannels; i++)
				dst.buffers64[i] = src.buffers64[i] + offset;
		}
	}

	void* bufferArray[kMaxAudioStreamChannels] = {};
};

//************************************************************************************************
// StaticAudioData
/** Template for statically allocated audio data buffer. */
//************************************************************************************************

template<int kNumChannels, int kNumSamples, typename Float = float32>
struct StaticAudioData: AudioData
{
	StaticAudioData ()
	{
		numChannels = kNumChannels;
		numSamples = kNumSamples;
		floatType = sizeof(Float) == sizeof(float32) ? kAudioFloat32 : kAudioFloat64;
		buffers32 = (float32**)bufferArray;
		for(int i = 0; i < kNumChannels; i++)
			buffers32[i] = (float32*)(floatData + i * numSamples);
		::memset (floatData, 0, sizeof(floatData));
	}

	StaticAudioData (const StaticAudioData&) = delete;
	StaticAudioData& operator= (const StaticAudioData&) = delete;

private:
	Float* bufferArray[kNumChannels];
	Float floatData[kNumChannels * kNumSamples];
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioData inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<> inline float32** AudioData::getBuffers<float32> () const { return buffers32; }
template<> inline float64** AudioData::getBuffers<float64> () const { return buffers64; }
template<class FloatType> FloatType* AudioData::getBuffer (int index) const { return getBuffers<FloatType> ()[index]; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coreaudiodata_h
