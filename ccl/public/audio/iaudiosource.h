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
// Filename    : ccl/public/audio/iaudiosource.h
// Description : Audio Source/Sink Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudiosource_h
#define _ccl_iaudiosource_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

interface IProgressNotify;

//************************************************************************************************
// AudioContentFormat
//************************************************************************************************

struct AudioContentFormat
{
	int channelCount = 0;
	SampleRate sampleRate = 0;
	SpeakerBitField speakerFormat = SpeakerDef::kEmpty;
	AudioSampleType sampleType = kAudioSampleTypeInt;
	int bitDepth = 0;
	AudioFormatType formatType = kAudioFormatPCM;

	bool isValid () const;
	bool isFloat () const;

	AudioContentFormat& makeDefaultForBitDepth (int bitDepth);
	AudioContentFormat& makeFloat (AudioFloatType floatType = kAudioFloat32);
};

//************************************************************************************************
// IAudioFormatProvider
/** Interface to query audio content format. */
//************************************************************************************************

interface IAudioFormatProvider: IUnknown
{
	/** Get audio content format. */
	virtual tresult CCL_API getFormat (AudioContentFormat& format) const = 0;

	DECLARE_IID (IAudioFormatProvider)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioFormatProvider, 0x8d2460ec, 0xe860, 0x452f, 0x9c, 0x82, 0x83, 0x95, 0x2, 0xcd, 0xa7, 0x50)
#endif

//************************************************************************************************
// IAudioContent
/** Shared properties of readable/writable audio content. */
//************************************************************************************************

interface IAudioContent: IUnknown
{
	/** Get number of audio channels. */
	virtual int CCL_API getNumChannels () const = 0;

	/** Get sample rate. */
	virtual SampleRate CCL_API getSampleRate () const = 0;

	/**	Get length in audio frames.
		For a source: total readable length, or -1 if unbounded.
		For a sink: number of frames written so far. */
	virtual TSamples CCL_API getFrameCount () const = 0;

	/** Get speaker format of audio channels. */
	virtual SpeakerBitField CCL_API getSpeakerFormat () const = 0;

	DECLARE_IID (IAudioContent)

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get duration in seconds. */
	TSeconds getDuration () const;
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioContent, 0x57cedd7b, 0x86d9, 0x471e, 0xa6, 0x77, 0xfd, 0x01, 0x32, 0x1f, 0x16, 0x40)
#endif

//************************************************************************************************
// IAudioSource
/** Abstract source of floating-point audio, can be implemented by decoders, generators, etc. */
//************************************************************************************************

interface IAudioSource: IAudioContent
{
	/**	Read the next block of audio.
		Returns the number of samples actually read or negative value on error. */
	virtual int CCL_API read (AudioData& data, int sampleFrames) = 0;

	/**	Seek to audio frame position.
		May fail if the source is not seekable. */
	virtual TSamples CCL_API seek (TSamples position) = 0;

	/** Get current read position, in audio frames. */
	virtual TSamples CCL_API tell () const = 0;

	DECLARE_IID (IAudioSource)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioSource, 0xa1f9bb20, 0x3b5d, 0x47e7, 0xaf, 0x39, 0xed, 0x10, 0x4d, 0x36, 0xa9, 0x1d)
#endif

//************************************************************************************************
// IAudioSink
/** Abstract destination for floating-point audio, can be implemented by encoders and others. */
//************************************************************************************************

interface IAudioSink: IAudioContent
{
	/**	Write a block of audio.
		Returns the number of samples actually written, or a negative value on error. */
	virtual int CCL_API write (const AudioData& data, int sampleFrames) = 0;

	DECLARE_IID (IAudioSink)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioSink, 0x786dcc91, 0xae52, 0x47c9, 0x96, 0x2c, 0xa9, 0x93, 0x5b, 0xe3, 0x5e, 0x83)
#endif

//************************************************************************************************
// IWrappedAudioSource
/** Common base for an IAudioSource that wraps another IAudioSource. */
//************************************************************************************************

interface IWrappedAudioSource: IAudioSource
{
	/** Assign the underlying source to wrap. Retains it. */
	virtual tresult CCL_API setSource (IAudioSource* source) = 0;

	/** Get the underlying source. Does not transfer ownership. */
	virtual IAudioSource* CCL_API getSource () const = 0;

