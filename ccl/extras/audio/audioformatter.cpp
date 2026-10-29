//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/extras/audio/audioformatter.cpp
// Description : Audio Formatter
//
//************************************************************************************************

#include "ccl/extras/audio/audioformatter.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Audio")
	XSTRING (NSamples, "%(1) samples")
	XSTRING (Mono, "Mono")
	XSTRING (Stereo, "Stereo")
END_XSTRINGS

//************************************************************************************************
// Format::BlockSize
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::BlockSize, Format::kBlockSize)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::BlockSize::print (VariantRef value)
{
	return String ().appendFormat (XSTR (NSamples), value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::BlockSize::scan (Variant& value, StringRef string)
{
	int64 samples = 0;
	if(!string.getIntValue (samples))
		return false;

	value = samples;
	return true;
}

//************************************************************************************************
// Format::Frequency
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Frequency, Format::kFrequency)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::Frequency::print (VariantRef value, Unit unit)
{
	MutableCString temp;
	float frequency = value;
	if(unit == kKilohertz)
	{

		int precision = 1;
		int hz = int(value);
		if(hz % 10 != 0)
			precision = 3;
		else if(hz % 100 != 0)
			precision = 2;

		temp.appendFormat ("%.*f kHz", precision, ccl_round<3> (frequency / 1000.f));
	}
	else
	{
		ASSERT (unit == kAuto)

		if(frequency < 10.f)
			temp.appendFormat ("%1.2f Hz", frequency);
		else if(frequency < 100.f)
			temp.appendFormat ("%2.1f Hz", frequency);
		else if(frequency < 1000.f)
			temp.appendFormat ("%3.f Hz", frequency);
		else
			temp.appendFormat ("%1.2f kHz", frequency * 1e-3f);
	}
	return String (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::Frequency::scan (Variant& value, StringRef string)
{
	double frequency = 0.;
	if(string.getFloatValue (frequency))
	{
		if(string.contains ("k", false))
			frequency *= 1000.0;

		value = frequency;
		return true;
	}
	return false;
}

//************************************************************************************************
// Format::SampleRate
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::SampleRate, Format::kSampleRate)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::SampleRate::print (VariantRef value)
{
	return Format::Frequency::print (value, Frequency::kKilohertz);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::SampleRate::scan (Variant& value, StringRef string)
{
	return Format::Frequency::scan (value, string);
}

//************************************************************************************************
// Format::Channels
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Channels, Format::kChannels)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::Channels::print (VariantRef value, bool alwaysDigits)
{
	int channels = value;
	if(alwaysDigits == false)
	{
		switch(channels)
		{
		case 1 : return XSTR (Mono);
		case 2 : return XSTR (Stereo);
		}
	}
	String string;
	string.appendIntValue (channels);
	string << " ch";
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::Channels::scan (Variant& value, StringRef string)
{
	if(string.compare (XSTR (Mono), false) == Text::kEqual)
		value = 1;
	else if(string.compare (XSTR (Stereo), false) == Text::kEqual)
		value = 2;
	else
		return string.getIntValue (value.lValue);
	return true;
}

//************************************************************************************************
// Format::BitDepth
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::BitDepth, Format::kBitDepth)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::BitDepth::print (VariantRef value, bool isFloat)
{
	int bitDepth = value;
	MutableCString temp;
	temp.appendFormat ("%d bit", bitDepth);
	if(isFloat == true)
		temp.append (" float");
	return String (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::BitDepth::scan (Variant& value, StringRef string)
{
	CCL_NOT_IMPL ("Format::BitDepth::scan not implemented!")
	return false;
}

//************************************************************************************************
// Format::Seconds
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Seconds, Format::kSeconds)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::Seconds::print (VariantRef value, Unit unit)
{
	MutableCString temp;
	float seconds = value;

	if(unit == kMilliseconds)
	{
		temp.appendFormat ("%d ms", int(ccl_round<0> (seconds * 1000.f)));
	}
	else
	{
		ASSERT (unit == kAuto)

		if(seconds == 0.f)
			temp = "0 s";
		else if(seconds < 1e-5f)
			temp.appendFormat ("%f ms", seconds * 1000.f);
		else if(seconds < 1e-2f)
			temp.appendFormat ("%1.2f ms", seconds * 1000.f);
		else if(seconds < 0.1f)
			temp.appendFormat ("%2.1f ms", seconds * 1000.f);
		else if(seconds < 1.f)
			temp.appendFormat ("%3.0f ms", seconds * 1000.f);
		else if(seconds < 10.)
			temp.appendFormat ("%1.3f s", seconds);
		else if(seconds < 100.)
			temp.appendFormat ("%2.2f s", seconds);
		else
			temp.appendFormat ("%.1f s", seconds);
	}
	return String (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::Seconds::scan (Variant& value, StringRef string, Unit unit)
{
	double seconds = 0.;
	string.getFloatValue (seconds);

	if(unit == kMilliseconds || string.contains (CCLSTR ("ms")))
		seconds *= 0.001;

	value = seconds;
	return true;
}

//************************************************************************************************
// Format::SecondsMS
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::SecondsMS, Format::kSecondsMS)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::SecondsMS::print (VariantRef value)
{
	return Format::Seconds::print (value, Seconds::kMilliseconds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::SecondsMS::scan (Variant& value, StringRef string)
{
	return Format::Seconds::scan (value, string, Seconds::kMilliseconds);
}

//************************************************************************************************
// Format::DB
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::DB, Format::kDB)

//////////////////////////////////////////////////////////////////////////////////////////////////

Format::DB::DBValue Format::DB::getValue (float db)
{
	DBValue result;
	const float kMinDb = Math::factorToDb (0.f);
	if(db <= kMinDb)
	{
		result.type = DBValue::kMinusInf;
		result.db = kMinDb;
	}
	else
	{
		if(db < 0)
			db = float(0.1f * int(10.f * db - 0.5f));
		else
			db = float(0.1f * int(10.f * db + 0.5f));

		if(db == 0.f)
		{
			result.type = DBValue::kZero;
			result.db = 0.f;
		}
		else
		{
			result.type = db > 0.f ? DBValue::kPlus : DBValue::kMinus;
			result.db = db; 
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::DB::printValue (const DBValue& value, Unit unit)
{
	if(unit == kAuto)
	{
		static const String strMinusInfinite = CCLSTR ("-oo");
		static const String strZero = CCLSTR ("0dB");

		if(value.type == DBValue::kMinusInf)
			return strMinusInfinite;

		if(value.type == DBValue::kZero)
			return strZero;

		MutableCString temp;
		temp.appendFormat (value.type == DBValue::kPlus ? "+%.1f" : "%.1f", value.db);
		return String (temp);
	}
	else
	{
		static const String strMinusInfinite = CCLSTR ("-oo dB");

		if(value.type == DBValue::kMinusInf)
			return strMinusInfinite;

		MutableCString temp;
		temp.appendFormat (value.type == DBValue::kPlus ? "+%.1f dB" : "%.1f dB", value.db);
		return String (temp);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::DB::print (VariantRef value)
{
	return printValue (getValue (value.asFloat ()), kAlways);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::DB::scan (Variant& value, StringRef string)
{
	double f = 0.;
	if(!string.getFloatValue (f))
		return false;
	value = f;
	return true;
}

//************************************************************************************************
// Format::Gain
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::Gain, Format::kGain)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::Gain::print (VariantRef value, bool printUnits)
{
	float db = Math::factorToDb (value.asFloat ());
	return DB::printValue (DB::getValue (db), printUnits ? DB::kAlways : DB::kAuto);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::Gain::scan (Variant& value, StringRef string)
{
	double f = 0.;
	if(!string.getFloatValue (f))
		return false;
	value = Math::dbToFactor (f);
	return true;
}

//************************************************************************************************
// Format::GainDB
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::GainDB, Format::kGainDB)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::GainDB::print (VariantRef value)
{
	return Format::Gain::print (value, true);
}

//************************************************************************************************
// Format::StereoPan
//************************************************************************************************

DEFINE_FORMATTER_FACTORY (Format::StereoPan, Format::kStereoPan)

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::StereoPan::printValue (const PanValue& pv)
{
	static const String strLeft = CCLSTR ("<L>");
	static const String strRight = CCLSTR ("<R>");
	static const String strCenter = CCLSTR ("<C>");

	if(pv.position == PanValue::kCenter)
		return strCenter;

	int intValue = (int)(ccl_bound<float> (pv.normalizedValue, 0, 1) * 100.f + .5f);

	if(pv.position == PanValue::kLeft)
	{
		if(intValue >= 100)
			return strLeft;

		return String (MutableCString ().appendFormat ("L%d", intValue));
	}
	else
	{
		ASSERT (pv.position == PanValue::kRight)

		if(intValue >= 100)
			return strRight;

		return String (MutableCString ().appendFormat ("R%d", intValue));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::StereoPan::scanValue (PanValue& result, StringRef _string)
{
	MutableCString string (_string);
	string.toUppercase ();

	if(string.contains ("C"))
	{
		result = {PanValue::kCenter};
	}
	else
	{
		CStringPtr scanStart = string;

		int bracketIndex = string.index ("<");
		if(bracketIndex >= 0)
			scanStart += (bracketIndex + 1);

		char c = 0;
		int intValue = 0;
		::sscanf (scanStart, "%c%d", &c, &intValue);
		
		float normalizedValue = ccl_bound<float> (float(intValue)/100.f, 0, 1);

		if(c == 'R')
			result = {PanValue::kRight, normalizedValue};
		else
			result = {PanValue::kLeft, normalizedValue};
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Format::StereoPan::print (VariantRef value)
{
	float v = value;

	if(ccl_equals (v, 0.f, 0.00001f))
		return printValue ({PanValue::kCenter});
	else if(v < 0)
		return printValue ({PanValue::kLeft, 1.f - v});
	else
		return printValue ({PanValue::kRight, v});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Format::StereoPan::scan (Variant& value, StringRef string)
{
	PanValue result;
	if(!scanValue (result, string))
		return false;

	if(result.position == PanValue::kCenter)
		value = 0.f;
	else if(result.position == PanValue::kLeft)
		value = -result.normalizedValue;
	else
		value = result.normalizedValue;
	return true;
}
