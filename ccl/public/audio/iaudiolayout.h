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
// Filename    : ccl/public/audio/iaudiolayout.h
// Description : Audio Stream Layout Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudiolayout_h
#define _ccl_iaudiolayout_h

#include "ccl/public/text/cclstring.h"

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

//************************************************************************************************
// AudioStreamDirection
//************************************************************************************************

DEFINE_ENUM (AudioStreamDirection)
{ 
	kAudioRenderDirection,
	kAudioCaptureDirection,

	kAudioOutputDirection = kAudioRenderDirection,
	kAudioInputDirection = kAudioCaptureDirection
};

//************************************************************************************************
// AudioStreamDescription
//************************************************************************************************

struct AudioStreamDescription
{
	static constexpr int kActive = 1<<0;

	String name;
	int flags = 0;
	int channelCount = 0;
	SpeakerBitField speakerType = SpeakerDef::kEmpty;
};

//************************************************************************************************
// IAudioStreamLayout
/**	Interface describing layout of multiple audio streams. */
//************************************************************************************************

interface IAudioStreamLayout: IUnknown
{
	/**	Get number of audio streams for given direction. */
	virtual int CCL_API getStreamCount (AudioStreamDirection direction) const = 0;

	/** Get description for audio stream at given direction/index. */
	virtual tresult CCL_API getStreamDescription (AudioStreamDescription& description,
												  AudioStreamDirection direction,
												  int index) const = 0;

	DECLARE_IID (IAudioStreamLayout)

	//////////////////////////////////////////////////////////////////////////////////////////////

	bool isEmpty () const;
	int getChannelCount (AudioStreamDirection direction, int index = 0) const;
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioStreamLayout, 0xa2b733b4, 0x7d04, 0x4779, 0xaa, 0x3a, 0xa9, 0x2a, 0x14, 0x73, 0xfb, 0x64)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioStreamLayout inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IAudioStreamLayout::isEmpty () const
{
	return	getStreamCount (kAudioInputDirection) == 0 && 
			getStreamCount (kAudioOutputDirection) == 0;
}

inline int IAudioStreamLayout::getChannelCount (AudioStreamDirection direction, int index) const
{
	AudioStreamDescription description;
	getStreamDescription (description, direction, index);
	return description.channelCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iaudiolayout_h
