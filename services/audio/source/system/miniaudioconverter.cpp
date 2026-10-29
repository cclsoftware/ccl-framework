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
// Filename    : miniaudioconverter.cpp
// Description : Channel Conversion based on miniaudio
//
//************************************************************************************************

#include "miniaudioconverter.h"

#include "ccl/extras/audio/audioscratchbuffer.h"

using namespace CCL;

//************************************************************************************************
// MiniAudioChannelConverter
//************************************************************************************************

MiniAudioChannelConverter::MiniAudioChannelConverter ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioChannelConverter::~MiniAudioChannelConverter ()
{
	if(initDone)
		ma_channel_converter_uninit (&converter, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioChannelConverter::setChannelCounts (int newNumChannelsIn, int newNumChannelsOut)
{
	if(newNumChannelsIn <= 0 || newNumChannelsOut <= 0)
		return kResultInvalidArgument;

	numChannelsIn = newNumChannelsIn;
	numChannelsOut = newNumChannelsOut;

	return reinitialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioChannelConverter::setSpeakerFormats (SpeakerBitField newSpeakerFormatIn, SpeakerBitField newSpeakerFormatOut)
{
	speakerFormatIn = newSpeakerFormatIn;
	speakerFormatOut = newSpeakerFormatOut;

	if(numChannelsIn > 0 && numChannelsOut > 0)
		return reinitialize ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioChannelConverter::setChannelMixMode (ChannelMixMode mode)
{
	channelMixMode = mode;

	if(numChannelsIn > 0 && numChannelsOut > 0)
		return reinitialize ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioChannelConverter::reinitialize ()
{
	if(initDone)
	{
		ma_channel_converter_uninit (&converter, nullptr);
		initDone = false;
	}

	ma_channel channelMapIn[MA_MAX_CHANNELS] = {};
	ma_channel channelMapOut[MA_MAX_CHANNELS] = {};

	bool hasMapIn = speakerFormatIn != SpeakerDef::kEmpty;
	bool hasMapOut = speakerFormatOut != SpeakerDef::kEmpty;

	if(hasMapIn)
		MiniAudioUtils::fromSpeakerFormat (channelMapIn, numChannelsIn, speakerFormatIn);
	if(hasMapOut)
		MiniAudioUtils::fromSpeakerFormat (channelMapOut, numChannelsOut, speakerFormatOut);

	ma_channel_mix_mode maChannelMixMode = channelMixMode == kChannelMixModeSimple ? ma_channel_mix_mode_simple : ma_channel_mix_mode_rectangular;

	ma_channel_converter_config config = ma_channel_converter_config_init (
		ma_format_f32, ma_uint32(numChannelsIn), hasMapIn ? channelMapIn : nullptr,
		ma_uint32(numChannelsOut), hasMapOut ? channelMapOut : nullptr, maChannelMixMode);

	if(ma_channel_converter_init (&config, nullptr, &converter) != MA_SUCCESS)
		return kResultFailed;

	initDone = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioChannelConverter::process (AudioData& dst, const AudioData& src, int sampleFrames)
{
	if(!initDone)
		return kResultUnexpected;

	AudioScratchBuffer::ScopedBuffer interleavedSrc (sampleFrames, numChannelsIn, kAudioFloat32);
	AudioScratchBuffer::ScopedBuffer interleavedDst (sampleFrames, numChannelsOut, kAudioFloat32);
	if(!interleavedSrc || !interleavedDst)
		return kResultOutOfMemory;

	MiniAudioUtils::interleaveFloat32From (interleavedSrc, src, numChannelsIn, sampleFrames);

	if(ma_channel_converter_process_pcm_frames (&converter, interleavedDst, interleavedSrc, ma_uint64(sampleFrames)) != MA_SUCCESS)
		return kResultFailed;

	MiniAudioUtils::deinterleaveFloat32To (dst, interleavedDst, numChannelsOut, sampleFrames);

	return kResultOk;
}
