//************************************************************************************************
//
// Audio Service
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
// Filename    : miniaudioglue.cpp
// Description : miniaudio Glue Code
//
//************************************************************************************************

#include "miniaudioglue.h"

#include "ccl/public/audio/iaudiosource.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/primitives.h"

using namespace CCL;

static_assert (sizeof(ma_wchar_win32) == sizeof(uchar), "Unicode character size mismatch!");

static const String kFallbackDeviceID = CCLSTR ("default");

//************************************************************************************************
// MiniAudioUtils
//************************************************************************************************

String MiniAudioUtils::deviceIdToString (const ma_device_id& id, ma_backend backend)
{
	String idString;

	switch(backend)
	{
	// Windows
	case ma_backend_wasapi : 
		idString.append (UStringPtr(id.wasapi));
		break;

	// macOS/iOS
	case ma_backend_coreaudio :
		idString.appendASCII (id.coreaudio);
		break;

	// Linux
	case ma_backend_pulseaudio :
		idString.appendASCII (id.pulse);
		break;
	case ma_backend_alsa :
		idString.appendASCII (id.alsa);
		break;
	case ma_backend_jack :
		idString.appendIntValue (id.jack);
		break;

	// Android
	case ma_backend_aaudio :
		idString.appendHexValue (int64(id.aaudio));
		break;

	// Wasm
	case ma_backend_webaudio :
		// Web Audio only has the default devices and miniaudio leaves their id empty; we need a non-empty id.
		if(CString (id.webaudio).isEmpty ())
			idString = kFallbackDeviceID;
		else
			idString.appendASCII (id.webaudio);
		break;

	default :
		ASSERT (0) // what's missing?
		break;
	}
	
	ASSERT (!idString.isEmpty ())
	return idString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioUtils::stringToDeviceId (ma_device_id& id, StringRef idString, ma_backend backend)
{
	id = ma_device_id ();
	if(idString.isEmpty ())
		return false;

	switch(backend)
	{
	// Windows
	case ma_backend_wasapi :
		idString.copyTo ((uchar*)(id.wasapi), ARRAY_COUNT(id.wasapi));
		return true;

	// macOS/iOS
	case ma_backend_coreaudio :
		MutableCString (idString).copyTo (id.coreaudio, sizeof(id.coreaudio));
		return true;

	// Linux
	case ma_backend_pulseaudio :
		MutableCString (idString).copyTo (id.pulse, sizeof(id.pulse));
		return true;
	case ma_backend_alsa :
		MutableCString (idString).copyTo (id.alsa, sizeof(id.alsa));
		return true;
	case ma_backend_jack :
		id.jack = idString.scanInt ();
		return true;

	// Android
	case ma_backend_aaudio :
	{
		int64 value = 0;
		idString.getHexValue (value);
		id.aaudio = ma_int32(value);
		return true;
	}

	// Wasm
	case ma_backend_webaudio :
		// make fallback id empty again, see deviceIdToString
		if(idString != kFallbackDeviceID)
			MutableCString (idString).copyTo (id.webaudio, sizeof(id.webaudio));
		return true;

	default :
		ASSERT (0) // what's missing?
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioUtils::toCCLStreamSeekMode (ma_seek_origin origin)
{
	switch(origin)
	{
	case ma_seek_origin_current : return IStream::kSeekCur;
	case ma_seek_origin_end : return IStream::kSeekEnd;
	default : return IStream::kSeekSet;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ma_format MiniAudioUtils::toSampleFormat (const AudioContentFormat& format)
{
	switch(format.sampleType)
	{
	case kAudioSampleTypeFloat :
		return format.bitDepth == 32 ? ma_format_f32 : ma_format_unknown;

	case kAudioSampleTypeUnsigned :
		return format.bitDepth == 8 ? ma_format_u8 : ma_format_unknown;

	case kAudioSampleTypeInt :
		switch(format.bitDepth)
		{
		case 16 : return ma_format_s16;
		case 24 : return ma_format_s24;
		case 32 : return ma_format_s32;
		}
		break;
	}
	return ma_format_unknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioUtils::fromSampleFormat (AudioContentFormat& format, ma_format sampleFormat)
{
	switch(sampleFormat)
	{
	case ma_format_u8 :
		format.sampleType = kAudioSampleTypeUnsigned;
		format.bitDepth = 8;
		break;

	case ma_format_s16 :
		format.sampleType = kAudioSampleTypeInt;
		format.bitDepth = 16;
		break;

	case ma_format_s24 :
		format.sampleType = kAudioSampleTypeInt;
		format.bitDepth = 24;
		break;

	case ma_format_s32 :
		format.sampleType = kAudioSampleTypeInt;
		format.bitDepth = 32;
		break;

	case ma_format_f32 :
		format.sampleType = kAudioSampleTypeFloat;
		format.bitDepth = 32;
		break;

	default :
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField MiniAudioUtils::toSpeakerFormat (const ma_channel channelMap[], int numChannels)
{
	// SpeakerDef bits match the classic Windows/WAVEFORMATEXTENSIBLE SPEAKER_* mask, which
	// ma_channel positions (MA_CHANNEL_FRONT_LEFT..MA_CHANNEL_TOP_BACK_RIGHT) also follow 1:1;
	// positions beyond that (AUX_*) have no equivalent.

	SpeakerBitField speakers = SpeakerDef::kEmpty;

	for(int i = 0; i < numChannels; i++)
	{
		ma_channel channel = channelMap[i];
		if(channel == MA_CHANNEL_MONO)
			speakers |= SpeakerDef::kFrontLeft;
		else if(channel >= MA_CHANNEL_FRONT_LEFT && channel <= MA_CHANNEL_TOP_BACK_RIGHT)
			speakers |= SpeakerBitField(1) << (channel - MA_CHANNEL_FRONT_LEFT);
	}

	return speakers;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioUtils::fromSpeakerFormat (ma_channel channelMap[MA_MAX_CHANNELS], int numChannels, SpeakerBitField speakerFormat)
{
	// Build a channel map for the given speaker layout (SpeakerDef::kEmpty leaves entries unset,
	// i.e. MA_CHANNEL_NONE, so the caller can fall back to a standard layout). A single logical
	// channel is always mapped to MA_CHANNEL_MONO regardless of which bit (if any) is set.

	if(numChannels == 1)
	{
		channelMap[0] = MA_CHANNEL_MONO;
		return;
	}

	int index = 0;
	for(int bit = 0; bit < (MA_CHANNEL_TOP_BACK_RIGHT - MA_CHANNEL_FRONT_LEFT + 1) && index < numChannels; bit++)
	{
		if(get_bit<SpeakerBitField> (speakerFormat, bit))
			channelMap[index++] = ma_channel(MA_CHANNEL_FRONT_LEFT + bit);
	}

	for(; index < numChannels; index++)
		channelMap[index] = MA_CHANNEL_NONE;
}
