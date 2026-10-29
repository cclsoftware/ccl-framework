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
// Filename    : miniaudioresampler.h
// Description : Sample Rate Conversion based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudioresampler_h
#define _miniaudioresampler_h

#include "miniaudioglue.h"

namespace CCL {

//************************************************************************************************
// MiniAudioResampler
/** Floating-point sample rate converter. */
//************************************************************************************************
// LATER TODO: Make public IAudioResampler interface...

class MiniAudioResampler
{
public:
	MiniAudioResampler ();
	~MiniAudioResampler ();

	/** Set the number of audio channels to convert. */
	tresult setNumChannels (int numChannels);

	/** Set source and target sample rates. */
	tresult setRates (SampleRate sourceRate, SampleRate targetRate);

	/**	Convert audio from src to dst.
		On input, dstSampleFrames/srcSampleFrames give the output capacity/available input frame
		counts; on output, they hold the frame counts actually produced/consumed. */
	tresult process (AudioData& dst, int& dstSampleFrames, const AudioData& src, int& srcSampleFrames);

	/** Get the number of source frames required to produce the given number of target frames. */
	int getRequiredSourceFrameCount (int targetSampleFrames) const;

	/** Get the number of target frames expected from the given number of source frames. */
	int getExpectedTargetFrameCount (int sourceSampleFrames) const;

	/** Reset internal filter state, e.g. after seeking. */
	void reset ();

protected:
	ma_resampler resampler = {};
	bool initDone = false;

	int numChannels = 0;
	SampleRate sourceRate = 0;
	SampleRate targetRate = 0;

	tresult reinitialize ();
};

} // namespace CCL

#endif // _miniaudioresampler_h
