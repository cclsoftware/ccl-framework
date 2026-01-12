//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : core/public/corethreading.h
// Description : Thread definitions
//
//************************************************************************************************

#ifndef _corethreading_h
#define _corethreading_h

#include "core/public/coretypes.h"

namespace Core {
namespace Threads {
namespace Threading {

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Thread timeout. \ingroup core */
enum ThreadTimeout
{
	kWaitForever = 0xFFFFFFFF ///< infinite timeout
};

/** Thread priority. \ingroup core */
DEFINE_ENUM (ThreadPriority)
{
	kPriorityLow,
	kPriorityBelowNormal,
	kPriorityNormal,
	kPriorityAboveNormal,
	kPriorityHigh,
	kPriorityTimeCritical,

	// realtime priorities
	kPriorityRealtimeBase,
	kPriorityRealtimeMiddle,
	kPriorityRealtimeTop,
	
	kPriorityRealtime = kPriorityRealtimeBase 
};

/** Thread errors. \ingroup core */
DEFINE_ENUM (ThreadErrors)
{
	kErrorThreadNotStarted = 1 << 0,
	kErrorThreadPriority = 1 << 1,
	kErrorThreadCPUAffinity = 1 << 2
};

/** Thread identifier. \ingroup core */
typedef int64 ThreadID;

/** Process identifier. \ingroup core */
typedef int64 ProcessID;

/** Thread Local Storage (TLS) reference. */
typedef uint32 TLSRef;

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Threading

using namespace Threading;

} // namespace Threads
} // namespace Core

#endif // _corethreading_h
