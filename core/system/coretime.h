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
// Filename    : core/system/coretime.h
// Description : Timing Functions
//
//************************************************************************************************

#ifndef _coretime_h
#define _coretime_h

#include "core/platform/corefeatures.h"

#if CORE_TIME_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coretime)
#elif CORE_TIME_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coretime)
#elif CORE_TIME_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/coretime.posix.h"
#elif CORE_TIME_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_CMSIS
	#include "core/platform/shared/cmsis/coretime.cmsis.h"
#endif

#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// SystemClock
/** Timing Functions
	\ingroup core_time */
//************************************************************************************************

namespace SystemClock
{
	/**
	* \brief Get the current time
	* \ingroup core_time
	* \details If possible, platform implementations use a high performance clock.
	* \attention The time representation used with \a abs_time is platform-specific.
	* \return The current time
	*/
	abs_time getTime ();

	/**
	* \brief Get the frequency of the system clock
	* \ingroup core_time
	* \return The frequency of the system clock in Hz
	*/
	uint64 getFrequency ();
	
	/**
	* \brief Get a factor to convert the platform-specific time representation to seconds
	* \ingroup core_time
	* \return A factor to convert an \a abs_time time value to seconds
	*/
	double getTimeToSecondsFactor ();

	/**
	* \brief Convert an \a abs_time time value to seconds
	* \ingroup core_time
	* \param t Platform-specific time value retrieved with \a getTime
	* \return Time in seconds
	*/
	double toSeconds (abs_time t);

	/**
	* \brief Convert an \a abs_time time value to milliseconds
	* \ingroup core_time
	* \param t Platform-specific time value retrieved with \a getTime
	* \return Time in milliseconds
	*/
	abs_time toMilliseconds (abs_time t);

	/**
	* \brief Convert an \a abs_time time value to microseconds
	* \ingroup core_time
	* \param t Platform-specific time value retrieved with \a getTime
	* \return Time in microseconds
	*/
	abs_time toMicroseconds (abs_time t);

	/**
	* \brief Get the current time in seconds
	* \ingroup core_time
	* \details If possible, platform implementations use a high performance clock.
	* \return The current time in seconds
	*/
	double getSeconds ();

	/**
	* \brief Get the current time in milliseconds
	* \ingroup core_time
	* \details If possible, platform implementations use a high performance clock.
	* \return The current time in milliseconds
	*/
	abs_time getMilliseconds ();

	/**
	* \brief Get the current time in microseconds
	* \ingroup core_time
	* \details If possible, platform implementations use a high performance clock.
	* \return The current time in microseconds
	*/
	abs_time getMicroseconds ();
}

//************************************************************************************************
// HighPerformanceClock
/** High Performance Timing Functions (microsecond precision or better)
	\ingroup core_time */
//************************************************************************************************

namespace HighPerformanceClock
{
	/**
	* \brief Get the current tick count
	* \return The current tick count
	*/
	abs_time getCount ();
	
	/**
	* \brief Get the tick frequency
	* \return The tick frequency
	*/
	uint64 getFrequency ();
}

//************************************************************************************************
// SystemClock implementation
//************************************************************************************************

inline abs_time SystemClock::getTime () 
{ return Platform::SystemClock::getTime (); }

inline uint64 SystemClock::getFrequency () 
{ return Platform::SystemClock::getFrequency (); }

inline double SystemClock::getTimeToSecondsFactor () 
{ return Platform::SystemClock::getTimeToSecondsFactor (); }

inline double SystemClock::toSeconds (abs_time t) 
{ return Platform::SystemClock::toSeconds (t); }

inline abs_time SystemClock::toMilliseconds (abs_time t)
{ return Platform::SystemClock::toMilliseconds (t); }

inline abs_time SystemClock::toMicroseconds (abs_time t)
{ return Platform::SystemClock::toMicroseconds (t); }

inline double SystemClock::getSeconds () 
{ return toSeconds (getTime ()); }

inline abs_time SystemClock::getMilliseconds () 
{ return toMilliseconds (getTime ()); }

inline abs_time SystemClock::getMicroseconds () 
{ return toMicroseconds (getTime ()); }

//************************************************************************************************
// HighPerformanceClock implementation
//************************************************************************************************

inline abs_time HighPerformanceClock::getCount () 
{ return Platform::HighPerformanceClock::getCount (); }

inline uint64 HighPerformanceClock::getFrequency () 
{ return Platform::HighPerformanceClock::getFrequency (); }

} // namespace Core

#endif // _coretime_h