	DECLARE_IID (IWrappedAudioSource)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IWrappedAudioSource, 0x9e7458cf, 0x78d1, 0x4520, 0xaa, 0xe1, 0x47, 0x6e, 0x97, 0xfa, 0xa4, 0xad)
#endif

//************************************************************************************************
// IWrappedAudioSink
/** Common base for an IAudioSink that wraps another IAudioSink. */
//************************************************************************************************

interface IWrappedAudioSink: IAudioSink
{
	/** Assign the underlying sink to wrap. Retains it. */
	virtual tresult CCL_API setSink (IAudioSink* sink) = 0;

	/** Get the underlying sink. Does not transfer ownership. */
	virtual IAudioSink* CCL_API getSink () const = 0;

	DECLARE_IID (IWrappedAudioSink)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IWrappedAudioSink, 0x7cf506d3, 0x932f, 0x4093, 0x88, 0x26, 0x92, 0xf5, 0x37, 0xf9, 0xc0, 0xcd)
#endif

//************************************************************************************************
// IBufferedAudioSource
/**	Realtime-friendly buffered audio source for a consumer from a wrapped source that may block.
	The caller is responsible for driving fill() from whatever thread it likes.
	setSource()/resize() are not safe to call while fill()/read() are concurrently in progress. */
//************************************************************************************************

interface IBufferedAudioSource: IWrappedAudioSource
{
	/**	Resize the internal buffer to hold the given capacity, in audio frames.
		Discards any currently buffered data. */
	virtual tresult CCL_API resize (int capacity) = 0;

	/** Get the total buffer capacity, in frames. */
	virtual int CCL_API getCapacity () const = 0;

	/**	Pull more data from the assigned source into the internal buffer, checks for
		cancelation via progress callback.

		kResultOk : data was accepted or the buffer is simply already full
		kResultFalse : assigned source has no more data
		kResultFailed : a source error occured
		kResultAborted : canceled via progress before completion. */
	virtual tresult CCL_API fill (IProgressNotify* progress = nullptr) = 0;

	/**	Get the number of frames currently buffered and available to read. */
	virtual int CCL_API getBufferedAmount () const = 0;

	DECLARE_IID (IBufferedAudioSource)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IBufferedAudioSource, 0x9d3a6c14, 0x7b2f, 0x4e91, 0xa4, 0x5d, 0x1c, 0x8e, 0x3f, 0x72, 0xb6, 0x09)
#endif

//************************************************************************************************
// IBufferedAudioSink
/** Realtime-friendly buffered audio sink for a producer that must not block on a wrapped sink.
	The caller is responsible for driving flush() from whatever thread it likes.
	setSink()/resize() are not safe to call while write()/flush() are concurrently in progress. 
	
	The write() method accept frames into the internal buffer without calling the wrapped sink
	and returns a partial frame count when the buffer is full; the caller owns the dropped
	remainder. */
//************************************************************************************************

interface IBufferedAudioSink: IWrappedAudioSink
{
	/**	Resize the internal buffer to hold the given capacity, in audio frames.
		Discards any currently buffered data. */
	virtual tresult CCL_API resize (int capacity) = 0;

	/** Get the total buffer capacity, in frames. */
	virtual int CCL_API getCapacity () const = 0;

	/**	Push buffered data to the assigned sink, checks for cancelation via progress callback.

		kResultOk : all currently buffered data was written
		kResultFalse : the assigned sink accepted only part of the buffered data
		kResultFailed : no sink is assigned or a sink error occurred
		kResultAborted : canceled via progress before completion. */
	virtual tresult CCL_API flush (IProgressNotify* progress = nullptr) = 0;

	/** Get the number of frames currently buffered and awaiting flush. */
	virtual int CCL_API getBufferedAmount () const = 0;

	DECLARE_IID (IBufferedAudioSink)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IBufferedAudioSink, 0x5c2a1c93, 0x8f4e, 0x4b6d, 0x9a, 0x71, 0x3e, 0xd8, 0x2b, 0x46, 0xc0, 0x15)
#endif

//************************************************************************************************
// IResamplingAudioSource
/** Wraps an IAudioSource, converting its audio to a different sample rate on read(). */
//************************************************************************************************

interface IResamplingAudioSource: IWrappedAudioSource
{
	/** Set the sample rate to resample to. */
	virtual tresult CCL_API setTargetRate (SampleRate sampleRate) = 0;

	DECLARE_IID (IResamplingAudioSource)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IResamplingAudioSource, 0x4c7f3d1b, 0x875d, 0x4800, 0x9f, 0x85, 0xcc, 0xed, 0x45, 0xaf, 0xd2, 0x45)
