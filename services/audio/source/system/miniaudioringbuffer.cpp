//************************************************************************************************
//
// Audio Service
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
// Filename    : miniaudioringbuffer.cpp
// Description : Ring Buffer based on miniaudio
//
//************************************************************************************************

#include "miniaudioringbuffer.h"

#include "ccl/extras/audio/cclaudiodataops.h"

using namespace CCL;

//************************************************************************************************
// MiniAudioRingBuffer
//************************************************************************************************

MiniAudioRingBuffer::MiniAudioRingBuffer ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioRingBuffer::~MiniAudioRingBuffer ()
{
	uninitialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioRingBuffer::uninitialize ()
{
	if(initialized)
	{
		ma_pcm_rb_uninit (&ringBuffer);
		initialized = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioRingBuffer::resize (int newNumChannels, int newCapacityInFrames)
{
	uninitialize ();

	numChannels = 0;
	capacityInFrames = 0;

	if(newNumChannels <= 0 || newNumChannels > kMaxAudioStreamChannels || newCapacityInFrames <= 0)
		return false;

	// channels=1 on purpose: each subbuffer must be sized for exactly one channel's samples, not
	// numChannels-many. subbufferCount is what actually gives us one region per channel, all
	// sharing a single read/write cursor. See miniaudio.h's ma_pcm_rb_init_ex/get_subbuffer_ptr.
	initialized = ma_pcm_rb_init_ex (ma_format_f32, 1, ma_uint32(newCapacityInFrames), ma_uint32(newNumChannels),
									 ma_uint32(newCapacityInFrames), nullptr, nullptr, &ringBuffer) == MA_SUCCESS;
	if(!initialized)
		return false;

	numChannels = newNumChannels;
	capacityInFrames = newCapacityInFrames;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioRingBuffer::getAvailableRead () const
{
	return initialized ? int(ma_pcm_rb_available_read (const_cast<ma_pcm_rb*> (&ringBuffer))) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioRingBuffer::getAvailableWrite () const
{
	return initialized ? int(ma_pcm_rb_available_write (const_cast<ma_pcm_rb*> (&ringBuffer))) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioRingBuffer::reset (bool clear)
{
	if(!initialized)
		return;

	ma_pcm_rb_reset (&ringBuffer);

	if(!clear)
		return;

	AudioDataSpan span;
	int framesAcquired = acquireWrite (span, capacityInFrames);
	ASSERT (framesAcquired == capacityInFrames) // acquiring full buffer must succeed after reset
	AudioDataOpsGeneric::clear (span);

	ma_pcm_rb_reset (&ringBuffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioRingBuffer::acquireWrite (AudioDataSpan& span, int sampleFrames)
{
	if(!initialized)
		return 0;

	ma_uint32 framesAcquired = ma_uint32(sampleFrames);
	void* basePointer = nullptr;
	if(ma_pcm_rb_acquire_write (&ringBuffer, &framesAcquired, &basePointer) != MA_SUCCESS)
		return 0;

	span.numChannels = numChannels;
	span.numSamples = int(framesAcquired);
	span.floatType = kAudioFloat32;
	for(int ch = 0; ch < numChannels; ch++)
		span.buffers32[ch] = static_cast<float32*> (ma_pcm_rb_get_subbuffer_ptr (&ringBuffer, ma_uint32(ch), basePointer));

	return int(framesAcquired);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioRingBuffer::commitWrite (int sampleFrames)
{
	if(initialized)
		ma_pcm_rb_commit_write (&ringBuffer, ma_uint32(sampleFrames));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioRingBuffer::acquireRead (AudioDataSpan& span, int sampleFrames)
{
	if(!initialized)
		return 0;

	ma_uint32 framesAcquired = ma_uint32(sampleFrames);
	void* basePointer = nullptr;
	if(ma_pcm_rb_acquire_read (&ringBuffer, &framesAcquired, &basePointer) != MA_SUCCESS)
		return 0;

	span.numChannels = numChannels;
	span.numSamples = int(framesAcquired);
	span.floatType = kAudioFloat32;
	for(int ch = 0; ch < numChannels; ch++)
		span.buffers32[ch] = static_cast<float32*> (ma_pcm_rb_get_subbuffer_ptr (&ringBuffer, ma_uint32(ch), basePointer));

	return int(framesAcquired);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioRingBuffer::commitRead (int sampleFrames)
{
	if(initialized)
		ma_pcm_rb_commit_read (&ringBuffer, ma_uint32(sampleFrames));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioRingBuffer::write (const AudioData& data, int sampleFrames)
{
	int framesWritten = 0;
	while(framesWritten < sampleFrames)
	{
		AudioDataSpan ringSpan;
		int framesToWrite = acquireWrite (ringSpan, sampleFrames - framesWritten);
		if(framesToWrite == 0)
			break;

		AudioDataSpan sourceSpan (data, framesWritten, framesToWrite);
		AudioDataOpsGeneric::copyFrom (ringSpan, sourceSpan);

		commitWrite (framesToWrite);
		framesWritten += framesToWrite;
	}

	return framesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioRingBuffer::read (AudioData& data, int sampleFrames)
{
	int framesRead = 0;
	while(framesRead < sampleFrames)
	{
		AudioDataSpan ringSpan;
		int framesToRead = acquireRead (ringSpan, sampleFrames - framesRead);
		if(framesToRead == 0)
			break;

		AudioDataSpan destSpan (data, framesRead, framesToRead);
		AudioDataOpsGeneric::copyFrom (destSpan, ringSpan);

		commitRead (framesToRead);
		framesRead += framesToRead;
	}

	return framesRead;
}
