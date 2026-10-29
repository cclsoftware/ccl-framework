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
// Filename    : miniaudioresampler.cpp
// Description : Sample Rate Conversion based on miniaudio
//
//************************************************************************************************

#include "miniaudioresampler.h"
#include "ccl/extras/audio/audioscratchbuffer.h"

using namespace CCL;

//************************************************************************************************
// MiniAudioResampler
//************************************************************************************************

MiniAudioResampler::MiniAudioResampler ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioResampler::~MiniAudioResampler ()
{
	if(initDone)
		ma_resampler_uninit (&resampler, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioResampler::setNumChannels (int newNumChannels)
{
	if(newNumChannels <= 0)
		return kResultInvalidArgument;

	numChannels = newNumChannels;

	if(sourceRate > 0 && targetRate > 0)
		return reinitialize ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioResampler::setRates (SampleRate newSourceRate, SampleRate newTargetRate)
{
	if(newSourceRate <= 0 || newTargetRate <= 0)
		return kResultInvalidArgument;

	sourceRate = newSourceRate;
	targetRate = newTargetRate;

	if(initDone)
		return ma_resampler_set_rate (&resampler, ma_uint32(sourceRate), ma_uint32(targetRate)) == MA_SUCCESS ?
			   kResultOk : kResultFailed;

	if(numChannels > 0)
		return reinitialize ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioResampler::reinitialize ()
{
	if(initDone)
	{
		ma_resampler_uninit (&resampler, nullptr);
		initDone = false;
	}

	ma_resampler_config config = ma_resampler_config_init (ma_format_f32, ma_uint32(numChannels),
															ma_uint32(sourceRate), ma_uint32(targetRate),
															ma_resample_algorithm_linear);
	if(ma_resampler_init (&config, nullptr, &resampler) != MA_SUCCESS)
		return kResultFailed;

	initDone = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioResampler::process (AudioData& dst, int& dstSampleFrames, const AudioData& src, int& srcSampleFrames)
{
	if(!initDone)
		return kResultUnexpected;

	AudioScratchBuffer::ScopedBuffer interleavedSrc (srcSampleFrames, numChannels, kAudioFloat32);
	AudioScratchBuffer::ScopedBuffer interleavedDst (dstSampleFrames, numChannels, kAudioFloat32);
	if(!interleavedSrc || !interleavedDst)
		return kResultOutOfMemory;

	MiniAudioUtils::interleaveFloat32From (interleavedSrc, src, numChannels, srcSampleFrames);

	ma_uint64 framesIn = ma_uint64(srcSampleFrames);
	ma_uint64 framesOut = ma_uint64(dstSampleFrames);
	if(ma_resampler_process_pcm_frames (&resampler, interleavedSrc, &framesIn, interleavedDst, &framesOut) != MA_SUCCESS)
		return kResultFailed;

	MiniAudioUtils::deinterleaveFloat32To (dst, interleavedDst, numChannels, int(framesOut));

	srcSampleFrames = int(framesIn);
	dstSampleFrames = int(framesOut);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioResampler::getRequiredSourceFrameCount (int targetSampleFrames) const
{
	if(!initDone)
		return 0;

	ma_uint64 count = 0;
	ma_resampler_get_required_input_frame_count (&resampler, ma_uint64(targetSampleFrames), &count);
	return int(count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioResampler::getExpectedTargetFrameCount (int sourceSampleFrames) const
{
	if(!initDone)
		return 0;

	ma_uint64 count = 0;
	ma_resampler_get_expected_output_frame_count (&resampler, ma_uint64(sourceSampleFrames), &count);
	return int(count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioResampler::reset ()
{
	if(initDone)
		ma_resampler_reset (&resampler);
}