#endif

//************************************************************************************************
// IResamplingAudioSink
/** Wraps an IAudioSink, converting incoming audio from a different sample rate on write(). */
//************************************************************************************************

interface IResamplingAudioSink: IWrappedAudioSink
{
	/** Set the sample rate of the audio that will be passed to write(). */
	virtual tresult CCL_API setSourceRate (SampleRate sampleRate) = 0;

	DECLARE_IID (IResamplingAudioSink)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IResamplingAudioSink, 0xa2692afe, 0x6958, 0x4f7a, 0xa6, 0xce, 0x40, 0x1e, 0x57, 0xcd, 0xc0, 0xb7)
#endif

//************************************************************************************************
// ChannelMixMode
//************************************************************************************************

DEFINE_ENUM (ChannelMixMode)
{
	kChannelMixModeSpatial,  ///< default: spatial-locality-based blending between input/output channels
	kChannelMixModeSimple    ///< drop excess input channels / silence excess output channels, no blending
};

//************************************************************************************************
// IConvertingAudioSource
/** Wraps an IAudioSource, converting its channel count/layout on read(). */
//************************************************************************************************

interface IConvertingAudioSource: IWrappedAudioSource
{
	/**	Set the number of channels to convert to, and optionally the speaker layout
		(SpeakerDef::kEmpty picks a standard layout for channelCount). */
	virtual tresult CCL_API setTargetChannels (int channelCount, SpeakerBitField speakerFormat = SpeakerDef::kEmpty) = 0;

	/** Set the mixing algorithm to use when combining channels. */
	virtual tresult CCL_API setChannelMixMode (ChannelMixMode mode) = 0;

	/** Get the mixing algorithm currently in use. */
	virtual ChannelMixMode CCL_API getChannelMixMode () const = 0;

	DECLARE_IID (IConvertingAudioSource)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IConvertingAudioSource, 0x8a3f9c21, 0x4d6e, 0x4a1b, 0x9c, 0x02, 0x1e, 0x7f, 0x6b, 0x3d, 0x88, 0x4a)
#endif

//************************************************************************************************
// IConvertingAudioSink
/** Wraps an IAudioSink, converting incoming audio from a different channel count/layout on write(). */
//************************************************************************************************

interface IConvertingAudioSink: IWrappedAudioSink
{
	/**	Set the number of channels of the audio that will be passed to write(), and optionally its
		speaker layout (SpeakerDef::kEmpty picks a standard layout for channelCount). */
	virtual tresult CCL_API setSourceChannels (int channelCount, SpeakerBitField speakerFormat = SpeakerDef::kEmpty) = 0;

	/** Set the mixing algorithm to use when combining channels. */
	virtual tresult CCL_API setChannelMixMode (ChannelMixMode mode) = 0;

	/** Get the mixing algorithm currently in use. */
	virtual ChannelMixMode CCL_API getChannelMixMode () const = 0;

	DECLARE_IID (IConvertingAudioSink)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IConvertingAudioSink, 0x2f6b7d14, 0x9a3c, 0x4e58, 0xb1, 0x77, 0x0d, 0x42, 0xa9, 0xc6, 0x5e, 0x33)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioContent inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline TSeconds IAudioContent::getDuration () const
{
	TSamples frames = getFrameCount ();
	SampleRate sampleRate = getSampleRate ();
	return sampleRate > 0 ? TSeconds(frames) / sampleRate : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioContentFormat inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioContentFormat::isValid () const
{
	return channelCount > 0 && sampleRate > 0 && bitDepth > 0; 
}

inline bool AudioContentFormat::isFloat () const
{
	return sampleType == kAudioSampleTypeFloat;
}

inline AudioContentFormat& AudioContentFormat::makeDefaultForBitDepth (int _bitDepth)
{
	bitDepth = _bitDepth;
	sampleType = bitDepth <= 8 ? sampleType = kAudioSampleTypeUnsigned : 
				 bitDepth >= 32 ? kAudioSampleTypeFloat :
				 kAudioSampleTypeInt;
	return *this;
}

inline AudioContentFormat& AudioContentFormat::makeFloat (AudioFloatType floatType)
{
	sampleType = kAudioSampleTypeFloat;
	bitDepth = floatType == kAudioFloat32 ? 32 : 64;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iaudiosource_h
