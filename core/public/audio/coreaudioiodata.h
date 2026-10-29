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
// Filename    : core/public/audio/coreaudioiodata.h
// Description : Audio I/O Data
//
//************************************************************************************************

#ifndef _coreaudioiodata_h
#define _coreaudioiodata_h

#include "core/public/audio/coreaudiodatatypes.h"

namespace Core {

//************************************************************************************************
// AudioIOData
/** Audio data block definition for synchronous I/O of multiple audio streams. */
//************************************************************************************************

struct AudioIOData
{
	/** Audio substream data block. */
	struct StreamData
	{
		int numChannels;				///< number of audio channels
		SilenceBitField silenceBits;	///< audio channel silence bit field
		union
		{
			float32** buffers32;		///< 32 Bit audio buffers
			float64** buffers64;		///< 64 Bit audio buffers
		};

		StreamData ()
		: numChannels (0),
		  silenceBits (0),
		  buffers32 (nullptr)
		{}

		bool isSilent () const;
	};

	int numInputs;						///< number of input streams
	int numOutputs;						///< number of output streams
	int numSamples;						///< number of samples per stream buffer
	AudioFloatType floatType;			///< audio floating-point type (32 or 64 Bit)
	StreamData* inputs;					///< input streams
	StreamData* outputs;				///< output streams

	AudioIOData ()
	: numInputs (0),
	  numOutputs (0),
	  numSamples (0),
	  floatType (kAudioFloat32),
	  inputs (nullptr),
	  outputs (nullptr)
	{}

	/** Allocate stream data arrays for specified number of I/Os. */
	void allocateStreamData (int numInputs, int numOutputs);

	/** Deallocate stream data arrays. */
	void deallocateStreamData ();

	/** Allocate stream arrays and audio buffer arrays for specified number of I/Os. Calls allocateStreamData. */
	void allocateStreams (int numInputs, int numOutputs, int maxChannels = kMaxAudioStreamChannels);

	/** Deallocate stream arrays and audio buffer arrays. Calls deallocateStreamData. */
	void deallocateStreams ();

	/** Assign buffer addresses for given input stream. */
	void setInputStreamBuffers (int index, float32* buffers[], int numChannels);

	/** Assign buffer addresses for given output stream. */
	void setOutputStreamBuffers (int index, float32* buffers[], int numChannels);
};

//************************************************************************************************
// StaticAudioIOData
/** Template for statically allocated audio I/O data. */
//************************************************************************************************

template<int kNumInputs, int kNumOutputs, int kMaxChannels = kMaxAudioStreamChannels>
struct StaticAudioIOData: AudioIOData
{
	StaticAudioIOData ()
	{
		numInputs = kNumInputs;
		numOutputs = kNumOutputs;
		inputs = inputStreamData;
		outputs = outputStreamData;

		for(int i = 0; i < kNumInputs; i++)
		{
			inputs[i].buffers32 = inputBufferPointers[i];
			::memset (inputBufferPointers[i], 0, sizeof(inputBufferPointers[i]));
		}
		
		for(int i = 0; i < kNumOutputs; i++)
		{
			outputs[i].buffers32 = outputBufferPointers[i];
			::memset (outputBufferPointers[i], 0, sizeof(outputBufferPointers[i]));
		}
	}

	StaticAudioIOData (const StaticAudioIOData&) = delete;
	StaticAudioIOData& operator= (const StaticAudioIOData&) = delete;

private:
	StreamData inputStreamData[kNumInputs];
	StreamData outputStreamData[kNumOutputs];
	float32* inputBufferPointers[kNumInputs][kMaxChannels];
	float32* outputBufferPointers[kNumOutputs][kMaxChannels];
};

//************************************************************************************************
// StaticAudioStreamBuffers
//************************************************************************************************

template <typename FloatType, int kStreamCount, int kSampleCount, int kChannelCount = kMaxAudioStreamChannels>
struct StaticAudioStreamBuffers
{
	static const int kSamplesPerStream = kChannelCount * kSampleCount;
	FloatType samples[kStreamCount * kSamplesPerStream];

	StaticAudioStreamBuffers ()
	{
		::memset (samples, 0, sizeof(samples));
	}

	FloatType* getBuffer (int streamIndex, int channelIndex)
	{
		ASSERT (streamIndex < kStreamCount && channelIndex < kChannelCount)
		return samples + streamIndex * kSamplesPerStream + channelIndex * kSampleCount;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioIOData inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioIOData::StreamData::isSilent () const
{
	if(silenceBits != 0)
	{
		SilenceBitField bitField = 0;
		for(int i = 0; i < numChannels; i++)
			bitField |= 1 << i;
		return (silenceBits & bitField) == bitField;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AudioIOData::allocateStreamData (int _numInputs, int _numOutputs)
{
	numInputs = _numInputs;
	numOutputs = _numOutputs;

	ASSERT (inputs == nullptr && outputs == nullptr)

	if(numInputs > 0)
		inputs = NEW StreamData[numInputs];
	if(numOutputs > 0)
		outputs = NEW StreamData[numOutputs];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AudioIOData::deallocateStreamData ()
{
	delete [] inputs;
	delete [] outputs;
	
	inputs = nullptr;
	outputs = nullptr;

	numInputs = 0;
	numOutputs = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AudioIOData::allocateStreams (int _numInputs, int _numOutputs, int maxChannels)
{
	allocateStreamData (_numInputs, _numOutputs);

	for(int i = 0; i < numInputs; i++)
	{
		inputs[i].buffers32 = NEW float32*[maxChannels];
		::memset (inputs[i].buffers32, 0, maxChannels * sizeof(float32*));
	}

	for(int o = 0; o < numOutputs; o++)
	{
		outputs[o].buffers32 = NEW float32*[maxChannels];
		::memset (outputs[o].buffers32, 0, maxChannels * sizeof(float32*));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AudioIOData::deallocateStreams ()
{
	if(inputs)
	{
		for(int i = 0; i < numInputs; i++)
		{
			if(inputs[i].buffers32)
				delete [] inputs[i].buffers32;
		}
	}

	if(outputs)
	{
		for(int o = 0; o < numOutputs; o++)
		{
			if(outputs[o].buffers32)
				delete [] outputs[o].buffers32;
		}
	}

	deallocateStreamData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
inline void AudioIOData::setInputStreamBuffers (int index, float32* buffers[], int numChannels)
{
	ASSERT (index >= 0 && index < numInputs)
	StreamData& inputStream = inputs[index];
	
	inputStream.numChannels = numChannels;
	for(int ch = 0; ch < numChannels; ch++)
		inputStream.buffers32[ch] = buffers[ch];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
inline void AudioIOData::setOutputStreamBuffers (int index, float32* buffers[], int numChannels)
{
	ASSERT (index >= 0 && index < numOutputs)
	StreamData& outputStream = outputs[index];
	
	outputStream.numChannels = numChannels;
	for(int ch = 0; ch < numChannels; ch++)
		outputStream.buffers32[ch] = buffers[ch];
}

} // namespace Core

#endif // _coreaudioiodata_h
