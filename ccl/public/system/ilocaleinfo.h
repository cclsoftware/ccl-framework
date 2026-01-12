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
// Filename    : ccl/public/system/ilocaleinfo.h
// Description : Locale Interfaces
//
//************************************************************************************************

#ifndef _ccl_ilocaleinfo_h
#define _ccl_ilocaleinfo_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/datetime.h"

namespace CCL {

//************************************************************************************************
// ILocaleInfo
/**	\ingroup ccl_system */
//************************************************************************************************

interface ILocaleInfo: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get name of locale. */
	virtual StringRef CCL_API getTitle () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Calendar
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Determine day of week for given date (0 = Sunday, 1 = Monday, etc.). */
	virtual int CCL_API getDayOfWeek (const Date& date) const = 0;

	/** Get name of weekday (0 = Sunday). */
	virtual StringRef CCL_API getWeekdayName (int dayOfWeek) const = 0;

	/** Get name of month (1 = January). */
	virtual StringRef CCL_API getMonthName (int month) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Formatting
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Format options. */
	enum PrintDateFlags
	{
		kFriendlyDate = 1<<0	///< day and month name
	};

	virtual tresult CCL_API printDate (String& result, const Date& date, int flags = 0) const = 0;

	virtual tresult CCL_API printTime (String& result, const Time& time, int flags = 0) const = 0;

	virtual tresult CCL_API printTimeAgo (String& result, const DateTime& localTime, int flags = 0) const = 0;

	virtual tresult CCL_API printTimeAhead (String& result, const DateTime& localTime, int flags = 0) const = 0;

	enum PrintDurationFlags
	{
		kCompactDuration = 1<<0	///< do not print more than 2 segments
	};

	virtual tresult CCL_API printDuration (String& result, double seconds, int flags = 0) const = 0;

	virtual tresult CCL_API printCurrency (String& result, double value, int flags = 0) const = 0;

	enum PrintByteFlags ///< used with printByteSize() and printBytesPerSecond()
	{
		kSIByteUnit = 1<<0	///< use SI byte units based on power of 10
	};

	virtual tresult CCL_API printByteSize (String& result, double byteSize, int flags = 0) const = 0;

	enum TimeUnit
	{
		kDays, kHours, kMinutes, kSeconds
	};

	enum ScanFlags
	{
		kDetectUnit	 = 1<<0,	///< try to scan unit from string, overrides given default unit
		kRequireUnit = 1<<1		///< scan fails if no unit scanned
	};

	virtual tresult CCL_API scanDuration (double& seconds, StringRef string, int defaultUnit = kSeconds, int flags = 0) const = 0;

	virtual tresult CCL_API printBytesPerSecond (String& result, double bytesPerSecond, int flags = 0) const = 0;

	DECLARE_IID (ILocaleInfo)
};

DEFINE_IID (ILocaleInfo, 0x6aed125b, 0xcca0, 0x41a4, 0x8f, 0x55, 0x20, 0x2e, 0x13, 0x28, 0x3f, 0x49)

//************************************************************************************************
// IGeographicRegion
/**	\ingroup ccl_system */
//************************************************************************************************

interface IGeographicRegion: IUnknown
{
	/** Get name of region in English (e.g. "Germany"). */
	virtual StringRef CCL_API getEnglishName () const = 0;

	/** Get name of region in native language (e.g. "Deutschland"). */
	virtual StringRef CCL_API getNativeName () const = 0;

	/** Get name of region in system UI language. */
	virtual StringRef CCL_API getLocalizedName () const = 0;

	/** Get ISO 3166-1 alpha-2 region code (e.g. "DE"). */
	virtual StringRef CCL_API getISO2Code () const = 0;

	DECLARE_IID (IGeographicRegion)
};

DEFINE_IID (IGeographicRegion, 0x9c61dc26, 0x2b35, 0x4970, 0x95, 0xe5, 0xa8, 0x2d, 0xe6, 0x37, 0x86, 0x96)

} // namespace CCL

#endif // _ccl_ilocaleinfo_h
