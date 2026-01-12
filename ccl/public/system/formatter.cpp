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
// Filename    : ccl/public/system/formatter.cpp
// Description : Basic Value Formatters
//
//************************************************************************************************

#include "ccl/public/system/formatter.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"

#include "core/public/coreinterpolator.h"
#include "core/public/coreproperty.h"

using namespace CCL;

//************************************************************************************************
// Format::Normalized
//************************************************************************************************

Format::Normalized::Normalized (IFormatter* formatter, Core::Interpolator* interpolator, bool ownsInterpolator)
: formatter (formatter),
  interpolator (interpolator),
  ownsInterpolator (ownsInterpolator)
{
	ASSERT (formatter && !formatter->isNormalized () && interpolator)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Format::Normalized::~Normalized ()
{
	if(ownsInterpolator)
		delete interpolator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Normalized::printString (String& string, VariantRef value) const
{
	Variant rangeValue = interpolator->normalizedToRange (value.asFloat ());
	return formatter->printString (string, rangeValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Normalized::scanString (Variant& value, StringRef string) const
{
	Variant rangeValue;
	if(!formatter->scanString (rangeValue, string))
		return false;

	value = interpolator->rangeToNormalized (rangeValue);
	return true;
}

//************************************************************************************************
// Format::Linear
//************************************************************************************************

Format::Linear::Linear (IFormatter* formatter, double factor, double offset)
: formatter (formatter),
  factor (factor),
  offset (offset)
{
	ASSERT (factor != 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Linear::printString (String& string, VariantRef value) const
{
	double v = factor * value.asDouble () + offset;

	if(formatter)
		return formatter->printString (string, v);
	else
	{
		string.appendFloatValue (v, 2); // fallback as in FloatParam
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Linear::scanString (Variant& value, StringRef string) const
{
	double v = 0;

	if(formatter)
	{
		if(!formatter->scanString (value, string))
			return false;

		v = value.asDouble ();
	}
	else
	{
		if(!string.getFloatValue (v))
			return false;
	}

	value = factor == 0 ? 0 : (v - offset) / factor;
	return true;
}

//************************************************************************************************
// Format::Bipolar
//************************************************************************************************

Format::Bipolar::Bipolar (IFormatter* formatter)
: formatter (formatter)
{
	ASSERT (formatter && formatter->isNormalized ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Bipolar::printString (String& string, VariantRef value) const
{
	// translate value from [0, 1] to [-1, 1]
	double v = 2. * value.asDouble () - 1.;
	ASSERT (-1. <= v && v <= 1.)
	return formatter->printString (string, v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Bipolar::scanString (Variant& value, StringRef string) const
{
	if(!formatter->scanString (value, string))
		return false;

	// translate value from [-1, 1] to [0, 1]
	double v = value.asDouble ();
	value = ccl_bound (v * .5 + .5, 0., 1.);
	return true;
}

//************************************************************************************************
// Format::Duration
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Duration, "System.Duration")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::Duration::print (VariantRef seconds, bool compact)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();

	String string;
	locale.printDuration (string, seconds, compact ? ILocaleInfo::kCompactDuration : 0);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::Duration::scan (Variant& value, StringRef string, ILocaleInfo::TimeUnit defaultUnit)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();

	double seconds;
	if(locale.scanDuration (seconds, string, defaultUnit, ILocaleInfo::kDetectUnit) == kResultOk)
	{
		value = seconds;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Format::Duration::Duration (ILocaleInfo::TimeUnit defaultUnit)
: defaultUnit (defaultUnit),
  compactMode (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Duration::printString (CCL::String& string, CCL::VariantRef value) const
{ string = print (value, compactMode); return true; }

tbool CCL_API Format::Duration::scanString (CCL::Variant& value, CCL::StringRef string) const
{ return scan (value, string, defaultUnit); }

//************************************************************************************************
// Format::ByteSize
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::ByteSize, "System.ByteSize")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::ByteSize::print (VariantRef value)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();
	
	String string;
	locale.printByteSize (string, value);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::ByteSize::scan (Variant& value, StringRef string)
{
	return scan (value, string, 1024.);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::ByteSize::scan (Variant& value, StringRef string, double base)
{
	double byteSize = 0.;
	if(!string.getFloatValue (byteSize))
		return false;

	// LATER TODO: doesn't work with localized strings here!
	if(string.contains ("KB"))
		byteSize *= base;
	else if(string.contains ("MB"))
		byteSize *= base * base;
	else if(string.contains ("GB"))
		byteSize *= base * base * base;

	value = byteSize;
	return true;
}

//************************************************************************************************
// Format::SIByteSize
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::SIByteSize, "System.SIByteSize")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::SIByteSize::print (VariantRef value)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();

	String string;
	locale.printByteSize (string, value, ILocaleInfo::kSIByteUnit);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::SIByteSize::scan (Variant& value, StringRef string)
{
	return Format::ByteSize::scan (value, string, 1000.);
}

//************************************************************************************************
// Format::BytesPerSecond
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::BytesPerSecond, "System.BytesPerSecond")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::BytesPerSecond::print (VariantRef value)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();

	String string;
	locale.printBytesPerSecond (string, value);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::BytesPerSecond::scan (Variant& value, StringRef string)
{
	CCL_NOT_IMPL ("Format::BytesPerSecond::scan not implemented!")
	return false;
}

//************************************************************************************************
// Format::SIBytesPerSecond
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::SIBytesPerSecond, "System.SIBytesPerSecond")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::SIBytesPerSecond::print (VariantRef value)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();

	String string;
	locale.printBytesPerSecond (string, value, ILocaleInfo::kSIByteUnit);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::SIBytesPerSecond::scan (Variant& value, StringRef string)
{
	CCL_NOT_IMPL ("Format::SIBytesPerSecond::scan not implemented!")
	return false;
}

//************************************************************************************************
// Format::Percent
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Percent, Format::kPercent)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::Percent::print (VariantRef value)
{
	MutableCString temp;
	temp.appendFormat ("%d%%", ccl_to_int (value.asDouble () * 100.));

	String string (temp);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::Percent::scan (Variant& value, StringRef string)
{
	double v = 0.;
	if(string.getFloatValue (v))
	{
		value = v * .01;
		return true;
	}
	return false;
}

//************************************************************************************************
// Format::PercentFloat
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::PercentFloat, "System.PercentFloat")

//************************************************************************************************
// Format::PercentInt
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::PercentInt, "System.PercentInt")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::PercentInt::print (VariantRef value)
{
	MutableCString temp;
	temp.appendFormat ("%d%%", ccl_to_int (value.asDouble ()));

	String string (temp);
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::PercentInt::scan (Variant& value, StringRef string)
{
	int64 v = 0.;
	if(string.getIntValue (v))
	{
		value = v;
		return true;
	}
	return false;
}

//************************************************************************************************
// Format::Int
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Int, "System.Int")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::Int::print (VariantRef value)
{
	int64 intValue = ccl_to_int (value.asDouble ());
	return String ().appendIntValue (intValue);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::Int::scan (Variant& value, StringRef string)
{
	int64 intValue = 0;
	if(string.getIntValue (intValue))
	{	
		value = intValue;
		return true;
	}
	return false;
}

//************************************************************************************************
// Format::FourCharID
//************************************************************************************************

String Format::FourCharID::print (VariantRef value)
{
	int32 id = value.asInt ();
	CStringPtr ptr = (CStringPtr)&id;
	char temp[5] = {ptr[3], ptr[2], ptr[1], ptr[0], 0};
	if(temp[1] == 0)
		return String (temp + 2);
	else
		return String (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::FourCharID::scan (Variant& value, StringRef string)
{
	MutableCString str (string);
	if(str.length () >= 4)
	{
		int32 id = FOUR_CHAR_ID (str[0], str[1], str[2], str[3]);
		value = id;
		return true;
	}
	return false;
}

//************************************************************************************************
// Format::DateTime
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::DateTime, "System.DateTime")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::DateTime::print (const CCL::DateTime& dateTime, int flags)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();
	bool friendly = (flags & kFriendly) != 0;

	String dateString;
	if(flags & kDate)
		locale.printDate (dateString, dateTime.getDate (), friendly ? ILocaleInfo::kFriendlyDate : 0);

	String timeString;
	if(flags & kTime)
		locale.printTime (timeString, dateTime.getTime ());

	String result;
	result << dateString;
	if(!result.isEmpty ())
		result << " ";
	result << timeString;

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::DateTime::scan (CCL::DateTime& dateTime, StringRef string)
{
	CCL_NOT_IMPL ("Format::DateTime::scan not implemented!")
	return false;
}

//************************************************************************************************
// Format::PortableDateTime
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::PortableDateTime, "System.PortableDateTime")
CStringPtr Format::PortableDateTime::formatString = "%04d/%02d/%02d %02d:%02d:%02d.%03d";

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::PortableDateTime::print (const CCL::DateTime& dateTime)
{
	const Date& date = dateTime.getDate ();
	const Time& time = dateTime.getTime ();

	MutableCString temp;
	temp.appendFormat (formatString, date.getYear (), date.getMonth (), date.getDay (),
					   time.getHour (), time.getMinute (), time.getSecond (),
					   time.getMilliseconds ());

	return String (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::PortableDateTime::scan (CCL::DateTime& dateTime, StringRef string)
{
	char temp[255] = {0};
	string.toASCII (temp, sizeof(temp));

	int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, ms = 0;
	::sscanf (temp, formatString, &year, &month, &day, &hour, &minute, &second, &ms);

	dateTime.setDate (Date (year, month, day));
	dateTime.setTime (Time (hour, minute, second, ms));
	return true;
}

//************************************************************************************************
// Format::ISODateTime
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::ISODateTime, "System.ISODateTime")
CStringPtr Format::ISODateTime::formatString = "%04d-%02d-%02dT%02d:%02d:%02d";

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::ISODateTime::print (const CCL::DateTime& dateTime)
{
	// See https://en.wikipedia.org/wiki/ISO_8601
	// Example: 2023-01-04T09:11:59

	const Date& date = dateTime.getDate ();
	const Time& time = dateTime.getTime ();

	MutableCString temp;
	temp.appendFormat (formatString, date.getYear (), date.getMonth (), date.getDay (),
					   time.getHour (), time.getMinute (), time.getSecond ());

	return String (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::ISODateTime::scan (CCL::DateTime& dateTime, StringRef string)
{
	char temp[255] = {0};
	string.toASCII (temp, sizeof(temp));

	int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
	::sscanf (temp, formatString, &year, &month, &day, &hour, &minute, &second);

	dateTime.setDate (Date (year, month, day));
	dateTime.setTime (Time (hour, minute, second));
	return true;
}

//************************************************************************************************
// Format::TimeAgo
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::TimeAgo, "System.TimeAgo")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::TimeAgo::print (const CCL::DateTime& dateTime)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();

	String result;
	locale.printTimeAgo (result, dateTime);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::TimeAgo::scan (CCL::DateTime& dateTime, StringRef string)
{
	CCL_NOT_IMPL ("Format::TimeAgo::scan not implemented!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::TimeAgo::print (VariantRef value)
{
	UnknownPtr<IDateTime> iDateTime (value);
	if(iDateTime)
	{
		CCL::DateTime dateTime;
		iDateTime->copyTo (dateTime);
		return print (dateTime);
	}
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::TimeAgo::scan (Variant& value, StringRef string)
{
	CCL_NOT_IMPL ("Format::TimeAgo::scan not implemented!")
	return false;
}

//************************************************************************************************
// Format::TimeAhead
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::TimeAhead, "System.TimeAhead")

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::TimeAhead::print (const CCL::DateTime& dateTime)
{
	const ILocaleInfo& locale = System::GetLocaleManager ().getCurrentLocale ();

	String result;
	locale.printTimeAhead (result, dateTime);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::TimeAhead::scan (CCL::DateTime& dateTime, StringRef string)
{
	CCL_NOT_IMPL ("Format::TimeAhead::scan not implemented!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::TimeAhead::print (VariantRef value)
{
	UnknownPtr<IDateTime> iDateTime (value);
	if(iDateTime)
	{
		CCL::DateTime dateTime;
		iDateTime->copyTo (dateTime);
		return print (dateTime);
	}
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::TimeAhead::scan (Variant& value, StringRef string)
{
	CCL_NOT_IMPL ("Format::TimeAhead::scan not implemented!")
	return false;
}

//************************************************************************************************
// Format::StringList
//************************************************************************************************

Format::StringList::StringList ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Format::StringList::appendString (CCL::StringRef string)
{
	strings.add (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Format::StringList::countStrings () const
{
	return strings.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Format::StringList::removeAll ()
{
	strings.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::StringList::printString (CCL::String& string, CCL::VariantRef value) const
{
	int idx = value.asInt ();
	if(idx < strings.count ())
	{
		string = strings.at (idx);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tbool CCL_API Format::StringList::scanString (CCL::Variant& value, CCL::StringRef string) const
{
	for(int i = 0; i < strings.count (); i++)
	{
		if(strings.at (i).compare (string, false) == 0)
		{
			value = i;
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// Format::Offset
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Offset, "System.PlusOne")

//////////////////////////////////////////////////////////////////////////////////////////////////

Format::Offset::Offset (int offset)
: offset (offset)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Format::Offset::set (int _offset)
{
	offset = _offset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Offset::printString (String& string, VariantRef value) const
{
	string.empty ();
	string.appendIntValue (value.asInt () + offset);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Format::Offset::scanString (Variant& value, StringRef string) const
{
	value.fromString (string);
	value = value.asInt () - offset;
	return true;
}

