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
// Filename    : core/public/audio/coreaudiodatatypes.h
// Description : Audio Data Types
//
//************************************************************************************************

#ifndef _coreaudiodatatypes_h
#define _coreaudiodatatypes_h

#include "core/public/coretypes.h"

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Fundamental audio types
//////////////////////////////////////////////// //////////////////////////////////////////////////

/** Audio floating-point precision. */
DEFINE_ENUM (AudioFloatType)
{
	kAudioFloat32 = 0,	///< single precision
	kAudioFloat64		///< double precision
};

/** Audio PCM sample type. */
DEFINE_ENUM (AudioSampleType)
{
	kAudioSampleTypeInt = 0,	///< integer samples
	kAudioSampleTypeFloat,		///< floating-point samples
	kAudioSampleTypeUnsigned	///< integer samples (unsigned)
};

/** Audio format type. */
DEFINE_ENUM (AudioFormatType)
{
	kAudioFormatPCM,			///< format is PCM (e.g. WAV)
	kAudioFormatNonPCM			///< format is non-PCM (e.g. MP3)
};

/** Maximum number of channels per audio stream. */
static constexpr int kMaxAudioStreamChannels = 32;

/** Bit field for audio stream silence (max. 32 channels). */
typedef uint32 SilenceBitField;

/** Bitmask for all audio stream channels. */
static constexpr uint32 kAudioAllChannelsMask = 0xFFFFFFFF;

/** Number or position of audio samples. */
typedef int64 TSamples;

/** Time represented in seconds. */
typedef double TSeconds;

/** Sample rate in Hz. */
typedef double SampleRate;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get size of audio data in Bytes. */
inline int getAudioByteSize (int numSamples, AudioFloatType type)
{
	return numSamples * (4 << (int)type);
}

} // namespace Core

#endif // _coreaudiodatatypes_h
