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
// Filename    : miniaudiocodec.cpp
// Description : Audio Codecs based on miniaudio
//
//************************************************************************************************

#include "miniaudiocodec.h"
#include "ccl/extras/audio/audioscratchbuffer.h"

#include "ccl/public/base/istream.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (WaveFile, "Wave File")
	XSTRING (FLACFile, "FLAC File")
	XSTRING (MP3File, "MP3 File")
END_XSTRINGS

//************************************************************************************************
// MiniAudioCodecFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MiniAudioCodecFactory, AudioCodecFactoryBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioCodecFactory::MiniAudioCodecFactory ()
{
	AudioCodecDesc wavDesc;
	wavDesc.fileType = FileType (nullptr, "wav", AudioContentTypes::kWave);
	FileTypes::init (wavDesc.fileType, XSTR (WaveFile));
	addCodec (wavDesc, MiniAudioDecoder::createInstance<ma_encoding_format_wav>, 
					   MiniAudioEncoder::createInstance<ma_encoding_format_wav>);

	AudioCodecDesc flacDesc;
	flacDesc.fileType = FileType (nullptr, "flac", AudioContentTypes::kFLAC);
	FileTypes::init (flacDesc.fileType, XSTR (FLACFile));
	addCodec (flacDesc, MiniAudioDecoder::createInstance<ma_encoding_format_flac>, nullptr);

	AudioCodecDesc mp3Desc;
	mp3Desc.fileType = FileType (nullptr, "mp3", AudioContentTypes::kMP3);
	FileTypes::init (mp3Desc.fileType, XSTR (MP3File));
	addCodec (mp3Desc, MiniAudioDecoder::createInstance<ma_encoding_format_mp3>, nullptr);
}

