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
// Filename    : ccl/public/audio/cclaudiotypes.h
// Description : Audio Data Types
//
//************************************************************************************************

#ifndef _ccl_cclaudiotypes_h
#define _ccl_cclaudiotypes_h

#include "ccl/public/base/platform.h"

#include "core/public/audio/coreaudiodata.h"
#include "core/public/audio/coreaudioiodata.h"
#include "core/public/audio/coreaudiometerdata.h"
#include "core/public/audio/coreaudiospeakers.h"

namespace CCL {

using Core::AudioFloatType;
using Core::kAudioFloat32;
using Core::kAudioFloat64;

using Core::AudioSampleType;
using Core::kAudioSampleTypeInt;
using Core::kAudioSampleTypeFloat;
using Core::kAudioSampleTypeUnsigned;

using Core::AudioFormatType;
using Core::kAudioFormatPCM;
using Core::kAudioFormatNonPCM;

using Core::kMaxAudioStreamChannels;
using Core::SilenceBitField;
using Core::kAudioAllChannelsMask;

using Core::TSamples;
using Core::TSeconds;
using Core::SampleRate;

using Core::getAudioByteSize;

using Core::AudioData;
using Core::AudioDataSpan;
using Core::StaticAudioData;

using Core::AudioIOData;
using Core::StaticAudioIOData;
using Core::StaticAudioStreamBuffers;

using Core::AudioMeterData;
using Core::StaticAudioMeterData;
using Core::kMaxAudioMeterChannels;
using Core::AudioMeterDataMax;

using Core::SpeakerID;
using Core::SpeakerBitField;

namespace SpeakerDef
{
	using namespace Core::SpeakerDef;
}

} // namespace CCL

#endif // _ccl_cclaudiotypes_h