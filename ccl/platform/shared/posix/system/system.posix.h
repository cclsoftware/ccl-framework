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
// Filename    : ccl/platform/shared/posix/system/system.posix.h
// Description : POSIX system implementation helpers
//
//************************************************************************************************

#ifndef _ccl_system_posix_h
#define _ccl_system_posix_h

#include "ccl/public/base/datetime.h"

#include <time.h>

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// PosixTimeConversion
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace PosixTimeConversion
{
	inline void convertUnixTimeToUTC (DateTime& utc, int64 unixTime)
	{
		time_t rawtime = unixTime;
		tm timeinfo = {0};
		if(::gmtime_r (&rawtime, &timeinfo))
			CRTTypeConverter::tmToDateTime (utc, timeinfo);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	inline int64 convertUTCToUnixTime (const DateTime& utc)
	{
		tm timeinfo = {0};
		CRTTypeConverter::tmFromDateTime (timeinfo, utc);
		return ::timegm (&timeinfo); // Note: timegm is not strictly POSIX, but available on all relevant systems
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	inline void getLocalTime (DateTime& dateTime)
	{
		time_t rawtime = ::time (nullptr);
		tm timeinfo = {0};
		if(::localtime_r (&rawtime, &timeinfo))
			CRTTypeConverter::tmToDateTime (dateTime, timeinfo);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	inline void convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime)
	{
		tm localTimeinfo = {0};
		CRTTypeConverter::tmFromDateTime (localTimeinfo, localTime);
		time_t rawtime = ::mktime (&localTimeinfo);
		convertUnixTimeToUTC (utc, rawtime);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	inline void convertUTCToLocalTime (DateTime& localTime, const DateTime& utc)
	{
		time_t rawtime = convertUTCToUnixTime (utc);
		tm localTimeInfo = {0};
		if(::localtime_r (&rawtime, &localTimeInfo))
			CRTTypeConverter::tmToDateTime (localTime, localTimeInfo);
	}
}

} // namespace CCL

#endif // _ccl_system_posix_h
