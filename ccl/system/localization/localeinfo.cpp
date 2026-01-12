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
// Filename    : ccl/system/localization/localeinfo.cpp
// Description : Locale Info
//
//************************************************************************************************

#include "ccl/system/localization/localeinfo.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include <time.h>

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Locales")
	XSTRING (Days, "days")
	XSTRING (Hours, "hours")
	XSTRING (Minutes, "min")
	XSTRING (Seconds, "sec")

	XSTRING (LastYear, "Last year")
	XSTRING (YearsAgo, "%(1) years ago")
	XSTRING (LastMonth, "Last month")
	XSTRING (MonthsAgo, "%(1) months ago")
	XSTRING (LastWeek, "Last week")
	XSTRING (WeeksAgo, "%(1) weeks ago")
	XSTRING (Today, "Today")
	XSTRING (Yesterday, "Yesterday")
	XSTRING (DaysAgo, "%(1) days ago")

	XSTRING (InXDays, "In %(1) days")
	XSTRING (InOneDay, "In one day")
	XSTRING (InXHours, "In %(1) hours")
	XSTRING (InOneHour, "In one hour")
	XSTRING (InLessThanOneHour, "In less than one hour")

	XSTRING (Bytes, "bytes")
	XSTRING (KB, "KB")
	XSTRING (MB, "MB")
	XSTRING (GB, "GB")
	XSTRING (TB, "TB")

	XSTRING (MBPerSecond, "MB/sec")
	XSTRING (KBPerSecond, "KB/sec")
END_XSTRINGS

