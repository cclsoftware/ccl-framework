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
// Filename    : miniaudioconverter.h
// Description : Channel Conversion based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudioconverter_h
#define _miniaudioconverter_h

#include "miniaudioglue.h"

#include "ccl/public/audio/iaudiosource.h"

namespace CCL {

//************************************************************************************************
// MiniAudioChannelConverter
/** Floating-point channel count/layout converter. */
//************************************************************************************************
// LATER TODO: Make public IAudioChannelConverter interface...

class MiniAudioChannelConverter
{
public:
	MiniAudioChannelConverter ();
	~MiniAudioChannelConverter ();

	/** Set the number of input and output channels to convert between. */
	tresult setChannelCounts (int numChannelsIn, int numChannelsOut);

	/**	Set the speaker layout of the input/output audio (SpeakerDef::kEmpty picks a standard
		layout for the corresponding channel count). */
	tresult setSpeakerFormats (SpeakerBitField speakerFormatIn, SpeakerBitField speakerFormatOut);

	/** Set the mixing algorithm to use when combining channels. */
	tresult setChannelMixMode (ChannelMixMode mode);

	/** Get the mixing algorithm currently in use. */
	ChannelMixMode getChannelMixMode () const { return channelMixMode; }

	/**	Convert audio from src to dst (conversion does not change the frame count). */
	tresult process (AudioData& dst, const AudioData& src, int sampleFrames);

protected:
	ma_channel_converter converter = {};
	bool initDone = false;

	int numChannelsIn = 0;
	int numChannelsOut = 0;
	SpeakerBitField speakerFormatIn = SpeakerDef::kEmpty;
	SpeakerBitField speakerFormatOut = SpeakerDef::kEmpty;
	ChannelMixMode channelMixMode = kChannelMixModeSpatial;

	tresult reinitialize ();
};

} // namespace CCL

#endif // _miniaudioconverter_h
