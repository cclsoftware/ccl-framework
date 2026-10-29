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
// Filename    : miniaudiocodec.h
// Description : Audio Codecs based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudiocodec_h
#define _miniaudiocodec_h

#include "miniaudioglue.h"

#include "../base/audiocodecbase.h"

namespace CCL {

//************************************************************************************************
// MiniAudioCodecFactory
//************************************************************************************************

class MiniAudioCodecFactory: public AudioCodecFactoryBase
{
public:
	DECLARE_CLASS (MiniAudioCodecFactory, AudioCodecFactoryBase)

	MiniAudioCodecFactory ();
};

//************************************************************************************************
// MiniAudioDecoder
//************************************************************************************************

class MiniAudioDecoder: public AudioDecoderBase
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioDecoder, AudioDecoderBase)

	MiniAudioDecoder (IStream& stream, const AudioContentFormat& format, ma_encoding_format encoding);
	~MiniAudioDecoder ();

	template <ma_encoding_format encoding>
	static AudioDecoderBase* createInstance (IStream& stream, const AudioContentFormat& format)
	{
		return NEW MiniAudioDecoder (stream, format, encoding);
	}

	// AudioDecoderBase
	tresult CCL_API getFormat (AudioContentFormat& format) const override;
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API read (AudioData& data, int sampleFrames) override;
	TSamples CCL_API seek (TSamples position) override;
	TSamples CCL_API tell () const override;

protected:
	IStream& stream;
	ma_encoding_format encoding = ma_encoding_format_unknown;
	ma_decoder decoder = {};
	ma_format sampleFormat = ma_format_unknown;
	bool initDone = false;

	/** Read interleaved float32 samples, converted from the native sample format if needed. */
	ma_result readFloat (ma_uint64& framesRead, float32 output[], int sampleFrames);

	static ma_result onRead (ma_decoder* pDecoder, void* pBufferOut, size_t bytesToRead, size_t* pBytesRead);
	static ma_result onSeek (ma_decoder* pDecoder, ma_int64 byteOffset, ma_seek_origin origin);
};

//************************************************************************************************
// MiniAudioEncoder
//************************************************************************************************

class MiniAudioEncoder: public AudioEncoderBase
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioEncoder, AudioEncoderBase)

	MiniAudioEncoder (IStream& stream, const AudioContentFormat& format, ma_encoding_format encoding);
	~MiniAudioEncoder ();

	template <ma_encoding_format encoding>
	static AudioEncoderBase* createInstance (IStream& stream, const AudioContentFormat& format)
	{
		return NEW MiniAudioEncoder (stream, format, encoding);
	}

	// AudioEncoderBase
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	int CCL_API write (const AudioData& data, int sampleFrames) override;

protected:
	IStream& stream;
	ma_encoder encoder = {};
	ma_format sampleFormat = ma_format_unknown;
	bool initDone = false;
	TSamples framesWritten = 0;

	/** Write interleaved float32 samples, converted to the sample format of the encoder if needed. */
	ma_result writeFloat (ma_uint64& framesWrittenThisCall, const float32 interleaved[], int sampleFrames);

	static ma_result onWrite (ma_encoder* pEncoder, const void* pBufferIn, size_t bytesToWrite, size_t* pBytesWritten);
	static ma_result onSeek (ma_encoder* pEncoder, ma_int64 offset, ma_seek_origin origin);
};

} // namespace CCL

#endif // _miniaudiocodec_h
