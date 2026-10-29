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
// Filename    : ccl/extras/audio/audiodatabuffer.cpp
// Description : Audio Data Buffer
//
//************************************************************************************************

#define DEBUG_LOG 0
#define ALIGNMENT_ASSERTS 1

#include "ccl/extras/audio/audiodatabuffer.h"
#include "ccl/extras/audio/cclaudiodataops.h"

#include "ccl/public/system/iallocator.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

// Audio buffer should be aligned on cachelines for performance reasons
#define BUFFER_ALIGNMENT 32
#define ALIGN_BUFFER(x) ((((int64)x) + BUFFER_ALIGNMENT - 1) & ~0x1F)
#if ALIGNMENT_ASSERTS
#define ALIGNMENT_ASSERT(x) ASSERT ((((int64)x) % BUFFER_ALIGNMENT) == 0)
#else
#define ALIGNMENT_ASSERT(x)
#endif

//************************************************************************************************
// AudioDataBuffer
//************************************************************************************************

DEFINE_CLASS (AudioDataBuffer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::AudioDataBuffer (int numSamples, int numChannels, AudioFloatType type)
: allocatedBuffer (nullptr),
  allocatedSize (0),
  flags (0)
{
	data.floatType = type;
	data.buffers32 = bufferArray32;
	clearBufferArray ();

	resize (numSamples, numChannels, type, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::AudioDataBuffer (const AudioDataBuffer& buffer)
: allocatedBuffer (nullptr),
  allocatedSize (0),
  flags (0)
{
	data.buffers32 = bufferArray32;
	clearBufferArray ();

	if(!buffer.isEmpty ())
	{
		if(allocate (buffer.data.numChannels, buffer.data.numSamples, buffer.data.floatType))
		{
			AudioDataOpsGeneric::copyFrom (data, buffer.data);
			data.silenceBits = buffer.data.silenceBits;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::AudioDataBuffer (const AudioData& data)
: data (data),
  allocatedBuffer (nullptr),
  allocatedSize (0),
  flags (0)
{
	clearBufferArray ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::AudioDataBuffer (float32* buffer, int numSamples)
: allocatedBuffer (nullptr),
  allocatedSize (0),
  flags (0)
{
	data.numChannels = 1;
	data.numSamples = numSamples;
	data.floatType = kAudioFloat32;
	data.buffers32 = bufferArray32;
	clearBufferArray ();

	ASSERT (buffer != nullptr)
	bufferArray32[0] = buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::AudioDataBuffer (float64* buffer, int numSamples)
: allocatedBuffer (nullptr),
  allocatedSize (0),
  flags (0)
{
	data.numChannels = 1;
	data.numSamples = numSamples;
	data.floatType = kAudioFloat64;
	data.buffers64 = bufferArray64;
	clearBufferArray ();

	ASSERT (buffer != nullptr)
	bufferArray64[0] = buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::AudioDataBuffer (const AudioDataBuffer& buffer, int sampleOffset, int sampleCount)
: allocatedBuffer (nullptr),
  allocatedSize (0),
  flags (0)
{
	data.numChannels = buffer.data.numChannels;
	data.silenceBits = buffer.data.silenceBits;
	data.floatType = buffer.data.floatType;
	data.buffers32 = bufferArray32;
	clearBufferArray ();

	int maxSamples = buffer.data.numSamples - sampleOffset;
	int samples = ccl_min (sampleCount, maxSamples);
	ASSERT (samples > 0)
	if(samples > 0)
	{
		data.numSamples = samples;

		int byteOffset = getAudioByteSize (sampleOffset, data.floatType);
		for(int i = 0; i < data.numChannels; i++)
			data.buffers32[i] = (float32*)((char*)buffer.data.buffers32[i] + byteOffset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::AudioDataBuffer (const AudioData& bufferData, int sampleOffset, int sampleCount)
: allocatedBuffer (nullptr),
  allocatedSize (0),
  flags (0)
{
	data.numChannels = bufferData.numChannels;
	data.silenceBits = bufferData.silenceBits;
	data.floatType = bufferData.floatType;
	data.buffers32 = bufferArray32;
	clearBufferArray ();

	int maxSamples = bufferData.numSamples - sampleOffset;
	int samples = ccl_min (sampleCount, maxSamples);
	ASSERT (samples > 0)
	if(samples > 0)
	{
		data.numSamples = samples;

		int byteOffset = getAudioByteSize (sampleOffset, data.floatType);
		for(int i = 0; i < data.numChannels; i++)
			data.buffers32[i] = (float32*)((char*)bufferData.buffers32[i] + byteOffset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDataBuffer::~AudioDataBuffer ()
{
	if(allocatedBuffer)
	{
		if(flags & kLocked)
			System::LockMemory (false, allocatedBuffer, allocatedSize);
		System::GetMemoryAllocator ().dispose (allocatedBuffer);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDataBuffer::setLocked (bool state)
{
	if(state)
	{
		if(allocatedBuffer)
			System::LockMemory (true, allocatedBuffer, allocatedSize);
		flags |= kLocked;
	}
	else
	{
		if(allocatedBuffer)
			System::LockMemory (false, allocatedBuffer, allocatedSize);
		flags &= ~kLocked;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioDataBuffer::allocate (int numChannels, int numSamples, AudioFloatType type, bool clear)
{
	CCL_PRINTF ("AudioDataBuffer::allocate () %p %d %d\n", this, numChannels, numSamples)
	
	ASSERT (numChannels > 0 && numChannels <= kMaxChannels && numSamples > 0)
	if(numChannels <= 0 || numChannels > kMaxChannels || numSamples <= 0)
		return false;

	uint32 bytesPerChannel = uint32 (ALIGN_BUFFER (getAudioByteSize (numSamples, type)));
	int bytesAllocated = numChannels * bytesPerChannel + BUFFER_ALIGNMENT;

	void* baseAddress = System::GetMemoryAllocator ().allocate (bytesAllocated);
	if(!baseAddress)
		return false;

	// dispose old buffer
	dispose ();

	// assign channel addresses
	allocatedBuffer = baseAddress;
	allocatedSize = bytesAllocated;
	
	data.numChannels = numChannels;
	data.numSamples = numSamples;
	data.floatType = type;

	char* address = (char*)ALIGN_BUFFER (baseAddress);
	for(int i = 0; i < numChannels; i++, address += bytesPerChannel)
		data.buffers32[i] = (float32*)address;

	// Lock memory, if needed
	if(flags & kLocked)
		System::LockMemory (true, allocatedBuffer, allocatedSize);
	
	// clear samples
	data.silenceBits = 0;
	if(clear)
		AudioDataOpsGeneric::clear (data);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDataBuffer::dispose ()
{
	CCL_PRINTF ("AudioDataBuffer::dispose () %p\n", this)
	
	if(allocatedBuffer)
	{
	   // Unlock memory, if needed
	   if(flags & kLocked)
			System::LockMemory (false, allocatedBuffer, allocatedSize);
	
		System::GetMemoryAllocator ().dispose (allocatedBuffer),
		allocatedBuffer = nullptr;
	}
	   
	allocatedSize = 0;
	data.numChannels = 0;
	data.numSamples = 0;
	data.silenceBits = 0;
	clearBufferArray ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioDataBuffer::resize (int numSamples, int numChannels, AudioFloatType floatType, bool clear)
{
	bool result = true;
	if(data.numChannels != numChannels || data.numSamples != numSamples || data.floatType != floatType)
	{
		if(numChannels > 0 && numSamples > 0)
			result = allocate (numChannels, numSamples, floatType, clear);
		else
		{
			dispose ();
			data.floatType = floatType; // float type possibly changed!
			data.numChannels = numChannels; // numChannels possibly changed!
		}
	}
	return result;
}
