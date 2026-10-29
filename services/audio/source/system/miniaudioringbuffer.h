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
// Filename    : miniaudioringbuffer.h
// Description : Ring Buffer based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudioringbuffer_h
#define _miniaudioringbuffer_h

#include "miniaudioglue.h"

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

//************************************************************************************************
// MiniAudioRingBuffer
/** Single-producer/single-consumer audio ring buffer. */
//************************************************************************************************
// LATER TODO: Make public IAudioRingBuffer interface...

class MiniAudioRingBuffer
{
public:
	MiniAudioRingBuffer ();
	~MiniAudioRingBuffer ();

	/** Resize ring buffer. Discards any data. */
	bool resize (int newNumChannels, int newCapacityInFrames);

	int getNumChannels () const { return numChannels; }
	int getCapacity () const { return capacityInFrames; }

	int getAvailableRead () const;
	int getAvailableWrite () const;

	/**	Discard data without changing size.
		If clear is true, also zero-fill the whole buffer. */
	void reset (bool clear = false);

	/**	Acquire write space pointing directly into ring buffer storage.
		Returns the number of frames acquired,
		can be less than requested or 0 when full. */
	int acquireWrite (AudioDataSpan& span, int sampleFrames);
	void commitWrite (int sampleFrames);

	/** Same as idea for the consumer side. */
	int acquireRead (AudioDataSpan& span, int sampleFrames);
	void commitRead (int sampleFrames);

	/**	Write to ring buffer. Returns the number of frames written, 
		can be less than requested or 0 when full. */
	int write (const AudioData& data, int sampleFrames);

	/**	Read from ring buffer. Returns the number of frames read, 
		can be less than requested underrun. */
	int read (AudioData& data, int sampleFrames);

protected:
	ma_pcm_rb ringBuffer = {};
	bool initialized = false;
	int numChannels = 0;
	int capacityInFrames = 0;

	void uninitialize ();
};

} // namespace CCL

#endif // _miniaudioringbuffer_h
