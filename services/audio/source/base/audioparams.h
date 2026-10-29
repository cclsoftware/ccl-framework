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
// Filename    : audioparams.h
// Description : Audio Parameters
//
//************************************************************************************************

#ifndef _audioparams_h
#define _audioparams_h

#include "ccl/base/object.h"

#include "ccl/public/collections/map.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/audio/iaudioprocessor.h"

namespace CCL {

//************************************************************************************************
// AudioParamState
//************************************************************************************************

typedef Map<int, AudioParamValue> AudioParamState;

//************************************************************************************************
// AudioParamQueue
/** Queue for parameter changes from control thread to audio thread. */
//************************************************************************************************

class AudioParamQueue
{
public:
	static constexpr int kMaxPendingEvents = 64;

	typedef FixedSizeVector<AudioParamEvent, kMaxPendingEvents> EventList;

	/**	Control thread: update pending entry for tag, or append a new one. */
	bool push (const AudioParamEvent& event);

	/**	Audio thread: move all pending events to given list, replacing its content.
		Returns false without touching anything if the control thread currently holds the lock. */
	bool popAll (EventList& events);

	/** Remove all pending events. */
	void removeAll ();

private:
	Threading::SpinLock lock;
	EventList content;
};

} // namespace CCL

#endif // _audioparams_h
