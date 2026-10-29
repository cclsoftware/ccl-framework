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
// Filename    : ccl/extras/audio/audiostreamlayout.cpp
// Description : Audio Stream Layout
//
//************************************************************************************************

#include "ccl/extras/audio/audiostreamlayout.h"

using namespace CCL;

//************************************************************************************************
// AudioStream
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioStream, Object)

//************************************************************************************************
// AudioStreamLayout
//************************************************************************************************

const AudioStreamLayout AudioStreamLayout::kEmpty;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AudioStreamLayout, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioStreamLayout::AudioStreamLayout ()
{
	renderStreams.objectCleanup ();
	captureStreams.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& AudioStreamLayout::getStreams (AudioStreamDirection direction)
{
	return direction == kAudioRenderDirection ? renderStreams : captureStreams;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& AudioStreamLayout::getStreams (AudioStreamDirection direction) const
{
	return direction == kAudioRenderDirection ? renderStreams : captureStreams;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioStreamLayout::add (AudioStreamDirection direction, AudioStream* stream)
{
	ObjectArray& streams = getStreams (direction);
	streams.add (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioStreamLayout::removeAll ()
{
	renderStreams.removeAll ();
	captureStreams.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioStreamLayout::getStreamCount (AudioStreamDirection direction) const
{
	return getStreams (direction).count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioStreamLayout::getStreamDescription (AudioStreamDescription& description, 
														 AudioStreamDirection direction, 
														 int index) const
{
	const ObjectArray& streams = getStreams (direction);
	
	ASSERT (streams.isValidIndex (index))
	if(!streams.isValidIndex (index))
		return kResultInvalidArgument;

	auto* stream = static_cast<AudioStream*> (streams.at (index));
	description.name = stream->getName ();
	description.flags = stream->isActive () ? AudioStreamDescription::kActive : 0;
	description.channelCount = stream->getChannelCount ();
	description.speakerType = stream->getSpeakerType ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioStreamLayout::addChannelCount (AudioStreamDirection direction, int channelCount)
{
	add (direction, NEW AudioStream (String::kEmpty, 0, channelCount));
}