//************************************************************************************************
// MiniAudioDecoder
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioDecoder, AudioDecoderBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioDecoder::MiniAudioDecoder (IStream& stream, const AudioContentFormat& format, ma_encoding_format encoding)
: stream (stream),
  encoding (encoding)
{
	stream.retain ();

	// decode in the native sample format (if the codec supports it), it's converted to float in read ()
	ma_decoder_config config = ma_decoder_config_init (ma_format_unknown, ma_uint32(format.channelCount), ma_uint32(format.sampleRate));
	config.encodingFormat = encoding;

	initDone = ma_decoder_init (onRead, onSeek, this, &config, &decoder) == MA_SUCCESS;
	ASSERT (initDone)

	if(initDone)
		sampleFormat = decoder.outputFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioDecoder::~MiniAudioDecoder ()
{
	if(initDone)
		ma_decoder_uninit (&decoder);

	stream.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ ma_result MiniAudioDecoder::onRead (ma_decoder* pDecoder, void* pBufferOut, size_t bytesToRead, size_t* pBytesRead)
{
	auto* This = static_cast<MiniAudioDecoder*> (pDecoder->pUserData);

	int bytesRead = This->stream.read (pBufferOut, int(bytesToRead));
	if(bytesRead < 0)
	{
		*pBytesRead = 0;
		return MA_ERROR;
	}

	*pBytesRead = size_t(bytesRead);
	return size_t(bytesRead) < bytesToRead ? MA_AT_END : MA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ ma_result MiniAudioDecoder::onSeek (ma_decoder* pDecoder, ma_int64 byteOffset, ma_seek_origin origin)
{
	auto* This = static_cast<MiniAudioDecoder*> (pDecoder->pUserData);

	if(!This->stream.isSeekable ())
		return MA_NOT_IMPLEMENTED;

	return This->stream.seek (byteOffset, MiniAudioUtils::toCCLStreamSeekMode (origin)) >= 0 ? MA_SUCCESS : MA_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioDecoder::getFormat (AudioContentFormat& format) const
{
	if(!initDone)
		return kResultFailed;

	format = AudioContentFormat ();
	format.channelCount = getNumChannels ();
	format.sampleRate = getSampleRate ();
	format.speakerFormat = getSpeakerFormat ();
	MiniAudioUtils::fromSampleFormat (format, sampleFormat);
	// bit depth only reported correctly for WAV
	format.formatType = encoding == ma_encoding_format_wav ? kAudioFormatPCM : kAudioFormatNonPCM;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioDecoder::getNumChannels () const
{
	return initDone ? int(decoder.outputChannels) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioDecoder::getSampleRate () const
{
	return initDone ? SampleRate(decoder.outputSampleRate) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioDecoder::getFrameCount () const
{
	if(!initDone)
		return -1;

	ma_uint64 length = 0;
	if(ma_decoder_get_length_in_pcm_frames (const_cast<ma_decoder*> (&decoder), &length) != MA_SUCCESS)
		return -1;

	return TSamples(length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API MiniAudioDecoder::getSpeakerFormat () const
{
	if(!initDone)
		return SpeakerDef::kEmpty;

	ma_channel channelMap[kMaxAudioStreamChannels] = {};
	if(ma_decoder_get_data_format (const_cast<ma_decoder*> (&decoder), nullptr, nullptr, nullptr, channelMap, kMaxAudioStreamChannels) != MA_SUCCESS)
		return SpeakerDef::kEmpty;

	return MiniAudioUtils::toSpeakerFormat (channelMap, int(decoder.outputChannels));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioDecoder::read (AudioData& data, int sampleFrames)
{
	if(!initDone)
		return -1;

	int numChannels = int(decoder.outputChannels);

	ASSERT (data.numChannels == numChannels)
	if(data.numChannels != numChannels)
		return -1;

	ma_uint64 framesRead = 0;
	ma_result result = MA_SUCCESS;
	if(numChannels == 1 && data.floatType == kAudioFloat32)
	{
		// mono: the planar buffer is the interleaved buffer, no deinterleaving needed
		result = readFloat (framesRead, data.buffers32[0], sampleFrames);
		if(result != MA_SUCCESS && result != MA_AT_END)
			return -1;
	}
	else
	{
		AudioScratchBuffer::ScopedBuffer interleavedBuffer (sampleFrames, numChannels, kAudioFloat32);
		ASSERT (interleavedBuffer != nullptr)
		if(!interleavedBuffer)
			return -1;

		void* interleavedPointer = interleavedBuffer;
		result = readFloat (framesRead, static_cast<float32*> (interleavedPointer), sampleFrames);
		if(result != MA_SUCCESS && result != MA_AT_END)
			return -1;

		MiniAudioUtils::deinterleaveFloat32To (data, interleavedPointer, numChannels, int(framesRead));
	}

	return int(framesRead);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ma_result MiniAudioDecoder::readFloat (ma_uint64& framesRead, float32 output[], int sampleFrames)
{
	if(sampleFormat == ma_format_f32)
		return ma_decoder_read_pcm_frames (&decoder, output, ma_uint64(sampleFrames), &framesRead);
	else
	{
		// read in the native sample format and convert to float
		int numChannels = int(decoder.outputChannels);

		AudioScratchBuffer::ScopedBuffer nativeBuffer (sampleFrames, numChannels, kAudioFloat32); // float32 samples are large enough for any format
		ASSERT (nativeBuffer != nullptr)
		if(!nativeBuffer)
			return MA_OUT_OF_MEMORY;

		ma_result result = ma_decoder_read_pcm_frames (&decoder, nativeBuffer, ma_uint64(sampleFrames), &framesRead);
		if(result == MA_SUCCESS || result == MA_AT_END)
			ma_pcm_convert (output, ma_format_f32, nativeBuffer, sampleFormat, framesRead * ma_uint64(numChannels), ma_dither_mode_none);
	
		return result;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioDecoder::seek (TSamples position)
{
	if(!initDone)
		return -1;

	return ma_decoder_seek_to_pcm_frame (&decoder, ma_uint64(position)) == MA_SUCCESS ? position : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioDecoder::tell () const
{
	if(!initDone)
		return -1;

	ma_uint64 cursor = 0;
	if(ma_decoder_get_cursor_in_pcm_frames (const_cast<ma_decoder*> (&decoder), &cursor) != MA_SUCCESS)
		return -1;

	return TSamples(cursor);
}

//************************************************************************************************
// MiniAudioEncoder
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioEncoder, AudioEncoderBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioEncoder::MiniAudioEncoder (IStream& stream, const AudioContentFormat& format, ma_encoding_format encoding)
: stream (stream)
{
	stream.retain ();

	// unlike decoding, encoding has no "native format" to fall back to - the complete format is required
	ASSERT (format.isValid ())
	if(!format.isValid ())
		return;

	sampleFormat = MiniAudioUtils::toSampleFormat (format);
	ASSERT (sampleFormat != ma_format_unknown)
	if(sampleFormat == ma_format_unknown)
		return;

	ma_encoder_config config = ma_encoder_config_init (encoding, sampleFormat, ma_uint32(format.channelCount), ma_uint32(format.sampleRate));

	initDone = ma_encoder_init (onWrite, onSeek, this, &config, &encoder) == MA_SUCCESS;
	ASSERT (initDone)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioEncoder::~MiniAudioEncoder ()
{
	if(initDone)
		ma_encoder_uninit (&encoder);

	stream.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ ma_result MiniAudioEncoder::onWrite (ma_encoder* pEncoder, const void* pBufferIn, size_t bytesToWrite, size_t* pBytesWritten)
{
	auto* This = static_cast<MiniAudioEncoder*> (pEncoder->pUserData);

	int written = This->stream.write (pBufferIn, int(bytesToWrite));
	if(written < 0)
	{
		*pBytesWritten = 0;
		return MA_ERROR;
	}

	*pBytesWritten = size_t(written);
	return size_t(written) < bytesToWrite ? MA_ERROR : MA_SUCCESS; // a short write is always an error for a sink
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ ma_result MiniAudioEncoder::onSeek (ma_encoder* pEncoder, ma_int64 offset, ma_seek_origin origin)
{
	auto* This = static_cast<MiniAudioEncoder*> (pEncoder->pUserData);

	if(!This->stream.isSeekable ())
		return MA_NOT_IMPLEMENTED;

	return This->stream.seek (offset, MiniAudioUtils::toCCLStreamSeekMode (origin)) >= 0 ? MA_SUCCESS : MA_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioEncoder::getNumChannels () const
{
	return initDone ? int(encoder.config.channels) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API MiniAudioEncoder::getSampleRate () const
{
	return initDone ? SampleRate(encoder.config.sampleRate) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API MiniAudioEncoder::getFrameCount () const
{
	return framesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioEncoder::write (const AudioData& data, int sampleFrames)
{
	if(!initDone)
		return -1;

	int numChannels = int(encoder.config.channels);

	ASSERT (data.numChannels == numChannels)
	if(data.numChannels != numChannels)
		return -1;

	ma_uint64 framesWrittenThisCall = 0;
	ma_result result = MA_SUCCESS;
	if(numChannels == 1 && data.floatType == kAudioFloat32)
	{
		// mono: the planar buffer is the interleaved buffer, no interleaving needed
		result = writeFloat (framesWrittenThisCall, data.buffers32[0], sampleFrames);
	}
	else
	{
		AudioScratchBuffer::ScopedBuffer interleavedBuffer (sampleFrames, numChannels, kAudioFloat32);
		ASSERT (interleavedBuffer != nullptr)
		if(!interleavedBuffer)
			return -1;

		void* interleavedPointer = interleavedBuffer;
		MiniAudioUtils::interleaveFloat32From (interleavedPointer, data, numChannels, sampleFrames);

		result = writeFloat (framesWrittenThisCall, static_cast<const float32*> (interleavedPointer), sampleFrames);
	}

	if(result != MA_SUCCESS)
		return -1;

	framesWritten += TSamples(framesWrittenThisCall);
	return int(framesWrittenThisCall);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ma_result MiniAudioEncoder::writeFloat (ma_uint64& framesWrittenThisCall, const float32 interleaved[], int sampleFrames)
{
	if(sampleFormat == ma_format_f32)
		return ma_encoder_write_pcm_frames (&encoder, interleaved, ma_uint64(sampleFrames), &framesWrittenThisCall);
	else
	{
		// convert float to the native sample format
		int numChannels = int(encoder.config.channels);

		AudioScratchBuffer::ScopedBuffer convertedBuffer (sampleFrames, numChannels, kAudioFloat32); // float32 samples are large enough for any format
		ASSERT (convertedBuffer != nullptr)
		if(!convertedBuffer)
			return MA_OUT_OF_MEMORY;

		bool useDither = sampleFormat == ma_format_u8 || sampleFormat == ma_format_s16;
		ma_dither_mode ditherMode = useDither ? ma_dither_mode_triangle : ma_dither_mode_none;
		ma_pcm_convert (convertedBuffer, sampleFormat, interleaved, ma_format_f32, ma_uint64(sampleFrames) * ma_uint64(numChannels), ditherMode);

		return ma_encoder_write_pcm_frames (&encoder, convertedBuffer, ma_uint64(sampleFrames), &framesWrittenThisCall);
	}
}
