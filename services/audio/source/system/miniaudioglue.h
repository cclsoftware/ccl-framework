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
// Filename    : miniaudioglue.h
// Description : miniaudio Glue Code
//
//************************************************************************************************

#ifndef _miniaudioglue_h
#define _miniaudioglue_h

#include "miniaudio.h"

#include "ccl/public/audio/cclaudiotypes.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

struct AudioContentFormat;

//************************************************************************************************
// MiniAudioUtils
//************************************************************************************************

namespace MiniAudioUtils
{
	String deviceIdToString (const ma_device_id& id, ma_backend backend);
	bool stringToDeviceId (ma_device_id& id, StringRef idString, ma_backend backend);

	int toCCLStreamSeekMode (ma_seek_origin origin);

	ma_format toSampleFormat (const AudioContentFormat& format);
	void fromSampleFormat (AudioContentFormat& format, ma_format sampleFormat);

	SpeakerBitField toSpeakerFormat (const ma_channel channelMap[], int numChannels);
	void fromSpeakerFormat (ma_channel channelMap[MA_MAX_CHANNELS], int numChannels, SpeakerBitField speakerFormat);

	inline void interleaveFloat32 (void* outputInterleaved, float32* inputs[], int numChannels, int numFrames)
	{
		ma_interleave_pcm_frames (ma_format_f32, numChannels, numFrames,
								  (const void**)(inputs), outputInterleaved);
	}

	inline void deinterleaveFloat32 (float32* outputs[], const void* inputInterleaved, int numChannels, int numFrames)
	{
		ma_deinterleave_pcm_frames (ma_format_f32, numChannels, numFrames,
									inputInterleaved, (void**)(outputs));
	}

	/** Deinterleave a range of channels. Planar outputs that have no matching interleaved channel are set to silence. */
	inline void deinterleaveFloat32 (float32* outputs[], int numOutputChannels,
									 const void* inputInterleaved, int numInputChannels, int inputChannelOffset,
									 int numFrames)
	{
		if(numInputChannels == numOutputChannels && inputChannelOffset == 0) // same geometry: use miniaudio's implementation
		{
			deinterleaveFloat32 (outputs, inputInterleaved, numOutputChannels, numFrames);
			return;
		}

		ASSERT (inputChannelOffset >= 0 && inputChannelOffset + numOutputChannels <= numInputChannels)
		int numCopied = ccl_bound<int> (numInputChannels - inputChannelOffset, 0, numOutputChannels);

		const float32* source = static_cast<const float32*> (inputInterleaved) + inputChannelOffset;
		for(int ch = 0; ch < numCopied; ch++)
		{
			const float32* in = source + ch;
			float32* out = outputs[ch];
			for(int i = 0; i < numFrames; i++, in += numInputChannels)
				out[i] = *in;
		}

		for(int ch = numCopied; ch < numOutputChannels; ch++)
			ma_silence_pcm_frames (outputs[ch], numFrames, ma_format_f32, 1);
	}

	/** Interleave a range of channels. All other channels of each frame are set to silence. */
	inline void interleaveFloat32 (void* outputInterleaved, int numOutputChannels, int outputChannelOffset,
								   float32* inputs[], int numInputChannels,
								   int numFrames)
	{
		if(numInputChannels == numOutputChannels && outputChannelOffset == 0) // same geometry: use miniaudio's implementation
		{
			interleaveFloat32 (outputInterleaved, inputs, numInputChannels, numFrames);
			return;
		}

		ASSERT (outputChannelOffset >= 0 && outputChannelOffset + numInputChannels <= numOutputChannels)
		int numCopied = ccl_bound<int> (numOutputChannels - outputChannelOffset, 0, numInputChannels);

		ma_silence_pcm_frames (outputInterleaved, numFrames, ma_format_f32, numOutputChannels);

		float32* destination = static_cast<float32*> (outputInterleaved) + outputChannelOffset;
		for(int ch = 0; ch < numCopied; ch++)
		{
			float32* out = destination + ch;
			const float32* in = inputs[ch];
			for(int i = 0; i < numFrames; i++, out += numOutputChannels)
				*out = in[i];
		}
	}

	/**	Interleave into a float32 buffer from src, which may be either float32 or float64. */
	inline void interleaveFloat32From (void* outputInterleaved, const AudioData& src, int numChannels, int numFrames)
	{
		if(src.floatType == kAudioFloat32)
		{
			interleaveFloat32 (outputInterleaved, src.buffers32, numChannels, numFrames);
			return;
		}

		float32* dst = static_cast<float32*> (outputInterleaved);
		for(int ch = 0; ch < numChannels; ch++)
			for(int i = 0; i < numFrames; i++)
				dst[i * numChannels + ch] = float32(src.buffers64[ch][i]);
	}

	/**	Deinterleave a float32 buffer into dst, which may be either float32 or float64. */
	inline void deinterleaveFloat32To (AudioData& dst, const void* inputInterleaved, int numChannels, int numFrames)
	{
		if(dst.floatType == kAudioFloat32)
		{
			deinterleaveFloat32 (dst.buffers32, inputInterleaved, numChannels, numFrames);
			return;
		}

		const float32* src = static_cast<const float32*> (inputInterleaved);
		for(int ch = 0; ch < numChannels; ch++)
			for(int i = 0; i < numFrames; i++)
				dst.buffers64[ch][i] = float64(src[i * numChannels + ch]);
	}
}

} // namespace CCL

#endif // _miniaudioglue_h