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
// Filename    : ccl/public/system/formatter.h
// Description : Basic Value Formatters
//
//************************************************************************************************

#ifndef _ccl_formatter_h
#define _ccl_formatter_h

#include "ccl/public/base/iformatter.h"
#include "ccl/public/base/datetime.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/system/ilocaleinfo.h"
#include "ccl/public/collections/vector.h"

namespace Core {
class Interpolator; }

namespace CCL {
namespace Format {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Formatter Names
//////////////////////////////////////////////////////////////////////////////////////////////////

const CStringPtr kPercent = "System.Percent";

//************************************************************************************************
// Format::Duration
/** Format duration as string (like "12:38.123").
	\ingroup gui_param */
//************************************************************************************************

class Duration: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Duration)

	Duration (ILocaleInfo::TimeUnit defaultUnit = ILocaleInfo::kSeconds);

	static String print (VariantRef seconds, bool compact = false);
	static bool scan (Variant& value, StringRef string, ILocaleInfo::TimeUnit defaultUnit = ILocaleInfo::kSeconds);

	PROPERTY_VARIABLE (ILocaleInfo::TimeUnit, defaultUnit, DefaultUnit) ///< default unit for scanning
	PROPERTY_BOOL (compactMode, CompactMode) ///< default unit for scanning

	tbool CCL_API printString (String& string, VariantRef value) const override;
	tbool CCL_API scanString (Variant& value, StringRef string) const override;
};

//************************************************************************************************
// Format::ByteSize
/** Format a byte size as string (like "42.10 bytes", "42.00 MB") based on power of 2.
	\ingroup gui_param	*/
//************************************************************************************************

class ByteSize: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (ByteSize)

	static String print (VariantRef bytes);
	static bool scan (Variant& value, StringRef string);
	static bool scan (Variant& value, StringRef string, double base);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::SIByteSize
/** Format a byte size as string (like "42.10 bytes", "42.00 MB") based on power of 10 (SI unit).
	\ingroup gui_param	*/
//************************************************************************************************

class SIByteSize: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (ByteSize)

	static String print (VariantRef bytes);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::BytesPerSecond
/** Format data rate (like "1 MB/s") based on power of 2.
	\ingroup gui_param	*/
//************************************************************************************************

class BytesPerSecond: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (BytesPerSecond)

	static String print (VariantRef bytes);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::SIBytesPerSecond
/** Format data rate (like "1 MB/s") based on power of 10 (SI unit).
	\ingroup gui_param	*/
//************************************************************************************************

class SIBytesPerSecond: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (SIBytesPerSecond)

	static String print (VariantRef bytes);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::DateTimeVariant
//************************************************************************************************

template <class T>
class DateTimeVariant
{
public:
	static String print (VariantRef value)
	{
		UnknownPtr<IDateTime> iDateTime (value);
		ASSERT (iDateTime.isValid ())
		if(iDateTime)
		{
			CCL::DateTime dateTime;
			iDateTime->copyTo (dateTime);
			return T::print (dateTime);
		}
		return String ();
	}

	static bool scan (Variant& value, StringRef string)
	{
		UnknownPtr<IDateTime> iDateTime (value);
		ASSERT (iDateTime.isValid ())
		if(iDateTime)
		{
			CCL::DateTime dateTime;
			if(T::scan (dateTime, string))
			{
				iDateTime->assign (dateTime);
				return true;
			}
		}
		return false;
	}
};

//************************************************************************************************
// Format::DateTime
/** Format date/time as string depending on current locale.
	\ingroup gui_param */
//************************************************************************************************

class DateTime: public CCL::Formatter,
				public DateTimeVariant<DateTime>
{
public:
	DECLARE_FORMATTER_FACTORY (DateTime)

	enum Flags
	{
		kDate = 1<<0,
		kTime = 1<<1,
		kFriendly = 1<<2,

		kDateTime = kDate|kTime,
		kFriendlyDateTime = kDateTime|kFriendly
	};

	static String print (const CCL::DateTime& dateTime, int flags = kDateTime);
	static bool scan (CCL::DateTime& dateTime, StringRef string);

	using DateTimeVariant::print;
	using DateTimeVariant::scan;

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::PortableDateTime
/** Format date/time as string (portable).
	\ingroup gui_param */
//************************************************************************************************

class PortableDateTime: public CCL::Formatter,
						public DateTimeVariant<PortableDateTime>
{
public:
	DECLARE_FORMATTER_FACTORY (PortableDateTime)

	static String print (const CCL::DateTime& dateTime);
	static bool scan (CCL::DateTime& dateTime, StringRef string);

	using DateTimeVariant::print;
	using DateTimeVariant::scan;

	DEFINE_FORMATTER_METHODS

protected:
	static CStringPtr formatString; ///< YYYY/MM/DD HH:MM:SS.ms
};

//************************************************************************************************
// Format::ISODateTime
/** Format date/time according to ISO 8601.
	\ingroup gui_param */
//************************************************************************************************

class ISODateTime: public CCL::Formatter,
				   public DateTimeVariant<ISODateTime>
{
public:
	DECLARE_FORMATTER_FACTORY (ISODateTime)

	static String print (const CCL::DateTime& dateTime);
	static bool scan (CCL::DateTime& dateTime, StringRef string);

	using DateTimeVariant::print;
	using DateTimeVariant::scan;

	DEFINE_FORMATTER_METHODS

protected:
	static CStringPtr formatString; ///< YYYY-MM-DDTHH:MM:SS
};

//************************************************************************************************
// Format::TimeAgo
/** Format time ago from now (like "3 weeks ago"). DateTime is in local timezone.
	\ingroup gui_param */
//************************************************************************************************

class TimeAgo: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (TimeAgo)

	static String print (const CCL::DateTime& dateTime);
	static bool scan (CCL::DateTime& dateTime, StringRef string);

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);
};