//************************************************************************************************
// LocaleInfoList
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (LocaleInfoList, Object, "Locales")
DEFINE_CLASS_NAMESPACE (LocaleInfoList, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleInfoList::LocaleInfoList ()
{
	locales.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectList& LocaleInfoList::getLocales () const
{
	return locales;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleInfoList::load (const Storage& storage)
{
	storage.getAttributes ().unqueue (locales, nullptr, ccl_typeid<LocaleInfo> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleInfoList::save (const Storage& storage) const
{
	storage.getAttributes ().queue (nullptr, locales);
	return true;
}

//************************************************************************************************
// LocaleInfo
//************************************************************************************************

const String LocaleInfo::weekdayID[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const String LocaleInfo::monthID[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_PERSISTENT (LocaleInfo, Object, "LocaleInfo")
DEFINE_CLASS_NAMESPACE (LocaleInfo, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleInfo::isValid () const
{
	return !language.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API LocaleInfo::getTitle () const
{
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API LocaleInfo::getWeekdayName (int dayOfWeek) const
{
	ASSERT (dayOfWeek >= 0 && dayOfWeek < 7)
	return weekdays.lookupValue (weekdayID[ccl_bound (dayOfWeek, 0, 6)]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API LocaleInfo::getMonthName (int month) const
{
	ASSERT (month >= 1 && month <= 12)
	return months.lookupValue (monthID[ccl_bound (month-1, 0, 11)]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfo::printDate (String& result, const Date& date, int flags) const
{
	result.empty ();

	Variant args[5];
	args[0] = date.getYear ();
	args[1] = date.getMonth ();
	args[2] = date.getDay ();
	int argCount = 3;

	bool friendly = (flags & kFriendlyDate) != 0;
	if(friendly)
	{
		args[3] = getMonthName (date.getMonth ());
		int dayOfWeek = getDayOfWeek (date);
		args[4] = getWeekdayName (dayOfWeek);
		argCount = 5;
	}

	String format (friendly ? friendlyDateFormat : dateFormat);
	result.appendFormat (format, args, argCount);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfo::printTime (String& result, const Time& time, int flags) const
{
	result.empty ();

	Variant args[3];
	args[0] = time.getHour ();
	args[1] = time.getMinute ();
	args[2] = time.getSecond ();

	result.appendFormat (timeFormat, args, 3);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleInfo::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	language = a.getCString ("language");
	if(language.isEmpty ())
		return false;

	title = a.getString ("title");

	Boxed::String dateFormat;
	a.get (static_cast<Object&> (dateFormat), "dateFormat");
	if(dateFormat.isEmpty ())
		return false;
	setDateFormat (dateFormat);

	Boxed::String friendlyDateFormat;
	a.get (static_cast<Object&> (friendlyDateFormat), "friendlyDateFormat");
	if(friendlyDateFormat.isEmpty ())
		return false;
	setFriendlyDateFormat (friendlyDateFormat);

	Boxed::String timeFormat;
	a.get (static_cast<Object&> (timeFormat), "timeFormat");
	if(timeFormat.isEmpty ())
		return false;
	setTimeFormat (timeFormat);

	if(!a.get (weekdays, "weekdays"))
		return false;
	if(!a.get (months, "months"))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleInfo::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	a.set ("language", language);
	a.set ("title", title);

	a.set ("dateFormat", Boxed::String (dateFormat), true);
	a.set ("friendlyDateFormat", Boxed::String (friendlyDateFormat), true);
	a.set ("timeFormat", Boxed::String (timeFormat), true);

	a.set ("weekdays", weekdays);
	a.set ("months", months);
	return true;
}

//************************************************************************************************
// LocaleInfoBase
//************************************************************************************************

struct DurationUnit
{
	ILocaleInfo::TimeUnit unit;
	const LocalString& unitString;
	const char* format;
	int maxSegments;
	int seconds;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static DurationUnit durationsUnits[] =
{
	{ ILocaleInfo::kDays,	 XSTR_REF (Days),    "%(1):%(2):%(3):%float(4)", 4, 24 * 3600 },
	{ ILocaleInfo::kHours,   XSTR_REF (Hours),	  "%(1):%(2):%float(3)", 3, 3600 },
	{ ILocaleInfo::kMinutes, XSTR_REF (Minutes), "%(1):%float(2)", 2,	60 },
	{ ILocaleInfo::kSeconds, XSTR_REF (Seconds), "%float(1)", 1, 1 }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (LocaleInfoBase, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API LocaleInfoBase::getTitle () const
{
	static const String builtInTitle (CCLSTR ("English"));
	return builtInTitle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LocaleInfoBase::getDayOfWeek (const Date& date) const
{
	::tm cTime = {0};
	cTime.tm_wday = -1;
	cTime.tm_mday = date.getDay ();
	cTime.tm_mon = date.getMonth () - 1;
	cTime.tm_year = date.getYear () - 1900;
	::mktime (&cTime);
	ASSERT (cTime.tm_wday >= 0 && cTime.tm_wday < 7)
	return cTime.tm_wday;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API LocaleInfoBase::getWeekdayName (int dayOfWeek) const
{
	CCL_NOT_IMPL ("LocaleInfoBase::getWeekdayName() not implemented!")
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API LocaleInfoBase::getMonthName (int month) const
{
	CCL_NOT_IMPL ("LocaleInfoBase::getMonthName() not implemented!")
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printDate (String& result, const Date& date, int flags) const
{
	MutableCString temp;
	temp.appendFormat ("%d.%d.%d", date.getDay (), date.getMonth (), date.getYear ());
	result.empty ();
	result.appendASCII (temp);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printTime (String& result, const Time& time, int flags) const
{
	MutableCString temp;
	temp.appendFormat ("%02d:%02d:%02d", time.getHour (), time.getMinute (), time.getSecond ());
	result.empty ();
	result.appendASCII (temp);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printTimeAgo (String& result, const DateTime& localTime, int flags) const
{
	result.empty ();
	const DateTime& past = localTime;

	DateTime now;
	System::GetSystem ().getLocalTime (now);

	int64 presentDays = now.getDate ().toOrdinal ();
	int64 pastDays = past.getDate ().toOrdinal ();

	int64 daysAgo = presentDays - pastDays;
	if(daysAgo < 0) // back to the future???
		return kResultInvalidArgument;

	if(daysAgo >= 365)
	{
		int64 yearsAgo = daysAgo / 365;
		//if(daysAgo % 365)
		//	yearsAgo++;

		if(yearsAgo == 1)
			result = XSTR (LastYear);
		else
			result.appendFormat (XSTR (YearsAgo), yearsAgo);
	}
	else if(daysAgo >= 31)
	{
		int64 monthsAgo = daysAgo / 31;
		if(daysAgo % 31)
			monthsAgo++;

		if(monthsAgo == 1)
			result = XSTR (LastMonth);
		else
			result.appendFormat (XSTR (MonthsAgo), monthsAgo);
	}
	else if(daysAgo >= 7)
	{
		int64 weeksAgo = daysAgo / 7;
		if(daysAgo % 7)
			weeksAgo++;

		if(weeksAgo == 1)
			result = XSTR (LastWeek);
		else
			result.appendFormat (XSTR (WeeksAgo), weeksAgo);
	}
	else switch(daysAgo)
	{
	case 0  : result = XSTR (Today); break;
	case 1  : result = XSTR (Yesterday); break;
	default : result.appendFormat (XSTR (DaysAgo), daysAgo);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printTimeAhead (String& result, const DateTime& localTime, int flags) const
{
	result.empty ();

	DateTime utc;
	System::GetSystem ().convertLocalTimeToUTC (utc, localTime);
	int64 future = UnixTime::fromUTC (utc);
	int64 now = UnixTime::getTime ();

	int64 secondsAhead = future - now;

	if(secondsAhead >= DateTime::kSecondsInDay)
	{
		int64 daysAhead = secondsAhead / DateTime::kSecondsInDay;
		if(daysAhead == 1)
			result = XSTR (InOneDay);
		else
			result.appendFormat (XSTR (InXDays), daysAhead);
	}
	else if(secondsAhead >= Time::kSecondsPerHour)
	{
		int64 hoursAhead = secondsAhead / Time::kSecondsPerHour;
		if(hoursAhead == 1)
			result = XSTR (InOneHour);
		else
			result.appendFormat (XSTR (InXHours), hoursAhead);
	}
	else
		result = XSTR (InLessThanOneHour);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printDuration (String& result, double seconds, int flags) const
{
	int64 t = (int64)ccl_round<0> (seconds * 1000);
	int sign = 1;
	if(t < 0)
	{
		sign = -1;
		t = -1;
	}
	int ms = int (t % 1000); t /= 1000;
	int s  = int (t % 60); t /= 60;
	int m  = int (t % 60); t /= 60;
	int h  = int (t % 24);
	int d  = int (t / 24);

	TimeUnit unit = kSeconds;
	MutableCString temp;
	if(sign < 0)
		temp.append ("-");

	bool compact = (flags & kCompactDuration) != 0;

	if(d)
	{
		if(compact)
			temp.appendFormat ("%d:%.2d", d, h);
		else
			temp.appendFormat ("%d:%.2d:%.2d:%.2d", d, h, m, s);
		unit = kDays;
	}
	else if(h)
	{
		if(compact)
			temp.appendFormat ("%d:%.2d", h, m);
		else
			temp.appendFormat ("%d:%.2d:%.2d", h, m, s);
		unit = kHours;
	}
	else if(m)
	{
		temp.appendFormat ("%d:%.2d", m, s);
		unit = kMinutes;
	}
	else
		temp.appendFormat ("%d", s);

	if(ms && (compact == false || (compact && ccl_abs(seconds) < 10.0)))
		temp.appendFormat (".%03d", ms);

	const String* units[] = { &XSTR (Days), &XSTR (Hours), &XSTR (Minutes), &XSTR (Seconds) };

	temp.append (" ");
	String string (temp);
	string.append (*units[unit]);

	result = string;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::scanDuration (double& seconds, StringRef string, int defaultUnit, int flags) const
{
	int timeUnit = defaultUnit;

	if(flags & kDetectUnit)
	{
		// try all unit strings
		for(int u = 0; u < ARRAY_COUNT (durationsUnits); u++)
		{
			int unitIndex = string.index (durationsUnits[u].unitString, false);
			if(unitIndex >= 0)
			{
				String valueStr (string.subString (0, unitIndex));
				return scanDuration (seconds, valueStr, u, flags &~(kDetectUnit|kRequireUnit));
			}
		}

		// no unit string found
		if(flags & kRequireUnit)
			return kResultFailed;

		// no unit found: start with default unit, each additional segment increases the dimension (e.g. enter "1:20", defaultUnit seconds -> use minutes)
		Variant segments[4];
		int numSegments = string.scanFormat ("%(1):%(2):%(3):%(4)", segments, 4);
		if(numSegments > 1 && numSegments <= 4)
			timeUnit = ccl_bound<int> (defaultUnit - (numSegments - 1), kDays, kSeconds);
	}

	if(timeUnit >= kDays && timeUnit <= kSeconds)
	{
		seconds = 0;
		Variant segments[4];
		int numSegments = string.scanFormat (durationsUnits[timeUnit].format, segments, durationsUnits[timeUnit].maxSegments);
		if(numSegments > 0)
		{
			int unit = timeUnit;
			for(int s = 0; s < numSegments; s++, unit++)
			{
				double segmentSeconds = (unit == kSeconds)
					? ccl_round<3> (segments[s].asDouble () * durationsUnits[unit].seconds)
					: segments[s].asInt () * durationsUnits[unit].seconds;
				seconds += segmentSeconds;
			}
		}
		return kResultOk;
	}
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printCurrency (String& result, double value, int flags) const
{
	result.empty ();
	result.appendFloatValue (value, 2);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printByteSize (String& result, double bytes, int flags) const
{
	int dimension = 0;
	double base = (flags & kSIByteUnit) ? 1000. : 1024.;

	while(bytes > base && dimension < 4)
	{
		bytes /= base;
		dimension++;
	}

	MutableCString temp;
	if(dimension == 0)
		temp.appendFormat ("%d", (int)bytes);
	else
		temp.appendFormat ("%.2f", bytes);

	static const String* units[] = { &XSTR (Bytes), &XSTR (KB), &XSTR (MB), &XSTR (GB), &XSTR (TB) };

	temp.append (" ");
	String string (temp);
	string.append (*units[dimension]);

	result = string;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleInfoBase::printBytesPerSecond (String& result, double speed, int flags) const
{
	double base = (flags & kSIByteUnit) ? 1000. : 1024.;

	String unit;
	double unitsPerSecond = speed / (base * base); // try MB/sec first
	if(unitsPerSecond >= 1.)
		unit = XSTR (MBPerSecond);
	else
	{
		unit = XSTR (KBPerSecond);
		unitsPerSecond = speed / base;
	}

	result.empty ();
	result.appendFloatValue (unitsPerSecond, 2);
	result << " ";
	result << unit;
	return kResultOk;
}

//************************************************************************************************
// GeographicRegion
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GeographicRegion, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API GeographicRegion::getEnglishName () const
{
	return englishName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API GeographicRegion::getNativeName () const
{
	return nativeName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API GeographicRegion::getLocalizedName () const
{
	return localizedName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API GeographicRegion::getISO2Code () const
{
	return iso2Code;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GeographicRegion::compare (const Object& obj) const
{
	return localizedName.compareWithOptions (static_cast<const GeographicRegion&> (obj).localizedName, Text::kIgnoreDiacritic);
}

//************************************************************************************************
// GeographicRegionList
//************************************************************************************************

GeographicRegionList::GeographicRegionList ()
{
	objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeographicRegionList::containsRegion (StringRef englishName) const
{
	for(int i = 0; i < count (); i++)
	{
		if(GeographicRegion* info = ccl_cast<GeographicRegion> (at (i)))
			if(info->englishName == englishName)
				return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeographicRegionList::containsRegionISO (StringRef iso2) const
{
	for(int i = 0; i < count (); i++)
	{
		if(GeographicRegion* info = ccl_cast<GeographicRegion> (at (i)))
			if(info->iso2Code == iso2)
				return true;
	}
	return false;
}
