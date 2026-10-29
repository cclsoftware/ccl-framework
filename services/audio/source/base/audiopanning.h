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
// Filename    : audiopanning.h
// Description : Audio Pan Matrix
//
//************************************************************************************************

#ifndef _audiopanning_h
#define _audiopanning_h

#include "ccl/public/audio/cclaudiotypes.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// AudioPanMatrix
/**	Computes a sparse gain matrix panning an N-channel input stream to an M-channel output stream
	from a single scalar pan value (-1..1), based on each channel's Left/Right/Center/Lfe speaker
	role. A single-channel stream is always Center regardless of its (placeholder) speaker bit.

	The pan law is the standard equal-power curve: with t = (pan+1)/2 in 0..1,
	leftGain = cos (t * PI/2), rightGain = sin (t * PI/2) - unity at each extreme, -3dB blend at
	center. Each (output role, input role) pair maps to a fixed coefficient:

		out \ in    Left       Right      Center     Lfe
		Left        leftGain   0          leftGain   0
		Right       0          rightGain  rightGain  0
		Center      leftGain   rightGain  1          0
		Lfe         0          0          0          1

	Center-role channels are "movable" - a mono/Center source pans between Left and Right outputs,
	and a Center output sums whatever Left/Right content feeds it, weighted the same way. Left/Right
	channels never cross-feed the opposite side (no spatial blend, just a balance-style level
	change) except by folding into a Center output. Lfe never mixes with anything but Lfe - bass is
	never panned. Role pairs with a structurally zero coefficient (independent of the actual pan
	value) never get an Entry built for them at all. */
//************************************************************************************************

class AudioPanMatrix
{
public:
	enum class ChannelRole 
	{
		kLeft,
		kRight,
		kCenter,
		kLfe 
	};

	struct Entry
	{
		int outputChannel = 0;
		int inputChannel = 0;
		ChannelRole outputRole = ChannelRole::kCenter;
		ChannelRole inputRole = ChannelRole::kCenter;

		float gain = 0.f; ///< calculated gain coefficient
		bool first = false;

		bool isMixNeeded () const { return first == false; }
	};

	/**	(Re)build the sparse entry list for the given input/output channel counts and speaker
		bitfields. Entries whose role pair is structurally always 0 (e.g. Left-in/Right-out) are
		never built. Recomputes every entry's gain for the current pan afterward, so the matrix
		is always self-consistent. */
	void build (int numInputChannels, SpeakerBitField inputSpeakers,
				int numOutputChannels, SpeakerBitField outputSpeakers);

	/** Recompute every existing entry's gain for the given pan position (-1..1). */
	void setPan (float pan);

	int getCount () const { return entries.count (); }
	const Entry& getEntry (int index) const { return entries[index]; }

	/** Check whether any entry writes to the given output channel at all - false means the
		caller must silence that channel itself. */
	bool isOutputUsed (int outputChannel) const { return outputChannelUsed[outputChannel]; }

	/** Check whether any input channel feeds more than one entry (fan-out). */
	bool hasFanOut () const { return anyFanOut; }

protected:
	static ChannelRole classify (int channelIndex, int channelCount, SpeakerBitField speakerType);
	static float getCoefficient (ChannelRole outputRole, ChannelRole inputRole, float leftGain, float rightGain);

	Vector<Entry> entries;
	Vector<bool> outputChannelUsed;
	bool anyFanOut = false;
	float pan = 0.f;
};

} // namespace CCL

#endif // _audiopanning_h