//************************************************************************************************
// Format::TimeAhead
/** Format time ahead from now (like "3 days"). DateTime is in local timezone.
	\ingroup gui_param */
//************************************************************************************************

class TimeAhead: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (TimeAhead)

	static String print (const CCL::DateTime& dateTime);
	static bool scan (CCL::DateTime& dateTime, StringRef string);

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);
};

//************************************************************************************************
// Format::Percent
/** Format the normalized value as percent string (like "79%").
	\ingroup gui_param */
//************************************************************************************************

class Percent: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Percent)

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	// Formatter
	int CCL_API getFlags () const override { return kNormalized; }
	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::PercentFloat
/** Format the float value * 100 as percent string (like 0.79 -> "79%").
	\ingroup gui_param */
//************************************************************************************************

class PercentFloat: public Percent
{
public:
	DECLARE_FORMATTER_FACTORY (PercentFloat)

	// Percent
	int CCL_API getFlags () const override { return 0; }
};

//************************************************************************************************
// Format::PercentInt
/** Format integer value as percent string (like "79%").
	\ingroup gui_param */
//************************************************************************************************

class PercentInt: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (PercentInt)

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	// Formatter
	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::Int
/** Format integer value as string (like "-60").
	\ingroup gui_param */
//************************************************************************************************

class Int: public CCL::Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Int)

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	// Formatter
	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Fromat::FourCharID
/** Formats an int32 value as 4 characters \ingroup core_format */
//************************************************************************************************

class FourCharID: public CCL::Formatter
{
public:
	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	// Formatter
	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::Normalized
/** Used to wrap a non-normalized formatter, i.e. converts values to range internally.
	\ingroup gui_param */
//************************************************************************************************

class Normalized: public CCL::Formatter
{
public:
	Normalized (IFormatter* formatter, Core::Interpolator* interpolator, bool ownsInterpolator = true);	///< takes ownership
	~Normalized ();

	// Formatter
	int CCL_API getFlags () const override { return kNormalized; }
	tbool CCL_API printString (String& string, VariantRef value) const override;
	tbool CCL_API scanString (Variant& value, StringRef string) const override;

private:
	AutoPtr<IFormatter> formatter;
	Core::Interpolator* interpolator;
	bool ownsInterpolator;
};

//************************************************************************************************
// Format::Linear
/** Modifies the value with a factor and offset before delegating to another formatter.
	printed value = factor * value + offset
	\ingroup gui_param */
//************************************************************************************************

class Linear: public CCL::Formatter
{
public:
	Linear (IFormatter* formatter, double factor = 1, double offset = 0);	///< takes ownership

	PROPERTY_VARIABLE (double, factor, Factor)
	PROPERTY_VARIABLE (double, offset, Offset)

	// Formatter
	tbool CCL_API printString (String& string, VariantRef value) const override;
	tbool CCL_API scanString (Variant& value, StringRef string) const override;

private:
	AutoPtr<IFormatter> formatter;
};

//************************************************************************************************
// Format::Bipolar
/** To be used with a normalized Formatter that handles non-bipolar values.
	e.g. Format::Bipolar (NEW Format::Percent) prints 0 -> "-100%", 0.5 -> "0%", 1 -> "100%"
	\ingroup gui_param */
//************************************************************************************************

class Bipolar: public CCL::Formatter
{
public:
	Bipolar (IFormatter* formatter); ///< takes ownership

	// Formatter
	int CCL_API getFlags () const override { return kNormalized; }
	tbool CCL_API printString (String& string, VariantRef value) const override;
	tbool CCL_API scanString (Variant& value, StringRef string) const override;

private:
	AutoPtr<IFormatter> formatter;
};

//************************************************************************************************
// Format::Offset
/** Formats an integer value with an offset (like offset=1 => 0 -> 1)
	\ingroup gui_param */
//************************************************************************************************

class Offset: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Offset)

	Offset (int offset = 1);

	void set (int offset);

	// Formatter
	tbool CCL_API printString (String& string, VariantRef value) const override;
	tbool CCL_API scanString (Variant& value, StringRef string) const override;

private:
	int offset;
};

//************************************************************************************************
// Format::StringList
/** Formats an integer value as item from string list.
	\ingroup gui_param */
//************************************************************************************************

class StringList: public Formatter
{
public:
	StringList ();

	void appendString (StringRef string);
	int countStrings () const;
	void removeAll ();

	// Formatter
	tbool CCL_API printString (String& string, VariantRef value) const override;
	tbool CCL_API scanString (Variant& value, StringRef string) const override;

private:
	Vector<String> strings;
};

} // namespace Format
} // namespace CCL

#endif // _ccl_formatter_h

