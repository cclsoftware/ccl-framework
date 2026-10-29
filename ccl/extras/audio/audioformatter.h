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
// Filename    : ccl/extras/audio/audioformatter.h
// Description : Audio Formatter
//
//************************************************************************************************

#ifndef _ccl_audioformatter_h
#define _ccl_audioformatter_h

#include "ccl/public/base/iformatter.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {
namespace Format {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Formatter Names
//////////////////////////////////////////////////////////////////////////////////////////////////

const CStringPtr kBlockSize = "Audio.BlockSize";
const CStringPtr kFrequency = "Audio.Frequency";
const CStringPtr kSampleRate = "Audio.SampleRate";
const CStringPtr kChannels = "Audio.Channels";
const CStringPtr kBitDepth = "Audio.BitDepth";
const CStringPtr kSeconds = "Audio.Seconds";
const CStringPtr kSecondsMS = "Audio.Seconds.ms";
const CStringPtr kGain = "Audio.Gain";
const CStringPtr kGainDB = "Audio.Gain.db";
const CStringPtr kDB = "Audio.dB";
const CStringPtr kStereoPan = "Audio.StereoPan";

//************************************************************************************************
// Format::BlockSize
/** Format block size as string (e.g. "64 samples"). */
//************************************************************************************************

class BlockSize: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (BlockSize)

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::Frequency
/** Format frequency as string (e.g. "100 Hz"). */
//************************************************************************************************

class Frequency: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Frequency)

	enum Unit
	{
		kAuto,
		kKilohertz
	};

	static String print (VariantRef value, Unit unit = kAuto);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::SampleRate
/** Format sample rate as string (e.g. "44.1 kHz"). */
//************************************************************************************************

class SampleRate: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (SampleRate)

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::Channels
/** Format channels as string (e.g. "Mono", "Stereo", "6 ch"). */
//************************************************************************************************

class Channels: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Channels)

	static String print (VariantRef value, bool alwaysDigits = false);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::BitDepth
/** Format bit depth as string (e.g. "32 bit"). */
//************************************************************************************************

class BitDepth: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (BitDepth)

	static String print (VariantRef value, bool isFloat = false);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::Seconds
/** Format a seconds value as milliseconds or seconds (e.g. 0.1 = "100 ms", 1 = "1 s"). */
//************************************************************************************************

class Seconds: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Seconds)

	enum Unit
	{
		kAuto,
		kMilliseconds
	};

	static String print (VariantRef value, Unit unit = kAuto);
	static bool scan (Variant& value, StringRef string, Unit unit = kAuto);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::SecondsMS
/** Format a seconds value always as milliseconds (e.g. 0.1 = "100 ms", 1 = "1000 ms"). */
//************************************************************************************************

class SecondsMS: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (SecondsMS)

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::DB
/** Format a plain dB value (e.g. "0 dB"). */
//************************************************************************************************

class DB: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (DB)

	struct DBValue
	{
		enum Type { kMinusInf, kMinus, kZero, kPlus };
		Type type = kZero;
		float db = 0.f;
	};

	enum Unit
	{
		kAuto,
		kAlways
	};

	static DBValue getValue (float db);
	static String printValue (const DBValue& value, Unit unit);

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::Gain
/** Format gain factor as string (0.0 = -oo, 1.0 = 0dB, 2.0 = +6, etc.) */
//************************************************************************************************

class Gain: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (Gain)

	static String print (VariantRef value, bool printUnits = false);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::GainDB
/** Like Format::Gain with always adding a "dB" label (e.g. 2.0 = +6 dB) */
//************************************************************************************************

class GainDB: public Gain
{
public:
	DECLARE_FORMATTER_FACTORY (GainDB)

	static String print (VariantRef value);

	DEFINE_FORMATTER_METHODS
};

//************************************************************************************************
// Format::StereoPan
/** Format stereo pan value as string (-1 = <L>, 0 = <C>, +1 = <R>). */
//************************************************************************************************

class StereoPan: public Formatter
{
public:
	DECLARE_FORMATTER_FACTORY (StereoPan)

	struct PanValue
	{
		enum Position { kLeft, kRight, kCenter };
		Position position = kCenter;
		float normalizedValue = 0.f;
	};

	static String printValue (const PanValue& value);
	static bool scanValue (PanValue& value, StringRef string);

	static String print (VariantRef value);
	static bool scan (Variant& value, StringRef string);

	DEFINE_FORMATTER_METHODS
};

} // namespace Format
} // namespace CCL

#endif // _ccl_audioformatter_h