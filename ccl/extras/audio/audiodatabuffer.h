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
// Filename    : ccl/extras/audio/audiodatabuffer.h
// Description : Audio Data Buffer
//
//************************************************************************************************

#ifndef _ccl_audiodatabuffer_h
#define _ccl_audiodatabuffer_h

#include "ccl/base/object.h"

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

//************************************************************************************************
// AudioDataBuffer
/** Multi-channel floating-point audio buffer management class. */
//************************************************************************************************

class AudioDataBuffer: public Object
{
public:
	DECLARE_CLASS (AudioDataBuffer, Object)

	/** [HEAVY] Default constructor, allocates new memory. */
	AudioDataBuffer (int numSamples = 0, int numChannels = 1, AudioFloatType type = kAudioFloat32);
	
	/** [HEAVY] Copy constructor, allocates new memory. */
	AudioDataBuffer (const AudioDataBuffer& buffer);

	/** [LIGHT] Construct with AudioData, does not copy memory. */
	AudioDataBuffer (const AudioData& data);

	/** [LIGHT] Construct with single channel (32 Bit), does not copy memory. */
	AudioDataBuffer (float32* buffer, int numSamples);

	/** [LIGHT] Construct with single channel (64 Bit), does not copy memory. */
	AudioDataBuffer (float64* buffer, int numSamples);

	/** [LIGHT] Construct subsection of other buffer, does not copy memory. */
	AudioDataBuffer (const AudioDataBuffer& buffer, int sampleOffset, int sampleCount);

	/** [LIGHT] Construct subsection of AudioData, does not copy memory. */
	AudioDataBuffer (const AudioData& data, int sampleOffset, int sampleCount);

	~AudioDataBuffer ();

	static constexpr int kMaxChannels = kMaxAudioStreamChannels;

	AudioDataBuffer& operator= (const AudioDataBuffer& buffer) = delete; // no assignment

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer properties
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Check if buffer is empty. */
	bool isEmpty () const;

	/** Get number of audio channels. */
	int getNumChannels () const;

	/** Get number of audio samples. */
	int getNumSamples () const;

	/** Get floating-point type. */
	AudioFloatType getFloatType () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer access
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get 32 Bit channel buffer (check type before!). */
	float32* getBuffer32 (int channel = 0) const;

	/** Get 64 Bit channel buffer (check type before!). */
	float64* getBuffer64 (int channel = 0) const;

	/** Get channel buffer by type. */
	template<typename FloatType>
	FloatType* getBuffer (int channel = 0) const;
	
	/** Get channel buffer ignoring type. */
	void* getBuffer (int channel = 0) const;

	/** Get AudioData struct (non-const). */
	AudioData& getAudioData ();

	/** Get AudioData struct (const). */
	const AudioData& getAudioData () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer management
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Resize buffer. */
	virtual bool resize (int numSamples, int numChannels, AudioFloatType floatType, bool clear = true);

	/** Dispose allocated buffer. */
	void dispose ();

	/** Lock memory pages to physical memory. */
	void setLocked (bool state);

	/** Return the size in bytes of the allocated memory block. */
	int getAllocatedSize () const;

protected:
	AudioData data;
	void* allocatedBuffer;
	int allocatedSize;

	static constexpr int kLocked = 1<<0;
	int flags;
	
	union
	{
		float32* bufferArray32[kMaxChannels];
		float64* bufferArray64[kMaxChannels];
	};

	INLINE void clearBufferArray () // could be empty in release build?
	{ ::memset (bufferArray32, 0, sizeof(bufferArray32)); }

	bool allocate (int numChannels, int numSamples, AudioFloatType type, bool clear = true);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioDataBuffer inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioDataBuffer::isEmpty () const
{ return data.numSamples == 0 || data.numChannels == 0; }

inline int AudioDataBuffer::getNumChannels () const
{ return data.numChannels; }

inline int AudioDataBuffer::getNumSamples () const
{ return data.numSamples; }

inline AudioFloatType AudioDataBuffer::getFloatType () const
{ return data.floatType; }

inline float32* AudioDataBuffer::getBuffer32 (int channel) const
{ return getBuffer<float32> (channel); }

inline float64* AudioDataBuffer::getBuffer64 (int channel) const
{ return getBuffer<float64> (channel); }

inline void* AudioDataBuffer::getBuffer (int channel) const
{ ASSERT (channel >= 0 && channel < kMaxChannels) 
  return data.buffers32[channel]; }

template<typename FloatType>
inline FloatType* AudioDataBuffer::getBuffer (int channel) const
{ ASSERT (channel >= 0 && channel < kMaxChannels) 
  ASSERT (getAudioByteSize (1, data.floatType) == sizeof(FloatType))
  return reinterpret_cast<FloatType*> (data.buffers32[channel]); }

inline AudioData& AudioDataBuffer::getAudioData ()
{ return data; }

inline int AudioDataBuffer::getAllocatedSize () const
{ return allocatedSize; }

inline const AudioData& AudioDataBuffer::getAudioData () const
{ return data; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_audiodatabuffer_h
