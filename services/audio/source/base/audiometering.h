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
// Filename    : audiometering.h
// Description : Audio Level Detection
//
//************************************************************************************************

#ifndef _audiometering_h
#define _audiometering_h

#include "ccl/base/object.h"

#include "ccl/public/audio/iaudiometer.h"
#include "ccl/public/system/threadsync.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// AudioLevelDetector
/**	Computes peak/RMS/clip levels per processed block on the realtime thread, and exposes
	them through a delay ring read from a non-realtime thread.

	The delay compensates for the display latency, which is measured in frames: every slot
	records the frame range it covers, and a read returns the slot containing the position
	that is currently audible (newest processed frame minus latency). This is independent of
	how the audio is split into blocks or chunks for process ().
	Single producer (process, realtime thread) / single consumer (readMeterData,
	any other thread) - no locking. */
//************************************************************************************************

class AudioLevelDetector: public Object,
						  public IAudioMeterSource
{
public:
	DECLARE_CLASS (AudioLevelDetector, Object)

	/**	(Re)configure for the given channel count and display latency (in frames). */
	void resize (int channelCount, SampleRate sampleRate, int latencyFrames);

	void setProcessing (bool state) { processing = state; }

	/** Scan one block for peak/RMS/clip and publish it into the delay ring. */
	void process (const AudioData& data);

	// IAudioMeterSource
	int CCL_API getChannelCount () const override;
	tresult CCL_API readMeterData (AudioMeterData& meterData) override;

	CLASS_INTERFACE (IAudioMeterSource, Object)

protected:
	static constexpr int kRingSafetyMargin = 4;

	/** Ring size assumes that blocks are not shorter than this (in frames); with shorter blocks
		the delay is limited to what the ring holds. */
	static constexpr int kMinSlotFrames = 16;

	struct Slot
	{
		AudioMeterDataMax data;
		int64 startFrame = 0; ///< first frame covered by this slot
		int64 endFrame = 0; ///< first frame after this slot
	};

	Vector<Slot> ring;
	int64 framesProcessed = 0; ///< realtime thread only
	int latencyFrames = -1; ///< < 0: not configured
	int channelCount = 0;
	SampleRate sampleRate = 0;
	Threading::AtomicInt writeIndex;
	bool processing = false;
};

} // namespace CCL

#endif // _audiometering_h
