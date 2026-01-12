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
// Filename    : ccl/public/base/datetime.h
// Description : Date/Time classes
//
//************************************************************************************************

#ifndef _ccl_datetime_h
#define _ccl_datetime_h

#include "ccl/public/base/iunknown.h"

#include "core/public/coredatetime.h"

namespace CCL {

using Core::Time;
using Core::Date;
using Core::DateTime;

//************************************************************************************************
// IDateTime
//************************************************************************************************

interface IDateTime: IUnknown
{	
	virtual void CCL_API assign (const DateTime& dateTime) = 0;

	virtual void CCL_API copyTo (DateTime& dateTime) const = 0;

	DECLARE_IID (IDateTime)
};

//************************************************************************************************
// UnixTime
/** Helper for Unix Epoch Time (seconds since January 1 1970 00:00 UTC). */
//************************************************************************************************

namespace UnixTime
{
	int64 getTime ();
	DateTime toUTC (int64 unixTime);
	int64 fromUTC (const DateTime& utc);
	DateTime toLocal (int64 unixTime);
	int64 fromLocal (const DateTime& local);
}

} // namespace CCL

#endif // _ccl_datetime_h
