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
// Filename    : audiopanning.cpp
// Description : Audio Pan Matrix
//
//************************************************************************************************

#include "audiopanning.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// AudioPanMatrix
//************************************************************************************************

AudioPanMatrix::ChannelRole AudioPanMatrix::classify (int channelIndex, int channelCount, SpeakerBitField speakerType)
{
	// A single channel carries no spatial position of its own
	if(channelCount == 1)
		return ChannelRole::kCenter;

	int index = 0;
	for(int bit = 0; bit < SpeakerDef::kNumSpeakerBits; bit++)
	{
		if(!get_bit<SpeakerBitField> (speakerType, bit))
			continue;

		if(index == channelIndex)
		{
			switch(SpeakerID(1) << bit)
			{
			case SpeakerDef::kFrontLeft :
			case SpeakerDef::kBackLeft :
			case SpeakerDef::kFrontLeftOfCenter :
			case SpeakerDef::kSideLeft :
			case SpeakerDef::kTopFrontLeft :
			case SpeakerDef::kTopBackLeft :
			case SpeakerDef::kTopMiddleLeft :
			case SpeakerDef::kWideLeft :
				return ChannelRole::kLeft;

			case SpeakerDef::kFrontRight :
			case SpeakerDef::kBackRight :
			case SpeakerDef::kFrontRightOfCenter :
			case SpeakerDef::kSideRight :
			case SpeakerDef::kTopFrontRight :
			case SpeakerDef::kTopBackRight :
			case SpeakerDef::kTopMiddleRight :
			case SpeakerDef::kWideRight :
				return ChannelRole::kRight;

			case SpeakerDef::kLowFrequency :
				return ChannelRole::kLfe;

			default: // kFrontCenter, kBackCenter, kTopCenter, kTopFrontCenter, kTopBackCenter, etc.
				return ChannelRole::kCenter;
			}
		}

		index++;
	}

	return ChannelRole::kCenter; // fall back to center, same as mono.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AudioPanMatrix::getCoefficient (ChannelRole outputRole, ChannelRole inputRole, float leftGain, float rightGain)
{
	switch(outputRole)
	{
	case ChannelRole::kLeft :
		switch(inputRole)
		{
		case ChannelRole::kLeft : return leftGain;
		case ChannelRole::kCenter : return leftGain;
		default : return 0.f; // Right, Lfe never feed a Left output
		}

	case ChannelRole::kRight :
		switch(inputRole)
		{
		case ChannelRole::kRight : return rightGain;
		case ChannelRole::kCenter : return rightGain;
		default : return 0.f; // Left, Lfe never feed a Right output
		}

	case ChannelRole::kCenter :
		switch(inputRole)
		{
		case ChannelRole::kLeft : return leftGain;
		case ChannelRole::kRight : return rightGain;
		case ChannelRole::kCenter : return 1.f; // unaffected by pan
		default : return 0.f; // Lfe never feeds a Center output
		}

	default : // ChannelRole::kLfe
		return inputRole == ChannelRole::kLfe ? 1.f : 0.f; // bass is never panned
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPanMatrix::build (int numInputChannels, SpeakerBitField inputSpeakers,
							int numOutputChannels, SpeakerBitField outputSpeakers)
{
	entries.removeAll ();
	outputChannelUsed.removeAll ();
	anyFanOut = false;

	Vector<bool> inputChannelUsed;
	for(int ch = 0; ch < numInputChannels; ch++)
		inputChannelUsed.add (false);

	for(int outputChannel = 0; outputChannel < numOutputChannels; outputChannel++)
	{
		ChannelRole outputRole = classify (outputChannel, numOutputChannels, outputSpeakers);
		bool isFirstForOutput = true;

		for(int inputChannel = 0; inputChannel < numInputChannels; inputChannel++)
		{
			ChannelRole inputRole = classify (inputChannel, numInputChannels, inputSpeakers);

			if(getCoefficient (outputRole, inputRole, 1.f, 1.f) == 0.f)
				continue;

			Entry entry;
			entry.outputChannel = outputChannel;
			entry.inputChannel = inputChannel;
			entry.outputRole = outputRole;
			entry.inputRole = inputRole;
			entry.first = isFirstForOutput;
			isFirstForOutput = false;
			entries.add (entry);

			if(inputChannelUsed[inputChannel])
				anyFanOut = true;
			inputChannelUsed[inputChannel] = true;
		}

		outputChannelUsed.add (!isFirstForOutput); // used if at least one entry was added above
	}

	setPan (pan); // recompute gains for the current pan
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPanMatrix::setPan (float newPan)
{
	pan = newPan;

	// Standard equal-power pan law: unity at each extreme, -3dB (0.707) blend at center.
	float t = (pan + 1.f) * 0.5f; // -1..1 -> 0..1
	float leftGain = Math::Functions<float>::cos (t * Math::Constants<float>::kHalfPi);
	float rightGain = Math::Functions<float>::sin (t * Math::Constants<float>::kHalfPi);

	for(int i = 0; i < entries.count (); i++)
	{
		Entry& entry = entries[i];
		entry.gain = getCoefficient (entry.outputRole, entry.inputRole, leftGain, rightGain);
	}
}
