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
// Filename    : ccl/extras/audio/audiostreamlayout.h
// Description : Audio Stream Layout
//
//************************************************************************************************

#ifndef _audiostreamlayout_h
#define _audiostreamlayout_h

#include "ccl/public/audio/iaudiolayout.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// AudioStream
//************************************************************************************************

class AudioStream: public Object
{
public:
	DECLARE_CLASS (AudioStream, Object)

	AudioStream (StringRef name = nullptr, int flags = 0, int channelCount = 0,
				 SpeakerBitField speakerType = SpeakerDef::kEmpty)
	: name (name),
	  flags (flags),
	  channelCount (channelCount),
	  speakerType (speakerType)
	{}
		
	PROPERTY_STRING (name, Name)
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_VARIABLE (int, channelCount, ChannelCount)
	PROPERTY_VARIABLE (SpeakerBitField, speakerType, SpeakerType)

	static constexpr int kActive = AudioStreamDescription::kActive;

	PROPERTY_FLAG (flags, kActive, isActive)
};

//************************************************************************************************
// AudioStreamLayout
//************************************************************************************************

class AudioStreamLayout: public Object,
						 public IAudioStreamLayout
{
public:
	DECLARE_CLASS (AudioStreamLayout, Object)

	AudioStreamLayout ();

	static const AudioStreamLayout kEmpty;

	void add (AudioStreamDirection direction, AudioStream* stream);
	void removeAll ();

	void addChannelCount (AudioStreamDirection direction, int channelCount);
	int getTotalChannelCount (AudioStreamDirection direction) const;

	// IAudioStreamLayout
	int CCL_API getStreamCount (AudioStreamDirection direction) const override;
	tresult CCL_API getStreamDescription (AudioStreamDescription& description,
										  AudioStreamDirection direction,
										  int index) const override;

	CLASS_INTERFACE (IAudioStreamLayout, Object)

protected:
	ObjectArray renderStreams;
	ObjectArray captureStreams;
	
	ObjectArray& getStreams (AudioStreamDirection direction);
	const ObjectArray& getStreams (AudioStreamDirection direction) const;
};

//************************************************************************************************
// AudioLayoutHelper
//************************************************************************************************

struct AudioLayoutHelper
{
	const IAudioStreamLayout& layout;
	
	AudioLayoutHelper (const IAudioStreamLayout& layout)
	: layout (layout)
	{}

	bool matches (const IAudioStreamLayout& other) const
	{
		int inputCount = other.getStreamCount (kAudioInputDirection);
		int outputCount = other.getStreamCount (kAudioOutputDirection);
		if(!matchesStreamCount (inputCount, outputCount))
			return false;

		auto matchesChannelCounts = [&] (AudioStreamDirection direction, int streamCount)
		{
			for(int i = 0; i < streamCount; i++)
				if(layout.getChannelCount (direction, i) != other.getChannelCount (direction, i))
					return false;
			return true;
		};

		return	matchesChannelCounts (kAudioInputDirection, inputCount) &&
				matchesChannelCounts (kAudioOutputDirection, outputCount);
	}

	bool matchesStreamCount (int numInputs, int numOutputs) const
	{
		return	layout.getStreamCount (kAudioInputDirection) == numInputs &&
				layout.getStreamCount (kAudioOutputDirection) == numOutputs;
	}

	int getTotalChannelCount (AudioStreamDirection direction) const
	{
		int totalCount = 0;
		for(int i = 0; i < layout.getStreamCount (direction); i++)
			totalCount += layout.getChannelCount (direction, i);
		return totalCount;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioStreamLayout inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int AudioStreamLayout::getTotalChannelCount (AudioStreamDirection direction) const
{
	return AudioLayoutHelper (*this).getTotalChannelCount (direction);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _audiostreamlayout_h