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
// Filename    : core/public/coredatetime.h
// Description : Date/Time classes
//
//************************************************************************************************

#ifndef _coredatetime_h
#define _coredatetime_h

#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// Time
/**	Time data structure expressed in hours, minutes, seconds, and milliseconds.
	\ingroup core */
//************************************************************************************************

class Time
{
public:
	/** Construct time. */
	Time (int _hour = 0,
		  int _minute = 0,
		  int _second = 0,
		  int _ms = 0)
	: hour (_hour),
	  minute (_minute),
	  second (_second),
	  milliseconds (_ms)
	{}

	enum Constants
	{
		kSecondsPerMinute = 60,
		kSecondsPerHour = 60 * 60
	};

	int getHour () const;
	int getMinute () const;
	int getSecond () const;
	int getMilliseconds () const;

	void setHour (int _hour);
	void setMinute (int _minute);
	void setSecond (int _second);
	void setMilliseconds (int _ms);

	/** Convert time to seconds. */
	int64 toSeconds () const;
	
	/** Assign time from seconds. */
	void fromSeconds (int64 seconds);

	/** Convert time to milliseconds. */
	int64 toMilliseconds () const;

	Time& operator () (int _hour, int _minute = 0, int _second = 0, int _ms = 0);

	bool operator == (const Time& t) const;
	bool operator != (const Time& t) const;
	bool operator >  (const Time& t) const;
	bool operator <  (const Time& t) const;
	bool operator >= (const Time& t) const;
	bool operator <= (const Time& t) const;

protected:
	int hour;
	int minute;
	int second;
	int milliseconds;
};

//************************************************************************************************
// Date
/**	Date data structure expressed in year, month, day.
	\ingroup core */
//************************************************************************************************

class Date
{
public:
	/** Construct date. */
	Date (int _year = 1900,
		  int _month = 1,
		  int _day = 1)
	: year (_year),
	  month (_month),
	  day (_day)
	{}

	int getYear () const;
	int getMonth () const;
	int getDay () const;

	void setYear (int _year);
	void setMonth (int _month);
	void setDay (int _day);

	/** Convert to ordinal number for comparison. */
	int64 toOrdinal () const;

	Date& operator () (int _year, int _month = 1, int _day = 1);

	bool operator == (const Date& d) const;
	bool operator != (const Date& d) const;
	bool operator >  (const Date& d) const;
	bool operator <  (const Date& d) const;
	bool operator >= (const Date& d) const;
	bool operator <= (const Date& d) const;

protected:
	int year;
	int month;
	int day;
};

//************************************************************************************************
// DateTime
/** Combined date and time data structure.
	\ingroup core */
//************************************************************************************************

class DateTime
{
public:
	/** Construct date/time. */
	DateTime (const Date& date = Date (),
			  const Time& time = Time ())
	: date (date),
	  time (time)
	{}

	enum Constants
	{
		kSecondsInDay = 60 * 60 * 24
	};

	/** Get date component. */
	const Date& getDate () const;
	
	/** Get time component. */
	const Time& getTime () const;

	/** Set date component. */
	void setDate (const Date& _date);
	
	/** Set time component. */
	void setTime (const Time& _time);

	/** Convert to ordinal number for comparison. */
	int64 toOrdinal () const;

	bool operator == (const DateTime& dt) const;
	bool operator != (const DateTime& dt) const;
	bool operator >  (const DateTime& dt) const;
	bool operator <  (const DateTime& dt) const;
	bool operator >= (const DateTime& dt) const;
	bool operator <= (const DateTime& dt) const;

protected:
	Date date;
	Time time;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Time inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int Time::getHour () const
{ return hour; }

inline int Time::getMinute () const
{ return minute; }

inline int Time::getSecond () const
{ return second; }

inline int Time::getMilliseconds () const
{ return milliseconds; }

inline void Time::setHour (int _hour)
{ hour = _hour; }

inline void Time::setMinute (int _minute)
{ minute = _minute; }

inline void Time::setSecond (int _second)
{ second = _second; }

inline void Time::setMilliseconds (int _ms)
{ milliseconds = _ms; }

inline int64 Time::toSeconds () const
{ return second + minute * 60 + hour * kSecondsPerHour; }

inline void Time::fromSeconds (int64 _seconds)
{ hour = (int)(_seconds / kSecondsPerHour); minute = (int)((_seconds - hour * kSecondsPerHour) / 60); second = (int)(_seconds % 60);}

inline int64 Time::toMilliseconds () const
{ return toSeconds () * 1000 + milliseconds; }

inline bool Time::operator == (const Time& t) const
{ return toSeconds () == t.toSeconds (); }

inline bool Time::operator != (const Time& t) const
{ return toSeconds () != t.toSeconds (); }

inline bool Time::operator >  (const Time& t) const
{ return toSeconds () > t.toSeconds (); }

inline bool Time::operator <  (const Time& t) const
{ return toSeconds () < t.toSeconds (); }

inline bool Time::operator >= (const Time& t) const
{ return toSeconds () >= t.toSeconds (); }

inline bool Time::operator <= (const Time& t) const
{ return toSeconds () <= t.toSeconds (); }

inline Time& Time::operator () (int _hour, int _minute, int _second, int _ms)
{ hour = _hour; minute = _minute; second = _second; milliseconds = _ms; return *this; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Date inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int Date::getYear () const
{ return year; }

inline int Date::getMonth () const
{ return month; }

inline int Date::getDay () const
{ return day; }

inline void Date::setYear (int _year)
{ year = _year; }

inline void Date::setMonth (int _month)
{ month = _month; }

inline void Date::setDay (int _day)
{ day = _day; }

inline int64 Date::toOrdinal () const
{ return day + ((month-1) * 31) + ((year-1) * 12 * 31); }

inline bool Date::operator == (const Date& d) const
{ return toOrdinal () == d.toOrdinal (); }

inline bool Date::operator != (const Date& d) const
{ return toOrdinal () != d.toOrdinal (); }

inline bool Date::operator >  (const Date& d) const
{ return toOrdinal () > d.toOrdinal (); }

inline bool Date::operator <  (const Date& d) const
{ return toOrdinal () < d.toOrdinal (); }

inline bool Date::operator >= (const Date& d) const
{ return toOrdinal () >= d.toOrdinal (); }

inline bool Date::operator <= (const Date& d) const
{ return toOrdinal () <= d.toOrdinal (); }

inline Date& Date::operator () (int _year, int _month, int _day)
{ year = _year; month = _month; day = _day; return *this; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// DateTime inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline const Date& DateTime::getDate () const
{ return date; }

inline const Time& DateTime::getTime () const
{ return time; }

inline void DateTime::setDate (const Date& _date)
{ date = _date; }

inline void DateTime::setTime (const Time& _time)
{ time = _time; }

inline int64 DateTime::toOrdinal () const
{ return date.toOrdinal () * kSecondsInDay + time.toSeconds (); }

inline bool DateTime::operator == (const DateTime& dt) const
{ return toOrdinal () == dt.toOrdinal (); }

inline bool DateTime::operator != (const DateTime& dt) const
{ return toOrdinal () != dt.toOrdinal (); }

inline bool DateTime::operator >  (const DateTime& dt) const
{ return toOrdinal () > dt.toOrdinal (); }

inline bool DateTime::operator <  (const DateTime& dt) const
{ return toOrdinal () < dt.toOrdinal (); }

inline bool DateTime::operator >= (const DateTime& dt) const
{ return toOrdinal () >= dt.toOrdinal (); }

inline bool DateTime::operator <= (const DateTime& dt) const
{ return toOrdinal () <= dt.toOrdinal (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coredatetime_h
