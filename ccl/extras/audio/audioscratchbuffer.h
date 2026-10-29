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
// Filename    : ccl/extras/audio/audioscratchbuffer.h
// Description : Audio Scratch Buffer Pools
//
//************************************************************************************************

#ifndef _ccl_audioscratchbuffer_h
#define _ccl_audioscratchbuffer_h

#include "ccl/extras/audio/audiodatabuffer.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/system/threadlocal.h"

namespace CCL {

/** Max. concurrently acquired buffers per thread. */
static constexpr int kMaxAudioScratchNestedUse = 10;

/** Number of audio float types (32 and 64 bit). */
static constexpr int kNumAudioFloatTypes = 2;

//************************************************************************************************
// ScopedScratchBuffer
/** Guard shared by the scratch buffer pools below: acquires a nested buffer from TPool on
	construction, releases it on scope exit. */
//************************************************************************************************

template<class TPool, class TResult>
class ScopedScratchBuffer
{
public:
	template<class... TArgs>
	ScopedScratchBuffer (TArgs&&... args)
	: buffer (TPool::instance ().use (args...))
	{}

	~ScopedScratchBuffer ()
	{
		if(buffer)
			TPool::instance ().unuse (buffer);
	}

	ScopedScratchBuffer (const ScopedScratchBuffer&) = delete;
	ScopedScratchBuffer& operator= (const ScopedScratchBuffer&) = delete;

	operator TResult* () const { return buffer; }

private:
	TResult* buffer;
};

//************************************************************************************************
// AudioScratchBuffer
/** Lazily grown interleaved scratch buffer pool, used per thread and supporting nested use (LIFO).
	Prefer ScopedBuffer over raw use()/unuse(). */
//************************************************************************************************

class AudioScratchBuffer: public Threading::ThreadSingleton<AudioScratchBuffer>
{
public:
	/**	Acquire the next nested buffer, growing it to fit if needed.
		Returns nullptr on allocation failure, or if kMaxAudioScratchNestedUse is already
		exhausted. */
	void* use (int sampleFrames, int channelCount, AudioFloatType floatType = kAudioFloat32);

	/** Release the most recently acquired buffer (LIFO). Must be the pointer use() returned. */
	void unuse (void* buffer);

	/** Free all allocated memory for current thread.. */
	void dispose ();

	using ScopedBuffer = ScopedScratchBuffer<AudioScratchBuffer, void>;

private:
	Buffer buffers[kMaxAudioScratchNestedUse];
	int depth = 0;
};

//************************************************************************************************
// AudioScratchDataBuffer
/** Lazily grown, deinterleaved multi-channel scratch buffer pool, used per thread and
	supporting nested use (LIFO). Prefer ScopedBuffer over raw use()/unuse(). */
//************************************************************************************************

class AudioScratchDataBuffer: public Threading::ThreadSingleton<AudioScratchDataBuffer>
{
public:
	/**	Acquire the next nested buffer, growing it to fit if needed.
		Returns nullptr on allocation failure, or if kMaxAudioScratchNestedUse is already
		exhausted. */
	AudioData* use (int sampleFrames, int channelCount, AudioFloatType floatType = kAudioFloat32);

	/** Release the most recently acquired buffer (LIFO). Must be the pointer use() returned. */
	void unuse (AudioData* buffer);

	/** Free all allocated memory for current thread.. */
	void dispose ();

	using ScopedBuffer = ScopedScratchBuffer<AudioScratchDataBuffer, AudioData>;

private:
	AudioDataBuffer buffers[kMaxAudioScratchNestedUse][kNumAudioFloatTypes];
	AudioData views[kMaxAudioScratchNestedUse][kNumAudioFloatTypes];
	int depth = 0;
};

} // namespace CCL

#endif // _ccl_audioscratchbuffer_h
