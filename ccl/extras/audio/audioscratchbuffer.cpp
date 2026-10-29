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
// Filename    : ccl/extras/audio/audioscratchbuffer.cpp
// Description : Audio Scratch Buffer Pools
//
//************************************************************************************************

#include "ccl/extras/audio/audioscratchbuffer.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (AudioScratchBufferPools, kFirstRun)
{
	// dispose manually for the main thread, otherwise it's too late
	AudioScratchBuffer::instance ().dispose ();
	AudioScratchDataBuffer::instance ().dispose ();
}

//************************************************************************************************
// AudioScratchBuffer
//************************************************************************************************

DEFINE_THREAD_SINGLETON (AudioScratchBuffer)

//////////////////////////////////////////////////////////////////////////////////////////////////

void* AudioScratchBuffer::use (int sampleFrames, int channelCount, AudioFloatType floatType)
{
	if(depth >= kMaxAudioScratchNestedUse)
	{
		ASSERT (false) // increase kMaxAudioScratchNestedUse, or a call site is leaking
		return nullptr;
	}

	Buffer& buffer = buffers[depth];
	uint32 neededCapacity = uint32(getAudioByteSize (sampleFrames * channelCount, floatType));
	if(neededCapacity > buffer.getSize () && !buffer.resize (neededCapacity))
		return nullptr;

	depth++;
	return buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioScratchBuffer::unuse (void* buffer)
{
	ASSERT (depth > 0)
	if(depth <= 0)
		return;

	depth--;

	// use()/unuse() calls must nest strictly LIFO
	ASSERT (buffer == buffers[depth])
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioScratchBuffer::dispose ()
{
	ASSERT (depth == 0)

	for(int i = 0; i < kMaxAudioScratchNestedUse; i++)
		buffers[i].resize (0);
}

//************************************************************************************************
// AudioScratchDataBuffer
//************************************************************************************************

DEFINE_THREAD_SINGLETON (AudioScratchDataBuffer)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioData* AudioScratchDataBuffer::use (int sampleFrames, int channelCount, AudioFloatType floatType)
{
	if(depth >= kMaxAudioScratchNestedUse)
	{
		ASSERT (false) // increase kMaxAudioScratchNestedUse, or a call site is leaking
		return nullptr;
	}

	ASSERT (floatType == kAudioFloat32 || floatType == kAudioFloat64)
	AudioDataBuffer& buffer = buffers[depth][floatType];
	if(buffer.getNumSamples () < sampleFrames || buffer.getNumChannels () < channelCount || buffer.getFloatType () != floatType)
	{
		int growSamples = ccl_max (buffer.getNumSamples (), sampleFrames);
		int growChannels = ccl_max (buffer.getNumChannels (), channelCount);
		if(!buffer.resize (growSamples, growChannels, floatType, false))
			return nullptr;
	}

	// the cached buffer may have more samples/channels allocated than this call needs - narrow a
	// local copy rather than the buffer's own AudioData, which must keep reporting its real capacity
	AudioData& view = views[depth][floatType];
	view = buffer.getAudioData ();
	view.numSamples = sampleFrames;
	view.numChannels = channelCount;

	depth++;
	return &view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioScratchDataBuffer::unuse (AudioData* buffer)
{
	ASSERT (depth > 0)
	if(depth <= 0)
		return;

	depth--;

	// use()/unuse() calls must nest strictly LIFO
	ASSERT (buffer != nullptr && buffer == &views[depth][buffer->floatType])
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioScratchDataBuffer::dispose ()
{
	ASSERT (depth == 0)

	for(int i = 0; i < kMaxAudioScratchNestedUse; i++)
		for(int t = 0; t < kNumAudioFloatTypes; t++)
			buffers[i][t].dispose ();
}
