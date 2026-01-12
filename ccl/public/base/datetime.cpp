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
// Filename    : ccl/public/base/datetime.cpp
// Description : Date/Time classes
//
//************************************************************************************************

#include "ccl/public/base/datetime.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IDateTime, 0x1bb2ef84, 0xf2fc, 0x4c79, 0xb2, 0x19, 0xc6, 0xe7, 0xe8, 0xa2, 0x55, 0x2f)

//************************************************************************************************
// UnixTime
//************************************************************************************************

int64 UnixTime::getTime ()
{
	return System::GetSystem ().getUnixTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DateTime UnixTime::toUTC (int64 unixTime)
{
	DateTime utc;
	System::GetSystem ().convertUnixTimeToUTC (utc, unixTime);
	return utc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 UnixTime::fromUTC (const DateTime& utc)
{
	return System::GetSystem ().convertUTCToUnixTime (utc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DateTime UnixTime::toLocal (int64 unixTime)
{
	DateTime local;
	System::GetSystem ().convertUTCToLocalTime (local, toUTC (unixTime));
	return local;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 UnixTime::fromLocal (const DateTime& local)
{
	DateTime utc;
	System::GetSystem ().convertLocalTimeToUTC (utc, local);
	return fromUTC (utc);
}
