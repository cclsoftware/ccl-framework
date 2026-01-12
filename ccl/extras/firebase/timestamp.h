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
// Filename    : ccl/extras/firebase/timestamp.h
// Description : Firestore Timestamp
//
//************************************************************************************************

#ifndef _ccl_firestore_timestamp_h
#define _ccl_firestore_timestamp_h

#include "ccl/public/base/datetime.h"
#include "ccl/public/text/cstring.h"

#include <math.h>

namespace CCL {
namespace Firebase {
namespace Firestore {

//************************************************************************************************
// Firebase::Firestore::Timestamp
//************************************************************************************************

struct Timestamp
{
	int64 seconds = 0;	///< seconds in Unix epoch time
	int32 nanos = 0;	///< nanoseconds [0 to 999,999,999]

	/** Convert to combined double-precision value in seconds.
		Note that nanosecond precision is lost during conversion! */
	double toFractionalSeconds () const;
	Timestamp& fromFractionalSeconds (double fractionalSeconds);
	
	void toDateTime (DateTime& dateTime) const;
	Timestamp& fromDateTime (const DateTime& dateTime);

	bool operator == (const Timestamp& t) const;
};

//************************************************************************************************
// Firebase::Firestore::TimestampFormat
//************************************************************************************************

namespace TimestampFormat
{
	// A timestamp in RFC3339 UTC "Zulu" format, with nanosecond resolution and up to nine fractional digits
	// Examples: "2014-10-02T15:01:23Z" and "2014-10-02T15:01:23.045123456Z".
	// See https://developers.google.com/protocol-buffers/docs/reference/google.protobuf#google.protobuf.Timestamp
	
	const CStringPtr kFormat = "%04d-%02d-%02dT%02d:%02d:%lfZ";

	Timestamp scan (CStringRef string);
	MutableCString print (Timestamp t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Timestamp inline
//////////////////////////////////////////////////////////////////////////////////////////////////

#define TIMESTAMP_NANOS_SCALER 1e9f

inline double Timestamp::toFractionalSeconds () const
{
	return static_cast<double> (seconds) + static_cast<double> (nanos) / TIMESTAMP_NANOS_SCALER;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Timestamp& Timestamp::fromFractionalSeconds (double fractionalSeconds)
{
	double integerSeconds = ::floor (fractionalSeconds);
	seconds = static_cast<int64> (integerSeconds);
	nanos = static_cast<int32> ((fractionalSeconds - integerSeconds) * TIMESTAMP_NANOS_SCALER);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void Timestamp::toDateTime (DateTime& dateTime) const
{
	dateTime = UnixTime::toUTC (seconds);
	Time time = dateTime.getTime ();
	time.setMilliseconds (nanos / 1000000);
	dateTime.setTime (time);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Timestamp& Timestamp::fromDateTime (const DateTime& dateTime)
{
	seconds = UnixTime::fromUTC (dateTime);
	nanos = dateTime.getTime ().getMilliseconds () * 1000000;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Timestamp::operator == (const Timestamp& t) const
{
	return t.seconds == seconds && t.nanos == nanos; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// TimestampFormat inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Timestamp TimestampFormat::scan (CStringRef string)
{
	Timestamp t = {0};
	int year = 0, month = 0, day = 0, hour = 0, minute = 0;
	double fractionalSeconds = 0.;
	::sscanf (string, kFormat, &year, &month, &day, &hour, &minute, &fractionalSeconds);
	double integerSeconds = ::floor (fractionalSeconds);
	DateTime utc (Date (year, month, day), Time (hour, minute, static_cast<int> (integerSeconds)));
	t.seconds = UnixTime::fromUTC (utc);
	t.nanos = static_cast<int32> ((fractionalSeconds - integerSeconds) * TIMESTAMP_NANOS_SCALER);
	return t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline MutableCString TimestampFormat::print (Timestamp t)
{
	DateTime utc = UnixTime::toUTC (t.seconds);
	double fractionalSeconds = utc.getTime ().getSecond () + static_cast<double> (t.nanos) / TIMESTAMP_NANOS_SCALER;

	MutableCString string;
	string.appendFormat (kFormat, utc.getDate ().getYear (), utc.getDate ().getMonth (), utc.getDate ().getDay (), 
						 utc.getTime ().getHour (), utc.getTime ().getMinute (), fractionalSeconds);
	return string;
}

#undef TIMESTAMP_NANOS_SCALER

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Firestore
} // namespace Firebase
} // namespace CCL

#endif // _ccl_firestore_timestamp_h
